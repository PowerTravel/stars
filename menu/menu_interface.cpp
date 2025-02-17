#include "menu_interface.h"
#include "ecs/systems/system_render.h"
#include "commons/string.h"
#include "color_table.h"
#include "internal/menu_attributes.cpp"
#include "nodes/main_window.h"
#include "nodes/root_window.cpp"
#include "nodes/tabbed_window.cpp"
#include "menu_functions.h"

internal void
PivotNodes(container_node* ShiftLeft, container_node* ShiftRight)
{
  Assert(ShiftLeft->PreviousSibling == ShiftRight);
  Assert(ShiftRight->NextSibling == ShiftLeft);
  
  ShiftRight->NextSibling = ShiftLeft->NextSibling;
  if(ShiftRight->NextSibling)
  {
    ShiftRight->NextSibling->PreviousSibling = ShiftRight;
  }

  ShiftLeft->PreviousSibling = ShiftRight->PreviousSibling;
  if(ShiftLeft->PreviousSibling)
  {
    ShiftLeft->PreviousSibling->NextSibling = ShiftLeft;
  }else{
    Assert(ShiftRight->Parent->FirstChild == ShiftRight);
    ShiftRight->Parent->FirstChild = ShiftLeft;
  }

  ShiftLeft->NextSibling = ShiftRight;
  ShiftRight->PreviousSibling = ShiftLeft;

  Assert(ShiftRight->PreviousSibling == ShiftLeft);
  Assert(ShiftLeft->NextSibling == ShiftRight);
  
}

internal void
ShiftLeft(container_node* ShiftLeft)
{
  if(!ShiftLeft->PreviousSibling)
  {
    return;
  }
  PivotNodes(ShiftLeft, ShiftLeft->PreviousSibling);
}

internal void
ShiftRight(container_node* ShiftRight)
{
  if(!ShiftRight->NextSibling)
  {
    return;
  }
  PivotNodes(ShiftRight->NextSibling, ShiftRight); 
}

internal void
ReplaceNode(container_node* Out, container_node* In)
{  
  if(In == Out) return;

  In->Parent = Out->Parent;
  if(In->Parent->FirstChild == Out)
  {
    In->Parent->FirstChild = In;
  }

  In->NextSibling = Out->NextSibling;
  if(In->NextSibling)
  {
    In->NextSibling->PreviousSibling = In;  
  }

  In->PreviousSibling = Out->PreviousSibling;
  if(In->PreviousSibling)
  {
    
    In->PreviousSibling->NextSibling = In;
  }

  Out->NextSibling = 0;
  Out->PreviousSibling = 0;
  Out->Parent = 0;
}

container_node* NewContainer(menu_interface* Interface, container_type Type)
{
  u32 BaseNodeSize    = sizeof(container_node) + sizeof(memory_link);
  u32 NodePayloadSize = GetContainerPayloadSize(Type);
  midx ContainerSize = (BaseNodeSize + NodePayloadSize);
 
  container_node* Result = (container_node*) Allocate(&Interface->LinkedMemory, ContainerSize);
  Result->Type = Type;
  Result->Functions = GetMenuFunction(Type);

  return Result;
}

internal void
DeleteAllAttributes(menu_interface* Interface, container_node* Node)
{
  // TODO: Free attribute data here too?
  while(Node->FirstAttribute)
  {
    menu_attribute_header* Attribute = Node->FirstAttribute;
    Node->FirstAttribute = Node->FirstAttribute->Next;
    FreeMemory(&Interface->LinkedMemory, (void*) Attribute);
  }
  Node->Attributes = ATTRIBUTE_NONE;
}

internal void
DeleteAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  Assert(Node->Attributes & (u32)AttributeType);

  menu_attribute_header** AttributePtr = &Node->FirstAttribute;
  while((*AttributePtr)->Type != AttributeType)
  {
    AttributePtr = &(*AttributePtr)->Next;
  }  

  menu_attribute_header* AttributeToRemove = *AttributePtr;
  *AttributePtr = AttributeToRemove->Next;

  FreeMemory(&Interface->LinkedMemory, (void*) AttributeToRemove);
  Node->Attributes =Node->Attributes - (u32)AttributeType;
}


internal void CancelAllUpdateFunctions(menu_interface* Interface, container_node* Node )
{
  for(u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    update_args* Entry = &Interface->UpdateQueue[i];
    if(Entry->InUse && Entry->Caller == Node)
    {
      if(Entry->FreeDataWhenComplete && Entry->Data)
      {
        FreeMemory(&Interface->LinkedMemory, Entry->Data);
      }
      *Entry = {};
    }
  }
}

void ClearMenuEvents( menu_interface* Interface, container_node* Node);
void DeleteContainer( menu_interface* Interface, container_node* Node)
{
  CancelAllUpdateFunctions(Interface, Node );
  ClearMenuEvents(Interface, Node);
  DeleteAllAttributes(Interface, Node);
  FreeMemory(&Interface->LinkedMemory, (void*) Node);
}

menu_tree* NewMenuTree(menu_interface* Interface)
{
  menu_tree* Result = (menu_tree*) Allocate(&Interface->LinkedMemory, sizeof(menu_tree));
  Result->LosingFocus = DeclareFunction(menu_losing_focus, DefaultLosingFocus);
  Result->GainingFocus = DeclareFunction(menu_gaining_focus, DefaultGainingFocus);
  ListInsertBefore(&Interface->MenuSentinel, Result);
  return Result;
}


internal void DeleteMenuSubTree(menu_interface* Interface, container_node* Root)
{
  DisconnectNode(Root);
  // Free the nodes;
  // 1: Go to the bottom
  // 2: Step up Once
  // 3: Delete FirstChild
  // 4: Set NextSibling as FirstChild
  // 5: Repeat from 1
  container_node* Node = Root->FirstChild;
  while(Node)
  {

    while(Node->FirstChild)
    {
      Node = Node->FirstChild;
    }

    Node = Node->Parent;
    if(Node)
    {
      container_node* NodeToDelete = Node->FirstChild;
      Node->FirstChild = Next(NodeToDelete);
      DeleteContainer(Interface, NodeToDelete);
    }
  }
  DeleteContainer(Interface, Root);
}

inline internal menu_tree* GetNextSpawningWindow(menu_interface* Interface)
{ 
  menu_tree* Result = 0;
  menu_tree* SpawningMenu = Interface->MenuSentinel.Next;
  while(SpawningMenu != &(Interface->MenuSentinel))
  {
    if(SpawningMenu->Root->Type == container_type::Root &&
       SpawningMenu != Interface->SpawningWindow )
    {
      Result = SpawningMenu;
      break;
    }
    SpawningMenu = SpawningMenu->Next;
  }

  return Result;
}

//  PostOrder (Left, Right, Root),  Depth first.
u32_pair UpdateSubTreeDepthAndCount( u32 ParentDepth, container_node* SubTreeRoot )
{
  u32 TotalDepth = 0;
  u32 CurrentDepth = ParentDepth;
  u32 NodeCount = 0;

  // Make SubTreeRoot look like an actual root node
  container_node* SubTreeParent = SubTreeRoot->Parent;
  container_node* SubTreeSibling = Next(SubTreeRoot);

  SubTreeRoot->Parent = 0;
  SubTreeRoot->NextSibling = 0;

  container_node* CurrentNode = SubTreeRoot;

  while(CurrentNode != SubTreeRoot->Parent)
  {
    // Set the depth of the current Node
    CurrentNode->Depth = CurrentDepth++;
    ++NodeCount;

    // Step all the way down (setting depth as you go along)
    while(CurrentNode->FirstChild)
    {
      CurrentNode = CurrentNode->FirstChild;
      CurrentNode->Depth = CurrentDepth++;
      ++NodeCount;
    }

    // The depth is now set until the leaf.
    TotalDepth = Maximum(CurrentDepth, TotalDepth);

    // Step up until you find another sibling or we reach root
    while(!Next(CurrentNode) && CurrentNode->Parent)
    {
      CurrentNode = CurrentNode->Parent;
      CurrentDepth--;
      Assert(CurrentDepth >= 0)
    }

    CurrentDepth--;

    // Either we found another sibling and we can traverse that part of the tree
    //  or we are at root and root has no siblings and we are done.
    CurrentNode = Next(CurrentNode);
  }

  // Restore the Root
  SubTreeRoot->Parent = SubTreeParent;
  SubTreeRoot->NextSibling = SubTreeSibling;

  u32_pair Result = {};
  Result.a = NodeCount;
  Result.b = TotalDepth;

  return Result;
}

void TreeSensus( menu_tree* Menu )
{
  u32_pair Pair =  UpdateSubTreeDepthAndCount( 0, Menu->Root );

  Menu->NodeCount = Pair.a;
  Menu->Depth = Pair.b;
 // Platform.DEBUGPrint("Tree Sensus:  Depth: %d, Count: %d\n", Pair.b, Pair.a);
}


void FreeMenuTree(menu_interface* Interface, menu_tree* MenuToFree)
{
  TreeSensus(MenuToFree);
  if(MenuToFree == Interface->SpawningWindow)
  {
    Interface->SpawningWindow = GetNextSpawningWindow(Interface);
  }
  if(MenuToFree == Interface->MenuInFocus)
  {
    CallFunctionPointer(MenuToFree->LosingFocus, Interface, MenuToFree);
    Interface->MenuInFocus = 0;
  }


  Assert(MenuToFree != &Interface->MenuSentinel);

  ListRemove( MenuToFree );
  container_node* Root = MenuToFree->Root;

  FreeMemory(&Interface->LinkedMemory, (void*)MenuToFree);

  DeleteMenuSubTree(Interface, Root);
}

rect2f GetSizedParentRegion(size_attribute* SizeAttr, rect2f BaseRegion)
{
  rect2f Result = {};
  if(SizeAttr->Width.Type == menu_size_type::RELATIVE_)
  {
    Result.W = SizeAttr->Width.Value * BaseRegion.W;
  }else if(SizeAttr->Width.Type == menu_size_type::ABSOLUTE_){
    Result.W = SizeAttr->Width.Value;  
  }

  if(SizeAttr->Height.Type == menu_size_type::RELATIVE_)
  {
    Result.H = SizeAttr->Height.Value * BaseRegion.H;
  }else if(SizeAttr->Height.Type == menu_size_type::ABSOLUTE_){
    Result.H = SizeAttr->Height.Value;
  }
  
  switch(SizeAttr->XAlignment)
  {
    case menu_region_alignment::LEFT:
    {
      Result.X = BaseRegion.X;
    }break;
    case menu_region_alignment::RIGHT:
    {
      Result.X = BaseRegion.X + BaseRegion.W - Result.W;
    }break;
    case menu_region_alignment::CENTER:
    {
      Result.X = BaseRegion.X + (BaseRegion.W - Result.W)*0.5f;
    }break;
  }
  switch(SizeAttr->YAlignment)
  {
    case menu_region_alignment::TOP:
    {
      //Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
      Result.Y = BaseRegion.Y;
    }break;
    case menu_region_alignment::BOT:
    {
      Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
    }break;
    case menu_region_alignment::CENTER:
    {
      Result.Y = BaseRegion.Y + (BaseRegion.H - Result.H)*0.5f;
    }break;
  }

  if(SizeAttr->LeftOffset.Type == menu_size_type::RELATIVE_)
  {
    Result.X += SizeAttr->LeftOffset.Value * BaseRegion.W;
  }else if(SizeAttr->LeftOffset.Type == menu_size_type::ABSOLUTE_){
    Result.X += SizeAttr->LeftOffset.Value;
  }

  if(SizeAttr->TopOffset.Type == menu_size_type::RELATIVE_)
  {
    Result.Y -= SizeAttr->TopOffset.Value * BaseRegion.H;
  }else if(SizeAttr->TopOffset.Type == menu_size_type::ABSOLUTE_){
    Result.Y -= SizeAttr->TopOffset.Value;
  }
  
  return Result;
}

void UpdateRegions(menu_interface* Interface, menu_tree* Menu )
{
  temporary_memory TempMem =  BeginTemporaryMemory(GlobalTransientArena);

  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = Menu->NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(GlobalTransientArena, Menu->NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Menu->Root;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    if(HasAttribute(Parent, ATTRIBUTE_SIZE))
    {
      size_attribute* SizeAttr = (size_attribute*) GetAttributePointer(Parent, ATTRIBUTE_SIZE);
      Parent->Region = GetSizedParentRegion(SizeAttr, Parent->Region);
    }

    // Update the region of all children and push them to the stack
    CallFunctionPointer(Parent->Functions.UpdateChildRegions, Interface, Parent);
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Next(Child);
    }
  }

  EndTemporaryMemory(TempMem);
}

// Preorder breadth first.
void DrawMenu( memory_arena* Arena, menu_interface* Interface, menu_tree* Menu)
{
  u32 NodeCount = Menu->NodeCount;
  container_node* Container = Menu->Root;

  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;
  local_persist r32 t = 0;
  r32 Height =  0.5*(Sin(t) + 1);
  r32 Width =  0.5*(Cos(t) + 1);
  t+= 0.01;
  if(t>Pi32)
  {
    t-=2*Pi32;
  }
  
  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    if(HasAttribute(Parent, ATTRIBUTE_COLOR))
    {
      color_attribute* Color = (color_attribute*) GetAttributePointer(Parent, ATTRIBUTE_COLOR);
      rect2f DrawRegion = ecs::render::RectCenterBotLeft(Parent->Region);
      ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), DrawRegion, Color->Color);
    }

    if(Parent->Functions.Draw)
    {
      CallFunctionPointer(Parent->Functions.Draw, Interface, Parent);  
    }

    if(HasAttribute(Parent, ATTRIBUTE_TEXT))
    {
      text_attribute* Text = (text_attribute*) GetAttributePointer(Parent, ATTRIBUTE_TEXT);
      v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), Text->FontSize, (utf8_byte*) Text->Text);
      ecs::render::system* RenderSystem = GetRenderSystem();
      ecs::render::window_size_pixel Window = GetWindowSize(RenderSystem);

      r32 X0 = (Parent->Region.X + Parent->Region.W/2.f) - TextSize.X/2.f;
      r32 Y0 = (Parent->Region.Y + Parent->Region.H/2.f) - TextSize.Y/3.f;
      ecs::render::DrawTextCanonicalSpace(RenderSystem, V2(X0, Y0), Text->FontSize,  (utf8_byte*)Text->Text, Text->Color);
    }

    if(HasAttribute(Parent, ATTRIBUTE_TEXTURE))
    {
      texture_attribute* Texture = (texture_attribute*) GetAttributePointer(Parent, ATTRIBUTE_TEXTURE);
      ecs::render::DrawTexturedOverlayQuadCanonicalSpace(GetRenderSystem(), Parent->Region, Rect2f(0,0,1,1), Texture->Handle);
    }

    // Update the region of all children and push them to the stack
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Next(Child);
    }
  }

  for (int i = 0; i < Menu->FinalRenderCount; ++i)
  {
    draw_queue_entry* Entry = &Menu->FinalRenderFunctions[i];
    CallFunctionPointer(Entry->DrawFunction, Interface, Entry->CallerNode);
  }
  Menu->FinalRenderCount = 0;
}


u32 GetIntersectingNodes(u32 NodeCount, container_node* Container, v2 MousePos, u32 MaxCount, container_node** Result)
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  temporary_memory TempMem = BeginTemporaryMemory(GlobalTransientArena);
  container_node** ContainerStack = PushArray(GlobalTransientArena, NodeCount, container_node*);

  u32 IntersectingLeafCount = 0;

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Check if mouse is inside the child region and push those to the stack.
    if(Intersects(Parent->Region, MousePos))
    {
      u32 IntersectingChildren = 0;
      container_node* Child = Parent->FirstChild;
      while(Child)
      {
        if(Intersects(Child->Region, MousePos))
        {
          ContainerStack[StackCount++] = Child;
          IntersectingChildren++;
        }
        Child = Next(Child);
      }  

      if(IntersectingChildren==0)
      {
        Assert(IntersectingLeafCount < MaxCount);
        Result[IntersectingLeafCount++] = Parent;
      }
    }
  }
  EndTemporaryMemory(TempMem);
  return IntersectingLeafCount;
}

container_node* ConnectNodeToFront(container_node* Parent, container_node* NewNode)
{
  NewNode->Parent = Parent;

  if(!Parent->FirstChild){
    Parent->FirstChild = NewNode;
  }else{
    NewNode->NextSibling = Parent->FirstChild;
    NewNode->NextSibling->PreviousSibling = NewNode;
    Parent->FirstChild = NewNode;
    Assert(NewNode != NewNode->NextSibling);
    Assert(NewNode->PreviousSibling == 0);
  }

  return NewNode;
}

container_node* ConnectNodeToBack(container_node* Parent, container_node* NewNode)
{
  NewNode->Parent = Parent;

  if(!Parent->FirstChild){
    Parent->FirstChild = NewNode;
  }else{
    container_node* Child = Parent->FirstChild;
    while(Next(Child))
    {
      Child = Next(Child);
    }  
    Child->NextSibling = NewNode;
    NewNode->PreviousSibling = Child;
  }

  return NewNode;
}

void DisconnectNode(container_node* Node)
{
  container_node* Parent = Node->Parent;
  if(Parent)
  {
    Assert(Parent->FirstChild);
    if(Node->PreviousSibling)
    {
      Node->PreviousSibling->NextSibling = Node->NextSibling;  
    }
    if(Node->NextSibling)
    {
      Node->NextSibling->PreviousSibling = Node->PreviousSibling;  
    }
    if(Parent->FirstChild == Node)
    {
      Parent->FirstChild = Node->NextSibling;
    }  
  }
  Node->Parent = 0;
  Node->NextSibling = 0;
  Node->PreviousSibling = 0;
}


b32 IsFocusWindow(menu_interface* Interface, menu_tree* Menu)
{
  b32 Result = (Interface->MenuInFocus && Menu == Interface->MenuInFocus);
  return Result;
}


// Put Menu on top of the renderqueue and on the Focused Window Ptr and call it's "Gaining Focus"-Functions
// If Menu is 0 and Focused Window Ptr is not 0, call its LoosingFocus function and set Focused Window Ptr to 0
void SetFocusWindow(menu_interface* Interface, menu_tree* Menu)
{
  Assert(Menu != &Interface->MenuSentinel);

  if(Interface->MenuInFocus && Menu != Interface->MenuInFocus)
  {
    // Call loosing focus callback on previous focused window
    CallFunctionPointer(Interface->MenuInFocus->LosingFocus, Interface, Interface->MenuInFocus);
  }

  if(Menu)
  {
    // The MenuBar is always the background window.
    if(Menu == Interface->MenuBar)
    {
      ListRemove( Menu );
      ListInsertBefore(&Interface->MenuSentinel, Menu);
    }
    else if(Menu->Maximized)
    {
      if(Interface->MenuSentinel.Previous == Interface->MenuBar)
      {
        ListRemove(Menu);
        ListInsertBefore(Interface->MenuSentinel.Previous, Menu);
      }else{
        ListRemove(Menu);
        ListInsertBefore(&Interface->MenuSentinel, Menu);
      }
    }else{
      if(Menu != Interface->MenuSentinel.Next)
      {
        // We put the menu in focus at the top of the queue so it get's rendered in the right order
        ListRemove( Menu );
        ListInsertAfter(&Interface->MenuSentinel, Menu); 
      }
      if(Menu != Interface->MenuInFocus)
      {
        // Call gaining focus callback on previous focused window
        CallFunctionPointer(Menu->GainingFocus, Interface, Menu);      
      }
    }
  }

  Interface->MenuInFocus = Menu;
}

void FormatNodeString(container_node* Node, u32 BufferSize, c8 StringBuffer[])
{
  u32 Attributes = Node->Attributes;
  
  Platform.DEBUGFormatString(StringBuffer, BufferSize, BufferSize-1,
  "%s", ToString(Node->Type));
  b32 First = true;
  while(Attributes)
  {
    bit_scan_result ScanResult = FindLeastSignificantSetBit(Attributes);
    Assert(ScanResult.Found);
    u32 Attribute = (1 << ScanResult.Index);
    Attributes -= Attribute;
    if(First)
    {
      First = false;
      jstr::CatStrings( jstr::StringLength(StringBuffer), StringBuffer,
              2, ": ",
              BufferSize, StringBuffer);
    }else{
      jstr::CatStrings( jstr::StringLength(StringBuffer), StringBuffer,
              3, " | ",
              BufferSize, StringBuffer);  
    }
    
    const c8* AttributeString = ToString(Attribute);
    jstr::CatStrings( jstr::StringLength(StringBuffer), StringBuffer,
                jstr::StringLength(AttributeString), AttributeString,
                BufferSize - jstr::StringLength(StringBuffer), StringBuffer);
  }
}

r32 PrintTree(menu_interface* Interface, u32 Count, container_node** HotLeafs, r32 YStart, r32 PixelSize, r32 HeightStep, r32 WidthStep, menu_tree* FocusWindow  )
{
  container_node* Node = HotLeafs[0];
  r32 XOff = 0;
  r32 YOff = YStart;
  container_node* Nodes[64] = {};
  container_node* CheckPoints[64] = {};
  u32 DepthCount = Node->Depth;
  while(Node)
  {
    Nodes[Node->Depth] = Node;
    Node = Node->Parent;
  }
  Assert(Nodes[0]->Depth == 0);

  for (u32 Depth = 0; Depth <= DepthCount; ++Depth)
  {
    Assert(Depth < ArrayCount(Nodes));
    Assert(Depth < ArrayCount(CheckPoints));
    container_node* Sibling = Nodes[Depth];
    if(Depth!=0)
    {
      Sibling = Nodes[Depth]->Parent->FirstChild;
    }
    while(Sibling)
    {
      v4 Color = V4(1,1,1,1);
      if(Sibling == Nodes[Depth])
      {
        Color = V4(1,1,0,1);
        CheckPoints[Depth] = Next(Sibling);
      }

      for (u32 LeadIndex = 0; LeadIndex < Count; ++LeadIndex)
      {
        if (Sibling == HotLeafs[LeadIndex])
        {
          Color = V4(1,0,0,1);
        }
      }

      if(FocusWindow && Sibling == FocusWindow->Root)
      {
        Color = V4(0,1,0,1);
      }

      if(Interface->SelectedPlugin == Sibling)
      {
        Color = V4(1,0,1,1);
      }

      u32 Attributes = Sibling->Attributes;
      c8 StringBuffer[1024] = {};
      FormatNodeString(Sibling, ArrayCount(StringBuffer), StringBuffer);

      XOff = WidthStep * Sibling->Depth;

      ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), V2(XOff, YOff), PixelSize, (utf8_byte*)StringBuffer, Color);
      YOff -= HeightStep;

      if(CheckPoints[Depth])
      {
        break;
      }
      Sibling = Next(Sibling);
    }
  }

  for (s32 Depth = DepthCount; Depth >= 0; --Depth)
  {
    container_node* Sibling = CheckPoints[Depth];
    while(Sibling)
    {
      v4 Color = V4(1,1,1,1);
      if(Sibling == Nodes[Depth])
      {
        Color = V4(1,1,0,1);
        CheckPoints[Depth] = Sibling;
      }

      for (u32 LeadIndex = 0; LeadIndex < Count; ++LeadIndex)
      {
        if (Sibling == HotLeafs[LeadIndex])
        {
          Color = V4(1,0,0,1);
        }
      }

      c8 StringBuffer[1024] = {};
      FormatNodeString(Sibling, ArrayCount(StringBuffer), StringBuffer);
      XOff = WidthStep * Sibling->Depth;
      ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), V2(XOff, YOff), PixelSize, (utf8_byte*) StringBuffer, Color);
      YOff -= HeightStep;

      Sibling = Next(Sibling);
    }
  }

  return YStart-YOff;
}

void UpdateHotLeafs(menu_interface* Interface, menu_tree* Menu);
void PrintHotLeafs(menu_interface* Interface, r32 CanPosX, r32 CanPosY)
{
  ecs::render::system* RenderSystem = GetRenderSystem();
  r32 TargetPixelSize = 12;
  ecs::render::window_size_pixel WindowSize = GetWindowSize(RenderSystem);
  r32 HeightStep = ecs::render::GetLineSpacingCanonicalSpace(RenderSystem, TargetPixelSize);
  r32 WidthStep  = 0.02;
  r32 YOff = 1 - 2*HeightStep;
  YOff = 0.5f;
  menu_tree* Menu = Interface->MenuSentinel.Next;
  menu_tree* FocusWindow = Interface->MenuInFocus;
  while(Menu != &Interface->MenuSentinel)
  {
    UpdateHotLeafs(Interface, Menu);
    if(Menu->Visible && Menu->HotLeafCount > 0)
    {
      YOff -= PrintTree(Interface, Menu->HotLeafCount, Menu->HotLeafs, YOff, TargetPixelSize, HeightStep, WidthStep, FocusWindow);
    }
    Menu = Menu->Next;
  }
}


container_node* GetTabWindowFromOtherMenu(menu_interface* Interface, container_node* Node)
{
  container_node* Result = 0;
  container_node* OwnRoot = GetRoot(Node);
  menu_tree* Menu = Interface->MenuSentinel.Next;
  
  menu_tree* MenuRoot = Interface->MenuSentinel.Next;
  while(MenuRoot != &Interface->MenuSentinel)
  {
    if(OwnRoot != MenuRoot->Root)
    {
      for (u32 LeafIndex = 0; LeafIndex < MenuRoot->HotLeafCount; ++LeafIndex)
      {
        container_node* HotLeafNode = MenuRoot->HotLeafs[LeafIndex];
        while(HotLeafNode)
        {
          if(HotLeafNode->Type == container_type::TabWindow)
          {
            Result = HotLeafNode;
            return Result;
          }
          HotLeafNode = HotLeafNode->Parent;
        }
      }  
    }
    
    MenuRoot = MenuRoot->Next;
  }
  return Result;
}

b32 IsHotLeef(menu_tree* Menu, container_node* Container)
{
  for (int i = 0; i < Menu->HotLeafCount; ++i)
  {
    if(Menu->HotLeafs[i] == Container)
    {
      return true;
    }
  }
  return false;
}

b32 IsPluginSelected(menu_interface* Interface, container_node* Container)
{
  Assert(Container->Type == container_type::Plugin);
  if(!Interface->MenuVisible)
  {
    return false;
  }
  if(!Interface->SelectedPlugin)
  {
    return false;
  }
  b32 ContainerIsTheSelectedPlugin = Interface->SelectedPlugin == Container;
  return ContainerIsTheSelectedPlugin;
}


void * PushOrGetAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  void * Result = 0;
  if(Node->Attributes & AttributeType)
  {
    Result = GetAttributePointer(Node, AttributeType);
  }else{
    Result = PushAttribute(Interface, Node, AttributeType);
  }
  return Result;
}

#if 0
// Could be nice to keep these around, theyre perfectly fine utility functions
void MoveAttribute(container_node* From, container_node* To, container_attribute AttributeType)
{
  Assert(HasAttribute(From, AttributeType));
  Assert(!HasAttribute(To, AttributeType));

  menu_attribute_header** AttributePtr = &From->FirstAttribute;
  while((*AttributePtr)->Type != AttributeType)
  {
    AttributePtr = &(*AttributePtr)->Next;
  }

  menu_attribute_header* AttributeToMove = *AttributePtr;
  *AttributePtr = AttributeToMove->Next;

  menu_attribute_header* DstAttribute = To->FirstAttribute;
  if(To->FirstAttribute)
  {
    while(DstAttribute->Next)
    {
      DstAttribute = DstAttribute->Next;
    }
    DstAttribute->Next = AttributeToMove;
  }else{
    To->FirstAttribute = AttributeToMove;
  }

  From->Attributes = From->Attributes - (u32)AttributeType;
  To->Attributes = To->Attributes + (u32)AttributeType;
}

rect2f GetHorizontalListRegion(rect2f HeaderRect, u32 TabIndex, u32 TabCount)
{
  r32 TabWidth = HeaderRect.W / (r32) TabCount;
  rect2f Result = HeaderRect;
  Result.W = TabWidth;
  Result.X = HeaderRect.X + TabIndex * TabWidth;
  return Result;
}

rect2f GetVerticalListRegion(rect2f HeaderRect, u32 TabIndex, u32 TabCount)
{
  r32 TabHeight = HeaderRect.H / (r32) TabCount;
  rect2f Result = HeaderRect;
  Result.H = TabHeight;
  Result.Y = HeaderRect.Y + HeaderRect.H - (TabIndex+1) * TabHeight;
  return Result;
}
#endif

b32 SplitWindowSignal(menu_interface* Interface, container_node* HeaderNode)
{
  rect2f HeaderSplitRegion = Shrink(HeaderNode->Region, -2*HeaderNode->Region.H);
  if(!Intersects(HeaderSplitRegion, Interface->MousePos))
  {
    return true;
  }
  return false;
}

void RegisterEventWithNode(menu_interface* Interface, container_node* Node, u32 Handle)
{
  menu_event_handle_attribtue* EventHandles = (menu_event_handle_attribtue*) PushOrGetAttribute(Interface, Node, ATTRIBUTE_MENU_EVENT_HANDLE);
  EventHandles->Handles[EventHandles->HandleCount++] = Handle;
}


// From http://burtleburtle.net/bob/hash/integer.html
umm IntegerHash(umm a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}
umm PointerToHash(void* Ptr)
{
  umm Val = (umm) Ptr;
  umm Result = IntegerHash(Val);
  return Result;
}

u32 GetEmptyMenuEventHandle(menu_interface* Interface, container_node* CallerNode)
{
  u32 MaxEventCount = ArrayCount(Interface->MenuEventCallbacks);
  Interface->MenuEventCallbackCount++;
  Assert(Interface->MenuEventCallbackCount < MaxEventCount);
  u32 Hash = (u32)(PointerToHash((void*)CallerNode) % MaxEventCount);
  menu_event* Event = &Interface->MenuEventCallbacks[Hash];
  u32 Handle = Hash;
  while(Event->Active)
  {
    ++Event;
    ++Handle;
    if(Handle >= MaxEventCount)
    {
      Handle = 0;
      Event = Interface->MenuEventCallbacks;
    }
  }
  ListInsertBefore(&Interface->EventSentinel, Event);
  return Handle;
}

menu_event* GetMenuEvent(menu_interface* Interface, u32 Handle)
{
  Assert(Handle < ArrayCount(Interface->MenuEventCallbacks));
  menu_event* Event = &Interface->MenuEventCallbacks[Handle];
  return Event;
}

void _RegisterMenuEvent(menu_interface* Interface, menu_event_type EventType, container_node* CallerNode, void* Data, menu_event_callback** Callback,  menu_event_callback** OnDelete)
{

  u32 Index = GetEmptyMenuEventHandle(Interface, CallerNode);
  menu_event* Event  = GetMenuEvent(Interface, Index);
  Assert(!Event->Active);
  Event->Active = true;
  Event->Index = Index;
  Event->CallerNode = CallerNode;
  Event->EventType = EventType;
  Event->Callback = Callback;
  Event->OnDelete = OnDelete;
  Event->Data = Data;
  RegisterEventWithNode(Interface, CallerNode, Index);
}

void UpdateBorderPosition(container_node* BorderNode, v2 NewPosition, rect2f MinimumWindowSize, rect2f MaximumWindowSize)
{
  border_leaf* Border = GetBorderNode(BorderNode);
  switch(Border->Type)
  {
    // A Root border work with canonical positions
    case border_type::LEFT: {
      Border->Position = Clamp(
        NewPosition.X,
        MaximumWindowSize.X + Border->Thickness * 0.5f,
        MinimumWindowSize.X + MinimumWindowSize.W - Border->Thickness * 0.5f);
    }break;
    case border_type::RIGHT:{
      Border->Position = Clamp(
        NewPosition.X,
        MinimumWindowSize.X + Border->Thickness * 0.5f,
        MaximumWindowSize.X + MaximumWindowSize.W - Border->Thickness * 0.5f);
    }break;
    case border_type::TOP:{
      Border->Position = Clamp(
        NewPosition.Y,
        MinimumWindowSize.Y + Border->Thickness * 0.5f,
        MaximumWindowSize.Y + MaximumWindowSize.H - Border->Thickness * 0.5f);
    }break;
    case border_type::BOTTOM:{
      Border->Position = Clamp(
        NewPosition.Y,
        MaximumWindowSize.Y + Border->Thickness * 0.5f,
        MinimumWindowSize.Y + MinimumWindowSize.H - Border->Thickness * 0.5f);
    }break;
    // A SPLIT border don't work with canonical positions, they work with percentage of parent region.
    // That means if a border position is at 10% it means 10% of parent window is for left split region and 90% is for right split region
    case border_type::SPLIT_VERTICAL:{
      Border->Position = Clamp(
        NewPosition.X,
        MinimumWindowSize.X,
        MinimumWindowSize.W);
      Platform.DEBUGPrint("%1.2f\n", Border->Position);
    }break;
    case border_type::SPLIT_HORIZONTAL:{
      Border->Position = Clamp(
        NewPosition.Y,
        MinimumWindowSize.Y,
        MinimumWindowSize.H);
    }break;
  }
}

MENU_UPDATE_FUNCTION(SplitWindowBorderUpdate)
{
  container_node* BorderNode = CallerNode;
  Assert(BorderNode->Type == container_type::Border);
  container_node* SplitNode = BorderNode->Parent;
  Assert(SplitNode->Type == container_type::Split);
  r32 AspectRatio = GetAspectRatio(Interface);
  v2 MousePosWindowLocal = V2(
      Unlerp(Interface->MousePos.X, SplitNode->Region.X, SplitNode->Region.X + SplitNode->Region.W),
      Unlerp(Interface->MousePos.Y, SplitNode->Region.Y, SplitNode->Region.Y + SplitNode->Region.H)
    );

  rect2f MaximumWindowSize = Rect2f(0,0,1,1);
  rect2f MinimumWindowSize = Rect2f(
      Unlerp(Interface->MinSize * 0.5,                       0, SplitNode->Region.W),
      Unlerp(Interface->MinSize * 0.5,                       0, SplitNode->Region.H),
      Unlerp(SplitNode->Region.W - Interface->MinSize * 0.5, 0, SplitNode->Region.W),
      Unlerp(SplitNode->Region.H - Interface->MinSize * 0.5, 0, SplitNode->Region.H)
    );

  UpdateBorderPosition(BorderNode, MousePosWindowLocal, MinimumWindowSize, MaximumWindowSize);
  return Interface->MouseLeftButton.Active;
}

MENU_EVENT_CALLBACK(InitiateSplitWindowBorderDrag)
{
  PushToUpdateQueue(Interface, CallerNode, SplitWindowBorderUpdate, 0, false);
}



rect2f GetMinimumRootWindowSize(root_border_collection* BorderCollection, r32 MinimumRegionWidth, r32 MinimumRegionHeight)
{
  rect2f Result = {};
  Result.X = GetBorderNode(BorderCollection->Left)->Position  + MinimumRegionWidth*0.5;
  Result.W = GetBorderNode(BorderCollection->Right)->Position - MinimumRegionWidth*0.5 - Result.X;
  Result.Y = GetBorderNode(BorderCollection->Bot)->Position   + MinimumRegionHeight*0.5;
  Result.H = GetBorderNode(BorderCollection->Top)->Position   - MinimumRegionHeight*0.5 - Result.Y;
  return Result;
}

MENU_UPDATE_FUNCTION(RootBorderDragUpdate)
{
  Assert(CallerNode->Parent->Type == container_type::Root);

  r32 AspectRatio = GetAspectRatio(Interface);
  rect2f MaximumWindowSize = Rect2f(0,0,AspectRatio, 1 - Interface->HeaderSize);
  root_border_collection BorderCollection = GetRoorBorders(CallerNode->Parent);
  rect2f MinimumWindowSize = GetMinimumRootWindowSize(&BorderCollection, Interface->MinSize, Interface->MinSize);
  UpdateBorderPosition(CallerNode, Interface->MousePos, MinimumWindowSize, MaximumWindowSize);
  return Interface->MouseLeftButton.Active;
}

MENU_EVENT_CALLBACK(InitiateBorderDrag)
{
  PushToUpdateQueue(Interface, CallerNode, RootBorderDragUpdate, 0, false);
}

container_node* CreateBorderNode(menu_interface* Interface, v4 Color)
{
  container_node* Result = NewContainer(Interface, container_type::Border);
 
  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Result, ATTRIBUTE_COLOR);
  ColorAttr->Color = Color;

  Assert(Result->Type == container_type::Border);
  return Result;
}

b32 IsPointerInArray(u32 ArrayCount, container_node** NodeArray, container_node* NodeToCheck, u32* Position)
{
  b32 Result = false;
  for (u32 i = 0; i < ArrayCount; ++i)
  {
    container_node* Node = NodeArray[i];
    if(Node == NodeToCheck)
    {
      Result = true;
      *Position = i;
      break;
    }
  }
  return Result;
}

// Destroys NewHotLeafs;
void SortHotLeafs(u32 OldHotLeafCount,   container_node** OldHotLeafs,
                  u32 NewHotLeafCount,   container_node** NewHotLeafs,
                  u32* NewCount,      container_node** New,
                  u32* RemovedCount,  container_node** Removed,
                  u32* ExistingCount, container_node** Existing)
{
  u32 NrNew = 0;
  u32 NrRemoved = 0;
  u32 NrExisting = 0;

  // Put all ptr's that exist in both arrays in Existing
  // And remove it from OldHotLeafs and NewHotLeafs
  for (u32 i = 0; i < OldHotLeafCount; ++i)
  {
    container_node* Leaf = OldHotLeafs[i];
    u32 Position = 0;
    if(IsPointerInArray(NewHotLeafCount, NewHotLeafs, Leaf, &Position))
    {
      Existing[NrExisting++] = Leaf;
      NewHotLeafs[Position] = 0;
      OldHotLeafs[i] = 0;
    }
  }
  // The leftover ptr's in NewHotLeafs gets put in New
  for (u32 i = 0; i < NewHotLeafCount; ++i)
  {
    if(NewHotLeafs[i])
    {
      New[NrNew++] = NewHotLeafs[i];
    }
  }
  // The leftover ptr's in OldHotLeafs gets put in Removed
  for (u32 i = 0; i < OldHotLeafCount; ++i)
  {
    if(OldHotLeafs[i])
    {
      Removed[NrRemoved++] = OldHotLeafs[i];
    }
  }
 
  *NewCount = NrNew;
  *RemovedCount = NrRemoved;
  *ExistingCount = NrExisting;
}

internal void UpdateHotLeafs(menu_interface* Interface, menu_tree* Menu)
{
  SCOPED_TRANSIENT_ARENA;

  // Get all new intersecting nodes
  u32 HotLeafsMaxCount = ArrayCount(Menu->HotLeafs);
  container_node** CurrentHotLeafs = (container_node**) PushArray(GlobalTransientArena, HotLeafsMaxCount, container_node*);
  u32 CurrentHotLeafCount = GetIntersectingNodes(Menu->NodeCount, Menu->Root, Interface->MousePos, HotLeafsMaxCount, CurrentHotLeafs);

  Assert(CurrentHotLeafCount < HotLeafsMaxCount);

  container_node** OldHotLeafs = (container_node**) PushCopy(GlobalTransientArena, HotLeafsMaxCount, Menu->HotLeafs);
  
  // Sort the newly gathered hot leafs into "new", "existing", "removed"
  u32 RemovedHotLeafsMaxCount = ArrayCount(Menu->RemovedHotLeafs);

  u32 NewCount = 0;
  u32 RemovedCount = 0;
  u32 ExistingCount = 0;
  container_node** New = PushArray(GlobalTransientArena, HotLeafsMaxCount, container_node*);
  container_node** Existing = PushArray(GlobalTransientArena, HotLeafsMaxCount, container_node*);
  container_node** Removed = PushArray(GlobalTransientArena, RemovedHotLeafsMaxCount, container_node*);
  SortHotLeafs(Menu->HotLeafCount,  OldHotLeafs,
               CurrentHotLeafCount, CurrentHotLeafs,
               &NewCount,           New,
               &RemovedCount,       Removed,
               &ExistingCount,      Existing);

  Assert(NewCount + ExistingCount < HotLeafsMaxCount);
  CopyArray( ExistingCount, Existing, Menu->HotLeafs);
  CopyArray( NewCount, New, Menu->HotLeafs+ExistingCount);
  Menu->HotLeafCount = NewCount + ExistingCount;
  Menu->NewLeafOffset = ExistingCount;
  
  Assert(RemovedCount < RemovedHotLeafsMaxCount);
  CopyArray( RemovedCount, Removed, Menu->RemovedHotLeafs);
  Menu->RemovedHotLeafCount = RemovedCount;
}

inline internal container_node* GetTabWindowFromTab(container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);
  container_node* TabHeader = Tab->Parent;
  Assert(TabHeader->Type == container_type::Grid);
  container_node* TabRegion = TabHeader->Parent;
  Assert(TabRegion->Type == container_type::None);
  container_node* TabWindow = TabRegion->Parent;

  Assert(TabWindow->Type == container_type::TabWindow);
  return TabWindow;
}

internal container_node* PushTab(container_node* TabbedWindow,  container_node* Tab)
{
  container_node* TabContainer = GetTabGridFromWindow(TabbedWindow);
  ConnectNodeToFront(TabContainer, Tab);
  return Tab;
}

internal void SetTabAsActive(container_node* Tab)
{
  tab_node* TabNode = GetTabNode(Tab);
  container_node* TabWindow = GetTabWindowFromTab(Tab);
  container_node* TabBody = TabWindow->FirstChild->NextSibling;
  if(!TabBody)
  {
    ConnectNodeToBack(TabWindow, TabNode->Payload);
  }else{
    ReplaceNode(TabBody, TabNode->Payload);  
  }
}


internal container_node* PopTab(container_node* Tab)
{
  tab_node* TabNode = GetTabNode(Tab);
  DisconnectNode(TabNode->Payload);
  if(Next(Tab))
  {
    SetTabAsActive(Next(Tab));  
  }else if(Previous(Tab)){
    SetTabAsActive(Previous(Tab));
  }

  DisconnectNode(Tab);
  return Tab;
}


internal u32 FillArrayWithTabs(u32 MaxArrSize, container_node* TabArr[], menu_tree* Menu)
{
  container_node* StartNode = GetBodyFromRoot(Menu->Root);
  SCOPED_TRANSIENT_ARENA;
  u32 StackCount = 0;
  u32 TabCount = 0;

  container_node** ContainerStack = PushArray(GlobalTransientArena, MaxArrSize, container_node*);

  // Push StartNode
  ContainerStack[StackCount++] = StartNode;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    if(Parent->Type == container_type::Split)
    {
      ContainerStack[StackCount++] = Next(Parent->FirstChild);
      ContainerStack[StackCount++] = Next(Next(Parent->FirstChild));
    }else if(Parent->Type == container_type::TabWindow)
    {
      ContainerStack[StackCount++] = GetTabGridFromWindow(Parent);
    }else if(Parent->Type == container_type::Grid)
    {
      container_node* Tab = Parent->FirstChild;
      Assert(Tab->Type == container_type::Tab);
      while(Tab)
      {
        container_node* TabToMove = Tab;
        Tab = Next(Tab);
        
        tab_node* TabNode = GetTabNode(TabToMove);
        if(TabNode->Payload->Type == container_type::Plugin)
        {
          PopTab(TabToMove);
          TabArr[TabCount++] = TabToMove;
        }else{
          ContainerStack[StackCount++] = TabNode->Payload;
        }
      }
      Assert(TabCount < MaxArrSize);
    }else{
      INVALID_CODE_PATH;
    }
  }
  return TabCount;
}

void UpdateMergableAttribute( menu_interface* Interface, container_node* Node )
{
  container_node* TabWindow = GetTabWindowFromOtherMenu(Interface, Node);
  if(!TabWindow)
  {
    return;
  }
  tab_window_node* TabWindowNode = GetTabWindowNode(TabWindow);
  TabWindowNode->HotMergeZone = merge_zone::NONE;
  if(TabWindow)
  {
    rect2f Rect = TabWindow->Region;
    r32 W = Rect.W;
    r32 H = Rect.H;
    r32 S = Minimum(W,H)/4;

    v2 CP = CenterPoint(Rect);  // Middle Point
    v2 LS = V2(CP.X-S, CP.Y);   // Left Square
    v2 RS = V2(CP.X+S, CP.Y);   // Right Square
    v2 BS = V2(CP.X,   CP.Y-S); // Bot Square
    v2 TS = V2(CP.X,   CP.Y+S); // Top Square

    TabWindowNode->MergeZone[(u32) merge_zone::CENTER] = Rect2f(CP.X-S/2.f, CP.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::LEFT]   = Rect2f(LS.X-S/2.f, LS.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::RIGHT]  = Rect2f(RS.X-S/2.f, RS.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::TOP]    = Rect2f(BS.X-S/2.f, BS.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::BOT]    = Rect2f(TS.X-S/2.f, TS.Y-S/2.f,S/1.1f,S/1.1f);

    if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::CENTER)], Interface->MousePos))
    {
      TabWindowNode->HotMergeZone = merge_zone::CENTER;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::LEFT)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::LEFT;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::RIGHT)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::RIGHT;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::TOP)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::TOP;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::BOT)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::BOT;
    }else{
      TabWindowNode->HotMergeZone = merge_zone::HIGHLIGHTED;
    }
  }

  if(Interface->MouseLeftButton.Edge &&
    !Interface->MouseLeftButton.Active )
  {
    merge_zone MergeZone = TabWindowNode->HotMergeZone;
    container_node* TabWindowToAccept = TabWindow;
    container_node* NodeToInsert = Node;
    switch(TabWindowNode->HotMergeZone)
    {
      case merge_zone::CENTER:
      {
        menu_tree* MenuToRemove = GetMenu(Interface, NodeToInsert);

        Assert(TabWindowToAccept->Type == container_type::TabWindow);

        container_node* TabArr[64] = {};
        u32 TabCount = FillArrayWithTabs(ArrayCount(TabArr), TabArr, MenuToRemove);

        for(u32 Index = 0; Index < TabCount; ++Index)
        {
          container_node* TabToMove = TabArr[Index];
          PushTab(TabWindowToAccept, TabToMove);
        }

        SetTabAsActive(TabArr[TabCount-1]);
        
        FreeMenuTree(Interface, MenuToRemove);
        TabWindowNode->HotMergeZone = merge_zone::NONE;
      }break;
      case merge_zone::LEFT:    // Fallthrough 
      case merge_zone::RIGHT:   // Fallthrough
      case merge_zone::TOP:     // Fallthrough
      case merge_zone::BOT:
      {
        menu_tree* MenuToRemove = GetMenu(Interface, NodeToInsert);
        menu_tree* MenuToRemain = GetMenu(Interface, TabWindowToAccept);

        while(NodeToInsert && NodeToInsert->Type != container_type::Split)
        {
          NodeToInsert = NodeToInsert->Parent;
        }
        if(!NodeToInsert)
        {
          NodeToInsert = GetBodyFromRoot(MenuToRemove->Root);
          Assert(NodeToInsert->Type == container_type::TabWindow);
        }else{
          Assert(NodeToInsert->Type == container_type::Split);
        }
        
        Assert(TabWindowToAccept->Type == container_type::TabWindow);

        DisconnectNode(NodeToInsert);

        b32 Vertical = (TabWindowNode->HotMergeZone == merge_zone::LEFT || TabWindowNode->HotMergeZone == merge_zone::RIGHT);

        container_node* SplitNode = CreateSplitWindow(Interface, Vertical);
        ReplaceNode(TabWindowToAccept, SplitNode);

        b32 VisitorIsFirstChild = (TabWindowNode->HotMergeZone == merge_zone::LEFT || TabWindowNode->HotMergeZone == merge_zone::TOP);
        if(VisitorIsFirstChild)
        {
          ConnectNodeToBack(SplitNode, NodeToInsert);
          ConnectNodeToBack(SplitNode, TabWindowToAccept);
        }else{
          ConnectNodeToBack(SplitNode, TabWindowToAccept);
          ConnectNodeToBack(SplitNode, NodeToInsert);
        }

        FreeMenuTree(Interface, MenuToRemove);
        TabWindowNode->HotMergeZone = merge_zone::NONE;
      }break;
    }
  }

  if(!Interface->MouseLeftButton.Active)
  {
    TabWindowNode->HotMergeZone = merge_zone::NONE;
  }
}


void DeregisterEvent(menu_interface* Interface, u32 Handle)
{
  menu_event* Event = GetMenuEvent(Interface, Handle);
  Interface->MenuEventCallbackCount--;
  ListRemove(Event);
  if(Event->OnDelete)
  {
    CallFunctionPointer(Event->OnDelete, Interface, Event->CallerNode, Event->Data);
  }
  *Event = {};
}

void ClearMenuEvents(menu_interface* Interface, container_node* Node)
{
  if(HasAttribute(Node, ATTRIBUTE_MENU_EVENT_HANDLE))
  {
    menu_event_handle_attribtue* EventHandles = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
    for(u32 Idx = 0; Idx < EventHandles->HandleCount; ++Idx)
    {
      u32 Handle = EventHandles->Handles[Idx];
      DeregisterEvent(Interface, Handle);
      EventHandles->Handles[Idx] = 0;
    }
    EventHandles->HandleCount = 0;  
  }
}

menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f MaxRegion)
{
  menu_tree* Root = NewMenuTree(Interface); // Root
  Root->Visible = true;
  Root->Root = NewContainer(Interface, container_type::Root);

  //  Root Node Complex, 4 Borders, 1 None
  { 
    r32 Thickness = Interface->BorderSize;

    container_node* Border1 = CreateBorderNode(Interface, Interface->BorderColor);
    ConnectNodeToBack(Root->Root, Border1);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border1, 0, InitiateBorderDrag, 0);
    SetBorderData(Border1, Thickness, MaxRegion.X, border_type::LEFT);

    container_node* Border2 = CreateBorderNode(Interface, Interface->BorderColor);
    ConnectNodeToBack(Root->Root, Border2);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border2, 0, InitiateBorderDrag, 0);
    SetBorderData(Border2, Thickness, MaxRegion.X + MaxRegion.W, border_type::RIGHT);

    container_node* Border3 = CreateBorderNode(Interface, Interface->BorderColor);
    ConnectNodeToBack(Root->Root, Border3);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border3, 0, InitiateBorderDrag, 0);
    SetBorderData(Border3, Thickness, MaxRegion.Y,  border_type::BOTTOM);
    
    container_node* Border4 = CreateBorderNode(Interface, Interface->BorderColor);
    ConnectNodeToBack(Root->Root, Border4);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border4, 0, InitiateBorderDrag, 0);
    SetBorderData(Border4, Thickness, MaxRegion.Y + MaxRegion.H,  border_type::TOP);

    ConnectNodeToBack(Root->Root, BaseWindow); // Body
  }

  TreeSensus(Root);

  UpdateRegions(Interface, Root);

  UpdateHotLeafs(Interface, Root);

  SetFocusWindow(Interface, Root);

  return Root;
}

void ReduceSplitWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  container_node* SplitNodeToSwapOut = WindowToRemove->Parent;
  container_node* WindowToRemain = WindowToRemove->NextSibling;
  if(!WindowToRemain)
  {
    WindowToRemain = WindowToRemove->Parent->FirstChild->NextSibling;
    Assert(WindowToRemain);
    Assert(WindowToRemain->NextSibling == WindowToRemove);
  }

  DisconnectNode(WindowToRemove);
  DeleteMenuSubTree(Interface, WindowToRemove);

  DisconnectNode(WindowToRemain);

  ReplaceNode(SplitNodeToSwapOut, WindowToRemain);
  DeleteMenuSubTree(Interface, SplitNodeToSwapOut);
}

void ReduceWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  // Just to let us know if we need to handle another case
  Assert(WindowToRemove->Type == container_type::TabWindow);

  // Make sure the tab window is empty
  Assert(GetChildCount(GetTabGridFromWindow(WindowToRemove)) == 0);

  container_node* ParentContainer = WindowToRemove->Parent;
  switch(ParentContainer->Type)
  {
    case container_type::Split:
    {
      ReduceSplitWindowTree(Interface, WindowToRemove);
    }break;
    case container_type::Root:
    {
      FreeMenuTree(Interface, GetMenu(Interface,ParentContainer));
    }break;
    default:
    {
      INVALID_CODE_PATH;
    } break;
  }
}

container_node* GetWindowDragNode(container_node* TabWindow)
{
  Assert(TabWindow->Type == container_type::TabWindow);
  return TabWindow->FirstChild->FirstChild->NextSibling;
}


void SplitTabToNewWindow(menu_interface* Interface, container_node* Tab, rect2f TabbedWindowRegion)
{
  rect2f NewRegion = Move(TabbedWindowRegion, V2(-Interface->BorderSize, Interface->BorderSize)*0.5); 
  
  container_node* NewTabWindow = CreateTabWindow(Interface);
  CreateNewRootContainer(Interface, NewTabWindow, NewRegion);

  PopTab(Tab);
  PushTab(NewTabWindow, Tab);

  ConnectNodeToBack(NewTabWindow, GetTabNode(Tab)->Payload);

  // Trigger the window-drag instead of this tab-drag
  mouse_position_in_window* Position = (mouse_position_in_window*) Allocate(&Interface->LinkedMemory, sizeof(mouse_position_in_window));
  *Position = GetPositionInRootWindow(Interface->MousePos, NewTabWindow);
  PushToUpdateQueue(Interface, Tab, WindowDragUpdate, Position, true);
}

b32 TabDrag(menu_interface* Interface, container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);

  container_node* TabWindow = GetTabWindowFromTab(Tab);
  container_node* TabHeader = GetTabGridFromWindow(TabWindow);


  b32 Continue = Interface->MouseLeftButton.Active; // Tells us if we should continue to call the update function;

  SetTabAsActive(Tab);

  u32 Count = GetChildCount(TabHeader);
  if(Count > 1)
  {
    r32 dX = Interface->MousePos.X - Interface->PreviousMousePos.X;
    u32 Index = GetChildIndex(Tab);
    if(dX < 0 && Index > 0)
    {
      container_node* PreviousTab = Previous(Tab);
      Assert(PreviousTab);
      if(Interface->MousePos.X < (PreviousTab->Region.X + PreviousTab->Region.W/2.f))
      {
        // Note: A bit of a hacky way to handle an issue. When the user pushes the mouse down, we store that mouse location in the Interface.
        // If a person shifts the tab that mouse down position no longer holds the distance within the tab where the button was pushed.
        // This means if a user Shifts a tab then splits it into a new window, the calculation below for NewRegion is wrong because 
        // Mouse Left Button Push will no longer hold the information about where in the tab the user klicked.
        // Therefore we update the "MouseLeftButtonPush" here to keep that information. It's hacky because it's no longer hold the actual mouseDownPosition.
        // This is not an issue as long as we don't make use of the original mouseLeftButtonPush location somewehre else before the user klicks again.
        // Should not be an issue since a tab-trag ends with a mouse-up event. Next mouse klick should not need to know where the previous klick was,
        // but if any weird behaviour occurs later related to MouseLeftButtonPush, know that this is a possible cause of the issue.
        Interface->MouseLeftButtonPush.X -= PreviousTab->Region.W;
        ShiftLeft(Tab);
      }
    }else if(dX > 0 && Index < Count-1){
      container_node* NextTab = Next(Tab);
      Assert(NextTab);
      if(Interface->MousePos.X > (NextTab->Region.X + NextTab->Region.W/2.f))
      {
        // Note: See above
        Interface->MouseLeftButtonPush.X += NextTab->Region.W;
        ShiftRight(Tab);
      }
    }else if(SplitWindowSignal(Interface, Tab->Parent))
    {
      // Note: See above
      v2 MouseDiffFromClick = Interface->MousePos - Interface->MouseLeftButtonPush - (LowerLeftPoint(Tab->Parent->Region) - LowerLeftPoint(Tab->Region));
      rect2f NewRegion = Move(TabWindow->Region, MouseDiffFromClick);
      SplitTabToNewWindow(Interface, Tab, NewRegion);
      Continue = false;
    }
  }else if(TabWindow->Parent->Type == container_type::Split)
  {
    if(SplitWindowSignal(Interface, Tab->Parent))
    {
      // Note: See above
      v2 MouseDiffFromClick = Interface->MousePos - Interface->MouseLeftButtonPush - (LowerLeftPoint(Tab->Parent->Region) - LowerLeftPoint(Tab->Region));
      rect2f NewRegion = Move(TabWindow->Region, MouseDiffFromClick);
      SplitTabToNewWindow(Interface, Tab, NewRegion);
      ReduceWindowTree(Interface, TabWindow);
      Continue = false;
    }  
  }
  return Continue;
}

internal void GetInput(jwin::device_input* DeviceInput, menu_interface* Interface)
{
  Interface->PreviousMousePos = Interface->MousePos;
  Interface->MousePos = V2(DeviceInput->Mouse.X, DeviceInput->Mouse.Y);
  Interface->Time = DeviceInput->Time;
  Interface->DeltaTime = DeviceInput->deltaTime;
  Interface->DoubleKlick = false;

  Update(&Interface->TAB, Pushed(DeviceInput->Keyboard.Key_TAB));
  if(Interface->TAB.Active && Interface->TAB.Edge)
  {
    Interface->MenuVisible = !Interface->MenuVisible;
  }

  Update(&Interface->MouseLeftButton, DeviceInput->Mouse.Button[jwin::MouseButton_Left].Active);
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active)
    {
      if((Interface->MouseLeftButtonPush == Interface->MousePos) && (Interface->Time - Interface->MouseLeftButtonPushTime) < Interface->DoubleKlickTime )
      {
        Interface->DoubleKlick = true;
      }
      Interface->MouseLeftButtonPush = Interface->MousePos;
      Interface->MouseLeftButtonPushTime = Interface->Time;
    }else{
      Interface->MouseLeftButtonRelese = Interface->MousePos;
      Interface->MouseLeftButtonReleaseTime = Interface->Time;
    }
  }
}

internal b32 CallMouseExitFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < Menu->RemovedHotLeafCount; ++i)
  {
    container_node* Node = Menu->RemovedHotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event =   GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseExit)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

internal b32 CallMouseEnterFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 FunctionCalled = false;
  u32 NewLeafCount = Menu->HotLeafCount - Menu->NewLeafOffset;
  for (u32 i = Menu->NewLeafOffset; i < NewLeafCount; ++i)
  {
    container_node* Node = Menu->HotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event =   GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseEnter)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

internal b32 CallMouseDownFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < Menu->HotLeafCount; ++i)
  {
    container_node* Node = Menu->HotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node, ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseDown)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

internal b32 CallMouseUpFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < Menu->HotLeafCount; ++i)
  {
    container_node* Node = Menu->HotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseUp)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

void UpdateFocusWindow(menu_interface* Interface)
{
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active)
    {
      menu_tree* MenuFocusWindow = 0;
      menu_tree* Menu = Interface->MenuSentinel.Next;
      while(Menu != &Interface->MenuSentinel)
      {
        // MenuBar is the background menu whos position is always furthest back (Should maybe handle that when sorting menus)
        if(Menu->Visible && Intersects(Menu->Root->Region, Interface->MousePos))
        {
          MenuFocusWindow = Menu;
          break;
        }
        Menu = Menu->Next;
      }
      SetFocusWindow(Interface, MenuFocusWindow);
    }
  }
}

internal void CallUpdateFunctions(menu_interface* Interface)
{
  for (u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    update_args* Entry = &Interface->UpdateQueue[i];
    if(Entry->InUse)
    {
      b32 Continue = CallFunctionPointer(Entry->UpdateFunction, Interface, Entry->Caller, Entry->Data);
      if(!Continue)
      {
        if(Entry->FreeDataWhenComplete && Entry->Data)
        {
          FreeMemory(&Interface->LinkedMemory, Entry->Data);
        }
        *Entry = {};
      }
    }
  }
}

void UpdateSelectedPlugin(menu_interface* Interface)
{
  b32 MouseWasClicked = Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active;
  if(!MouseWasClicked)
  {
    return;
  }
  menu_tree* TopMostWindow = Interface->MenuSentinel.Next;
  container_node* SelectedPlugin = 0;

  while(TopMostWindow != &Interface->MenuSentinel)
  {
    if( TopMostWindow->Visible)
    {
      container_node* HotLeaf = 0;
      if(TopMostWindow->HotLeafCount > 0)
      {
        HotLeaf = TopMostWindow->HotLeafs[0];
      }

      // Get the closest plugin
      while(HotLeaf)
      {
        if(HotLeaf->Type == container_type::Plugin)
        {
          Interface->SelectedPlugin = HotLeaf;
          return;
        }else if(HotLeaf->Type == container_type::TabWindow)
        {
          Interface->SelectedPlugin = 0;
          return;
        }
        HotLeaf = HotLeaf->Parent;
      }
    }
    TopMostWindow = TopMostWindow->Next;
  }

  Interface->SelectedPlugin = SelectedPlugin;
}

void UpdateAndRenderMenuInterface(jwin::device_input* DeviceInput, menu_interface* Interface)
{ 
  // Sets input variables to Interface
  GetInput(DeviceInput, Interface);
  if(!Interface->MenuVisible)
  {
    return;
  }

  // Updates all the hot leaf struct for the active window windows
  menu_tree* Menu = Interface->MenuSentinel.Next;
  while(Menu != &Interface->MenuSentinel)
  {
    UpdateHotLeafs(Interface, Menu);
    Menu = Menu->Next;
  }


  // Checks if a window was selected/deselected
  // * Sets or clears MenuFocusWindow
  // * Calls Gaining/Loosing Focus Functions
  UpdateFocusWindow(Interface);
  
  UpdateSelectedPlugin(Interface);

  // Calls Mouse Exit on the top most window
  Menu = Interface->MenuSentinel.Next;
  b32 MouseEnterCalled = false;
  b32 MouseExitCalled = false;
  b32 MouseDownCalled = false;
  b32 MouseUpCalled = false;

  
  while(Menu != &Interface->MenuSentinel)
  {
    // If CallMouseEnter/ExitFunctions returns true, it means that a function wass successfully called
    // We only want to call it on the top most window
    if(!MouseExitCalled)
    {
      MouseExitCalled = CallMouseExitFunctions(Interface, Menu);
    }
    if(!MouseEnterCalled)
    {
      MouseEnterCalled = CallMouseEnterFunctions(Interface, Menu);
    }

    if(Menu->Visible && Menu == Interface->MenuInFocus)
    {
      if(!MouseDownCalled && jwin::Pushed(Interface->MouseLeftButton))
      {
        MouseDownCalled = CallMouseDownFunctions(Interface, Menu);  
      }

      if(!MouseUpCalled && jwin::Released(Interface->MouseLeftButton) && Menu == Interface->MenuInFocus)
      {
        MouseUpCalled = CallMouseUpFunctions(Interface, Menu);  
      }
    }
    Menu = Menu->Next;
  }

  CallUpdateFunctions(Interface);

  if(Interface->MenuVisible)
  {
    Menu = Interface->MenuSentinel.Previous;
    while(Menu != &Interface->MenuSentinel)
    {
      if(Menu->Visible)
      {
        TreeSensus(Menu);
        UpdateRegions( Interface, Menu );
        DrawMenu( GlobalTransientArena, Interface, Menu);
        ecs::render::NewRenderLevel(GetRenderSystem());
      }
      Menu = Menu->Previous;
    }
  }
  
  #if 1
  PrintHotLeafs(Interface, 1-0.05, 1);
  #endif
}


container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos)
{
  container_node* SplitNode  = NewContainer(Interface, container_type::Split);
  container_node* BorderNode = CreateBorderNode(Interface, Interface->BorderColor);
  SetBorderData(BorderNode, Interface->BorderSize, BorderPos, Vertical ? border_type::SPLIT_VERTICAL : border_type::SPLIT_HORIZONTAL);
  ConnectNodeToBack(SplitNode, BorderNode);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, BorderNode, 0, InitiateSplitWindowBorderDrag, 0);
  return SplitNode;
}

menu_interface* CreateMenuInterface(memory_arena* Arena, midx MaxMemSize, r32 AspectRatio)
{
  menu_interface* Interface = PushStruct(Arena, menu_interface);
  Interface->LinkedMemory = NewLinkedMemory(Arena, MaxMemSize);
  Interface->BorderSize = 0.007;
  Interface->HeaderFontSize = 16;
  Interface->BodyFontSize = 14;
  Interface->HeaderSize = ecs::render::PixelToCanonicalHeight(GetRenderSystem(), Interface->HeaderFontSize);
  Interface->BorderColor = menu::GetColor(GetColorTable(),"medium teal blue");
  Interface->MinSize = 0.2f;
  Interface->DoubleKlickTime = 0.5f;
  Interface->AspectRatio = AspectRatio;

  Interface->MenuColor = menu::GetColor(GetColorTable(),"charcoal");
  Interface->TextColor = menu::GetColor(GetColorTable(),"white smoke");

  ListInitiate(&Interface->MenuSentinel);
  ListInitiate(&Interface->EventSentinel);

  Interface->MenuBar = CreateMainWindow(Interface);

#if 0
  Interface->MenuBar = NewMenuTree(Interface);
  Interface->MenuBar->Visible = true;
  Interface->MenuBar->Root = NewContainer(Interface, container_type::MainHeader);

  container_node* RootWindow = Interface->MenuBar->Root;
  RootWindow->Region = Rect2f(0,0,AspectRatio,1);
  { // Main Menu Header
    container_node* HeaderBar = ConnectNodeToBack(RootWindow, NewContainer(Interface));

    color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, HeaderBar, ATTRIBUTE_COLOR);

    ColorAttr->Color = Interface->MenuColor;
    ColorAttr->HighlightedColor = ColorAttr->Color;
    ColorAttr->RestingColor = ColorAttr->Color;

    container_node* DropDownContainer = ConnectNodeToBack(HeaderBar, NewContainer(Interface, container_type::Grid));
    grid_node* Grid = GetGridNode(DropDownContainer);
    Grid->Col = 0;
    Grid->Row = 1;
    Grid->TotalMarginX = 0.0;
    Grid->TotalMarginY = 0.0;
    Grid->Stack = true;
  }

  { // Main Menu Body
    container_node* Body = ConnectNodeToBack(RootWindow, NewContainer(Interface));
  }
#endif
  return Interface;
}

void DisplayOrRemovePluginTab(menu_interface* Interface, container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);
  container_node* TabHeader = Tab->Parent;
  if(TabHeader)
  {
    Assert(TabHeader->Type == container_type::Grid);

    container_node* TabWindow = GetTabWindowFromTab(Tab);

    PopTab(Tab);
    if(GetChildCount(TabHeader) == 0)
    {
      ReduceWindowTree(Interface, TabWindow);
    }

  }else{

    container_node* TabWindow = 0;
    if(!Interface->SpawningWindow)
    {
      TabWindow = CreateTabWindow(Interface);
      Interface->SpawningWindow = CreateNewRootContainer(Interface, TabWindow, Rect2f( 0.25, 0.25, 0.7, 0.5));

    }else{
      TabWindow = GetBodyFromRoot(Interface->SpawningWindow->Root);
      while(TabWindow->Type != container_type::TabWindow)
      {
        if(TabWindow->Type ==  container_type::Split)
        {
          TabWindow = Next(TabWindow->FirstChild);
        }else{
          INVALID_CODE_PATH;
        }
      }
    }

    PushTab(TabWindow, Tab);

    container_node* Body = Next(TabWindow->FirstChild);
    tab_node* TabNode = GetTabNode(Tab);
    if(Body)
    {
      ReplaceNode(Body, TabNode->Payload);
      container_node* Plugin = TabNode->Payload;
      Assert(Plugin->Type == container_type::Plugin);
      plugin_node* PluginNode = GetPluginNode(Plugin);
    }else{
      ConnectNodeToBack(TabWindow, TabNode->Payload);
    }
  }

  SetFocusWindow(Interface, Interface->SpawningWindow);
}

MENU_EVENT_CALLBACK(DropDownMenuButton)
{
  menu_tree* Menu = (menu_tree*) Data;
  Menu->Root->Region.X = CallerNode->Region.X;
  Menu->Root->Region.Y = CallerNode->Region.Y - Menu->Root->Region.H;

  if(!Menu->Visible)
  {
    SetFocusWindow(Interface, Menu);
  }
}


menu_tree* RegisterMenu(menu_interface* Interface, const c8* Name)
{
  ecs::render::system* RenderSystem = GetRenderSystem();
  ecs::render::window_size_pixel WindowSize = ecs::render::GetWindowSize(RenderSystem);
  r32 PixelFontHeight = ecs::render::GetLineSpacingPixelSpace(RenderSystem, Interface->HeaderFontSize);
  r32 CanonicalFontHeight = Interface->HeaderSize;
  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(RenderSystem, Interface->HeaderFontSize, (utf8_byte*) Name);

  container_node* MainSettingBar =  Interface->MenuBar->Root->FirstChild;
  container_node* DropDownContainer = MainSettingBar->FirstChild;
  Assert(DropDownContainer->Type == container_type::Grid);

  container_node* NewMenu = ConnectNodeToBack(DropDownContainer, NewContainer(Interface));

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_COLOR);
  ColorAttr->Color = Interface->MenuColor;
  ColorAttr->RestingColor = ColorAttr->Color;
  ColorAttr->HighlightedColor = ColorAttr->Color*1.5;

  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, TextSize.X * 1.2f);
  SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, CanonicalFontHeight);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->XAlignment = menu_region_alignment::CENTER;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;
  
  text_attribute* Text = (text_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(Name, Text->Text);
  Text->FontSize = Interface->HeaderFontSize;
  Text->Color = Interface->TextColor;

  menu_tree* ViewMenuRoot = NewMenuTree(Interface);
  ViewMenuRoot->Root = NewContainer(Interface);
  ViewMenuRoot->Root->Region = {};
  ViewMenuRoot->LosingFocus = DeclareFunction(menu_losing_focus, DropDownLosingFocus);
  ViewMenuRoot->GainingFocus = DeclareFunction(menu_losing_focus, DropDownGainingFocus);

  container_node* ViewMenuItems = ConnectNodeToBack(ViewMenuRoot->Root, NewContainer(Interface, container_type::Grid));
  grid_node* Grid = GetGridNode(ViewMenuItems);
  Grid->Col = 1;
  Grid->Row = 0;
  Grid->TotalMarginX = 0.0;
  Grid->TotalMarginY = 0.0;

  // Do we need the following 2 attributes?
  color_attribute* ViewMenuColor = (color_attribute*) PushAttribute(Interface, ViewMenuRoot->Root, ATTRIBUTE_COLOR);
  ViewMenuColor->Color = Interface->MenuColor;
  ViewMenuColor->HighlightedColor = Interface->MenuColor;
  ViewMenuColor->RestingColor = Interface->MenuColor; 

  //color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, ViewMenuItems, ATTRIBUTE_COLOR);
  //ColorAttr->Color = menu::GetColor(&GlobalState->ColorTable,"charcoal");

  RegisterMenuEvent(Interface, menu_event_type::MouseDown,  NewMenu, (void*) ViewMenuRoot, DropDownMenuButton, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, NewMenu, (void*) ViewMenuRoot, HeaderMenuMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  NewMenu, (void*) ViewMenuRoot, HeaderMenuMouseExit, 0);

  return ViewMenuRoot;
}


MENU_UPDATE_FUNCTION(TabDragUpdate)
{
  b32 Continue = TabDrag(Interface, CallerNode);
  return Interface->MouseLeftButton.Active && Continue;
}

MENU_EVENT_CALLBACK(TabMouseDown)
{
  container_node* Tab = CallerNode;

  // Initiate Tab Drag
  container_node* TabWindow = GetTabWindowFromTab(Tab);
  container_node* TabGrid = GetTabGridFromWindow(TabWindow);
  if(GetChildCount(TabGrid) > 1 || TabWindow->Parent->Type == container_type::Split)
  {
    PushToUpdateQueue(Interface, CallerNode, TabDragUpdate, 0, false);
  }
}

MENU_EVENT_CALLBACK(TabMouseUp)
{
  
}

MENU_EVENT_CALLBACK(TabMouseEnter)
{
  tab_node* TabNode = GetTabNode(CallerNode);
  if(!IsPluginSelected(Interface, TabNode->Payload))
  {
    color_attribute* TabColor =(color_attribute*) GetAttributePointer(CallerNode,ATTRIBUTE_COLOR);
    TabColor->Color = TabColor->HighlightedColor;
  }
}

MENU_EVENT_CALLBACK(TabMouseExit)
{
  tab_node* TabNode = GetTabNode(CallerNode);
  if(!IsPluginSelected(Interface, TabNode->Payload))
  {
    color_attribute* TabColor =(color_attribute*) GetAttributePointer(CallerNode,ATTRIBUTE_COLOR);
    TabColor->Color = TabColor->RestingColor;
  }
}

internal container_node* CreateTab(menu_interface* Interface, container_node* Plugin)
{
  plugin_node* PluginNode = GetPluginNode(Plugin);

  container_node* Tab = NewContainer(Interface, container_type::Tab);  

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_COLOR);
  ColorAttr->Color = PluginNode->Color;
  ColorAttr->RestingColor = PluginNode->Color;
  ColorAttr->HighlightedColor = PluginNode->Color * 1.5;

  GetTabNode(Tab)->Payload = Plugin;
  PluginNode->Tab = Tab;

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Tab, 0, TabMouseDown, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseUp, Tab, 0, TabMouseUp, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, Tab, 0, TabMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  Tab, 0, TabMouseExit, 0);
  
  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(PluginNode->Title, MenuText->Text);
  MenuText->FontSize = Interface->BodyFontSize;
  MenuText->Color = Interface->TextColor;

  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), Interface->HeaderFontSize, (utf8_byte*) PluginNode->Title);
  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, TextSize.X * 1.05);
  SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE_, 1);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->XAlignment = menu_region_alignment::LEFT;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;


  return Tab;
}

MENU_EVENT_CALLBACK(HeaderMenuMouseEnter)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);

  menu_tree* DropDownMenu = (menu_tree*) Data;
  menu_tree* MenuBar = GetMenu(Interface, CallerNode);
  menu_tree* VisibleDropDownMenu = 0;
  
  // See if any drop down menu is visible
  for (int i = 0; i < Interface->MainMenuTabCount; ++i)
  {
    menu_tree* DropDown = Interface->MainMenuTabs[i];
    if(DropDown->Visible && DropDown != DropDownMenu)
    {
      VisibleDropDownMenu = DropDown;
      break;
    }
  }
  if(VisibleDropDownMenu)
  {
    DropDownMenu->Root->Region.X = CallerNode->Region.X;
    DropDownMenu->Root->Region.Y = CallerNode->Region.Y - DropDownMenu->Root->Region.H;
    SetFocusWindow(Interface, DropDownMenu);
  }
  MenuColor->Color = MenuColor->HighlightedColor;
}

MENU_EVENT_CALLBACK(HeaderMenuMouseExit)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = MenuColor->RestingColor;
}


MENU_EVENT_CALLBACK(DropDownMouseEnter)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = MenuColor->HighlightedColor;
}

MENU_EVENT_CALLBACK(DropDownMouseExit)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = MenuColor->RestingColor;
}

MENU_LOSING_FOCUS(DropDownLosingFocus)
{
  Menu->Visible = false;
}

MENU_GAINING_FOCUS(DropDownGainingFocus)
{
  Menu->Visible = true;

}
MENU_EVENT_CALLBACK(DropDownMouseUp)
{
  menu_tree* Menu = GetMenu(Interface, CallerNode);
  Assert(Menu->Visible);
  //Menu->Visible = false;
  DisplayOrRemovePluginTab(Interface, (container_node*) Data);
}

void RegisterWindow(menu_interface* Interface, menu_tree* DropDownMenu, container_node* Plugin)
{
  container_node* DropDownGrid = DropDownMenu->Root->FirstChild;
  Assert(DropDownGrid->Type == container_type::Grid);
  Assert(Plugin->Type == container_type::Plugin);

  container_node* MenuItem = ConnectNodeToBack(DropDownGrid, NewContainer(Interface));
  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(GetPluginNode(Plugin)->Title, MenuText->Text);
  MenuText->FontSize = Interface->BodyFontSize;
  MenuText->Color = Interface->TextColor;

  color_attribute* DropDownColor = (color_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_COLOR);
  DropDownColor->Color = Interface->MenuColor;
  DropDownColor->HighlightedColor = Interface->MenuColor * 1.5;
  DropDownColor->RestingColor = Interface->MenuColor;

  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), Interface->BodyFontSize, (utf8_byte*)GetPluginNode(Plugin)->Title );
  DropDownMenu->Root->Region.H += TextSize.Y*2;
  DropDownMenu->Root->Region.W = DropDownMenu->Root->Region.W >= TextSize.X*1.2 ? DropDownMenu->Root->Region.W : TextSize.X*1.2;

  container_node* Tab = CreateTab(Interface, Plugin);

  Interface->PermanentWindows[Interface->PermanentWindowCount++] = Tab;

  RegisterMenuEvent(Interface, menu_event_type::MouseUp,    MenuItem, Tab, DropDownMouseUp, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, MenuItem, Tab, DropDownMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  MenuItem, Tab, DropDownMouseExit, 0);

  Assert(Interface->MainMenuTabCount < ArrayCount(Interface->MainMenuTabs));
  Interface->MainMenuTabs[Interface->MainMenuTabCount++] = DropDownMenu;

}

container_node* CreatePlugin(menu_interface* Interface, menu_tree* WindowsDropDownMenu, c8* HeaderName, v4 HeaderColor, container_node* BodyNode)
{
  container_node* Plugin = NewContainer(Interface, container_type::Plugin);
  plugin_node* PluginNode = GetPluginNode(Plugin);
  jstr::CopyStringsUnchecked(HeaderName, PluginNode->Title);
  PluginNode->Color = HeaderColor;

  ConnectNodeToBack(Plugin, BodyNode);

  RegisterWindow(Interface, WindowsDropDownMenu, Plugin);

  return Plugin;
}

void ToggleWindow(menu_interface* Interface, char* WindowName)
{
  for(u32 PluginIndex = 0; PluginIndex < Interface->PermanentWindowCount; PluginIndex++)
  {
    container_node* PluginTab = Interface->PermanentWindows[PluginIndex];
    container_node* Plugin = GetTabNode(PluginTab)->Payload;

    if(jstr::ExactlyEquals(WindowName, GetPluginNode(Plugin)->Title))
    {
      DisplayOrRemovePluginTab(Interface,PluginTab);  
    }
  }
}


u32 GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::Root:
    case container_type::MainWindow:return 0;
    case container_type::Border:    return sizeof(border_leaf);
    case container_type::Grid:      return sizeof(grid_node);
    case container_type::TabWindow: return sizeof(tab_window_node);
    case container_type::Tab:       return sizeof(tab_node);
    case container_type::Plugin:    return sizeof(plugin_node);
    default: INVALID_CODE_PATH;
  }
  return 0;
}


void _PushToUpdateQueue(menu_interface* Interface, container_node* Caller, update_function** UpdateFunction, void* Data, b32 FreeData)
{
  update_args* Entry = 0;
  for (u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    Entry = &Interface->UpdateQueue[i];
    if(!Entry->InUse)
    {
      break;
    }
  }
  Assert(!Entry->InUse);

  Entry->Interface = Interface;
  Entry->Caller = Caller;
  Entry->InUse = true;
  Entry->FreeDataWhenComplete = FreeData;
  Entry->UpdateFunction = UpdateFunction;
  Entry->Data = Data;
}


menu_tree* GetMenu(menu_interface* Interface, container_node* Node)
{
  menu_tree* Result = 0;
  container_node* Root = Node;
  while(Root->Parent)
  {
    Root = Root->Parent; 
  }
  
  menu_tree* MenuRoot = Interface->MenuSentinel.Next;
  while(MenuRoot != &Interface->MenuSentinel)
  {
    if(Root == MenuRoot->Root)
    {
      Result = MenuRoot;
      break;
    }
    MenuRoot = MenuRoot->Next;
  }
  return Result;
}

rect2f GetActiveMenuRegion(menu_interface* Interface)
{
  return Interface->MenuBar->Root->FirstChild->NextSibling->Region;
}
