#pragma once


union root_border_collection {
  struct {
    container_node* Left;
    container_node* Right;
    container_node* Bot;
    container_node* Top;
  };
  container_node* E[4];
};

root_border_collection GetRoorBorders(container_node* RootContainer);
void SetBorderData(container_node* Border, r32 Thickness, r32 Position, border_type Type);
inline container_node* GetRoot(container_node* Node);
inline container_node* GetBodyFromRoot(container_node* RootWindow);

mouse_position_in_window GetPositionInRootWindow(menu_interface* Interface, container_node* Node);
void InitiateRootWindowDrag(menu_interface* Interface, container_node* Node);

void ToggleMaximizeWindow(menu_interface* Interface, menu_tree* Menu, container_node* TabHeader);

MENU_UPDATE_CHILD_REGIONS(RootUpdateChildRegions);