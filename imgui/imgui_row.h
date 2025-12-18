#pragma once

#include "imgui.h"

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
    imgui_id ImguiID;
    header* Next;
  };

  struct div_hint {
    r32 Padding; // If this div hint is breaking the row in two, use this padding
    struct header* Header;
    div_hint* Next;
  };
  static inline div_hint DivHint(r32 Padding);

  u32 m_divCount;
  div_hint* m_divHead;
  div_hint* m_divTail;
  header* m_head;
  header* m_tail;

  void Push(padding Padding);
  void Push(icon Icon, imgui_id ImguiID = {});
  void Push(text Text, imgui_id ImguiID = {});
  void Push(div_hint DivHint);
  void Push(header* Header);
};

v2 CalculateDivSize(imgui_row::header* H);
imgui_row ImguiRow();
