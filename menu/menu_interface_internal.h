#pragma once
// This file contains internal menu functions. Functiouns used to create menus, not necesarilly needed by the end menu user
#include "menu/menu_interface.h"


rect2f GetActiveMenuRegion(menu_interface* Interface);
u32 GetAttributeSize(container_attribute Attribute);
u32 GetContainerPayloadSize(container_type Type);
void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType);

menu_tree* GetMenu(menu_interface* Interface, container_node* Node);

#define PushToUpdateQueue(Interface, Caller, FunctionName, Data, FreeData) _PushToUpdateQueue(Interface, Caller, DeclareFunction(update_function, FunctionName), (void*) Data, FreeData)

inline u8* GetContainerPayload( container_node* Container )
{
  u8* Result = (u8*)(Container+1);
  return Result;
}

inline root_node* GetRootNode(container_node* Container)
{
  Assert(Container->Type == container_type::Root);
  root_node* Result = (root_node*) GetContainerPayload(Container);
  return Result;
}
inline border_leaf* GetBorderNode(container_node* Container)
{
  Assert(Container->Type == container_type::Border);
  border_leaf* Result = (border_leaf*) GetContainerPayload(Container);
  return Result;
}
inline grid_node* GetGridNode(container_node* Container)
{
  Assert(Container->Type == container_type::Grid);
  grid_node* Result = (grid_node*) GetContainerPayload(Container);
  return Result;
}
inline plugin_node* GetPluginNode(container_node* Container)
{
  Assert(Container->Type == container_type::Plugin);
  plugin_node* Result = (plugin_node*) GetContainerPayload(Container);
  return Result;
}
inline tab_window_node* GetTabWindowNode(container_node* Container)
{
  Assert(Container->Type == container_type::TabWindow);
  tab_window_node* Result = (tab_window_node*) GetContainerPayload(Container);
  return Result;
}
inline tab_node* GetTabNode(container_node* Container)
{
  Assert(Container->Type == container_type::Tab);
  tab_node* Result = (tab_node*) GetContainerPayload(Container);
  return Result;
}
