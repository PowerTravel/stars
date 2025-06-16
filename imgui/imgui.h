#pragma once

#include "commons/types.h"
#include "platform/jwin_platform_input.h"
#include "platform/text_input.h"

enum imgui_icon {
  ICON_DOUBLE_ANGLE_UP,
  ICON_DOUBLE_ANGLE_DOWN,
  ICON_DOUBLE_ANGLE_LEFT,
  ICON_DOUBLE_ANGLE_RIGHT,
  ICON_ANGLE_UP,
  ICON_ANGLE_DOWN,
  ICON_ANGLE_LEFT,
  ICON_ANGLE_RIGHT,
  ICON_ADD,
  ICON_SUBTRACT,
  ICON_SEARCH,
  ICON_FILTER,
  ICON_CHECBOX,
  ICON_EMPTY_CHECKBOX,
  ICON_WINDOW_MINIMIZE,
  ICON_WINDOW_X,
  ICON_WINDOW_MAXIMIZE,
  ICON_WINDOW_RESTORE,
  ICON_WINDOW_DETACH,
  ICON_COUNT
};

struct imgui_icon_atlas {
  u32 Atlas;
  v4 Coordinates[20];
};

struct imgui_id {
  u32 id;
  b32 idEdge;
};

struct imgui_context {
  
  imgui_icon_atlas Icons;

  u32 ButtonCounter;

  imgui_id ActiveID;
  imgui_id HotID;
  imgui_id SelectedID;

  r32 MouseX;
  r32 MouseY;
  r32 MouseDZ;
  jwin::binary_signal_state LeftMouse;

  r32 FontSize;
};


extern imgui_context* GlobalImguiContext;

imgui_icon_atlas LoadImguiIcons(render_group* RenderGroup);


inline imgui_id Update(imgui_id Id, u32 Value)
{
  Id.idEdge = (Id.id != Value); // <- Edge is true if Id changed
  Id.id = Value;
  return Id;
}

imgui_id NewButtonID()
{
  imgui_id Result = {};
  Result.id = ++GlobalImguiContext->ButtonCounter;
  return Result;
}

b32 ImguiIsActive(imgui_id Id){
  return GlobalImguiContext->ActiveID.id == Id.id;
}

b32 ImguiIsDragging(){
  return GlobalImguiContext->ActiveID.id == -1;
}

b32 ImguiIsInactive(){
  return GlobalImguiContext->ActiveID.id == 0;
}

b32 ImguiIsHot(imgui_id Id){
  return GlobalImguiContext->HotID.id == Id.id;
}

void ImguiSetActive(imgui_id Id) {
  GlobalImguiContext->ActiveID = Update(GlobalImguiContext->ActiveID, Id.id);
}

void ImguiSetInactive() {
  GlobalImguiContext->ActiveID = Update(GlobalImguiContext->ActiveID, 0);
}

void ImguiSetSelected(imgui_id Id) {
  GlobalImguiContext->SelectedID = Update(GlobalImguiContext->SelectedID, Id.id);
}

void ImguiDeselect() {
  GlobalImguiContext->SelectedID = Update(GlobalImguiContext->SelectedID, 0);
}

b32 ImguiNoneSelected() {
  return GlobalImguiContext->SelectedID.id == 0;
}

b32 ImguiIsSelected(imgui_id Id) {
  return GlobalImguiContext->SelectedID.id == Id.id;
}

void ImguiSetDragging() {
  GlobalImguiContext->ActiveID = Update(GlobalImguiContext->ActiveID, -1);
}

void ImguiSetHot(imgui_id Id){
  GlobalImguiContext->HotID = Update(GlobalImguiContext->HotID, Id.id);
}

void ImguiSetCold(){
  GlobalImguiContext->HotID = Update(GlobalImguiContext->HotID, 0);
}

b32 ImguiMenuPushed(imgui_id Id) 
{
  return GlobalImguiContext->ActiveID.id == Id.id && GlobalImguiContext->ActiveID.idEdge;
}

void ImguiBegin(jwin::device_input* Input){
  ImguiSetCold();
  ImguiSetActive(GlobalImguiContext->ActiveID);
  ImguiSetSelected(GlobalImguiContext->SelectedID);
  GlobalImguiContext->MouseX = Input->Mouse.X;
  GlobalImguiContext->MouseY = Input->Mouse.Y;
  GlobalImguiContext->MouseDZ = Input->Mouse.dZ;
  GlobalImguiContext->LeftMouse = Input->Mouse.Button[jwin::MouseButton_Left];
  GlobalImguiContext->FontSize = 14;
}

void ImguiEnd(){
  if(!jwin::Active(GlobalImguiContext->LeftMouse)) {
    ImguiSetInactive();
  }else if(ImguiIsInactive()){
    ImguiSetDragging();
  }
}


struct imgui_scrollable_list {
  imgui_id VerticalScrollbarId;
  imgui_id HorizontalScrollbarId;

  s32 SelectedRow;
  v2 ScrollAmmount;
  v2 ScrollButtonDiff;
};

imgui_scrollable_list CreateScrollableTextList();
b32 ImguiScrollableButtonList(imgui_scrollable_list* ScrollableList, v2 Pos, v2 Size, u32 RowCount, r32 RowHeight, imgui_id* RowIDs, void* Data, 
  void (RowRenderFunction)(imgui_context* ImguiContext, imgui_id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data));


struct imgui_text_input_buffer {
  imgui_id ID;
  utf8_string_buffer Buffer;
  u32 CaretPosition;
  u32 CharCount;
};

imgui_text_input_buffer ImguiNewTextInputBuffer(s32 InputLen, utf8_byte* InputBuffer);
void ImguiReadInput(imgui_text_input_buffer* TextInputBuffer, jwin::device_input* Input);
b32 ImguiTextDialog(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, v2 DialogPos, v2 TextWidth, v4 BackgroundColor);


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
void ImguiBorderWindow(imgui_bordered_window* BorderWindow, const char Header[]);


struct imgui_button_color {
  v4 InactiveColor;
  v4 ActiveAndHotColor;
  v4 ActiveColor;
  v4 HotColor;
};

imgui_button_color ImguiDefaultButtonColor();
v4 ImguiGetButtonColor(imgui_id ButtonId, imgui_button_color ButtonColors);
b32 ImguiButton(imgui_context* ImguiContext, imgui_id Id, rect2f ButtonRect);
b32 ImguiPlainButton(imgui_context* ImguiContext, imgui_id Id, rect2f ButtonRect, imgui_button_color ButtonColor);
u32 ImguiTextButton(imgui_id Id, u32 FontSize, c8* Text, r32 ButtonX, r32 ButtonY, r32 ButtonWidth, r32 ButtonHeight, r32 TextOffsetX, r32 TextOffsetY, r32 ClickOffsetPx, r32 ShadowOffsetPx);
b32 ImguiSelectabeRegion(imgui_context* ImguiContext, imgui_id Id, rect2f RegionRect);