#pragma once

struct container_node;

struct root_node
{
  r32 HeaderSize;
  r32 FooterSize;
  b32 Maximized;
  rect2f CachedRegion;
};

union root_border_collection {
  struct {
    container_node* Left;
    container_node* Right;
    container_node* Bot;
    container_node* Top;
  };
  container_node* E[4];
};

container_node* CreateRootContainer(menu_interface* Interface, container_node* BodyContainer, rect2f RootRegion);
menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f MaxRegion);

root_border_collection GetRoorBorders(container_node* RootContainer);
void SetBorderData(container_node* Border, r32 Thickness, r32 Position, border_type Type);
inline container_node* GetRoot(container_node* Node);
inline container_node* GetBodyFromRoot(container_node* RootWindow);
inline root_node* GetRootNode(container_node* Container);

mouse_position_in_window GetPositionInRootWindow(menu_interface* Interface, container_node* Node);
void InitiateRootWindowDrag(menu_interface* Interface, container_node* Node);

void ToggleMaximizeWindow(menu_interface* Interface, menu_tree* Menu, container_node* TabHeader);

MENU_UPDATE_CHILD_REGIONS(RootUpdateChildRegions);
MENU_UPDATE_FUNCTION(RootBorderDragUpdate);
MENU_EVENT_CALLBACK(InitiateBorderDrag);

menu_functions GetRootMenuFunctions();
