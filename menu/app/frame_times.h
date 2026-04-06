#pragma once

#include "menu/imgui/border_window.h"
#include "debug.h"

namespace imgui { 
namespace app {

struct frame_times {
  imgui_bordered_window BorderWindow;
  debug_state* DebugState;
};

frame_times* CreateFrameTimes(memory_arena* Arena, debug_state* DebugState);
void DrawFrameTimes(menu* AppImgui);
} // namespace app
} // namespace imgui

