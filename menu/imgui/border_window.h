#pragma once

#include "imgui.h"

namespace imgui { 

struct imgui_bordered_window {

  enum class position {
    FLOATING,
    LEFT,
    RIGHT,
    BOT,
    TOP,
    FULL_SCREEN,
  };

  v2 CornerSize;
  r32 HeaderSize;
  rect2f Region;
  rect2f PreviousRegion;

  position Position;

  r32 LeftDiff;
  r32 RightDiff;
  r32 TopDiff;
  r32 BotDiff;
  v2 BotLeftDiff;
  v2 BotRightDiff;
  v2 TopLeftDiff;
  v2 TopRightDiff;
  v2 HeaderDiff;
  id LeftID;
  id RightID;
  id TopID;
  id BotID;
  id BotLeftID;
  id BotRightID;
  id TopLeftID;
  id TopRightID;
  id HeaderID;
};

imgui_bordered_window ImguiBorderedWindow( rect2f Region, v2 CornerSize, r32 HeaderSize);
void DoImguiBorderWindow(imgui_bordered_window* BorderWindow, rect2f EnclosingRegion, const char Header[]);

rect2f GetContentRect(imgui_bordered_window* BorderWindow){
  rect2f Result = Rect2f(BorderWindow->Region.X, BorderWindow->Region.Y, BorderWindow->Region.W, BorderWindow->Region.H - BorderWindow->HeaderSize);
  return Result;
}

}// namespace imgui