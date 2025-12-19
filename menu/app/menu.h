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

  cmn::vector<item> TopItems;
  b32 Visible;
};

menu_bar* CreateMenuBar(memory_arena* Arena);
bool ToggleTopMenu();

struct menu {
  menu_bar* MenuBar;

  b32 ColorListActive;
  struct color_list* ColorList;
  b32 EntityListActive;
  struct entity_list* EntityList;
};

menu CreateAppllicationMenu(memory_arena* Arena, context* ImguiContext, u32 ColorCount);

void DoMenu();

} // namespace app
} // namespace imgui