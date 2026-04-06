#pragma once

#include "menu/imgui/imgui.h"
#include "menu/imgui/scrollbar.h"

namespace imgui { 
namespace app {


struct styling {
  text_styling HeaderTextStykling;
  text_styling BodyTextStyling;
  region_styling HeaderStyling;
  region_styling PlainButtonStyling;
  region_styling BorderStyling;
  region_styling ListStylingEven;
  region_styling ListStylingOdd;
};

struct menu_bar {
  struct item {
    c8 Name[128];
    id ButtonID;
    cmn::vector_p<item> Items;
  };
  rect2f HeaderBarRegion;
  cmn::vector_p<item> TopItems;
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

  struct frame_times* FrameTimes;
  b32 FrameTimesActive;

  styling DefaultStyling;
};

menu CreateAppllicationMenu(memory_arena* Arena, context* ImguiContext, u32 ColorCount);

void DoMenu();

} // namespace app
} // namespace imgui