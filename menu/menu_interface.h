#pragma once

// TODO: + Sätt x-knapp (stänga) till tabs
//       + Sätt Check-mark i dropdown menyn som visar vilka fönster som är öppna
//       + Highlighta den aktiva tabben
//       + Möjlighet att spara/ladda fönster-layout (behöver serialiseras på något vis)
//       + Extrahera interface till en egen mapp där olika "logiska"-element får sin egen fil. En fil för radio-button, en för scroll window etc etc
//       + ListWidget behöver en scrollfunktion

#include "containers/linked_memory.h"
#include "platform/jwin_platform_input.h"

enum class container_type
{
  None,
  Root,
  Border,
  Split,
  Grid,
  Plugin,
  TabWindow,
  DropDown,
  Tab
};

enum container_attribute
{
  ATTRIBUTE_NONE = 0x0,
  ATTRIBUTE_MERGE = 0x1,
  ATTRIBUTE_COLOR = 0x2,
  ATTRIBUTE_TEXT = 0x4,
  ATTRIBUTE_SIZE = 0x8,
  ATTRIBUTE_MENU_EVENT_HANDLE = 0x10,
  ATTRIBUTE_TEXTURE = 0x20
};


const c8* ToString(container_type Type)
{
  switch(Type)
  {
    case container_type::None: return "None";
    case container_type::Root: return "Root";
    case container_type::Border: return "Border";
    case container_type::Split: return "Split";
    case container_type::Grid: return "Grid";
    case container_type::Plugin: return "Plugin";
    case container_type::TabWindow: return "TabWindow";
    case container_type::Tab: return "Tab";
    case container_type::DropDown: return "DropDown";
  }
  return "";
};

const c8* ToString(u32 Type)
{
  switch(Type)
  {
    case ATTRIBUTE_MERGE: return "Merge";
    case ATTRIBUTE_COLOR: return "Color";
    case ATTRIBUTE_TEXT: return "Text";
    case ATTRIBUTE_SIZE: return "Size";
    case ATTRIBUTE_MENU_EVENT_HANDLE: return "Event";
    case ATTRIBUTE_TEXTURE: return "Texture";
  }
  return "";
};

struct container_node;
struct menu_interface;
struct menu_tree;

menu_interface* CreateMenuInterface(memory_arena* Arena, midx MaxMemSize);
void UpdateAndRenderMenuInterface(jwin::device_input* DeviceInput, menu_interface* Interface);
container_node* NewContainer(menu_interface* Interface, container_type Type = container_type::None);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* NewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
container_node* ConnectNodeToFront(container_node* Parent, container_node* NewNode);
container_node* ConnectNodeToBack(container_node* Parent, container_node* NewNode);
void DisconnectNode(container_node* Node);

container_node* CreateBorderNode(menu_interface* Interface, b32 Vertical=false, r32 Position = 0.5f,  v4 Color =  V4(0,0,0.4,1));

struct update_args;

#define MENU_UPDATE_FUNCTION(name) b32 name( menu_interface* Interface, container_node* CallerNode, void* Data )
typedef MENU_UPDATE_FUNCTION( update_function );

struct update_args
{
  menu_interface* Interface;
  container_node* Caller;
  void* Data;
  b32 InUse;
  update_function** Function;
};

#define MENU_UPDATE_CHILD_REGIONS(name) void name(container_node* Parent)
typedef MENU_UPDATE_CHILD_REGIONS( menu_get_region );

#define MENU_DRAW(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_DRAW( menu_draw );

b32 HasAttribute(container_node* Node, container_attribute Attri);
u8* GetAttributePointer(container_node* Node, container_attribute Attri);

struct menu_functions
{
  menu_get_region** UpdateChildRegions;
  menu_draw** Draw;
};

struct menu_attribute_header
{
  container_attribute Type;
  menu_attribute_header* Next;
};

struct container_node
{
  container_type Type;
  u32 Attributes;
  menu_attribute_header* FirstAttribute;

  // Tree Links (Menu Structure)
  u32 Depth;
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;
  container_node* PreviousSibling;

  rect2f Region;
  menu_functions Functions;
};

u32 GetChildCount(container_node* Node);

struct root_node
{
  r32 HeaderSize;
  r32 FooterSize;
};

struct hbf_node
{
  r32 HeaderSize;
  r32 FooterSize;
};

struct border_leaf
{
  b32 Vertical;
  r32 Position;
  r32 Thickness;
};

struct grid_node
{
  u32 Col;
  u32 Row;
  r32 TotalMarginX;
  r32 TotalMarginY;
  b32 Stack;
};

struct tab_window_node
{
  r32 HeaderSize;
};

struct tab_node
{
  container_node* Payload;
};

struct plugin_node
{
  char Title[256];
  container_node* Tab;
  v4 Color;
};

struct drop_down_node{
  b32 Active;
};

enum class merge_zone
{
  // (0 through 4 indexes into merge_slot_attribute::MergeZone)
  // (The rest do not)
  CENTER,      // Tab (idx 0)
  LEFT,        // Left Split (idx 1)
  RIGHT,       // Left Split (idx 2)
  TOP,         // Left Split (idx 3)
  BOT,         // Left Split (idx 4)
  HIGHLIGHTED, // We are mousing over a mergable window and want to draw it
  NONE         // No mergable window present
};

struct mergable_attribute
{
  merge_zone HotMergeZone;
  rect2f MergeZone[5];
};

#define MENU_EVENT_CALLBACK(name) void name( menu_interface* Interface, container_node* CallerNode, void* Data)
typedef MENU_EVENT_CALLBACK( menu_event_callback );

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


struct texture_attribute
{
  // Handle for a texture
  u32 Handle;
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

#define MENU_LOSING_FOCUS(name) void name(struct menu_interface* Interface, struct menu_tree* Menu)
typedef MENU_LOSING_FOCUS( menu_losing_focus );
#define MENU_GAINING_FOCUS(name) void name(struct menu_interface* Interface, struct menu_tree* Menu)
typedef MENU_GAINING_FOCUS( menu_gaining_focus );

struct menu_tree
{
  b32 Visible;

  u32 NodeCount;
  u32 Depth;
  container_node* Root;

  u32 HotLeafCount;
  u32 NewLeafOffset;  
  container_node* HotLeafs[64];
  u32 RemovedHotLeafCount;
  container_node* RemovedHotLeafs[64];

  menu_tree* Next;
  menu_tree* Previous;

  menu_losing_focus** LosingFocus;
  menu_gaining_focus** GainingFocus;
};

MENU_LOSING_FOCUS(DefaultLosingFocus)
{

}

MENU_GAINING_FOCUS(DefaultGainingFocus)
{

}

enum class menu_event_type
{
  MouseUp,
  MouseDown,
  MouseEnter,
  MouseExit,
};

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

struct menu_interface
{
  b32 MenuVisible;

  u32 PermanentWindowCount = 0;
  container_node* PermanentWindows[32];

  menu_tree* MenuInFocus;
  menu_tree* SpawningWindow; // This SpawningWinow is a root window that all new windows gets attached to.
  menu_tree* MenuBar;
  menu_tree MenuSentinel;

  u32 MainDropDownMenuCount;
  menu_tree* MainDropDownMenus[8];

  linked_memory LinkedMemory;

  v2 MousePos;
  v2 PreviousMousePos;
  jwin::binary_signal_state MouseLeftButton;
  jwin::binary_signal_state TAB;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;

  r32 BorderSize;
  r32 HeaderSize;
  r32 MinSize;

  v4 MenuColor;
  v4 TextColor;
  r32 HeaderFontSize;
  r32 BodyFontSize;

  menu_event EventSentinel;
  u32 MenuEventCallbackCount;
  menu_event MenuEventCallbacks[64];

  update_args UpdateQueue[64];
};


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

inline drop_down_node* GetDropDownNode(container_node* Container)
{
  Assert(Container->Type == container_type::DropDown);
  drop_down_node* Result = (drop_down_node*) GetContainerPayload(Container);
  return Result;
}

container_node* Next( container_node* Node );
container_node* Previous( container_node* Node );

menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f Region);
container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos = 0.5);
container_node* CreatePlugin(menu_interface* Interface, c8* HeaderName, v4 HeaderColor, container_node* BodyNode);
menu_tree* RegisterMenu(menu_interface* Interface, const c8* Name);
void RegisterWindow(menu_interface* Interface, menu_tree* DropDownMenu, container_node* Plugin);
void ToggleWindow(menu_interface* Interface, char* WindowName);

void _RegisterMenuEvent(menu_interface* Interface, menu_event_type EventType, container_node* CallerNode, void* Data, menu_event_callback** Callback,  menu_event_callback** OnDelete);
#define RegisterMenuEvent(Interface, EventType, CallerNode, Data, Callback, OnDeleteCallback ) \
    _RegisterMenuEvent(Interface, EventType, CallerNode, (void*) Data,                         \
    DeclareFunction(menu_event_callback, Callback), OnDeleteCallback)
