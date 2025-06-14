#include "commons/string.h"
#include "imgui.h"

inline imgui_icon_coordinate PositionToCoordinate(u32 X, u32 Y, u32 IconSizePx, u32 AtlasSizePx) {
  r32 X0 = (X * IconSizePx);
  r32 Y0 = (Y * IconSizePx);
  r32 X1 = ((X+1) * IconSizePx);
  r32 Y1 = ((Y+1) * IconSizePx);
  r32 OneOverSize = 1.f / (r32) AtlasSizePx;
  imgui_icon_coordinate Result = {};
  Result.u0 = X0*OneOverSize;
  Result.v0 = Y0*OneOverSize;
  Result.u1 = X1*OneOverSize;
  Result.v1 = Y1*OneOverSize;
  return Result;
}

imgui_icon_atlas LoadImguiIcons(render_group* RenderGroup)
{
  imgui_icon_atlas Icons = {};
  Icons.Atlas = Push32BitColorTexture(RenderGroup, LoadTGA(GlobalTransientArena, "..\\data\\icons\\icons.tga"));

  u32 IconSize = 64;
  u32 AtlasSize = 512;
  Icons.Coordinates[ICON_DOUBLE_ANGLE_UP]    = PositionToCoordinate( 0, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_DOUBLE_ANGLE_DOWN]  = PositionToCoordinate( 1, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_DOUBLE_ANGLE_LEFT]  = PositionToCoordinate( 2, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_DOUBLE_ANGLE_RIGHT] = PositionToCoordinate( 3, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_UP]           = PositionToCoordinate( 4, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_DOWN]         = PositionToCoordinate( 5, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_LEFT]         = PositionToCoordinate( 6, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ANGLE_RIGHT]        = PositionToCoordinate( 7, 0, IconSize, AtlasSize);
  Icons.Coordinates[ICON_ADD]                = PositionToCoordinate( 0, 1, IconSize, AtlasSize); 
  Icons.Coordinates[ICON_SUBTRACT]           = PositionToCoordinate( 1, 1, IconSize, AtlasSize);
  Icons.Coordinates[ICON_SEARCH]             = PositionToCoordinate( 2, 1, IconSize, AtlasSize); 
  Icons.Coordinates[ICON_FILTER]             = PositionToCoordinate( 3, 1, IconSize, AtlasSize);
  Icons.Coordinates[ICON_CHECBOX]            = PositionToCoordinate( 4, 1, IconSize, AtlasSize);
  Icons.Coordinates[ICON_EMPTY_CHECKBOX]     = PositionToCoordinate( 5, 1, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_MINIMIZE]    = PositionToCoordinate( 0, 2, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_X]           = PositionToCoordinate( 1, 2, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_MAXIMIZE]    = PositionToCoordinate( 2, 2, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_RESTORE]     = PositionToCoordinate( 3, 2, IconSize, AtlasSize);
  Icons.Coordinates[ICON_WINDOW_DETACH]      = PositionToCoordinate( 4, 2, IconSize, AtlasSize);
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


imgui_text_input_buffer ImguiNewTextInputBuffer(s32 InputLen, utf8_byte* InputBuffer)
{
  imgui_text_input_buffer Result = {};
  Result.ID = NewButtonID();
  Result.Buffer = Utf8StringBuffer(InputLen, InputBuffer);
  Result.CaretPosition = 0;
  Result.CharCount = 0;
  return Result;
}

void ImguiReadInput(imgui_text_input_buffer* TextInputBuffer, jwin::device_input* Input)
{
  u32 InputLen = 512;
  if(jwin::Pushed(Input->Keyboard.Key_BACK) && TextInputBuffer->CaretPosition > 0){
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
    TextInputBuffer->CaretPosition--;
    TextInputBuffer->CharCount--;
  }else if(jwin::Pushed(Input->Keyboard.Key_LEFT) && TextInputBuffer->CaretPosition > 0){
    TextInputBuffer->CaretPosition--;
  }else if(jwin::Pushed(Input->Keyboard.Key_RIGHT) && TextInputBuffer->CaretPosition < TextInputBuffer->CharCount){
    TextInputBuffer->CaretPosition++;
  }else if(jwin::Pushed(Input->Keyboard.Key_END)){
    TextInputBuffer->CaretPosition = TextInputBuffer->CharCount;
  }else if(jwin::Pushed(Input->Keyboard.Key_HOME)){
    TextInputBuffer->CaretPosition = 0;
  }else if(jwin::Pushed(Input->Keyboard.Key_ENTER) || jwin::Pushed(Input->Keyboard.Key_ESCAPE)){
    ImguiDeselect();
  }else{
    utf8_string_buffer CharBuffer = CreateTempStringBuffer(32);
    if(PushInputToBuffer(&Input->Keyboard, &CharBuffer, ENGLISH))
    {
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
}


b32 ImguiSelectabeRegion(imgui_context* ImguiContext, imgui_id Id, rect2f RegionRect)
{
  if(Intersects(RegionRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    ImguiSetHot(Id);
    if(ImguiIsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ImguiSetActive(Id);
      ImguiSetSelected(Id);
    }
  }else if(jwin::Active(GlobalState->ImguiContext.LeftMouse)){
    ImguiDeselect();
  }

  return ImguiIsSelected(Id);
}


b32 ImguiTextDialog(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, v2 DialogPos, v2 TextWidth)
{
  rect2f DialogRect = Rect2f(DialogPos.X, DialogPos.Y, TextWidth.X, TextWidth.Y);
  b32 Result = ImguiSelectabeRegion(&GlobalState->ImguiContext, DialogID, DialogRect);

  s32 InputLen = 512;

  v4 Color = menu::GetColor(&GlobalState->ColorTable, "bole");
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), 14);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(DialogRect), Color);
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), V2(DialogPos.X, DialogPos.Y +DescentOffset), 14, TextInputBuffer->Buffer.Buffer, V4(1,1,1,1));

  if(ImguiIsSelected(DialogID))
  {
    utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
    v2 TextSizeToCaret = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), 14, TempBuffer.Buffer);
    r32 CaretWidth = ecs::render::PixelToCanonicalWidth(GetRenderSystem(), 1);
    rect2f CaretBox = Rect2f(DialogPos.X + CaretWidth/2.f + TextSizeToCaret.X, DialogPos.Y, CaretWidth, ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), 14));
    ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(CaretBox), V4(1,1,1,1));  
  }
  
  return Result;
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


  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), GlobalState->ImguiContext.FontSize);
  r32 TextWidth = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), GlobalState->ImguiContext.FontSize, (utf8_byte*) Header).X;
  v2 TextOrigin = V2(HeaderBarRect.X + (HeaderBarRect.W - TextWidth) * 0.5f, HeaderBarRect.Y + DescentOffset);
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextOrigin, HeaderBarRect, GlobalState->ImguiContext.FontSize, (utf8_byte *) Header, V4(1.0,1.0,1.0,1.0));  
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(HeaderBarRect), menu::GetColor(&GlobalState->ColorTable, "seal brown"));
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

  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ButtonRect), Color);
  return ImguiIsActive(Id);
}

utf8_string_buffer SetStringToFit(r32 FontSize, r32 MaxWidth, c8* Text, c8* Suffix = "...")
{
  size_t ByteSize = jstr::StringLength(Text) + jstr::StringLength(Suffix) + 1;
  utf8_string_buffer Buff = CreateTempStringBuffer(ByteSize);

  size_t CharCount = 0;
  if(GetCharsCountToFitCanonicalSpace(GetRenderSystem(), FontSize, MaxWidth, (utf8_byte*) Text, (utf8_byte*) Suffix, &CharCount)){
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

  ecs::render::window_size_pixel WindowSize = ecs::render::GetWindowSize(GetRenderSystem());
  v2 ClickOffset = {};
  v4 Color = menu::GetColor(&GlobalState->ColorTable, "plum");
  r32 ButtonTextWidth = ButtonWidth;
  r32 ButtonTextHeight = ButtonHeight;
  if(ImguiIsHot(Id) && ImguiIsActive(Id)) {
    // Button is Highlighted and pressed
    if(ClickOffsetPx != 0){
      ClickOffset.X = ClickOffsetPx/WindowSize.ApplicationWidth;
      ClickOffset.Y = -ClickOffsetPx/WindowSize.ApplicationWidth; 
    }
    Color = menu::GetColor(&GlobalState->ColorTable, "waterspout");
  }else if(ImguiIsActive(Id)){
    // Button is Pressed
    if(ClickOffsetPx != 0){
      ClickOffset.X = 4.f/WindowSize.ApplicationWidth;
      ClickOffset.Y = -4.f/WindowSize.ApplicationWidth;
    }
    Color = menu::GetColor(&GlobalState->ColorTable, "old gold");
  }else if(ImguiIsHot(Id)){
    // Button is only highlighted
    Color = menu::GetColor(&GlobalState->ColorTable, "khaki");
    if(FontSize && Text && *Text != '\0')
    {
      r32 TextWidth = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), FontSize, (utf8_byte*) Text).X;
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
    r32 ShadowOffsetX =  ShadowOffsetPx/WindowSize.ApplicationWidth;
    r32 ShadowOffsetY = -ShadowOffsetPx/WindowSize.ApplicationWidth;
    rect2f ShadowRect = Rect2f(
      CenterRect.X + ShadowOffsetX,
      CenterRect.Y + ShadowOffsetY, ButtonTextWidth, ButtonHeight);
    ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), ShadowRect, menu::GetColor(&GlobalState->ColorTable, "rich carmine"));
  }

  rect2f ButtonRect = Rect2f(
    CenterRect.X + ClickOffset.X,
    CenterRect.Y + ClickOffset.Y, ButtonTextWidth, ButtonHeight);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), ButtonRect, Color);

  if(FontSize && Text && *Text != '\0')
  {
    r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), FontSize);
    v2 TextOrigin = V2(ButtonX + TextOffsetX,ButtonY + TextOffsetY) + ClickOffset + V2(0, DescentOffset);
    utf8_string_buffer StringBuffer = SetStringToFit(FontSize, ButtonTextWidth, Text);
    ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextOrigin , Rect2f(ButtonX,ButtonY,ButtonTextWidth,ButtonHeight), FontSize, StringBuffer.Buffer, V4(1.0,1.0,1.0,1.0));  
  }
  
  return ImguiIsActive(Id);
}
