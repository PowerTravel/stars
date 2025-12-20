#include "border_window.h"

namespace imgui { 

imgui_bordered_window ImguiBorderedWindow( rect2f Region, v2 CornerSize, r32 HeaderSize )
{
  imgui_bordered_window Result = {};
  Result.LeftID = NewButtonID();
  Result.RightID = NewButtonID();
  Result.TopID = NewButtonID();
  Result.BotID = NewButtonID();
  Result.BotLeftID = NewButtonID();
  Result.BotRightID = NewButtonID();
  Result.TopLeftID = NewButtonID();
  Result.TopRightID = NewButtonID();
  Result.HeaderID = NewButtonID();
  Result.Region = Region;
  Result.CornerSize = CornerSize;
  Result.HeaderSize = HeaderSize;
  Result.Position = imgui_bordered_window::position::FLOATING;
  Result.PreviousRegion = Region;
  return Result;
}

void DoImguiBorderWindow(imgui_bordered_window* BorderWindow, rect2f EnclosingRegion, const char Header[])
{
  v2 CornerSize  = BorderWindow->CornerSize;
  rect2f Region  = BorderWindow->Region;
  v2 RegionPos   = V2(Region.X,Region.Y);
  v2 RegionSize  = V2(Region.W,Region.H);

  rect2f LeftBorder  = Rect2f(RegionPos - V2(CornerSize.X, 0), V2(CornerSize.X, Region.H));
  rect2f RightBorder = Rect2f(RegionPos + V2(Region.W, 0),    V2(CornerSize.X, Region.H));
  rect2f TopBorder   = Rect2f(RegionPos + V2(0, Region.H),    V2(Region.W, CornerSize.Y));
  rect2f BotBorder   = Rect2f(RegionPos - V2(0, CornerSize.Y), V2(Region.W, CornerSize.Y));

  rect2f BotLeftCorner  = Rect2f(RegionPos - CornerSize, CornerSize);
  rect2f BotRightCorner = Rect2f(RegionPos + V2(Region.W, - CornerSize.Y), CornerSize);
  rect2f TopLeftCorner  = Rect2f(RegionPos + V2(-CornerSize.X, Region.H), CornerSize);
  rect2f TopRightCorner = Rect2f(RegionPos + RegionSize, CornerSize);

  imgui_button_color BorderColor = {};
  BorderColor.InactiveColor = imgui::GetColor(&GlobalState->ColorTable, "taupe");
  BorderColor.ActiveAndHotColor = imgui::GetColor(&GlobalState->ColorTable, "sandy taupe");
  BorderColor.ActiveColor = V4(0.098039, 0.349020, 0.019608, 1.000000);
  BorderColor.HotColor =  imgui::GetColor(&GlobalState->ColorTable, "golden brown");
  
  rect2f HeaderBarRect = Rect2f(Region.X, Region.Y + Region.H - BorderWindow->HeaderSize, Region.W, BorderWindow->HeaderSize);


  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->LeftID, LeftBorder, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->LeftDiff = GlobalState->ImguiContext.MouseX - LeftBorder.X;
    }
    r32 OldX = Region.X + Region.W;
    Region.X = GlobalState->ImguiContext.MouseX + (LeftBorder.W - BorderWindow->LeftDiff);
    Region.W = OldX - Region.X;
  }

  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->RightID, RightBorder, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->RightDiff = GlobalState->ImguiContext.MouseX - RightBorder.X;
    }
    Region.W = GlobalState->ImguiContext.MouseX - Region.X - BorderWindow->RightDiff;
  }

  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->TopID, TopBorder, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->TopDiff = GlobalState->ImguiContext.MouseY - TopBorder.Y;
    }
    Region.H = GlobalState->ImguiContext.MouseY - Region.Y - BorderWindow->TopDiff;
  }

  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->BotID, BotBorder, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->BotDiff = GlobalState->ImguiContext.MouseY - BotBorder.Y;
    }

    r32 OldY = Region.Y + Region.H;
    Region.Y = GlobalState->ImguiContext.MouseY + (BotBorder.H - BorderWindow->BotDiff);
    Region.H = OldY - Region.Y;

  }

  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->BotLeftID, BotLeftCorner, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->BotLeftDiff.X = GlobalState->ImguiContext.MouseX - BotLeftCorner.X;
      BorderWindow->BotLeftDiff.Y = GlobalState->ImguiContext.MouseY - BotLeftCorner.Y;
    }

    r32 OldY = Region.Y + Region.H;
    Region.Y = GlobalState->ImguiContext.MouseY + (BotBorder.H - BorderWindow->BotLeftDiff.Y);
    Region.H = OldY - Region.Y;

    r32 OldX = Region.X + Region.W;
    Region.X = GlobalState->ImguiContext.MouseX + (LeftBorder.W - BorderWindow->BotLeftDiff.X);
    Region.W = OldX - Region.X;
  }


  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->BotRightID, BotRightCorner, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->BotRightDiff.X = GlobalState->ImguiContext.MouseX - BotRightCorner.X;
      BorderWindow->BotRightDiff.Y = GlobalState->ImguiContext.MouseY - BotRightCorner.Y;
    }

    r32 OldY = Region.Y + Region.H;
    Region.Y = GlobalState->ImguiContext.MouseY + (BotBorder.H - BorderWindow->BotRightDiff.Y);
    Region.H = OldY - Region.Y;

    Region.W = GlobalState->ImguiContext.MouseX - Region.X - BorderWindow->BotRightDiff.X;
  }

  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->TopLeftID, TopLeftCorner, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->TopLeftDiff.X = GlobalState->ImguiContext.MouseX - TopLeftCorner.X;
      BorderWindow->TopLeftDiff.Y = GlobalState->ImguiContext.MouseY - TopLeftCorner.Y;
    }

    Region.H = GlobalState->ImguiContext.MouseY - Region.Y - BorderWindow->TopLeftDiff.Y;

    r32 OldX = Region.X + Region.W;
    Region.X = GlobalState->ImguiContext.MouseX + (LeftBorder.W - BorderWindow->TopLeftDiff.X);
    Region.W = OldX - Region.X;
  }

  if(ImguiPlainButton(&GlobalState->ImguiContext, BorderWindow->TopRightID, TopRightCorner, BorderColor))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->TopRightDiff.X = GlobalState->ImguiContext.MouseX - TopRightCorner.X;
      BorderWindow->TopRightDiff.Y = GlobalState->ImguiContext.MouseY - TopRightCorner.Y;
    }
    Region.H = GlobalState->ImguiContext.MouseY - Region.Y - BorderWindow->TopRightDiff.Y;
    Region.W = GlobalState->ImguiContext.MouseX - Region.X - BorderWindow->TopRightDiff.X;
  }


  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(GlobalState->ImguiContext.FontSize);
  r32 TextWidth = GlobalRenderer->Font.GetTextSizeCanonicalSpace(GlobalState->ImguiContext.FontSize, (utf8_byte*) Header).X;
  v2 TextOrigin = V2(HeaderBarRect.X + (HeaderBarRect.W - TextWidth) * 0.5f, HeaderBarRect.Y + DescentOffset);
  render::DrawTextCanonicalSpace(TextOrigin, HeaderBarRect, GlobalState->ImguiContext.FontSize, (utf8_byte *) Header, V4(1.0,1.0,1.0,1.0));  
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(HeaderBarRect), imgui::GetColor(&GlobalState->ColorTable, "seal brown"));

  if(ImguiButton(&GlobalState->ImguiContext, BorderWindow->HeaderID, HeaderBarRect))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->HeaderDiff = V2(GlobalState->ImguiContext.MouseX - Region.X, GlobalState->ImguiContext.MouseY - Region.Y);
    }

    Region.X = Clamp(GlobalState->ImguiContext.MouseX - BorderWindow->HeaderDiff.X,
      Left(EnclosingRegion) - BorderWindow->HeaderDiff.X,
      Right(EnclosingRegion)- BorderWindow->HeaderDiff.X);
    Region.Y = Clamp(GlobalState->ImguiContext.MouseY - BorderWindow->HeaderDiff.Y,
      Bot(EnclosingRegion) - BorderWindow->HeaderDiff.Y,
      Top(EnclosingRegion) - Region.H);

    r32 SnapPadding = 0.01;

    imgui_bordered_window::position PreviousPosition = BorderWindow->Position;
    if(GlobalState->ImguiContext.MouseX < Left(EnclosingRegion) +  SnapPadding)
    {
      v4 Color = imgui::GetColor(&GlobalState->ColorTable, "seal brown");
      Color.W =0.7;
      rect2f Rect = Rect2f(EnclosingRegion.X, EnclosingRegion.Y, Region.W, EnclosingRegion.H);
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(Rect), Color);
      BorderWindow->Position = imgui_bordered_window::position::LEFT;
    }else if (GlobalState->ImguiContext.MouseX > Right(EnclosingRegion) -  SnapPadding){
      v4 Color = imgui::GetColor(&GlobalState->ColorTable, "seal brown");
      Color.W =0.7;
      rect2f Rect = Rect2f(EnclosingRegion.X + EnclosingRegion.W - Region.W, EnclosingRegion.Y, Region.W, EnclosingRegion.H);
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(Rect), Color);
      BorderWindow->Position = imgui_bordered_window::position::RIGHT;
    }else{
      //if(GlobalState->ImguiContext.ActiveID.idEdge &&
      //  PreviousPosition != imgui_bordered_window::position::FLOATING){
      //  Region = BorderWindow->PreviousRegion;
      //}
    }
  }
#if 1
  if(ImguiWasActive(BorderWindow->HeaderID)){
    /*
    switch(BorderWindow->Position){
      case imgui_bordered_window::position::LEFT: {
        BorderWindow->PreviousRegion = Region;
        Region = Rect2f(EnclosingRegion.X, EnclosingRegion.Y, Region.W, EnclosingRegion.H);
      }break;
      case imgui_bordered_window::position::RIGHT: {
        BorderWindow->PreviousRegion = Region;
        Region = Rect2f(EnclosingRegion.X + EnclosingRegion.W - Region.W, EnclosingRegion.Y, Region.W, EnclosingRegion.H);
      }break;
    }
    */
    Platform.DEBUGPrint("ActiveID %d HeaderID %d HeaderEdge %s\n",
      GlobalState->ImguiContext.ActiveID.id,
      BorderWindow->HeaderID.id,
      BorderWindow->HeaderID.idEdge ? "true" : "false");
  }

#endif
  BorderWindow->Region = Region;
}

} // namespace imgui