#include "menu.h"
#include "platform/jwin_platform_memory.h"
#include "entity_list.h"
#include "color_list.h"

namespace imgui { 
application_menu CreateApplicationImgui(memory_arena* Arena, context* ImguiContext, u32 ColorCount) {
  application_menu Result = {};

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

