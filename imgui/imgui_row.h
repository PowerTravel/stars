#pragma once

#include "imgui.h"

namespace imgui { 

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
    id ImguiID;
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
  void Push(icon Icon, id ImguiID = {});
  void Push(text Text, id ImguiID = {});
  void Push(div_hint DivHint);
  void Push(header* Header);
};

struct reactive_row_size {
  rect2f RowRect;// Rect holding the combined size of the total row;

  u32 DivCount;
  imgui_row::div_hint** Divs;       // All divHints for row
  rect2f* DivRects; // Rect holding the size of each div;

  u32 SplitCount;
  rect2f* SplitRowRects; // Rect holding the size of each split row.
};

struct reactive_size {
  u32 RowCount;       // Total number of rows to draw
  reactive_row_size* ReactiveRowSizes;
  v2 TotalSize;
};

reactive_size CreateReactiveSize(cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect, r32 ScrollAmount);

} // namespace imgui 