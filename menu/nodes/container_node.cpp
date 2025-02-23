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
    case container_type::Root:
    case container_type::MainWindow: return 0;
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

MENU_UPDATE_CHILD_REGIONS(UpdateChildRegions)
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
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, UpdateChildRegions);
  Result.Draw = 0;
  return Result;
}

menu_functions GetMenuFunction(container_type Type)
{
  switch(Type)
  {   
    case container_type::None:       return GetDefaultFunctions();
    case container_type::MainWindow: return GetDefaultFunctions();
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


u32 GetIntersectingNodes(u32 NodeCount, container_node* Container, v2 MousePos, u32 MaxCount, container_node** Result)
{
  u32 StackElementSize = sizeof(container_node*);
  SCOPED_TRANSIENT_ARENA;
  u32 StackByteSize = NodeCount * StackElementSize;

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
  return IntersectingLeafCount;
}

void UpdateRegionsOfContainerTree(menu_interface* Interface, u32 ContainerCount, container_node* RootContainer)
{
  Assert(!RootContainer->Parent);
  SCOPED_TRANSIENT_ARENA;

  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = ContainerCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(GlobalTransientArena, ContainerCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = RootContainer;

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
}
