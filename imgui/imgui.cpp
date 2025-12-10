#include "commons/string.h"
#include "imgui.h"
#include "platform/jwin_platform_memory.h"

inline v4 PositionToCoordinate(u32 X, u32 Y, u32 IconSizePx, u32 AtlasSizePx) {
  r32 X0 = (X * IconSizePx);
  r32 Y0 = (Y * IconSizePx);
  r32 X1 = ((X+1) * IconSizePx);
  r32 Y1 = ((Y+1) * IconSizePx);
  r32 OneOverSize = 1.f / (r32) AtlasSizePx;
  v4 Result = V4(
     X0*OneOverSize,  // u0
     Y0*OneOverSize,  // v0
     X1*OneOverSize, // u1
     Y1*OneOverSize); // v1
  return Result;
}

u32 PushImguiIconAtlasToGPU(render_group* RenderGroup)
{
  obj_bitmap* BitMap = LoadTGA([](u32 ByteSize){
    return PushSize(GlobalTransientArena, ByteSize);
  }, "..\\data\\icons\\icons.tga");
  texture_params Params = DefaultColorTextureParams();
  Params.TextureFormat = texture_format::RGBA_U8;
  Params.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  u32 Result = PushNewTexture2D(RenderGroup, BitMap->Width, BitMap->Height, Params, BitMap->Pixels);
  return Result;
}

imgui_icon_atlas LoadImguiIcons(render_group* RenderGroup)
{
  imgui_icon_atlas Icons = {};
  Icons.Atlas = PushImguiIconAtlasToGPU(RenderGroup);

  u32 IconSize = 64;
  u32 AtlasSize = 512;
  Icons.Coordinates[ICON_DOUBLE_ANGLE_UP]      = PositionToCoordinate( 0, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_DOUBLE_ANGLE_DOWN]    = PositionToCoordinate( 1, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_DOUBLE_ANGLE_LEFT]    = PositionToCoordinate( 2, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_DOUBLE_ANGLE_RIGHT]   = PositionToCoordinate( 3, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_UP]             = PositionToCoordinate( 4, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_DOWN]           = PositionToCoordinate( 5, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_LEFT]           = PositionToCoordinate( 6, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_RIGHT]          = PositionToCoordinate( 7, 7, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ADD]                  = PositionToCoordinate( 0, 6, IconSize, AtlasSize); 
  Icons.Coordinates[ICON_SUBTRACT]             = PositionToCoordinate( 1, 6, IconSize, AtlasSize);
  Icons.Coordinates[ICON_SEARCH]               = PositionToCoordinate( 2, 6, IconSize, AtlasSize); 
  Icons.Coordinates[ICON_FILTER]               = PositionToCoordinate( 3, 6, IconSize, AtlasSize);
  Icons.Coordinates[ICON_CHECBOX]              = PositionToCoordinate( 4, 6, IconSize, AtlasSize);
  Icons.Coordinates[ICON_EMPTY_CHECKBOX]       = PositionToCoordinate( 5, 6, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_MINIMIZE]      = PositionToCoordinate( 0, 5, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_X]             = PositionToCoordinate( 1, 5, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_MAXIMIZE]      = PositionToCoordinate( 2, 5, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_RESTORE]       = PositionToCoordinate( 3, 5, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_DETACH]        = PositionToCoordinate( 4, 5, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_GEOMETRY]   = PositionToCoordinate( 0, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_MATERIAL]   = PositionToCoordinate( 1, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_CAMERA]     = PositionToCoordinate( 2, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_LOCATION]   = PositionToCoordinate( 3, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_COLLIDER]   = PositionToCoordinate( 4, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_LIGHT]      = PositionToCoordinate( 5, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_CONTROLLER] = PositionToCoordinate( 6, 4, IconSize, AtlasSize);
  Icons.Coordinates[ICON_COMPONENT_UNKNOWN]    = PositionToCoordinate( 7, 4, IconSize, AtlasSize);

  return Icons;
}

imgui_scrollable_list CreateScrollableTextList()
{
  imgui_scrollable_list Result = {};
  Result.VerticalScrollbarId = NewButtonID();
  Result.HorizontalScrollbarId = NewButtonID();
  Result.SelectedRow = -1;
  return Result;
}

r32 MouseScroll(u32 RowCount, r32 RowHeight)
{
  r32 Result = 0;
  if(GlobalState->ImguiContext.MouseDZ)
  {
    r32 ScrollTick = 1/20.f;
    r32 TotalListSize = RowHeight * RowCount;
    r32 ScrollTickPercentage = ScrollTick / TotalListSize;
    Result = (GlobalState->ImguiContext.MouseDZ > 0) ? -ScrollTickPercentage : ScrollTickPercentage; 
  }
  return Result;
}

r32 GetScrollWheelSize(r32 ListHeight, r32 RowHeight, u32 RowCount, r32 Min, r32 Max)
{
  r32 LinesToFit = ListHeight / RowHeight;
  r32 SizePercentage = LinesToFit / RowCount;
  r32 Result = Clamp(ListHeight * SizePercentage, Min, Max);
  return Result;
}

b32 ImguiScrollBarVertical(imgui_scrollable_list* List, rect2f ListRegion, r32 ScrollbarWidth, r32 RowHeight, r32 RowCount)
{
  rect2f ScrollbarRect = Rect2f(ListRegion.X + ListRegion.W - ScrollbarWidth, ListRegion.Y, ScrollbarWidth, ListRegion.H);
  v2 ScrollButtonSize = V2(ScrollbarWidth, GetScrollWheelSize(ListRegion.H, RowHeight, RowCount, 0.03f, ListRegion.H));

  render::DrawOverlayQuadCanonicalSpace(CenteredRect(ScrollbarRect), V4(0.5,0.5,0.5,1.0));
  ImguiButton(&GlobalState->ImguiContext, List->VerticalScrollbarId, ScrollbarRect);

  v2 Padding =  V2(PixelToCanonicalWidth(1),
                   PixelToCanonicalWidth(1));
  
  r32 ScrollWheelPosY = Lerp(List->ScrollAmmount.Y, ScrollbarRect.Y + ScrollbarRect.H - ScrollButtonSize.Y, ScrollbarRect.Y);
  rect2f ScrollWheelRect = Rect2f(ScrollbarRect.X, ScrollWheelPosY, ScrollButtonSize.X, ScrollButtonSize.Y);
  ScrollWheelRect = Shrink(ScrollWheelRect, Padding);
  
  imgui_button_color ButtonColor = ImguiDefaultButtonColor();
  
  if(!ImguiIsHot(List->VerticalScrollbarId))
  {
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ScrollWheelRect), ButtonColor.InactiveColor);
  }else{
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ScrollWheelRect), ButtonColor.HotColor);
  }
  
  v2 MousePos = V2(GlobalState->ImguiContext.MouseX,GlobalState->ImguiContext.MouseY);
  if(ImguiIsActive(List->VerticalScrollbarId)) {
    // Mouse is clickedUp on the scrollbarButton, Cache the mouseDiff.
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      if(Intersects(ScrollWheelRect, MousePos))
      {
        List->ScrollButtonDiff.Y = MousePos.Y - (ScrollbarRect.Y + (1-List->ScrollAmmount.Y) * (ScrollbarRect.H - ScrollButtonSize.Y));  
      }else{
        List->ScrollButtonDiff.Y = ScrollButtonSize.Y*0.5f;
      }
    }else{
      r32 A = ScrollbarRect.Y + List->ScrollButtonDiff.Y;
      r32 B = ScrollbarRect.Y + ScrollbarRect.H - (ScrollButtonSize.Y-List->ScrollButtonDiff.Y);
      List->ScrollAmmount.Y = Unlerp(MousePos.Y, B, A);
    }
  }else{
    if(Intersects(ListRegion,MousePos))
    {
      List->ScrollAmmount.Y += MouseScroll(RowCount, RowHeight);
    }  
  }
  List->ScrollAmmount.Y = Clamp(List->ScrollAmmount.Y, 0,1);

  return ImguiIsActive(List->VerticalScrollbarId);
}

rect2f GetRowRect(rect2f ListRect, s32 Index, r32 FirstRow, r32 RowHeight)
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




b32 ImguiScrollableButtonList(imgui_scrollable_list* ScrollableList, v2 Pos, v2 Size, u32 RowCount, r32 RowHeight, imgui_id* RowIDs, void* Data, void (RowRenderFunction)(imgui_context* ImguiContext, imgui_id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data)) {

  // List Background
  rect2f BackgroundRect = Rect2f(Pos, Size);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(BackgroundRect), ImguiDefaultButtonColor().InactiveColor);

  r32 LinesToFit = Size.Y / RowHeight;
  v2 ListContentSize = Size;
  r32 StartRow = 0;
  if(LinesToFit < RowCount){
    r32 ScrollbarWidth = 0.01;
    ImguiScrollBarVertical(ScrollableList, BackgroundRect, ScrollbarWidth, RowHeight, RowCount);
    ListContentSize.X -= ScrollbarWidth;
    StartRow = ScrollableList->ScrollAmmount.Y * (RowCount - LinesToFit);
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
      RowRenderFunction(&GlobalState->ImguiContext, RowIDs[Index],  RowRect, ClippedRow, Index, Data);
      if(ImguiIsActive(RowIDs[Index]))
      {
        ScrollableList->SelectedRow = Index;
      }
    }
  }
  return Result;
}




imgui_text_input_buffer ImguiNewTextInputBuffer(s32 InputLen, utf8_byte* InputBuffer)
{
  imgui_text_input_buffer Result = {};
  Result.Buffer = Utf8StringBuffer(InputLen, InputBuffer);
  Result.CaretPosition = 0;
  Result.CharCount = 0;
  return Result;
}

void DeleteSelection(imgui_text_input_buffer* TextInputBuffer){
  u32 InputLen = 512;
  utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
  u32 Start = Minimum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
  u32 End = Maximum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
  CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, End);
  EraseFromBuffer(&TempBuffer,End-Start);
  CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, End, TextInputBuffer->CharCount - End);
  ClearBuffer(&TextInputBuffer->Buffer);
  CopyBufferContent(&TempBuffer, &TextInputBuffer->Buffer);
  TextInputBuffer->CaretPosition = Start;
  TextInputBuffer->CharCount -= End-Start;
}

void ImguiReadInput(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, jwin::device_input* Input, v2 TextPos, b32 HighlightAll)
{
  u32 InputLen = 512;
  r32 FontSize = 14;

  b32 ShiftDown = jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT);

  v2 MousePos = V2(Input->Mouse.X, Input->Mouse.Y);
  v2 MousePosRelText = MousePos - TextPos;
  if(jwin::Active(Input->Mouse.Button[jwin::MouseButton_Left]))
  {
    size_t CharCount = 0;
    GlobalRenderer->Font.GetCharsCountToFitCanonicalSpace(FontSize, MousePosRelText.X, TextInputBuffer->Buffer.Buffer, 0, &CharCount);
    
    if(HighlightAll)
    {
      if(jwin::Pushed(Input->Mouse.Button[jwin::MouseButton_Left]))
      {
        TextInputBuffer->SelectMode = true;
        TextInputBuffer->SelectionStart = 0;
        TextInputBuffer->CaretPosition = TextInputBuffer->CharCount;
      }
    }else{  
      if(jwin::Pushed(Input->Mouse.Button[jwin::MouseButton_Left]))
      {
        TextInputBuffer->SelectionStart = CharCount;
      }
      TextInputBuffer->CaretPosition = CharCount;
    }
    
    if(TextInputBuffer->SelectionStart != TextInputBuffer->CaretPosition)
    {
      TextInputBuffer->SelectMode = true;
    }
  }

  b32 KeyClicked = false;
  if(jwin::Pushed(Input->Keyboard.Key_BACK)){
    KeyClicked = true;
    if(TextInputBuffer->SelectMode)
    {        
      DeleteSelection(TextInputBuffer);
      TextInputBuffer->SelectMode = false;
    }else{
      if(TextInputBuffer->CaretPosition > 0)
      {
        if(TextInputBuffer->CaretPosition == TextInputBuffer->CharCount)
        {
          EraseFromBuffer(&TextInputBuffer->Buffer, 1);
        }else{
          utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
          CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
          EraseFromBuffer(&TempBuffer,1);
          CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, TextInputBuffer->CaretPosition, TextInputBuffer->CharCount - TextInputBuffer->CaretPosition);
          ClearBuffer(&TextInputBuffer->Buffer);
          CopyBufferContent(&TempBuffer, &TextInputBuffer->Buffer);
        }
        TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->CaretPosition-1, 0);
        TextInputBuffer->CharCount = Maximum(TextInputBuffer->CharCount-1, 0);;  
      }
    }

  }else if(jwin::Pushed(Input->Keyboard.Key_LEFT)){
    KeyClicked = true;

    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
      TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->CaretPosition-1, 0);
    }else{
      if(TextInputBuffer->SelectMode){
        TextInputBuffer->CaretPosition = Minimum(TextInputBuffer->SelectionStart, TextInputBuffer->CaretPosition);
        TextInputBuffer->SelectMode = false;
      }else{
        TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->CaretPosition-1, 0);
      }
    }
  }else if(jwin::Pushed(Input->Keyboard.Key_RIGHT)){
    KeyClicked = true;
    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
      TextInputBuffer->CaretPosition = Minimum(TextInputBuffer->CaretPosition+1, TextInputBuffer->CharCount);
    }else{
      if(TextInputBuffer->SelectMode){
        TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->SelectionStart, TextInputBuffer->CaretPosition);
        TextInputBuffer->SelectMode = false;
      }else{
        TextInputBuffer->CaretPosition = Minimum(TextInputBuffer->CaretPosition+1, TextInputBuffer->CharCount);
      }
    }
  }else if(jwin::Pushed(Input->Keyboard.Key_END)){
    KeyClicked = true;
    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
    }else{
      TextInputBuffer->SelectMode = false;
    }
    TextInputBuffer->CaretPosition = TextInputBuffer->CharCount;
  }else if(jwin::Pushed(Input->Keyboard.Key_HOME)){
    KeyClicked = true;
    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
    }else{
      TextInputBuffer->SelectMode = false;
    }
    TextInputBuffer->CaretPosition = 0;
  }else{
    utf8_string_buffer CharBuffer = CreateTempStringBuffer(InputLen);
    if(PushInputToBuffer(&Input->Keyboard, &CharBuffer, ENGLISH))
    {
      KeyClicked = true;
      if(TextInputBuffer->SelectMode)
      {
        DeleteSelection(TextInputBuffer);
        TextInputBuffer->SelectMode = false;
      }
      u32 CharCount = utf8_GetCharCountOfString(CharBuffer.Buffer);
      if(TextInputBuffer->CaretPosition == TextInputBuffer->CharCount)
      {
        CopyBufferContent(&CharBuffer, &TextInputBuffer->Buffer);
      }else{
        utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
        CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
        CopyBufferContent(&CharBuffer, &TempBuffer);
        CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, TextInputBuffer->CaretPosition, TextInputBuffer->CharCount - TextInputBuffer->CaretPosition);
        ClearBuffer(&TextInputBuffer->Buffer);
        CopyBufferContent(&TempBuffer, &TextInputBuffer->Buffer);
      }
      TextInputBuffer->CaretPosition += CharCount;
      TextInputBuffer->CharCount += CharCount;
    }
  }
  if(KeyClicked)
  {
    Platform.DEBUGPrint("Len %d, Caret Pos %d, Selection Start %d Selection Mode %s\n",
      TextInputBuffer->CharCount, TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart, TextInputBuffer->SelectMode ? "'On'" : "'off'");
  }
}


b32 ImguiSelectabeRegion(imgui_context* ImguiContext, imgui_id Id, rect2f RegionRect, jwin::device_input* Input)
{
  if(Intersects(RegionRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    ImguiSetHot(Id);
    if(ImguiIsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ImguiSetActive(Id);
      ImguiSetSelected(Id);
    }
  }else if(jwin::Pushed(GlobalState->ImguiContext.LeftMouse)){
    ImguiDeselect(Id);
  }

  if(jwin::Pushed(Input->Keyboard.Key_ENTER)) {
    ImguiDeselect(Id);
  }else if(jwin::Pushed(Input->Keyboard.Key_ESCAPE)){
    ImguiDeselect(Id);
  } 

  return ImguiIsSelected(Id) || ImguiIsActive(Id);
}


b32 ImguiTextDialog(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, v2 DialogPos, v2 TextWidth, v4 BackgroundColor)
{
  rect2f DialogRect = Rect2f(DialogPos.X, DialogPos.Y, TextWidth.X, TextWidth.Y);
  b32 Result = ImguiSelectabeRegion(&GlobalState->ImguiContext, DialogID, DialogRect, GlobalInput);

  r32 FontSize = 14;
  s32 InputLen = 512;
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(DialogRect), BackgroundColor);
  render::DrawTextCanonicalSpace(V2(DialogPos.X, DialogPos.Y +DescentOffset), FontSize, TextInputBuffer->Buffer.Buffer, V4(1,1,1,1));

  if(Result)
  {
    if(TextInputBuffer->SelectMode)
    {
      s32 Start = Minimum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
      s32 End   = Maximum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);

      utf8_string_buffer TempBuffer1 = CreateTempStringBuffer(InputLen);
      CopySubstring(&TextInputBuffer->Buffer, &TempBuffer1, 0, Start);

      utf8_string_buffer TempBuffer2 = CreateTempStringBuffer(InputLen);
      CopySubstring(&TextInputBuffer->Buffer, &TempBuffer2, 0, End);

      v2 TextSizeSelectStart = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer1.Buffer);
      v2 TextSizeSelectEnd = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer2.Buffer);

      rect2f HighlightRect = Rect2f(DialogPos.X + TextSizeSelectStart.X, DialogPos.Y, TextSizeSelectEnd.X - TextSizeSelectStart.X, TextWidth.Y);
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(HighlightRect), BackgroundColor * 1.2);
    }

    utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
    v2 TextSizeToCaret = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer.Buffer);
    r32 CaretWidth = PixelToCanonicalWidth(1);
    rect2f CaretBox = Rect2f(DialogPos.X + CaretWidth/2.f + TextSizeToCaret.X, DialogPos.Y, CaretWidth, GlobalRenderer->Font.GetLineSpacingCanonicalSpace(14));
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(CaretBox), V4(1,1,1,1));
  }

  return Result;
}


void ImguiRenderTextDialog(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, v2 DialogPos, v2 DialogSize, v4 BackgroundColor)
{
  r32 FontSize = 14;
  s32 InputLen = 512;
  rect2f DialogRect = Rect2f(DialogPos, DialogSize);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(DialogRect), BackgroundColor);
  render::DrawTextCanonicalSpace(V2(DialogPos.X, DialogPos.Y +DescentOffset), FontSize, TextInputBuffer->Buffer.Buffer, V4(1,1,1,1));
  if(TextInputBuffer->SelectMode)
  {
    s32 Start = Minimum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
    s32 End   = Maximum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
    
    utf8_string_buffer TempBuffer1 = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer1, 0, Start);

    utf8_string_buffer TempBuffer2 = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer2, 0, End);

    v2 TextSizeSelectStart = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer1.Buffer);
    v2 TextSizeSelectEnd = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer2.Buffer);

    rect2f HighlightRect = Rect2f(DialogPos.X + TextSizeSelectStart.X, DialogPos.Y, TextSizeSelectEnd.X - TextSizeSelectStart.X, DialogSize.Y);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(HighlightRect), BackgroundColor * 1.2);
  }

  utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
  CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
  v2 TextSizeToCaret = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer.Buffer);
  r32 CaretWidth = PixelToCanonicalWidth(1);
  rect2f CaretBox = Rect2f(DialogPos.X + CaretWidth/2.f + TextSizeToCaret.X, DialogPos.Y, CaretWidth, GlobalRenderer->Font.GetLineSpacingCanonicalSpace(14));
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(CaretBox), V4(1,1,1,1));  
}

void ClearBuffer(imgui_text_input_buffer* TextInputBuffer)
{
  ClearBuffer(&TextInputBuffer->Buffer);
  TextInputBuffer->CaretPosition = 0;
  TextInputBuffer->SelectMode = false;
  TextInputBuffer->SelectionStart = 0;
  TextInputBuffer->CharCount = 0;
}

void PushString(imgui_text_input_buffer* TextInputBuffer, c8* String)
{
  utf8_byte* Utf8String = (utf8_byte*) String;
  u32 CharCount = utf8_GetCharCountOfString(Utf8String);
  AppendStringToBuffer(Utf8String, &TextInputBuffer->Buffer);
  TextInputBuffer->CaretPosition += CharCount;
  TextInputBuffer->CharCount += CharCount;
}


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
  return Result;
}

void ImguiBorderWindow(imgui_bordered_window* BorderWindow, const char Header[])
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
  BorderColor.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "taupe");
  BorderColor.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "sandy taupe");
  BorderColor.ActiveColor = V4(0.098039, 0.349020, 0.019608, 1.000000);
  BorderColor.HotColor =  menu::GetColor(&GlobalState->ColorTable, "golden brown");
  
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
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(HeaderBarRect), menu::GetColor(&GlobalState->ColorTable, "seal brown"));
  if(ImguiButton(&GlobalState->ImguiContext, BorderWindow->HeaderID, HeaderBarRect))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      BorderWindow->HeaderDiff = V2(GlobalState->ImguiContext.MouseX - Region.X, GlobalState->ImguiContext.MouseY - Region.Y);
    }
    Region.X = GlobalState->ImguiContext.MouseX - BorderWindow->HeaderDiff.X; 
    Region.Y = GlobalState->ImguiContext.MouseY - BorderWindow->HeaderDiff.Y;
  }

  BorderWindow->Region = Region;
}



// Buttons
imgui_button_color ImguiDefaultButtonColor()
{
  imgui_button_color Result = {};
  Result.InactiveColor =  menu::GetColor(&GlobalState->ColorTable, "taupe");
  Result.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "old gold"); 
  Result.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "sandy taupe");
  Result.HotColor = menu::GetColor(&GlobalState->ColorTable, "sandy taupe");
  return Result;
}

v4 ImguiGetButtonColor(imgui_id ButtonId, imgui_button_color ButtonColors){
  v4 Color = ButtonColors.InactiveColor;
  if(ImguiIsHot(ButtonId) && ImguiIsActive(ButtonId)) {
    // Button is Highlighted and pressed
    Color = ButtonColors.ActiveAndHotColor;
  }else if(ImguiIsActive(ButtonId)){
    // Button is Pressed
    Color = ButtonColors.ActiveColor;
  }else if(ImguiIsHot(ButtonId)){
    // Button is only highlighted
    Color = ButtonColors.HotColor;
  }
  return Color;
}

b32 ImguiButton(imgui_context* ImguiContext, imgui_id Id, rect2f ButtonRect)
{
  if(Intersects(ButtonRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    ImguiSetHot(Id);
    if(ImguiIsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ImguiSetActive(Id);
    }
  }

  return ImguiIsActive(Id);
}

b32 ImguiPlainButton(imgui_context* ImguiContext, imgui_id Id, rect2f ButtonRect, imgui_button_color ButtonColor) {
  if(Intersects(ButtonRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    ImguiSetHot(Id);
    if(ImguiIsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ImguiSetActive(Id);
    }
  }
  v4 Color = ButtonColor.InactiveColor;
  if(ImguiIsHot(Id) && ImguiIsActive(Id)) {
    // Button is Highlighted and pressed
    Color = ButtonColor.ActiveAndHotColor;
  }else if(ImguiIsActive(Id)){
    // Button is Pressed
    Color = ButtonColor.ActiveColor;
  }else if(ImguiIsHot(Id)){
    // Button is only highlighted
    Color = ButtonColor.HotColor;
  }

  render::DrawOverlayQuadCanonicalSpace(CenteredRect(ButtonRect), Color);
  return ImguiIsActive(Id);
}

utf8_string_buffer SetStringToFit(r32 FontSize, r32 MaxWidth, const c8* Text, const c8* Suffix = "...")
{
  size_t ByteSize = jstr::StringLength(Text) + jstr::StringLength(Suffix) + 1;
  utf8_string_buffer Buff = CreateTempStringBuffer(ByteSize);

  size_t CharCount = 0;
  if(GlobalRenderer->Font.GetCharsCountToFitCanonicalSpace(FontSize, MaxWidth, (utf8_byte*) Text, (utf8_byte*) Suffix, &CharCount)){
    AppendStringToBuffer((utf8_byte*)Text, &Buff);
  }else{
    AppendStringToBuffer((u32)CharCount, (utf8_byte*)Text, &Buff);
    while(*PeakChar(&Buff) == ' ')
    {
      EraseFromBuffer(&Buff, 1);
    }
    AppendStringToBuffer((utf8_byte*)Suffix, &Buff);
  }
  return Buff;
}

u32 ImguiTextButton(imgui_id Id, u32 FontSize, c8* Text, r32 ButtonX, r32 ButtonY, r32 ButtonWidth, r32 ButtonHeight, r32 TextOffsetX, r32 TextOffsetY, r32 ClickOffsetPx, r32 ShadowOffsetPx) {
  if(GlobalState->ImguiContext.MouseX >= ButtonX && GlobalState->ImguiContext.MouseX <= ButtonX + ButtonWidth &&
     GlobalState->ImguiContext.MouseY >= ButtonY && GlobalState->ImguiContext.MouseY <= ButtonY + ButtonHeight)
  {
    ImguiSetHot(Id);
    if(ImguiIsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ImguiSetActive(Id);
    }
  }

  v2 ClickOffset = {};
  v4 Color = menu::GetColor(&GlobalState->ColorTable, "plum");
  r32 ButtonTextWidth = ButtonWidth;
  r32 ButtonTextHeight = ButtonHeight;
  if(ImguiIsHot(Id) && ImguiIsActive(Id)) {
    // Button is Highlighted and pressed
    if(ClickOffsetPx != 0){
      ClickOffset.X = ClickOffsetPx/GlobalWindowSize.ApplicationWidth;
      ClickOffset.Y = -ClickOffsetPx/GlobalWindowSize.ApplicationWidth; 
    }
    Color = menu::GetColor(&GlobalState->ColorTable, "waterspout");
  }else if(ImguiIsActive(Id)){
    // Button is Pressed
    if(ClickOffsetPx != 0){
      ClickOffset.X = 4.f/GlobalWindowSize.ApplicationWidth;
      ClickOffset.Y = -4.f/GlobalWindowSize.ApplicationWidth;
    }
    Color = menu::GetColor(&GlobalState->ColorTable, "old gold");
  }else if(ImguiIsHot(Id)){
    // Button is only highlighted
    Color = menu::GetColor(&GlobalState->ColorTable, "khaki");
    if(FontSize && Text && *Text != '\0')
    {
      r32 TextWidth = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, (utf8_byte*) Text).X;
      if(TextWidth > ButtonTextWidth)
      {
        ButtonTextWidth = TextWidth; 
      }
    }
  }else{
    // Button is inactive
    Color = menu::GetColor(&GlobalState->ColorTable, "taupe");
  }

  v2 CenterRect = V2(ButtonX + ButtonTextWidth * 0.5f, ButtonY + ButtonHeight * 0.5f); 
  if(ShadowOffsetPx!=0)
  {
    r32 ShadowOffsetX =  ShadowOffsetPx/GlobalWindowSize.ApplicationWidth;
    r32 ShadowOffsetY = -ShadowOffsetPx/GlobalWindowSize.ApplicationWidth;
    rect2f ShadowRect = Rect2f(
      CenterRect.X + ShadowOffsetX,
      CenterRect.Y + ShadowOffsetY, ButtonTextWidth, ButtonHeight);
    render::DrawOverlayQuadCanonicalSpace(ShadowRect, menu::GetColor(&GlobalState->ColorTable, "rich carmine"));
  }

  rect2f ButtonRect = Rect2f(
    CenterRect.X + ClickOffset.X,
    CenterRect.Y + ClickOffset.Y, ButtonTextWidth, ButtonHeight);
  render::DrawOverlayQuadCanonicalSpace(ButtonRect, Color);

  if(FontSize && Text && *Text != '\0')
  {
    r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
    v2 TextOrigin = V2(ButtonX + TextOffsetX,ButtonY + TextOffsetY) + ClickOffset + V2(0, DescentOffset);
    utf8_string_buffer StringBuffer = SetStringToFit(FontSize, ButtonTextWidth, Text);
    render::DrawTextCanonicalSpace(TextOrigin , Rect2f(ButtonX,ButtonY,ButtonTextWidth,ButtonHeight), FontSize, StringBuffer.Buffer, V4(1.0,1.0,1.0,1.0));  
  }
  
  return ImguiIsActive(Id);
}



/// imgui_row BEGIN

  imgui_row::padding imgui_row::Padding(r32 Width, r32 Height){
    padding Result = {};
    Result.Width = Width;
    Result.Height = Height;
    return Result;
  }


  imgui_row::icon imgui_row::Icon(r32 Size, u32 IconType ){
    icon Result = {};
    Result.Size = Size;
    Result.IconType = IconType;
    Result.Color = V4(1,1,1,1);
    return Result;
  }
 

  imgui_row::text imgui_row::Text(r32 FontSize, render::font* Font, size_t TextLen, const char* Text){
    text Result = {};
    Result.TextLen = TextLen;
    Result.Text = Text;
    Result.FontSize = FontSize;
    Result.Font = Font;
    Result.Color = V4(1,1,1,1);
    return Result;
  }


  imgui_row::div_hint imgui_row::DivHint(){
    div_hint Result = {};
    return Result;
  }


  static size_t TypeToSize(imgui_row::type Type) {
    switch(Type) {
      case imgui_row::type::PADDING: {
        return sizeof(imgui_row::padding);
      }break;
      case imgui_row::type::ICON: {
        return sizeof(imgui_row::icon);
      }break;
      case imgui_row::type::TEXT: {
        return sizeof(imgui_row::text);
      }break;
      case imgui_row::type::DIV_HINT: {
        return sizeof(imgui_row::div_hint);
      }break;
    }
    INVALID_CODE_PATH
    return 0;
  }

  void imgui_row::Push(imgui_row::header* Header)
  {
    if(!m_head)
    {
      m_head = Header;
      m_tail = Header;
    }else{
      m_tail->Next = Header;
      m_tail = Header;
    }
  }

  void imgui_row::Push(imgui_row::padding Padding){
    header* Header = (header*) PushStruct(GlobalTransientArena, header);
    Header->Type = type::PADDING;
    Header->Size = V2(Padding.Width, Padding.Height);
    padding* Tmp = PushStruct(GlobalTransientArena, padding);
    *Tmp = Padding;
    Header->Data = (void*) Tmp;
    Push(Header);
  }
  void imgui_row::Push(imgui_row::icon Icon){
    header* Header = (header*) PushStruct(GlobalTransientArena, header);
    Header->Type = type::ICON;
    Header->Size = PixelToCanonicalSpace(V2(Icon.Size,Icon.Size));

    icon* Tmp = PushStruct(GlobalTransientArena, icon);
    *Tmp = Icon;
    Header->Data = (void*) Tmp;
    Push(Header);
  }
  void imgui_row::Push(imgui_row::text Text) {
    header* Header = (header*) PushStruct(GlobalTransientArena, header);
    Header->Type = type::TEXT;
    Header->Size = GlobalRenderer->Font.GetTextSizeCanonicalSpace(Text.FontSize, (utf8_byte*) Text.Text);
    text* Tmp = PushStruct(GlobalTransientArena, text);
    Tmp->TextLen = Text.TextLen;
    Tmp->FontSize = Text.FontSize;
    Tmp->Text = (char*) PushCopy(GlobalTransientArena, Text.TextLen, (void*) Text.Text);
    Tmp->Font = Text.Font;
    Tmp->Color = Text.Color;
    Header->Data = (void*) Tmp;
    Push(Header);
  }

  // Sums the sizes of all elements up untill the next DivHint __or__ untill last element.
  v2 CalculateDivSize(imgui_row::header* H) {
    v2 DivSize = {};
    while(H && H->Type != imgui_row::type::DIV_HINT){
      DivSize.X += H->Size.X;
      DivSize.Y  = Maximum(DivSize.Y, H->Size.Y);
      H = H->Next;
    }
    return DivSize;
  }

  void imgui_row::Push(div_hint foo){
    header* Header = (header*) PushStruct(GlobalTransientArena, header);
    Header->Type = type::DIV_HINT;
    div_hint* DivHint = PushStruct(GlobalTransientArena, div_hint);
    DivHint->Header = Header;

    if(!m_divTail)
    {
      Assert(!m_divHead);
      m_divHead = DivHint;
      m_divTail = DivHint;
      Assert(m_head);
      Header->Size = CalculateDivSize(m_head);
    }else{
      header* H = m_divTail->Header->Next;
      Assert(!m_divTail->Next);
      m_divTail->Next = DivHint;
      m_divTail = DivHint;
      Header->Size = CalculateDivSize(H);
    }
    Header->Data = (void*) DivHint;
    Push(Header);
    m_divCount++;
  }

  file_local void InitiateDivList(cmn::list<imgui_row::header*>& DivList, imgui_row::header* First){
    if(!DivList.Initiated())
    {
      DivList = cmn::list<imgui_row::header*>::CreateTransient();
      imgui_row::header* Header = First;
      while(Header)
      {
        if(Header->Type == imgui_row::type::DIV_HINT)
        {
          DivList.PushBack(Header);
        }
        Header = Header->Next;
      }
    }
  }
#if 0
  v2 imgui_row::GetSizeOfDiv(imgui_row::header* Start, imgui_row::header** ResultEnd) {
    Assert(Start->Type != imgui_row::type::DIV_HINT);

    header* Header = Start;
    v2 Result = {};
    b32 NextDivFound = false;
    while(Header && !NextDivFound)
    {
      switch(Header->Type)
      {
        case type::PADDING: {
          padding* Padding = (padding*) Header->Data;
          Result.X += Padding->Width;
          Result.Y = Maximum(Result.Y, Padding->Height);
        }break;
        case type::ICON: {
          icon* Icon = (icon*) Header->Data;
          v2 CanSize = PixelToCanonicalSpace(V2(Icon->Size,Icon->Size));
          Result.X += CanSize.X;
          Result.Y = Maximum(Result.Y, CanSize.Y);
        }break;
        case type::TEXT: {
          text* Text = (text*) Header->Data;
          r32 TextWidht = GlobalRenderer->Font.GetTextSizeCanonicalSpace(Text->FontSize, (utf8_byte*) Text->Text).X;
          r32 LineHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(Text->FontSize);
          Result.X += TextWidht;
          Result.Y = Maximum(Result.Y, LineHeight);
        }break;
        case type::DIV_HINT: {
          NextDivFound = true;
        }break;
      }
      if(!NextDivFound){
        Header = Header->Next;
      }
    }
    *ResultEnd = Header;
    return Result;
  }
#endif
  v2 imgui_row::GetSize(v2* ResultVec = 0) {
    
    int DivCount = 0;
    r32 X = 0;
    r32 Y = 0;
    v2 Result = V2(0,0);
    header* Header = m_head;
    while(Header)
    {
      switch(Header->Type)
      {
        case type::PADDING: {
          padding* Padding = (padding*) Header->Data;
          X+=Padding->Width;
          Y = Maximum(Y, Padding->Height);
        }break;
        case type::ICON: {
          icon* Icon = (icon*) Header->Data;
          X += PixelToCanonicalWidth(Icon->Size);
          r32 CanHeight = PixelToCanonicalHeight(Icon->Size);
          Y = Maximum(Y, CanHeight);
        }break;
        case type::TEXT: {
          text* Text = (text*) Header->Data;
          v2 CanSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(Text->FontSize, (utf8_byte*) Text->Text);
          r32 LineHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(Text->FontSize);
          X += CanSize.X;
          Y = Maximum(Y, LineHeight);
        }break;
        case type::DIV_HINT: {
          if(ResultVec)
          {
            ResultVec[DivCount] = V2(X,Y);
            Result.X = Maximum(X,Result.X);
            Result.Y += Y;
          }
          DivCount++;
          X = 0;
          Y = 0;
        }break;
      }
      Header = Header->Next;
    }
    
    Result.X = Maximum(X,Result.X);
    Result.Y += Y;
    if(ResultVec)
    {
      ResultVec[DivCount] = V2(X,Y);
    }
    return Result;
  }

  void DrawDebugRect(r32 t, v2 Pos, v2 Size){
    static const v4 Color1 = V4(0,0,0,1);
    static const v4 Color2 = V4(1,1,1,1);        
    rect2f DivRect = Rect2f(Pos,Size);
    v4 Color = V4(Lerp(t,Color1.X,Color2.X), Lerp(t,Color1.Y,Color2.Y), Lerp(t,Color1.Z,Color2.Z),0.9);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(DivRect), Color);
  }

  r32 imgui_row::Draw(r32 X0, r32 Y0, rect2f ClipRect, r32 RowNum) {

    v2* DivLengths = PushArray(GlobalTransientArena, m_divCount+1, v2);
    GetSize(DivLengths);
    r32 X = X0;
    r32 Y = Y0 - DivLengths[0].Y;

    header* Header = m_head;
    r32 ElementCount = 0;
    while(Header)
    {
      ElementCount++;
      Header = Header->Next;
    }

    Header = m_head;
    r32 InterpolationStepSize = 1 / (r32 )ElementCount;
    r32 t = 0;
    u32 DivIndex = 0;
    while(Header)
    {
      v2 Lim = DivLengths[DivIndex];
      rect2f DivRect = Rect2f(V2(X,Y),Lim);      
      //render::DrawOverlayQuadCanonicalSpace(CenteredRect(DivRect), Color);
      switch(Header->Type)
      {
        case imgui_row::type::PADDING: {
          padding* Padding = (padding*) Header->Data;
          X += Padding->Width;
          DrawDebugRect(t, V2(X,Y), V2(Padding->Width,Lim.Y));
        }break;
        case imgui_row::type::ICON: {
          icon* Icon = (icon*) Header->Data;
          v2 IconCanSize = PixelToCanonicalSpace(V2(Icon->Size,Icon->Size));
          r32 DiffY = 0.5* (Lim.Y - IconCanSize.X);
          rect2f IconRect = Rect2f(X, Y + DiffY, IconCanSize.X, IconCanSize.Y);
          v4 TexCoords = GlobalImguiContext->Icons.Coordinates[Icon->IconType];
          render::DrawIconCanonicalSpace(CenteredRect(IconRect), TexCoords, Icon->Color);
          DrawDebugRect(t, V2(IconRect.X, IconRect.Y), V2(IconRect.W,IconRect.H));
          X += IconCanSize.X;
        }break;
        case imgui_row::type::TEXT: {
          text* Text = (text*) Header->Data;
          r32 DescentOffset = Text->Font->GetCanonicalFontDescenOffset(Text->FontSize);
          v2 TextSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(Text->FontSize, (const utf8_byte*) Text->Text);
          r32 LineHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(Text->FontSize);

          r32 DiffY = 0.5* (Lim.Y - LineHeight);

          render::DrawTextCanonicalSpace(V2(X, Y + DescentOffset + DiffY), Text->FontSize, (const utf8_byte*) Text->Text, Text->Color);
          DrawDebugRect(t, V2(X, Y+DiffY), V2(TextSize.X,LineHeight));
          X += TextSize.X;
        }break;
        case imgui_row::type::DIV_HINT: {
          div_hint* DivHint = (div_hint*) Header->Data;
          v2 PrevLim = Lim;
          Lim = DivLengths[++DivIndex];
          if(X + Lim.X > ClipRect.X + ClipRect.W)
          {
            Y -= Lim.Y;
            X = X0;
          }
        }break;
      }
      t += InterpolationStepSize;
      Header = Header->Next;
    }
    return Y;
  }
  imgui_row ImguiRow(){
    imgui_row Result = {};
    return Result;
  };


