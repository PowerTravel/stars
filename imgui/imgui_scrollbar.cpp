#include "imgui_scrollbar.h"


namespace imgui { 

file_local r32 MouseScroll(u32 TotalListSize)
{
  // Use as function parameter?
  const r32 ScrollTick = 1/20.f;
  r32 Result = 0;
  if(GlobalImguiContext->MouseDZ)
  {
    r32 ScrollTickPercentage = ScrollTick / TotalListSize;
    Result = (GlobalState->ImguiContext.MouseDZ > 0) ? -ScrollTickPercentage : ScrollTickPercentage; 
  }
  return Result;
}

b32 DoVerticalScrollbar(imgui_vertical_scrollbar* VerticalScrollbar, rect2f ScrollbarRect, b32 MousescrollActive,  r32 TotalContentHeight)
{
  // TODO: Place button min/max values in imgui_vertical_scrollbar?
  r32 ScrollButtonSizeMax = ScrollbarRect.H;
  r32 ScrollButtonSizeMin = 0.03;
  // TODO: ArgumentToFunction or use som environmental color values?
  v4 ScrollWheelBackgroundColor = V4(0.5,0.5,0.5,1.0);
  v2 Padding = V2(PixelToCanonicalWidth(1),
                  PixelToCanonicalHeight(1));

  r32 SizePercentage = ScrollbarRect.H / TotalContentHeight;
  r32 ScrollWheelButtonHeight = Clamp(SizePercentage * ScrollbarRect.H, ScrollButtonSizeMin, ScrollButtonSizeMax);
  v2 ScrollButtonSize = V2(ScrollbarRect.W, ScrollWheelButtonHeight);

  render::DrawOverlayQuadCanonicalSpace(CenteredRect(ScrollbarRect), ScrollWheelBackgroundColor);
  ImguiButton(GlobalImguiContext, VerticalScrollbar->ButtonID, ScrollbarRect);

  
  r32 ScrollWheelPosY = Lerp(VerticalScrollbar->ScrollAmmount.Y, ScrollbarRect.Y + ScrollbarRect.H - ScrollButtonSize.Y, ScrollbarRect.Y);
  rect2f ScrollWheelRect = Rect2f(ScrollbarRect.X, ScrollWheelPosY, ScrollButtonSize.X, ScrollButtonSize.Y);
  ScrollWheelRect = Shrink(ScrollWheelRect, Padding);
  
  imgui_button_color ButtonColor = ImguiDefaultButtonColor();
  
  if(!ImguiIsHot(VerticalScrollbar->ButtonID))
  {
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ScrollWheelRect), ButtonColor.InactiveColor);
  }else{
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ScrollWheelRect), ButtonColor.HotColor);
  }
  
  v2 MousePos = V2(GlobalState->ImguiContext.MouseX, GlobalState->ImguiContext.MouseY);
  if(ImguiIsActive(VerticalScrollbar->ButtonID)) {
    // Mouse is clickedUp on the scrollbarButton, Cache the mouseDiff.
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      if(Intersects(ScrollWheelRect, MousePos))
      {
        VerticalScrollbar->ScrollButtonDiff.Y = MousePos.Y - (ScrollbarRect.Y + (1-VerticalScrollbar->ScrollAmmount.Y) * (ScrollbarRect.H - ScrollButtonSize.Y));  
      }else{
        VerticalScrollbar->ScrollButtonDiff.Y = ScrollButtonSize.Y*0.5f;
      }
    }else{
      r32 A = ScrollbarRect.Y + VerticalScrollbar->ScrollButtonDiff.Y;
      r32 B = ScrollbarRect.Y + ScrollbarRect.H - (ScrollButtonSize.Y-VerticalScrollbar->ScrollButtonDiff.Y);
      if(A != B)
      {
        VerticalScrollbar->ScrollAmmount.Y = Unlerp(MousePos.Y, B, A);
      }else{
        VerticalScrollbar->ScrollAmmount.Y = Clamp(VerticalScrollbar->ScrollAmmount.Y,0,1);
      }
    }
  }else{
    if(MousescrollActive)
    {
      VerticalScrollbar->ScrollAmmount.Y += MouseScroll(TotalContentHeight);
    }  
  }
  VerticalScrollbar->ScrollAmmount.Y = Clamp(VerticalScrollbar->ScrollAmmount.Y, 0,1);

  return ImguiIsActive(VerticalScrollbar->ButtonID);
}

} // namespace imgui 