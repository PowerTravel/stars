#include "color_list.h"

namespace imgui { 

namespace app {

file_local void DrawColorRow(context* ImguiContext, id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data)
{
  color_list* ColorList = (color_list*) Data;
  umm ColorIndex = (umm) ColorList->ColorIDs[ListIndex];
  named_color_hex* NamedColor = imgui::GetNamedColor(&GlobalState->ColorTable, (umm) ColorIndex);
  char* ColorName = NamedColor->Name;
  v4 ColorValue   = HexCodeToColorV4(NamedColor->Color);

  v2 Padding = PixelToCanonicalSpace(V2(1,1));

  r32 RowWidth = RowRect.W;
  r32 ColorSquareWidth = RowRect.H;
  r32 TextWidth = RowRect.W - ColorSquareWidth;

  if(ImguiIsHot(ButtonID) && ImguiIsInactive() && RowRect.H == ClippedRowRect.H){
    r32 TextWidthTmp = GlobalRenderer->Font.GetTextSizeCanonicalSpace(ImguiContext->FontSize, (utf8_byte*) ColorName).X;
    if(TextWidthTmp > TextWidth)
    {
      TextWidth = TextWidthTmp + 2*Padding.X;
      RowWidth = TextWidthTmp + ColorSquareWidth + 2*Padding.X;
    }
  }

  // Button Background
  v4 ButtonColor = ImguiGetButtonColor(ButtonID, ImguiDefaultButtonColor());
  rect2f ButtonBackgroundRect = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, RowWidth, ClippedRowRect.H);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(ButtonBackgroundRect), ButtonColor);

  // Colored Square
  rect2f ColorSquare = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, ColorSquareWidth, ClippedRowRect.H);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(Shrink(ColorSquare,Padding)), ColorValue);

  // Color Name
  rect2f TextRect = Rect2f(RowRect.X + ColorSquareWidth, ClippedRowRect.Y, TextWidth, ClippedRowRect.H);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(ImguiContext->FontSize);
  v2 TextPos = V2(RowRect.X + ColorSquareWidth, RowRect.Y + DescentOffset);
  utf8_string_buffer StringBuffer = SetStringToFit(ImguiContext->FontSize, TextWidth, ColorName);
  render::DrawTextCanonicalSpace(TextPos, TextRect, ImguiContext->FontSize, StringBuffer.Buffer, V4(1.0,1.0,1.0,1.0));
}

file_local rect2f GetRowRect(rect2f ListRect, s32 Index, r32 FirstRow, r32 RowHeight)
{
  s32 FirstIndex = (s32) Floor(FirstRow);
  r32 RowOffset = (FirstRow - FirstIndex) * RowHeight;

  v2 RowSize = V2(ListRect.W, RowHeight);

  r32 ListBot =  ListRect.Y;
  r32 ListTop =  ListRect.Y + ListRect.H;

  r32 RowYPos = -(Index+1) * RowHeight + ListTop + RowOffset;
  v2 RowPos = V2(ListRect.X, RowYPos);
  return Rect2f(RowPos,RowSize);
}

file_local b32 DrawColorListContent(color_list* MenuColorList, v2 Pos, v2 Size, u32 RowCount, r32 RowHeight, id* RowIDs, void* Data) {

  imgui_vertical_scrollbar* VerticalScrollbar = &MenuColorList->VerticalScrollbar;

  // List Background
  rect2f BackgroundRect = Rect2f(Pos, Size);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(BackgroundRect), ImguiDefaultButtonColor().InactiveColor);

  r32 LinesToFit = Size.Y / RowHeight;
  v2 ListContentSize = Size;
  r32 StartRow = 0;
  if(LinesToFit < RowCount){
    r32 ScrollbarWidth = 0.01;

    v2 MousePos = V2(GlobalState->ImguiContext.MouseX, GlobalState->ImguiContext.MouseY);
    b32 MouseScrollActive = Intersects(BackgroundRect, MousePos);
    rect2f ScrollbarRect  = Rect2f(BackgroundRect.X + BackgroundRect.W - ScrollbarWidth, BackgroundRect.Y, ScrollbarWidth, BackgroundRect.H);
    r32 TotalContentHeight = RowHeight*RowCount;
    DoVerticalScrollbar(VerticalScrollbar, ScrollbarRect, MouseScrollActive, TotalContentHeight);
    ListContentSize.X -= ScrollbarWidth;
    StartRow = VerticalScrollbar->ScrollAmmount.Y * (RowCount - LinesToFit);
  }

  s32 StartIndex = (s32) Floor(StartRow);

  rect2f ListRect = Rect2f(Pos, ListContentSize);
  b32 Result = 0;
  for (s32 i = 0; i<=LinesToFit; ++i)
  {
    s32 Index = StartIndex + i;
    if(Index < RowCount)
    {
      rect2f RowRect = GetRowRect(ListRect, i, StartRow, RowHeight);
      rect2f ClippedRow = RowRect;
      if(Top(RowRect) > Top(ListRect) || Bot(RowRect) < Bot(ListRect)){
        ClippedRow = Clip(RowRect, Rect2f(Pos, ListContentSize));  
      }

      if(ImguiButton(&GlobalState->ImguiContext, RowIDs[Index], ClippedRow))
      {
        Result = true;    
      }

      DrawColorRow(&GlobalState->ImguiContext, RowIDs[Index],  RowRect, ClippedRow, Index, Data);

      if(ImguiIsActive(RowIDs[Index]))
      {
        MenuColorList->SelectedRow = Index;
      }
    }
  }
  return Result;
}

void DrawColorList(menu* Menu) {
  
  u32 ColorCount = GlobalState->ColorTable.ColorCount;
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);

  color_list* ColorList = Menu->ColorList;

  s32 RowCount = 0;
  ZeroArray(ColorCount, ColorList->ColorIDs);
  id* ImguiIDs = PushArray(GlobalTransientArena, ColorCount, id);
  for (u32 i = 0; i < ColorCount; ++i) {
    named_color_hex* NamedColor = imgui::GetNamedColor(&GlobalState->ColorTable, (umm) i);
    char ColorNameLower[512] = {};
    Utf8ToLower( (utf8_byte*) NamedColor->Name, (utf8_byte*) ColorNameLower);
    char InputStringLower[512] = {};
    Utf8ToLower( ColorList->TextInputBuffer.Buffer.Buffer, (utf8_byte*) InputStringLower);
    if(ColorList->TextInputBuffer.CharCount == 0 || jstr::Contains( InputStringLower, ColorNameLower))
    {
      ColorList->ColorIDs[RowCount] = i;
      ImguiIDs[RowCount] = ColorList->ImguiIDs[i];
      RowCount++;
    }
  }

  imgui_bordered_window* BorderWindow = &ColorList->BorderWindow;

  // SearchIcon
  v4 SearchBoxBackgroundColor = imgui::GetColor(&GlobalState->ColorTable, "bole");
  v2 SearchIconPos = V2(BorderWindow->Region.X, BorderWindow->Region.Y);
  v2 SearchIconSize = V2(RowHeight, RowHeight);
  v4 TexCoord = GlobalImguiContext->Icons.Coordinates[ICON_SEARCH];
  rect2f SearchIconRectBackground = Rect2f(SearchIconPos, SearchIconSize);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(SearchIconRectBackground), SearchBoxBackgroundColor);
  rect2f SearchIconRect = Shrink(SearchIconRectBackground, 0.1*SearchIconRectBackground.W);
  render::DrawIconCanonicalSpace(CenteredRect(SearchIconRect),  TexCoord, V4(1,1,1,1));
  
  v2 FilterBarDialogPos  = V2(BorderWindow->Region.X + RowHeight, BorderWindow->Region.Y);
  v2 FilterBarDialogSize = V2(BorderWindow->Region.W - RowHeight, RowHeight);
  if(ImguiTextDialog(&ColorList->TextInputBuffer, ColorList->TextInputID, FilterBarDialogPos, FilterBarDialogSize, SearchBoxBackgroundColor))
  {
    ImguiReadInput(&ColorList->TextInputBuffer, ColorList->TextInputID, GlobalInput, FilterBarDialogPos);
  }

  v2 ScrollListPos  = V2(BorderWindow->Region.X, BorderWindow->Region.Y + RowHeight);
  v2 ScrollListSize = V2(BorderWindow->Region.W, BorderWindow->Region.H - 2* RowHeight);
  
  DoImguiBorderWindow(BorderWindow, GlobalState->ApplicationMenu.EnclosingRegion, "Colors");

  if(DrawColorListContent(ColorList, ScrollListPos, ScrollListSize, RowCount, RowHeight, ImguiIDs, (void*) ColorList))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      named_color_hex* NamedColor = imgui::GetNamedColor(&GlobalState->ColorTable, ColorList->ColorIDs[ColorList->SelectedRow] );
      v4 Color =  HexCodeToColorV4(NamedColor->Color);
      v4 HexColor = 255 * Color;
      Platform.DEBUGPrint("V4(%f, %f, %f, %f) - %s\n", Color.X, Color.Y, Color.Z, Color.W, 
        NamedColor->Name);  
    }
  }
}

} // namespace app
} // namespace imgui