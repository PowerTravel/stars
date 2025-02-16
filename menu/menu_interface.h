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

menu_interface* CreateMenuInterface(memory_arena* Arena, midx MaxMemSize, r32 AspectRatio);
void UpdateAndRenderMenuInterface(jwin::device_input* DeviceInput, menu_interface* Interface);
container_node* NewContainer(menu_interface* Interface, container_type Type = container_type::None);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* NewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
container_node* ConnectNodeToFront(container_node* Parent, container_node* NewNode);
container_node* ConnectNodeToBack(container_node* Parent, container_node* NewNode);
void DisconnectNode(container_node* Node);

container_node* CreateBorderNode(menu_interface* Interface, v4 Color);

struct update_args;

#define MENU_UPDATE_FUNCTION(name) b32 name( menu_interface* Interface, container_node* CallerNode, void* Data )
typedef MENU_UPDATE_FUNCTION( update_function );

struct update_args
{
  menu_interface* Interface;
  container_node* Caller;
  void* Data;
  b32 InUse;
  b32 FreeDataWhenComplete;
  update_function** UpdateFunction;
};

#define MENU_UPDATE_CHILD_REGIONS(name) void name(menu_interface* Interface, container_node* Parent)
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

enum class border_type {
  LEFT,
  RIGHT,
  BOTTOM,
  TOP,
  SPLIT_VERTICAL,
  SPLIT_HORIZONTAL
};

struct border_leaf
{
  border_type Type;
  r32 Position;
  r32 Thickness;
  b32 Active;
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

  b32 Maximized;
  rect2f CachedRegion;

  menu_tree* Next;
  menu_tree* Previous;

  menu_losing_focus** LosingFocus;
  menu_gaining_focus** GainingFocus;
};

// Holds the shape of the pluin containers, or rather the TabWindows holding the Plugins
// Leaf nodes holds the actual plugins
struct menu_layout_node {

  c8* Name[64];
  b32 Vertical;
  r32 ChildBorderPosition; 

  menu_layout_node* Parent;
  menu_layout_node* FirstChild;
  menu_layout_node* SecondChild;
};

struct menu_layout {

  menu_layout_node* Root;
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

struct mouse_position_in_window{
  v2 MousePos;
  v2 RelativeWindow;
};

struct menu_interface
{
  b32 MenuVisible;

  u32 PermanentWindowCount = 0;
  container_node* PermanentWindows[32];

  // Menu In Focus means that the mouse is hovering over a menu_tree which is on top
  menu_tree* MenuInFocus;

  // Selected Plugin is the pluginwindow which was last clicked;
  container_node* SelectedPlugin;

  menu_tree* SpawningWindow; // This SpawningWinow is a root window that all new windows gets attached to.
  menu_tree* MenuBar;
  menu_tree MenuSentinel;

  b32 MainMenuBarActive;
  u32 MainMenuTabCount;
  menu_tree* MainMenuTabs[8];

  linked_memory LinkedMemory;

  v2 MousePos;
  v2 PreviousMousePos;
  b32 DoubleKlick;
  jwin::binary_signal_state MouseLeftButton;
  jwin::binary_signal_state TAB;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;
  r32 Time;
  r32 DeltaTime;
  r32 MouseLeftButtonPushTime;
  r32 MouseLeftButtonReleaseTime;
  r32 DoubleKlickTime; // Max time between klicks to register it as a double klick

  r32 BorderSize;
  v4 BorderColor;
  r32 HeaderSize;
  r32 MinSize; // Note: We maybe want to set this per container_node and have some ATTRIBUTE which is the "Ground truth window minimum size" for certain containers
               //       Could this attribute be the Size attribute? Or does that attribute do too many different things then?

  v4 MenuColor;
  v4 TextColor;
  r32 HeaderFontSize;
  r32 BodyFontSize;

  menu_event EventSentinel;
  u32 MenuEventCallbackCount;
  menu_event MenuEventCallbacks[64];

  update_args UpdateQueue[64];
};

menu_tree* BuildMenuTree(menu_interface* Interface, menu_layout* MenuLayout);


container_node* Next( container_node* Node );
container_node* Previous( container_node* Node );

menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f Region);
container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos = 0.5);
container_node* CreatePlugin(menu_interface* Interface, menu_tree* WindowsDropDownMenu, c8* HeaderName, v4 HeaderColor, container_node* BodyNode);
menu_tree* RegisterMenu(menu_interface* Interface, const c8* Name);
void ToggleWindow(menu_interface* Interface, char* WindowName);

b32 IsPluginSelected(menu_interface* Interface, container_node* Container);

void _RegisterMenuEvent(menu_interface* Interface, menu_event_type EventType, container_node* CallerNode, void* Data, menu_event_callback** Callback,  menu_event_callback** OnDelete);
#define RegisterMenuEvent(Interface, EventType, CallerNode, Data, Callback, OnDeleteCallback ) \
    _RegisterMenuEvent(Interface, EventType, CallerNode, (void*) Data,                         \
    DeclareFunction(menu_event_callback, Callback), OnDeleteCallback)

r32 GetAspectRatio(menu_interface* Interface)
{
  rect2f WindowRegion = Interface->MenuBar->Root->Region;
  return WindowRegion.W / WindowRegion.H;
}