#pragma once

#include "menu_interface.h"

struct main_window_node{
  r32 HeaderSize;
};

inline main_window_node* GetMainWindowNode(container_node* Container)
{
  Assert(Container->Type == container_type::MainWindow);
  main_window_node* Result = (main_window_node*) GetContainerPayload(Container);
  return Result;
}

menu_tree* CreateMainWindow(menu_interface* Interface);
menu_functions GetMainWindowFunctions();
