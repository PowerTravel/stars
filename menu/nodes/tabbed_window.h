#pragma once

struct container_node;
struct menu_interface;

// Tab Window looks like
// TabWindow
//   None
//     Grid
//       Tab (1) -> Plugin (1)
//       Tab (2) -> Plugin (2)
//       ...
//   Plugin


struct tab_node
{
  container_node* Payload;
};

struct plugin_node
{
  char Title[128];
  container_node* Tab;
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
  rect2f CachedRegion;
};


// A tabbed window is the window that holds plugins. A tabbed window can hold 1 to many plugins in tabs
container_node* CreateTabWindow(menu_interface* Interface);
container_node* CreateTab(menu_interface* Interface, container_node* Plugin);
container_node* CreatePlugin(menu_interface* Interface, menu_tree* WindowsDropDownMenu, c8* HeaderName, container_node* BodyNode);


inline plugin_node* GetPluginNode(container_node* Container);
inline tab_window_node* GetTabWindowNode(container_node* Container);
inline tab_node* GetTabNode(container_node* Container);
menu_functions GetTabWindowFunctions();
container_node* GetPluginFromTab(container_node* Tab);
void SplitTabToNewWindow(menu_interface* Interface, container_node* Tab, rect2f TabbedWindowRegion);

// Takes a Tab and return the TabWindow it's connected to
inline container_node* GetTabWindowFromTab(container_node* Tab);
// Removes the tab from the tab window it's attached to
container_node* RemoveTabFromTabWindow(container_node* Tab);

// Takes a tab window and returns the body
inline container_node* GetTabBodyFromTabWindow(container_node* TabWindow);