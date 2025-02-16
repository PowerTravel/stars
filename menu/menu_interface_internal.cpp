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
