#include "menu_interface.h"
#include "ecs/systems/system_render.h"
#include "commons/string.h"
#include "color_table.h"
#include "internal/menu_attributes.cpp"
#include "internal/menu_tree.cpp"
#include "nodes/container_node.cpp"
#include "nodes/grid_window.cpp"
#include "nodes/border_node.cpp"
#include "nodes/root_window.cpp"
#include "nodes/tabbed_window.cpp"
#include "nodes/main_window.cpp"
#include "nodes/split_window.cpp"
#include "menu_functions.h"

// Preorder breadth first.
void DrawMenu(memory_arena* Arena, menu_interface* Interface, menu_tree* Menu)
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
    else if(Menu->Root->Type == container_type::Root && GetRootNode(Menu->Root)->Maximized)
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
 

menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f MaxRegion)
{
  menu_tree* Root = NewMenuTree(Interface); // Root
  Root->Visible = true;
  Root->Root = CreateRootContainer(Interface, BaseWindow, MaxRegion);

  //  Root Node Complex, 4 Borders, 1 None

  TreeSensus(Root);

  UpdateRegionsOfContainerTree(Interface, Root->NodeCount, Root->Root);

  UpdateHotLeafs(Interface, Root);

  SetFocusWindow(Interface, Root);

  return Root;
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

void SetSelectedPlugin(menu_interface* Interface, container_node * Plugin)
{
  if (Plugin) {
    Assert(Plugin->Type == container_type::Plugin);
  }
  Interface->SelectedPlugin = Plugin;
}

void SetSelectedPluginTab(menu_interface* Interface, container_node * PluginTab)
{
  container_node* PluginToFocus = 0;
  if (PluginTab) {
    Assert(PluginTab->Type == container_type::Tab);
    PluginToFocus = GetPluginFromTab(PluginTab);

  }
  SetSelectedPlugin(Interface, PluginToFocus);
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

  SetSelectedPlugin(Interface, SelectedPlugin);
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
      MouseExitCalled = CallMouseExitFunctions(Interface, Menu->RemovedHotLeafCount, Menu->RemovedHotLeafs);
    }
    if(!MouseEnterCalled)
    {
      u32 NewLeafCount = Menu->HotLeafCount - Menu->NewLeafOffset;
      container_node** NewLeaves = Menu->HotLeafs + Menu->NewLeafOffset;
      MouseEnterCalled = CallMouseEnterFunctions(Interface, NewLeafCount, NewLeaves);
    }

    if(Menu->Visible && Menu == Interface->MenuInFocus)
    {
      if(!MouseDownCalled && jwin::Pushed(Interface->MouseLeftButton))
      {
        MouseUpCalled = CallMouseDownFunctions(Interface, Menu->HotLeafCount, Menu->HotLeafs); 
      }

      if(!MouseUpCalled && jwin::Released(Interface->MouseLeftButton) && Menu == Interface->MenuInFocus)
      {
        MouseUpCalled = CallMouseUpFunctions(Interface, Menu->HotLeafCount, Menu->HotLeafs);  
      }
    }
    Menu = Menu->Next;
  }

  CallUpdateFunctions(Interface, ArrayCount(Interface->UpdateQueue), Interface->UpdateQueue);

  if(Interface->MenuVisible)
  {
    Menu = Interface->MenuSentinel.Previous;
    while(Menu != &Interface->MenuSentinel)
    {
      if(Menu->Visible)
      {
        TreeSensus(Menu);
        UpdateRegionsOfContainerTree( Interface, Menu->NodeCount, Menu->Root);
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

  return Interface;
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
