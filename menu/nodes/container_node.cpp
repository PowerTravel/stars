#include "container_node.h"
#include "border_node.h"
#include "grid_window.h"
#include "main_window.h"
#include "root_window.h"
#include "tabbed_window.h"
#include "split_window.h"
#include "text_input_window.h"

u32 GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::MainWindow: return 0;
    case container_type::Root:       return sizeof(root_node);
    case container_type::Border:     return sizeof(border_leaf);
    case container_type::Grid:       return sizeof(grid_node);
    case container_type::TabWindow:  return sizeof(tab_window_node);
    case container_type::Tab:        return sizeof(tab_node);
    case container_type::Plugin:     return sizeof(plugin_node);
    case container_type::TextInput:  return sizeof(text_input_node);
    default: INVALID_CODE_PATH;
  }
  return 0;
}

void DefaultUpdateChildRegions(menu_interface* Interface, container_node* Parent)
{
  container_node* Child = Parent->FirstChild;
  while(Child)
  {
    Child->Region = Parent->Region;
    Child = Next(Child);
  }
}

menu_functions GetDefaultFunctions()
{
  menu_functions Result = {};
  return Result;
}

menu_functions GetMenuFunction(container_type Type)
{
  switch(Type)
  {   
    case container_type::None:       return GetDefaultFunctions();
    case container_type::MainWindow: return GetMainWindowFunctions();
    case container_type::Root:       return GetRootMenuFunctions();
    case container_type::Border:     return GetDefaultFunctions();
    case container_type::Split:      return GetSplitFunctions();
    case container_type::Grid:       return GetGridFunctions();
    case container_type::TabWindow:  return GetTabWindowFunctions();
    case container_type::Tab:        return GetDefaultFunctions();
    case container_type::Plugin:     return GetDefaultFunctions();
    case container_type::TextInput:  return GetTextInputfunctions();

    default: Assert(0);
  }
  return {};
}


container_node* NewContainer(menu_interface* Interface, container_type Type)
{
  u32 BaseNodeSize    = sizeof(container_node) + sizeof(memory_link);
  u32 NodePayloadSize = GetContainerPayloadSize(Type);
  midx ContainerSize = (BaseNodeSize + NodePayloadSize);

  container_node* Result = (container_node*) Allocate(&Interface->LinkedMemory, ContainerSize);
  Result->Type = Type;
  Result->Functions = GetMenuFunction(Type);
  Result->DebugID = Interface->DebugIDCounter++;
  Result->Active = true;
  Platform.DEBUGPrint("Creating %s Node %d\n",ToString(Result->Type), Result->DebugID);
  return Result;
}

void PivotNodes(container_node* ShiftLeft, container_node* ShiftRight)
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

void ShiftLeft(container_node* ShiftLeft)
{
  if(!ShiftLeft->PreviousSibling)
  {
    return;
  }
  PivotNodes(ShiftLeft, ShiftLeft->PreviousSibling);
}

void ShiftRight(container_node* ShiftRight)
{
  if(!ShiftRight->NextSibling)
  {
    return;
  }
  PivotNodes(ShiftRight->NextSibling, ShiftRight); 
}

void ReplaceNode(container_node* Out, container_node* In)
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

internal void FreeUpdateFunction(menu_interface* Interface, update_function_arguments* Args)
{
  if(Args->FreeDataWhenComplete && Args->Data)
  {
    FreeMemory(&Interface->LinkedMemory, Args->Data);
  }
  Args->Caller->UpdateFunctionRunning = 0;
  *Args = {};
}

internal void CancelAllUpdateFunctions(menu_interface* Interface, container_node* Node )
{
  for(u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    update_function_arguments* Entry = &Interface->UpdateQueue[i];
    if(Entry->InUse && Entry->Caller == Node)
    {
      FreeUpdateFunction(Interface, Entry);
    }
  }
}

void DeleteContainer( menu_interface* Interface, container_node* Node)
{
  Platform.DEBUGPrint("Deleting %s Node %d\n", ToString(Node->Type), Node->DebugID);
  CancelAllUpdateFunctions(Interface, Node );
  ClearMenuEvents(Interface, Node);
  DeleteAllAttributes(Interface, Node);
  FreeMemory(&Interface->LinkedMemory, (void*) Node);
}


void DeleteMenuSubTree(menu_interface* Interface, container_node* Root)
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

void CallUpdateFunctions(menu_interface* Interface, u32 UpdateCount, update_function_arguments* UpdateArgs)
{
  for (u32 i = 0; i < UpdateCount; ++i)
  {
    update_function_arguments* Entry = &UpdateArgs[i];
    if(Entry->Caller)
    {
      b32 Continue = Entry->InUse;
      if(Continue)
      {
        Continue = CallFunctionPointer(Entry->UpdateFunction, Interface, Entry->Caller, Entry->Data);
      }

      // A UpdateFunction may have called FreeUpdateFunction, so we need to check again here.
      if(!Continue && Entry->Caller) {
        FreeUpdateFunction(Interface, Entry);
      }
    }
  }
}

b32 IntersectsChildren(v2 MousePos, container_node* Parent)
{
  u32 IntersectingChildren = 0;
  container_node* Child = Parent->FirstChild;
  while(Child)
  {
    if(Intersects(Child->Region, MousePos))
    {
      IntersectingChildren++;
    }
    Child = Next(Child);
  }
  return IntersectingChildren!=0;
}

u32 GetIntersectingNodes(u32 NodeCount, container_node* Container, v2 MousePos, u32 MaxCount, container_node** Result)
{
  SCOPED_TRANSIENT_ARENA;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(GlobalTransientArena, NodeCount, container_node*);

  u32 IntersectingLeafCount = 0;

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;
    if(Intersects(Parent->Region, MousePos))
    {
      if(!IntersectsChildren(MousePos, Parent))
      {
        Result[IntersectingLeafCount++] = Parent;
      }
    }

    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Next(Child);
    }
  }
  return IntersectingLeafCount;
}


s32 GetIndexOfIntersectingChild(container_node* Node, v2 MousePos)
{
  container_node* Child = Node->FirstChild;
  u32 Index = 0;
  while(Child)
  {
    if(Intersects(Child->Region, MousePos))
    {
      return Index;
    }
    Index++;
    Child = Child->NextSibling;
  }
  return -1;
}

/*
              A
         /         \
        B           C
    /   |   \     /   \
    D   E    F    G    H
        |             / \
        I            J   K

  [A, B, D, E, I, F, C, G, H]

  Start. Add Root A, Set Size of A to 0;
  [A(0)]
  Look At A, A is not visited, Mark A Visited, Add Children C then B, Set size of B and C to 0
  [A(v), C(0), B(0)]
  Look At B, B is not visited, Mark B Visited, Add Children F then E then D, Set their size to 0
  [A(v), C(0), B(v), F(0), E(0), D(0)]
  Look At D, D is not visited, Mark D Visited, D is leaf, Remove D, Add size and relative position to self update parent B Size.
  [A(v), C(0), B(v,D), F(0), E(0)]
  Look at E, Add Children I and set their size to 0
  [A(v), C(0), B(D), F(0), E(0), I(0)]
  Look At I, I is leaf, Remove I, Add size and relative position to self update parent E Size.
  [A(v), C(0), B(D), F(0), E(I)]
  Look at E, E has size, Remove E, Add relative position to self update parent B Size.
  [A(v), C(0), B(D,E), F(0)]
  Look At F, F is leaf, Remove F, Add size and relative position to self update parent B Size. 
  [A(v), C(0), B(D,E,F)]
  Look at B, B has size, Remove B, Add relative position to self update parent A Size.
  [A(v), C(0)]
  ...
*/

struct container_stack_entry
{
  container_node* Node;
  b32 Visited;
};

struct container_stack
{
  container_stack_entry* Entries;
  u32 MaxCount;
  u32 Head;
};

container_stack NewContainerStack(memory_arena* Arena, u32 MaxCount)
{
  container_stack Result = {};
  Result.Entries = PushArray(GlobalTransientArena, MaxCount, container_stack_entry);
  Result.MaxCount = MaxCount;
  Result.Head = 0;
  return Result;
}

void Push(container_stack& Stack, container_node* Node)
{
  Assert(Stack.Head < Stack.MaxCount);
  Stack.Entries[Stack.Head++].Node = Node;
}

container_stack_entry Pop(container_stack& Stack)
{
  Assert(Stack.Head > 0);
  container_stack_entry Result = Stack.Entries[Stack.Head-1];
  Stack.Entries[Stack.Head-1] = {};
  Stack.Head--;
  return Result;
}

container_stack_entry* Peak(container_stack& Stack)
{
  Assert(Stack.Head > 0);
  return &Stack.Entries[Stack.Head-1];
}

b32 IsEmpty(container_stack& Stack)
{
  return Stack.Head == 0;
}

b32 IsLeaf(container_node* Node)
{
  return Node->FirstChild == 0;
}

rect2f ArrangeChildPositions(container_node* Node)
{
  if(!Node->FirstChild)
    return {};

  rect2f NodeRegion = GetFirstChild(Node)->Region;
  if(Node->StackHorizontal)
  {
    container_node* Child = GetFirstChild(Node);
    while(Child)
    {
      if(GetFirstChild(Node) == Child)
      {
        NodeRegion = Child->Region;
      }else{
        Child->Region.X = NodeRegion.W;
        Child->Region.Y = 0;
        NodeRegion.W += Child->Region.W;
        if(NodeRegion.H < Child->Region.H)
        {
          NodeRegion.H = Child->Region.H;
        }
      }
      Child = Next(Child);
    }
  }else{
    container_node* LastChild = GetFirstChild(Node);
    container_node* Child = LastChild;
    while(Child)
    {
      if(LastChild == Child)
      {
        NodeRegion = Child->Region;
      }else{
        Child->Region.X = 0;
        Child->Region.Y = NodeRegion.H;
        NodeRegion.H += Child->Region.H;
        if(NodeRegion.W < Child->Region.W)
        {
          NodeRegion.W = Child->Region.W;
        }
      }
      Child = Next(Child);
    }
  }
 
  return NodeRegion;
}

void AlignChildRegions(container_node* Node)
{
  if(!Node->FirstChild) return;

  container_node* Child = GetFirstChild(Node);
  while(Child)
  {
    v2 Alignment = V2(Node->Region.X,Node->Region.Y);
    if(HasAttribute(Child, ATTRIBUTE_ALIGNMENT))
    {
      alignment_attribute* AlignmentAttr = (alignment_attribute*) GetAttributePointer(Child, ATTRIBUTE_ALIGNMENT);
      Alignment = GetAlignedPosition(AlignmentAttr, Child->Region, Node->Region);
    }
    Child->Region.X += Alignment.X;
    Child->Region.Y += Alignment.Y;
    Child = Next(Child);
  }
}

void SetRelativeSizes(container_node* Node)
{
  if(!Node->FirstChild) return;

  if(HasAttribute(Node, ATTRIBUTE_ABS_SIZE))
  {
    container_node* Child = GetFirstChild(Node);
    r32 Top = Node->Region.Y + Node->Region.H;
    r32 Left = Node->Region.X;
    while(Child)
    {
      if(HasAttribute(Child, ATTRIBUTE_ABS_SIZE))
      {
        absolute_size_attribute* Size = (absolute_size_attribute*) GetAttributePointer(Child, ATTRIBUTE_ABS_SIZE);
        if(Size->Width == 0)
          Child->Region.W = Child->Parent->Region.W;

        if(Size->Height == 0)
          Child->Region.H = Child->Parent->Region.H;
      }
      if(Node->StackHorizontal)
      {
        Child->Region.X = Left;
        Left+=Child->Region.W;
      }else{
        Top -= Child->Region.H;
        Child->Region.Y = Top - Child->Region.H;
      }
      Child = Next(Child);
    }
  }
}

rect2f ArangeRootChildren( container_node* Node )
{
  absolute_size_attribute* Size = (absolute_size_attribute*) GetAttributePointer(Node, ATTRIBUTE_ABS_SIZE);

  container_node* Border1 = Node->FirstChild;
  container_node* Border2 = Border1->NextSibling;
  container_node* Border3 = Border2->NextSibling;
  container_node* Border4 = Border3->NextSibling;
  r32 BorderWidth = 0.007;
  Border1->Region = Rect2f(0,                         0,                        BorderWidth, Size->Height); // Left
  Border2->Region = Rect2f(Size->Width - BorderWidth, 0,                        BorderWidth, Size->Height); // Right
  Border3->Region = Rect2f(0,                         Size->Height-BorderWidth, Size->Width, BorderWidth);  // Top
  Border4->Region = Rect2f(0,                         0,                        Size->Width, BorderWidth);  // Bot

  container_node* Body = Border4->NextSibling;
  Body->Region = Rect2f(BorderWidth,BorderWidth,Size->Width - 2*BorderWidth,Size->Height - 2*BorderWidth);

  return Rect2f(0,0,Size->Width,Size->Height);
}

void SetSizeAndPositionsBottomUp(container_stack ContainerStack, container_node* Root)
{
  // Push Root
  Root->Region = {};
  Push(ContainerStack, Root);

  while(!IsEmpty(ContainerStack))
  {
    // Look at Top of stack
    container_stack_entry* Entry = Peak(ContainerStack);
    container_node* Node = Entry->Node;
    if(!Entry->Visited)
    {
      Entry->Visited = true;
      if(!IsLeaf(Node)) {
        // Add children in reverse order
        container_node* Child = GetFirstChild(Node);
        while(Child)
        {
          Child->Region = {};
          Push(ContainerStack, Child);
          Child = Next(Child);
        }
      }else{
        if(HasAttribute(Node, ATTRIBUTE_ABS_SIZE))
        {
          absolute_size_attribute* Size = (absolute_size_attribute*) GetAttributePointer(Node, ATTRIBUTE_ABS_SIZE);
          Node->Region.W = Size->Width;
          Node->Region.H = Size->Height;
        }
      }
    }else{
      rect2f NodeRegion = ArrangeChildPositions(Node);
      if(HasAttribute(Node, ATTRIBUTE_ABS_SIZE))
      {
        // Handle size of parent given that NodeRegion may not fit inside
        // For now just set to Size
        absolute_size_attribute* Size = (absolute_size_attribute*) GetAttributePointer(Node, ATTRIBUTE_ABS_SIZE);
        Node->Region.W = Size->Width == 0 ? NodeRegion.W : Size->Width;
        Node->Region.H = Size->Height == 0 ? NodeRegion.H : Size->Height;
      }else{
        Node->Region = NodeRegion;
      }

      Pop(ContainerStack);
    }
  }
}

void SetSizeAndPositionsTopDown(container_stack ContainerStack, container_node* Root)
{
  Push(ContainerStack, Root);
  while(!IsEmpty(ContainerStack))
  {
    // Pop new parent from Stack
    container_stack_entry Entry = Pop(ContainerStack);
    container_node* Node = Entry.Node;

    switch(Node->Type)
    {
      case container_type::Root: Node->Region = ArangeRootChildren( Node ); break;
      default:{
        // Here we want some scheme where we will, given Attribute Size, which can be ATTRIBUTE_REL_SIZE, ATTRIBUTE_ABS_SIZE, or ATTRIBUTE_SIZE or No size attribtue
        // Stack the children in a way that makes sense.
        // My thinking is that any
          // absolute size takes precident,
          // Next is Relative Sizes.
            // This assumes that the parent has a size.
          // Any child that have no size attribute but had its child size set from the bottom up stage keeps it's size
          // Any left over child that have no size attribute and have not had its size set bottom upp fills out any leftover space 

        if(HasAttribute(Node, ATTRIBUTE_REL_SIZE))
        {
          Assert(Node->Parent);
          Assert(Node->Parent->Region.W);
          Assert(Node->Parent->Region.H);
          rect2f ParentRegion = Node->Parent->Region;
          relative_size_attribute* Size = (relative_size_attribute*) GetAttributePointer(Node, ATTRIBUTE_REL_SIZE);
          Node->Region.W = Size->Width * ParentRegion.W;
          Node->Region.H = Size->Height* ParentRegion.H;
          if(!Node->PreviousSibling)
          {
            Node->Region.X = 0;
            Node->Region.Y = ParentRegion.H - Node->Region.H;
          }else{
            Assert(HasAttribute(Node->PreviousSibling, ATTRIBUTE_REL_SIZE))
            Node->Region.X = 0;
            Node->Region.Y = Node->PreviousSibling->Region.Y - Node->Region.H;
          }
        }
      }break;
    }

    
    container_node* Child = GetLastChild(Node);
    while(Child)
    {
      Push(ContainerStack, Child);
      Child = Previous(Child);
    }
  }

}

void UpdateRegionsOfContainerTree2(menu_interface* Interface, u32 ContainerCount, container_node* RootContainer)
{
  Assert(!RootContainer->Parent);
  SCOPED_TRANSIENT_ARENA;
  container_stack ContainerStack = NewContainerStack(GlobalTransientArena, ContainerCount);

  SetSizeAndPositionsBottomUp(ContainerStack, RootContainer);

  SetSizeAndPositionsTopDown(ContainerStack, RootContainer);
  

  Push(ContainerStack, RootContainer);
  while(!IsEmpty(ContainerStack))
  {
    // Pop new parent from Stack
    container_stack_entry Entry = Pop(ContainerStack);
    container_node* Node = Entry.Node;
    if(HasAttribute(Node, ATTRIBUTE_POSITION))
    {
      position_attribute* Position = (position_attribute*) GetAttributePointer(Node,ATTRIBUTE_POSITION);
      Node->Region.X = Position->X;
      Node->Region.Y = Position->Y;
    }
    // Align children
    AlignChildRegions(Node);
    container_node* Child = GetFirstChild(Node);
    while(Child)
    {
      Push(ContainerStack, Child);
      Child = Next(Child);
    }
  }

}

void UpdateRegionsOfContainerTree(menu_interface* Interface, u32 ContainerCount, container_node* RootContainer)
{
  Assert(!RootContainer->Parent);
  SCOPED_TRANSIENT_ARENA;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(GlobalTransientArena, ContainerCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = RootContainer;
  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    if(Parent->Functions.UpdateChildRegions)
    {
      CallFunctionPointer(Parent->Functions.UpdateChildRegions, Interface, Parent);
    }else{
      DefaultUpdateChildRegions(Interface, Parent);
    }
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Next(Child);
    }
  }
}
