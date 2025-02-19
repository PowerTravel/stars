#pragma once

#include "jwin/commons/types.h"
#include "container_node.h"

struct grid_node
{
  u32 Col;
  u32 Row;
  r32 TotalMarginX;
  r32 TotalMarginY;
  b32 Stack;
};

inline grid_node* GetGridNode(container_node* Container);
menu_functions GetGridFunctions();


MENU_UPDATE_CHILD_REGIONS( UpdateGridChildRegions );