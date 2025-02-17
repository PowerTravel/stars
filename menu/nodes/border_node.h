#pragma once

#include "container_node.h"

enum class border_type {
  LEFT,
  RIGHT,
  BOTTOM,
  TOP,
  SPLIT_VERTICAL,
  SPLIT_HORIZONTAL
};

struct border_leaf
{
  border_type Type;
  r32 Position;
  r32 Thickness;
  b32 Active;
};

inline border_leaf* GetBorderNode(container_node* Container);
container_node* CreateBorderNode(menu_interface* Interface, v4 Color);

void SetRootWindowRegion(menu_tree* MenuTree, rect2f Region);
void UpdateBorderPosition(container_node* BorderNode, v2 NewPosition, rect2f MinimumWindowSize, rect2f MaximumWindowSize);
