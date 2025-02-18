#pragma once

#include "jwin/commons/types.h"
struct menu_interface;

struct mouse_position_in_window{
  v2 MousePos;
  v2 RelativeWindow;
};

menu_tree* CreateNewDropDownMenuItem(menu_interface* Interface, const c8* Name);
void AddPlugintoMainMenu(menu_interface* Interface, menu_tree* DropDownMenu, container_node* Plugin);
menu_tree* CreateMainWindow(menu_interface* Interface);
menu_functions GetMainWindowFunctions();
