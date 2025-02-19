#pragma once

menu_functions GetSplitFunctions();
container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos);
MENU_UPDATE_CHILD_REGIONS(UpdateSplitChildRegions);