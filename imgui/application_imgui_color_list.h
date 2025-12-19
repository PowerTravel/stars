#pragma once

#include "imgui.h"

namespace imgui { 

struct menu_color_list {
  imgui_id* ImguiIDs; // ColorListIndeces
  s32* ColorIDs;      // Mapping IDS from Colors in the ColorTable to list indeces.
  imgui_id TextInputID;
  imgui_text_input_buffer TextInputBuffer;
  imgui_bordered_window   BorderWindow;
  imgui_vertical_scrollbar VerticalScrollbar;
  u32 SelectedRow;
};

menu_color_list* CreateColorList(memory_arena* Arena, size_t ColorCount){

  u32 InputLen = 512;
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);

  menu_color_list* Result   = PushStruct(Arena, menu_color_list);
  Result->TextInputBuffer   = ImguiNewTextInputBuffer(InputLen, PushArray(Arena, InputLen, utf8_byte));
  Result->TextInputID       = NewButtonID();
  Result->ImguiIDs          = PushArray(Arena, ColorCount, imgui_id);
  Result->ColorIDs          = PushArray(Arena, ColorCount, s32);
  Result->VerticalScrollbar = CreateVerticalScrollbar();
  Result->BorderWindow      = ImguiBorderedWindow(Rect2f(V2(0.1,0.25), V2(0.1,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  Result->SelectedRow       = 0;

  for (int i = 0; i < ColorCount; ++i)
  {
    Result->ImguiIDs[i] = NewButtonID();
  }
  return Result;
}
void DrawColorList(application_imgui* AppImgui);

} // namespace imgui 