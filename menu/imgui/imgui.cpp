#include "imgui.h"
#include "commons/string.h"
#include "platform/jwin_platform_memory.h"

namespace imgui { 

file_local void ActivateID(id Id) {
  GlobalImguiContext->ActiveID = Update(GlobalImguiContext->ActiveID, Id.id);
}

file_local void SelectID(id Id) {
  GlobalImguiContext->PreviouslySelectedID = Update(GlobalImguiContext->PreviouslySelectedID, GlobalImguiContext->SelectedID.id);
  GlobalImguiContext->SelectedID = Update(GlobalImguiContext->SelectedID, Id.id);
}

file_local void DeselectID(id Id) {
  if(IsSelected(Id)){
    SelectID({});
  }
}

file_local void SetHotID(id Id){
  GlobalImguiContext->PreviouslyHotID = Update(GlobalImguiContext->PreviouslyHotID, GlobalImguiContext->HotID.id);
  GlobalImguiContext->HotID = Update(GlobalImguiContext->HotID, Id.id);
}

file_local void SetDragging() {
  GlobalImguiContext->ActiveID = Update(GlobalImguiContext->ActiveID, -1);
}

file_local void SetCold(){
  GlobalImguiContext->HotID = Update(GlobalImguiContext->HotID, 0);
}

void Begin(jwin::device_input* Input){
  SetCold();
  ActivateID(GlobalImguiContext->ActiveID);
  SelectID(GlobalImguiContext->SelectedID);
  GlobalImguiContext->MouseX = Input->Mouse.X;
  GlobalImguiContext->MouseY = Input->Mouse.Y;
  GlobalImguiContext->MouseDZ = Input->Mouse.dZ;
  GlobalImguiContext->LeftMouse = Input->Mouse.Button[jwin::MouseButton_Left];
  GlobalImguiContext->FontSize = 14;
}

void End(){
  if(!jwin::Active(GlobalImguiContext->LeftMouse)) {
    // Set inactive.
    GlobalImguiContext->PreviouslyActiveID = Update(GlobalImguiContext->PreviouslyActiveID, GlobalImguiContext->ActiveID.id);
    GlobalImguiContext->ActiveID = Update(GlobalImguiContext->ActiveID, 0);
  }else if(IsInactive()){
    SetDragging();
  }
}



file_local inline v4 PositionToCoordinate(u32 X, u32 Y, u32 IconSizePx, u32 AtlasSizePx) {
  r32 X0 = (X * IconSizePx) + 1;
  r32 Y0 = (Y * IconSizePx) + 1;
  r32 X1 = ((X+1) * IconSizePx) - 1;
  r32 Y1 = ((Y+1) * IconSizePx) - 1;
  r32 OneOverSize = 1.f / (r32) AtlasSizePx;
  v4 Result = V4(
     X0*OneOverSize,  // u0
     Y0*OneOverSize,  // v0
     X1*OneOverSize,  // u1
     Y1*OneOverSize); // v1
  return Result;
}

file_local u32 PushImguiIconAtlasToGPU(render_group* RenderGroup)
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

icon_atlas LoadImguiIcons(render_group* RenderGroup)
{
  icon_atlas Icons = {};
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

b32 ImguiSelectabeRegion(context* ImguiContext, id Id, rect2f RegionRect, jwin::device_input* Input)
{
  if(Intersects(RegionRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    SetHotID(Id);
    if(IsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ActivateID(Id);
      SelectID(Id);
    }
  }else if(jwin::Pushed(GlobalState->ImguiContext.LeftMouse)){
    DeselectID(Id);
  }

  if(jwin::Pushed(Input->Keyboard.Key_ENTER)) {
    DeselectID(Id);
  }else if(jwin::Pushed(Input->Keyboard.Key_ESCAPE)){
    DeselectID(Id);
  } 

  return IsSelected(Id) || IsActive(Id);
}





// Buttons
imgui_button_color ImguiDefaultButtonColor()
{
  imgui_button_color Result = {};
  Result.InactiveColor =  imgui::GetColor(&GlobalState->ColorTable, "taupe");
  Result.ActiveAndHotColor = imgui::GetColor(&GlobalState->ColorTable, "old gold"); 
  Result.ActiveColor = imgui::GetColor(&GlobalState->ColorTable, "sandy taupe");
  Result.HotColor = imgui::GetColor(&GlobalState->ColorTable, "sandy taupe");
  return Result;
}

v4 ImguiGetButtonColor(id ButtonId, imgui_button_color ButtonColors){
  v4 Color = ButtonColors.InactiveColor;
  if(IsHot(ButtonId) && IsActive(ButtonId)) {
    // Button is Highlighted and pressed
    Color = ButtonColors.ActiveAndHotColor;
  }else if(IsActive(ButtonId)){
    // Button is Pressed
    Color = ButtonColors.ActiveColor;
  }else if(IsHot(ButtonId)){
    // Button is only highlighted
    Color = ButtonColors.HotColor;
  }
  return Color;
}

b32 ImguiButton(context* ImguiContext, id Id, rect2f ButtonRect)
{
  if(Intersects(ButtonRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    SetHotID(Id);
    if(IsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ActivateID(Id);
    }
  }

  return IsActive(Id);
}

u32 DoButton(context* ImguiContext, id Id, rect2f ButtonRect) {
  u32 Result = button_state::INACTIVE;
  if(Intersects(ButtonRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    SetHotID(Id);
    if(IsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ActivateID(Id);
      SelectID(Id);
    }
  }else if(jwin::Pushed(GlobalState->ImguiContext.LeftMouse)){
    DeselectID(Id);
  }

/* // Maybe pass some default "deselect keys"
  if(jwin::Pushed(Input->Keyboard.Key_ENTER)) {
    DeselectID(Id);
  }else if(jwin::Pushed(Input->Keyboard.Key_ESCAPE)){
    DeselectID(Id);
  } */


  if(IsHot(Id)){
    Result |= button_state::HOT;
  }
  if(IsActive(Id)){
    Result |= button_state::ACTIVE;
  }
  if(IsClicked(Id)){
    Result |= button_state::CLICKED;
  }
  if(IsReleased(Id)){
    Result |= button_state::RELEASED;
  }
  if(IsSelected(Id)){
    Result |= button_state::SELECTED;
  }
  if(IsDeselected(Id)){
    Result |= button_state::DESELECTED;
  }

  return Result;
}

void DrawButton(u32 ButtonState, rect2f ButtonRect, const region_styling& Styling)
{
  v4 Color = Styling.InactiveColor;
  rect2f Rect = ButtonRect;
  if((ButtonState & button_state::ACTIVE) && (ButtonState & button_state::HOT)) {
    // Button is Highlighted and pressed
    Color = Styling.ActiveAndHotColor;
    if(Styling.ClickOffset.X || Styling.ClickOffset.Y)
    {
      Rect.X += Styling.ClickOffset.X;
      Rect.Y -= Styling.ClickOffset.Y;
    }
  }else if(ButtonState & button_state::ACTIVE){
    // Button is Pressed
    Color = Styling.ActiveColor;
    if(Styling.ClickOffset.X || Styling.ClickOffset.Y)
    {
      Rect.X += Styling.ClickOffset.X;
      Rect.Y -= Styling.ClickOffset.Y;
    }
  }else if(ButtonState & button_state::HOT){
    // Button is only highlighted
    Color = Styling.HotColor;
  }else if(ButtonState & button_state::SELECTED){
    // Button is only highlighted
    Color = Styling.SelectedColor;
  }

  if(Styling.ShadowOffset.X || Styling.ShadowOffset.Y)
  {
    rect2f ShadowRect = Rect2f(
      ButtonRect.X + Styling.ShadowOffset.X,
      ButtonRect.Y - Styling.ShadowOffset.Y, ButtonRect.W, ButtonRect.H);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ShadowRect), Styling.ShadowColor);
  }

  render::DrawOverlayQuadCanonicalSpace(CenteredRect(Rect), Color);
}

b32 ImguiPlainButton(context* ImguiContext, id Id, rect2f ButtonRect, const imgui_button_color& ButtonColor) {
  if(Intersects(ButtonRect, V2(ImguiContext->MouseX, ImguiContext->MouseY)))
  {
    SetHotID(Id);
    if(IsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ActivateID(Id);
    }
  }

  v4 Color = ButtonColor.InactiveColor;
  if(IsHot(Id) && IsActive(Id)) {
    // Button is Highlighted and pressed
    Color = ButtonColor.ActiveAndHotColor;
  }else if(IsActive(Id)){
    // Button is Pressed
    Color = ButtonColor.ActiveColor;
  }else if(IsHot(Id)){
    // Button is only highlighted
    Color = ButtonColor.HotColor;
  }

  render::DrawOverlayQuadCanonicalSpace(CenteredRect(ButtonRect), Color);
  return IsActive(Id);
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

u32 ImguiTextButton(id Id, c8* Text, rect2f ButtonRect, r32 TextOffsetX, r32 TextOffsetY, r32 ClickOffsetPx, r32 ShadowOffsetPx) {
  v2 MousePos = V2(GlobalImguiContext->MouseX,GlobalImguiContext->MouseY);
  if(Intersects(ButtonRect, MousePos))
  {
    SetHotID(Id);
    if(IsInactive() && jwin::Active(GlobalState->ImguiContext.LeftMouse)){
      ActivateID(Id);
    }
  }
  return 0;
}

u32 ImguiTextButton(id Id, u32 FontSize, c8* Text, r32 ButtonX, r32 ButtonY, r32 ButtonWidth, r32 ButtonHeight, r32 TextOffsetX, r32 TextOffsetY, r32 ClickOffsetPx, r32 ShadowOffsetPx) {

  if(GlobalImguiContext->MouseX >= ButtonX && GlobalImguiContext->MouseX <= ButtonX + ButtonWidth &&
     GlobalImguiContext->MouseY >= ButtonY && GlobalImguiContext->MouseY <= ButtonY + ButtonHeight)
  {
    SetHotID(Id);
    if(IsInactive() && jwin::Active(GlobalImguiContext->LeftMouse)){
      ActivateID(Id);
    }
  }

  v2 ClickOffset = {};
  v4 Color = imgui::GetColor(&GlobalState->ColorTable, "plum");
  r32 ButtonTextWidth = ButtonWidth;
  r32 ButtonTextHeight = ButtonHeight;
  if(IsHot(Id) && IsActive(Id)) {
    // Button is Highlighted and pressed
    if(ClickOffsetPx != 0){
      ClickOffset.X = ClickOffsetPx/GlobalWindowSize.ApplicationWidth;
      ClickOffset.Y = -ClickOffsetPx/GlobalWindowSize.ApplicationWidth; 
    }
    Color = imgui::GetColor(&GlobalState->ColorTable, "waterspout");
  }else if(IsActive(Id)){
    // Button is Pressed
    if(ClickOffsetPx != 0){
      ClickOffset.X = 4.f/GlobalWindowSize.ApplicationWidth;
      ClickOffset.Y = -4.f/GlobalWindowSize.ApplicationWidth;
    }
    Color = imgui::GetColor(&GlobalState->ColorTable, "old gold");
  }else if(IsHot(Id)){
    // Button is only highlighted
    Color = imgui::GetColor(&GlobalState->ColorTable, "khaki");
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
    Color = imgui::GetColor(&GlobalState->ColorTable, "taupe");
  }

  v2 CenterRect = V2(ButtonX + ButtonTextWidth * 0.5f, ButtonY + ButtonHeight * 0.5f); 
  if(ShadowOffsetPx!=0)
  {
    r32 ShadowOffsetX =  ShadowOffsetPx/GlobalWindowSize.ApplicationWidth;
    r32 ShadowOffsetY = -ShadowOffsetPx/GlobalWindowSize.ApplicationWidth;
    rect2f ShadowRect = Rect2f(
      CenterRect.X + ShadowOffsetX,
      CenterRect.Y + ShadowOffsetY, ButtonTextWidth, ButtonHeight);
    render::DrawOverlayQuadCanonicalSpace(ShadowRect, imgui::GetColor(&GlobalState->ColorTable, "rich carmine"));
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
  
  return IsActive(Id);
}

} // namespace imgui