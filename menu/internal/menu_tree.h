#pragma once

#define MENU_LOSING_FOCUS(name) void name(struct menu_interface* Interface, struct menu_tree* Menu)
typedef MENU_LOSING_FOCUS( menu_losing_focus );
#define MENU_GAINING_FOCUS(name) void name(struct menu_interface* Interface, struct menu_tree* Menu)
typedef MENU_GAINING_FOCUS( menu_gaining_focus );

struct draw_queue_entry{
  container_node* CallerNode;
  menu_draw** DrawFunction;
};

struct menu_tree
{
  b32 Visible;

  u32 NodeCount;
  u32 Depth;
  container_node* Root;

  u32 HotLeafCount;
  u32 NewLeafOffset;  
  container_node* HotLeafs[64];
  u32 RemovedHotLeafCount;
  container_node* RemovedHotLeafs[64];

  b32 Maximized;
  rect2f CachedRegion;

  u32 FinalRenderCount; 
  draw_queue_entry FinalRenderFunctions[64];

  menu_tree* Next;
  menu_tree* Previous;

  menu_losing_focus** LosingFocus;
  menu_gaining_focus** GainingFocus;
};
