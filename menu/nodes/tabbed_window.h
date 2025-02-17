#pragma once

struct container_node;
struct menu_interface;

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


// A tabbed window is the window that holds plugins. A tabbed window can hold 1 to many plugins in tabs
container_node* CreateTabWindow(menu_interface* Interface);



inline plugin_node* GetPluginNode(container_node* Container);
inline tab_window_node* GetTabWindowNode(container_node* Container);
inline tab_node* GetTabNode(container_node* Container);

inline internal container_node* GetTabGridFromWindow(container_node* TabbedWindow);
MENU_EVENT_CALLBACK(TabWindowHeaderMouseDown);


menu_functions GetTabWindowFunctions();
b32 TabDrag(menu_interface* Interface, container_node* Tab);
container_node* CreateTab(menu_interface* Interface, container_node* Plugin);