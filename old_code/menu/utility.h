#pragma once

#include "math/vector_math.h"

void DrawText(v2 TextLineOrigin, r32 PixelSize, utf8_byte* Text);
void DrawTextInRect(rect2f Rect, r32 XOffset, r32 PixelSize, utf8_byte* Text);
