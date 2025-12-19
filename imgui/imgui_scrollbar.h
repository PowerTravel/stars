#pragma once

#include "imgui.h"

namespace imgui { 

struct imgui_vertical_scrollbar {
  imgui_id ButtonID;
  v2 ScrollAmmount;
  v2 ScrollButtonDiff;
};

imgui_vertical_scrollbar CreateVerticalScrollbar() {
  imgui_vertical_scrollbar Result = {};
  Result.ButtonID = NewButtonID();
  return Result;
}

b32 DoVerticalScrollbar(imgui_vertical_scrollbar* VerticalScrollbar, rect2f ScrollbarRect, b32 MousescrollActive,  r32 TotalContentHeight);

} // namespace imgui