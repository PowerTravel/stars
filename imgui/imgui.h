#pragma once

#include "commons/types.h"
#include "platform/jwin_platform_input.h"
#include "platform/text_input.h"
#include "render/render.h"

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
  ICON_COMPONENT_GEOMETRY,
  ICON_COMPONENT_MATERIAL,
  ICON_COMPONENT_CAMERA,
  ICON_COMPONENT_LOCATION,
  ICON_COMPONENT_LIGHT,
  ICON_COMPONENT_CONTROLLER,
  ICON_COMPONENT_COLLIDER,
  ICON_COMPONENT_UNKNOWN,
  ICON_COUNT
};

struct imgui_icon_atlas {
  u32 Atlas;
  v4 Coordinates[imgui_icon::ICON_COUNT];
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
  imgui_id PreviouslySelectedID;

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
  GlobalImguiContext->PreviouslySelectedID = Update(GlobalImguiContext->PreviouslySelectedID, GlobalImguiContext->SelectedID.id);
  GlobalImguiContext->SelectedID = Update(GlobalImguiContext->SelectedID, Id.id);
}

b32 ImguiIsSelected(imgui_id Id) {
  return GlobalImguiContext->SelectedID.id == Id.id;
}

b32 ImguiWasDeselected(imgui_id Id) {
  return GlobalImguiContext->PreviouslySelectedID.id == Id.id && Id.id != GlobalImguiContext->SelectedID.id;
}

void ImguiDeselect(imgui_id Id) {
  if(ImguiIsSelected(Id)){
    ImguiSetSelected({});
  }
}

b32 ImguiNoneSelected() {
  return GlobalImguiContext->SelectedID.id == 0;
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
  utf8_string_buffer Buffer;
  s32 CaretPosition;
  b32 SelectMode;
  s32 SelectionStart;
  s32 CharCount;
};

imgui_text_input_buffer ImguiNewTextInputBuffer(s32 InputLen, utf8_byte* InputBuffer);
void ImguiReadInput(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, jwin::device_input* Input, v2 MousePos, b32 HighlightAll = false);
b32 ImguiTextDialog(imgui_text_input_buffer* TextInputBuffer, imgui_id DialogID, v2 DialogPos, v2 TextWidth, v4 BackgroundColor);
void ClearBuffer(imgui_text_input_buffer* TextInputBuffer);
void PushString(imgui_text_input_buffer* TextInputBuffer, c8* String);

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

struct imgui_row { 

  struct padding {
    r32 Width;
    r32 Height;
  };
  static padding Padding(r32 Width, r32 Height);

  struct icon {
    r32 Size;
    u32 IconType;
    v4 Color;
  };
  static inline icon Icon(r32 Size, u32 IconType);

  struct text {
    size_t TextLen;
    const char* Text;
    r32 FontSize;
    v4 Color;
    render::font* Font;
  };
  static inline text Text(r32 FontSize, render::font* Font, size_t TextLen, const char* Text);

  enum class type {
    PADDING,
    ICON,
    TEXT,
    DIV_HINT
  };

  struct header { 
    type Type;
    v2 Size;
    void* Data;
    header* Next;
  };

  struct div_hint {
    struct header* Header;
    div_hint* Next;
  };
  static inline div_hint DivHint();

  u32 m_divCount;
  div_hint* m_divHead;
  div_hint* m_divTail;
  header* m_head;
  header* m_tail;
  
  v2 GetSize(v2* ResultVec);
  v2 GetSizeOfDiv(imgui_row::header* Start, imgui_row::header** ResultEnd);
  r32 Draw(r32 X, r32 Y, rect2f ClipRect, r32 RowNum);
  void Push(padding Padding);
  void Push(icon Icon);
  void Push(text Text);
  void Push(div_hint DivHint);
  void Push(header* Header);
};
imgui_row ImguiRow();
v2 CalculateDivSize(imgui_row::header* H);

imgui_button_color ImguiDefaultButtonColor();
v4 ImguiGetButtonColor(imgui_id ButtonId, imgui_button_color ButtonColors);
b32 ImguiButton(imgui_context* ImguiContext, imgui_id Id, rect2f ButtonRect);
b32 ImguiPlainButton(imgui_context* ImguiContext, imgui_id Id, rect2f ButtonRect, imgui_button_color ButtonColor);
u32 ImguiTextButton(imgui_id Id, u32 FontSize, c8* Text, r32 ButtonX, r32 ButtonY, r32 ButtonWidth, r32 ButtonHeight, r32 TextOffsetX, r32 TextOffsetY, r32 ClickOffsetPx, r32 ShadowOffsetPx);
b32 ImguiSelectabeRegion(imgui_context* ImguiContext, imgui_id Id, rect2f RegionRect, jwin::device_input* Input);