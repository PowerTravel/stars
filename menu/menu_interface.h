#pragma once

// TODO: + Sätt x-knapp (stänga) till tabs
//       + Sätt Check-mark i dropdown menyn som visar vilka fönster som är öppna
//       + Highlighta den aktiva tabben
//       + Möjlighet att spara/ladda fönster-layout (behöver serialiseras på något vis)
//       + Extrahera interface till en egen mapp där olika "logiska"-element får sin egen fil. En fil för radio-button, en för scroll window etc etc
//       + ListWidget behöver en scrollfunktion

#include "containers/linked_memory.h"
#include "platform/jwin_platform_input.h"
#include "internal/menu_interface_internal.h"
#include "nodes/grid_window.h"
#include "nodes/border_node.h"



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

u32 GetChildCount(container_node* Node);

struct root_node
{
  r32 HeaderSize;
  r32 FooterSize;
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

struct tab_window_node
{
  r32 HeaderSize;
  merge_zone HotMergeZone;
  rect2f MergeZone[5];
};

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

void _PushToFinalDrawQueue(menu_tree* Menu, container_node* Node, menu_draw** Draw)
{
  Assert(Menu->FinalRenderCount < ArrayCount(Menu->FinalRenderFunctions));
  draw_queue_entry* Entry = &Menu->FinalRenderFunctions[Menu->FinalRenderCount++];
  Entry->DrawFunction = Draw;
  Entry->CallerNode = Node;

}
#define PushToFinalDrawQueue(Interface, Caller, FunctionName) _PushToFinalDrawQueue(GetMenu(Interface, Caller), Caller, DeclareFunction(menu_draw, FunctionName))

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


  r32 AspectRatio;
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
 
container_node* GetTabWindowFromOtherMenu(menu_interface* Interface, container_node* Node);

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
  return Interface->AspectRatio;
}

void UpdateMergableAttribute( menu_interface* Interface, container_node* Node );
void UpdateFocusWindow(menu_interface* Interface);


container_node* GetChildFromIndex(container_node* Parent, u32 ChildIndex);
u32 GetChildIndex(container_node* Node);
u32 GetChildCount(container_node* Node);
inline container_node* Next(container_node* Node);
inline container_node* Previous(container_node* Node);

rect2f GetActiveMenuRegion(menu_interface* Interface);
u32 GetAttributeSize(container_attribute Attribute);
u32 GetContainerPayloadSize(container_type Type);
void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType);

menu_tree* GetMenu(menu_interface* Interface, container_node* Node);

void _PushToUpdateQueue(menu_interface* Interface, container_node* Caller, update_function** UpdateFunction, void* Data, b32 FreeData);
#define PushToUpdateQueue(Interface, Caller, FunctionName, Data, FreeData) _PushToUpdateQueue(Interface, Caller, DeclareFunction(update_function, FunctionName), (void*) Data, FreeData)

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
