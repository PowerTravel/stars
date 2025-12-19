#pragma once

#include "imgui.h"

namespace imgui { 

struct imgui_bordered_window {
  v2 CornerSize;
  r32 HeaderSize;
  rect2f Region;

  r32 LeftDiff;
  r32 RightDiff;
  r32 TopDiff;
  r32 BotDiff;
  v2 BotLeftDiff;
  v2 BotRightDiff;
  v2 TopLeftDiff;
  v2 TopRightDiff;
  v2 HeaderDiff;
  imgui_id LeftID;
  imgui_id RightID;
  imgui_id TopID;
  imgui_id BotID;
  imgui_id BotLeftID;
  imgui_id BotRightID;
  imgui_id TopLeftID;
  imgui_id TopRightID;
  imgui_id HeaderID;
};

imgui_bordered_window ImguiBorderedWindow( rect2f Region, v2 CornerSize, r32 HeaderSize);
void DoImguiBorderWindow(imgui_bordered_window* BorderWindow, const char Header[]);

rect2f GetContentRect(imgui_bordered_window* BorderWindow){
  rect2f Result = Rect2f(BorderWindow->Region.X, BorderWindow->Region.Y, BorderWindow->Region.W, BorderWindow->Region.H - BorderWindow->HeaderSize);
  return Result;
}

}// namespace imgui