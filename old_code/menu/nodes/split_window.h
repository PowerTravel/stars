#pragma once

menu_functions GetSplitFunctions();
container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos);

struct split_windows {
  container_node* Border;
  container_node* FirstWindow;
  container_node* SecondWindow;
};

split_windows GetSplitWindows(container_node* SplitWindowContainer);

MENU_UPDATE_CHILD_REGIONS(UpdateSplitChildRegions);