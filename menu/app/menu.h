#pragma once

#include "menu/imgui/imgui.h"
#include "menu/imgui/scrollbar.h"

namespace imgui { 
namespace app {

struct menu_bar {
  struct item {
    c8 Name[128];
    id ButtonID;
    cmn::vector<item> Items;
  };
  rect2f HeaderBarRegion;
  cmn::vector<item> TopItems;
};

menu_bar* CreateMenuBar(memory_arena* Arena);
bool ToggleMenu();

struct menu {

  rect2f EnclosingRegion;

  menu_bar* MenuBar;
  b32 MenuBarActive;

  struct color_list* ColorList;
  b32 ColorListActive;

  struct entity_list* EntityList;
  b32 EntityListActive;
};

menu CreateAppllicationMenu(memory_arena* Arena, context* ImguiContext, u32 ColorCount);

void DoMenu();

} // namespace app
} // namespace imgui