#include "utility.h"

void DrawText(v2 TextLineOrigin, r32 PixelSize, utf8_byte* Text)
{
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextLineOrigin, PixelSize, Text);
}

void DrawTextInRect(rect2f Rect, r32 XOffset, r32 PixelSize, utf8_byte* Text)
{
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), PixelSize);
  r32 LineSpacing = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), PixelSize);
  r32 YRectOffset = (Rect.H - LineSpacing) * 0.5f;
  v2 TextOriginInRect =  V2(Rect.X + XOffset, Rect.Y + DescentOffset + YRectOffset);
  DrawText(TextOriginInRect, PixelSize, Text);
}
