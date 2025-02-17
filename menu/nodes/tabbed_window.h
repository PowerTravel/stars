#pragma once

// A tabbed window is the window that holds plugins. A tabbed window can hold 1 to many plugins in tabs
container_node* CreateTabWindow(menu_interface* Interface);

inline internal container_node* GetTabGridFromWindow(container_node* TabbedWindow);
MENU_EVENT_CALLBACK(TabWindowHeaderMouseDown);


menu_functions GetTabWindowFunctions();
b32 TabDrag(menu_interface* Interface, container_node* Tab);