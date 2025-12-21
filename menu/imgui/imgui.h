#pragma once

#include "commons/types.h"
#include "platform/jwin_platform_input.h"
#include "platform/text_input.h"
#include "render/render.h"

enum icon {
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

namespace imgui { 

struct icon_atlas {
  u32 Atlas;
  v4 Coordinates[icon::ICON_COUNT];
};

struct id {
  u32 id;
  b32 idEdge;
};

enum button_state {
  INACTIVE   = 0,
  ACTIVE     = 1<<0,
  HOT        = 1<<1,
  CLICKED    = 1<<2,
  RELEASED   = 1<<3,
  SELECTED   = 1<<4,
  DESELECTED = 1<<5
};

// Styling of interactive elements
struct region_styling {
  v4 InactiveColor;
  v4 ActiveAndHotColor;
  v4 ActiveColor;
  v4 HotColor;
  v4 SelectedColor;
  v4 ShadowColor;
  v2 ClickOffset;
  v2 ShadowOffset;
};

struct text_styling {
  render::font* Font;
  r32 FontSize;
  v4 TextColor;
};


struct imgui_button_color {
  v4 InactiveColor;
  v4 ActiveAndHotColor;
  v4 ActiveColor;
  v4 HotColor;
};

struct context {
  
  icon_atlas Icons;

  u32 ButtonCounter;

  id ActiveID;
  id PreviouslyActiveID;
  id HotID;
  id PreviouslyHotID;
  id SelectedID;
  id PreviouslySelectedID;

  r32 MouseX;
  r32 MouseY;
  r32 MouseDZ;
  jwin::binary_signal_state LeftMouse;

  r32 FontSize;
};

} // namespace imgui
extern imgui::context* GlobalImguiContext;

namespace imgui {

icon_atlas LoadImguiIcons(render_group* RenderGroup);

inline id Update(id Id, u32 Value)
{
  Id.idEdge = (Id.id != Value); // <- Edge is true if Id changed
  Id.id = Value;
  return Id;
}

inline id NewButtonID()
{
  id Result = {};
  Result.id = ++GlobalImguiContext->ButtonCounter;
  return Result;
}

inline b32 IsActive(id Id) {
  return GlobalImguiContext->ActiveID.id == Id.id;
}
inline b32 IsActive(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::ACTIVE;
}
inline b32 IsInactive() {
  return GlobalImguiContext->ActiveID.id == 0;
}
inline b32 IsInactive(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::INACTIVE;
}
inline b32 IsHot(id Id) {
  return GlobalImguiContext->HotID.id == Id.id;
}
inline b32 IsHot(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::HOT;
}
inline b32 IsClicked(id Id) {
  return GlobalImguiContext->ActiveID.id == Id.id && GlobalImguiContext->ActiveID.idEdge;
}
inline b32 IsClicked(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::CLICKED;
}
inline b32 IsReleased(id Id) {
  return GlobalImguiContext->ActiveID.id != Id.id &&
         GlobalImguiContext->PreviouslyActiveID.id == Id.id &&
         GlobalImguiContext->PreviouslyActiveID.idEdge;
}
inline b32 IsReleased(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::RELEASED;
}
inline b32 IsSelected(id Id) {
  return GlobalImguiContext->SelectedID.id == Id.id;
}
inline b32 IsSlected(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::SELECTED;
}
inline b32 BecameSlected(u32 ButtonResult) {
  return (ButtonResult == imgui::button_state::SELECTED) && (ButtonResult == imgui::button_state::CLICKED);
}
inline b32 IsDeselected(id Id) {
  return GlobalImguiContext->PreviouslySelectedID.id == Id.id && Id.id != GlobalImguiContext->SelectedID.id;
}
inline b32 IsDeselected(u32 ButtonResult) {
  return ButtonResult & imgui::button_state::DESELECTED;
}
inline b32 ImguiIsDragging(){
  return GlobalImguiContext->ActiveID.id == -1;
}
inline b32 NoneSelected() {
  return GlobalImguiContext->SelectedID.id == 0;
}

void Begin(jwin::device_input* Input);

void End();




imgui_button_color ImguiDefaultButtonColor();
v4 ImguiGetButtonColor(id ButtonId, imgui_button_color ButtonColors);
b32 ImguiButton(context* ImguiContext, id Id, rect2f ButtonRect);
u32 DoButton(context* ImguiContext, id Id, rect2f ButtonRect);
void DrawButton(u32 ButtonState, rect2f ButtonRect, const region_styling& Styling);
b32 ImguiPlainButton(context* ImguiContext, id Id, rect2f ButtonRect, const imgui_button_color& ButtonColor);
u32 ImguiTextButton(id Id, u32 FontSize, c8* Text, r32 ButtonX, r32 ButtonY, r32 ButtonWidth, r32 ButtonHeight, r32 TextOffsetX, r32 TextOffsetY, r32 ClickOffsetPx, r32 ShadowOffsetPx);
b32 ImguiSelectabeRegion(context* ImguiContext, id Id, rect2f RegionRect, jwin::device_input* Input);

} // namespace imgui