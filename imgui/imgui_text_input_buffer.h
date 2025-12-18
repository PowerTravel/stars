#pragma once

#include "imgui.h"

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
void ReadNumber(imgui_context* ImguiContext, imgui_id ID, imgui_text_input_buffer* TextInputBuffer, rect2f DialogRect, jwin::device_input* Input, float (*DataToFloat)(void* Data), void (*StoreFloat)(float Val, void* Data), void* Data);
void ClearBuffer(imgui_text_input_buffer* TextInputBuffer);
void PushString(imgui_text_input_buffer* TextInputBuffer, c8* String);
