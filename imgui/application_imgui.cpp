#include "application_imgui.h"
#include "platform/jwin_platform_memory.h"
#include "application_imgui_entity_list.h"
#include "application_imgui_color_list.h"

namespace imgui { 
application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount) {
  application_imgui Result = {};

  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);

  Result.ColorListData = CreateColorList(Arena, ColorCount);

  Result.MenuEntityTree = PushStruct(Arena, menu_entity_tree);
  Result.MenuEntityTree->BorderWindow      = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  Result.MenuEntityTree->VerticalScrollbar = CreateVerticalScrollbar();
  Result.MenuEntityTree->EntityTree     = me_tree::Create();
  Result.MenuEntityTree->EntityTree.NewNode(); // EmptyRoot

  return Result;
}

} // namespace imgui

