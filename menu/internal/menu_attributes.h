#pragma once
#include "jwin/commons/types.h"
#include "jwin/math/vector_math.h"

struct color_attribute
{
  v4 Color;
  v4 RestingColor;
  v4 HighlightedColor;
};

struct text_attribute
{
  c8 Text[256];
  u32 FontSize;
  v4 Color;
};

enum class menu_region_alignment
{
  CENTER,
  LEFT,
  RIGHT,
  TOP,
  BOT
};

struct menu_event_handle_attribtue
{
  u32 HandleCount;
  u32 Handles[16]; // Can atm "only" have 16 events per node
};

enum class menu_size_type
{
  RELATIVE_,
  ABSOLUTE_,
};

struct container_size_t
{
  menu_size_type Type;
  r32 Value;
};

inline container_size_t
ContainerSizeT(menu_size_type T, r32 Val)
{
  container_size_t Result{};
  Result.Type = T;
  Result.Value = Val;
  return Result;
}

struct size_attribute
{
  container_size_t Width;
  container_size_t Height;
  container_size_t LeftOffset;
  container_size_t TopOffset;
  menu_region_alignment XAlignment;
  menu_region_alignment YAlignment;
};

enum class menu_event_type
{
  MouseUp,
  MouseDown,
  MouseEnter,
  MouseExit,
};

struct texture_attribute
{
  // Handle for a texture
  u32 Handle;
};


struct menu_interface;
struct container_node;

#define MENU_EVENT_CALLBACK(name) void name( menu_interface* Interface, container_node* CallerNode, void* Data)
typedef MENU_EVENT_CALLBACK( menu_event_callback );

struct menu_event
{
  u32 Index;
  b32 Active;
  container_node* CallerNode;
  menu_event_type EventType;
  menu_event_callback** Callback;
  menu_event_callback** OnDelete; // Can be used to do cleanup on Data if the node requires it.
  void* Data;
  menu_event* Next;
  menu_event* Previous;
};


enum container_attribute
{
  ATTRIBUTE_NONE = 0x0,
  ATTRIBUTE_COLOR = 0x1,
  ATTRIBUTE_TEXT = 0x2,
  ATTRIBUTE_SIZE = 0x4,
  ATTRIBUTE_MENU_EVENT_HANDLE = 0x8,
  ATTRIBUTE_TEXTURE = 0x10
};

const c8* ToString(u32 Type)
{
  switch(Type)
  {
    case ATTRIBUTE_COLOR: return "Color";
    case ATTRIBUTE_TEXT: return "Text";
    case ATTRIBUTE_SIZE: return "Size";
    case ATTRIBUTE_MENU_EVENT_HANDLE: return "Event";
    case ATTRIBUTE_TEXTURE: return "Texture";
  }
  return "";
};


struct menu_attribute_header
{
  container_attribute Type;
  menu_attribute_header* Next;
};

inline b32 HasAttribute(container_node* Node, container_attribute Attri);
u8* GetAttributePointer(container_node* Node, container_attribute Attri);
void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType);
void ClearMenuEvents(menu_interface* Interface, container_node* Node);
menu_event* GetMenuEvent(menu_interface* Interface, u32 Handle);
void DeleteAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType);
void DeleteAllAttributes(menu_interface* Interface, container_node* Node);

u32 GetAttributeSize(container_attribute Attribute)
{
  switch(Attribute)
  {
    case ATTRIBUTE_COLOR:       return sizeof(color_attribute);
    case ATTRIBUTE_TEXT:        return sizeof(text_attribute);
    case ATTRIBUTE_SIZE:        return sizeof(size_attribute);
    case ATTRIBUTE_MENU_EVENT_HANDLE: return sizeof(menu_event_handle_attribtue);
    case ATTRIBUTE_TEXTURE:     return sizeof(texture_attribute);
    default: INVALID_CODE_PATH;
  }
  return 0; 
}


