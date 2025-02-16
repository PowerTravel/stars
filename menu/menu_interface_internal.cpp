#include "menu_interface_internal.h"
#include "tabbed_window/root_window.h"

u32 GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::Root:      return 0;
    case container_type::Border:    return sizeof(border_leaf);
    case container_type::Grid:      return sizeof(grid_node);
    case container_type::TabWindow: return sizeof(tab_window_node);
    case container_type::Tab:       return sizeof(tab_node);
    case container_type::Plugin:    return sizeof(plugin_node);
    default: INVALID_CODE_PATH;
  }
  return 0;
}

u32 GetAttributeSize(container_attribute Attribute)
{
  switch(Attribute)
  {
    case ATTRIBUTE_MERGE:       return sizeof(mergable_attribute);
    case ATTRIBUTE_COLOR:       return sizeof(color_attribute);
    case ATTRIBUTE_TEXT:        return sizeof(text_attribute);
    case ATTRIBUTE_SIZE:        return sizeof(size_attribute);
    case ATTRIBUTE_MENU_EVENT_HANDLE: return sizeof(menu_event_handle_attribtue);
    case ATTRIBUTE_TEXTURE:     return sizeof(texture_attribute);
    default: INVALID_CODE_PATH;
  }
  return 0; 
}


void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  Assert(!(Node->Attributes & AttributeType));

  Node->Attributes = Node->Attributes | AttributeType;
  menu_attribute_header** HeaderPtr = &Node->FirstAttribute;
  while(*HeaderPtr)
  {
    menu_attribute_header* Header = *HeaderPtr;
    HeaderPtr = &(Header->Next);
  }

  u32 TotalSize = sizeof(menu_attribute_header) + GetAttributeSize(AttributeType);
  menu_attribute_header* Attr =  (menu_attribute_header*) Allocate(&Interface->LinkedMemory, TotalSize);
  Attr->Type = AttributeType;
  *HeaderPtr = Attr;
  void* Result = (void*)(((u8*)Attr) + sizeof(menu_attribute_header));
  return Result;
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
