#include "container_node.h"
#include "border_node.h"
#include "grid_window.h"
#include "main_window.h"
#include "root_window.h"
#include "tabbed_window.h"
#include "split_window.h"


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

menu_functions GetDefaultFunctions()
{
  menu_functions Result = {};
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, UpdateChildRegions);
  Result.Draw = 0;
  return Result;
}

menu_functions GetMainHeaderFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, MainHeaderUpdate);
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


void DeleteContainer( menu_interface* Interface, container_node* Node)
{
  CancelAllUpdateFunctions(Interface, Node );
  ClearMenuEvents(Interface, Node);
  DeleteAllAttributes(Interface, Node);
  FreeMemory(&Interface->LinkedMemory, (void*) Node);
}
