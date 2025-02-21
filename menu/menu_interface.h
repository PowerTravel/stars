#pragma once

// TODO: + Sätt x-knapp (stänga) till tabs
//       + Sätt Check-mark i dropdown menyn som visar vilka fönster som är öppna
//       + Möjlighet att spara/ladda fönster-layout (behöver serialiseras på något vis)
//       + Extrahera interface till en egen mapp där olika "logiska"-element får sin egen fil. En fil för radio-button, en för scroll window etc etc
//       + ListWidget behöver en scrollfunktion

#include "containers/linked_memory.h"
#include "platform/jwin_platform_input.h"
#include "internal/menu_interface_internal.h"
#include "utility.h"

struct container_node;
struct menu_interface;
struct menu_tree;

menu_interface* CreateMenuInterface(memory_arena* Arena, jwin::keyboard_input* Keyboard, midx MaxMemSize, r32 AspectRatio);
void UpdateAndRenderMenuInterface(jwin::device_input* DeviceInput, menu_interface* Interface);
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

  jwin::keyboard_input* Keyboard;

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

  update_function_arguments UpdateQueue[64];
};
 
container_node* GetTabWindowFromOtherMenu(menu_interface* Interface, container_node* Node);

container_node* CreatePlugin(menu_interface* Interface, menu_tree* WindowsDropDownMenu, c8* HeaderName, v4 HeaderColor, container_node* BodyNode);
menu_tree* RegisterMenu(menu_interface* Interface, const c8* Name);
void ToggleWindow(menu_interface* Interface, char* WindowName);
void SetFocusWindow(menu_interface* Interface, menu_tree* Menu);
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


void SetSelectedPluginTab(menu_interface* Interface, container_node * PluginTab);

menu_tree* GetMenu(menu_interface* Interface, container_node* Node);

void _PushToUpdateQueue(menu_interface* Interface, container_node* Caller, update_function** UpdateFunction, void* Data, b32 FreeData);
#define PushToUpdateQueue(Interface, Caller, FunctionName, Data, FreeData) _PushToUpdateQueue(Interface, Caller, DeclareFunction(update_function, FunctionName), (void*) Data, FreeData)

