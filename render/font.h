#pragma once

#include "commons/utf8.h"
#include "render.h"
namespace render {

// Everythign is in pixel space
struct font
{
  int CharCount;
  int TextPixelSize;
  int OnedgeValue;
  int Padding;
  float PixelDistanceScale;
  float FontRelativeScale;
  int FontMapHandle;
  jfont::sdf_font Font;
  jfont::sdf_atlas FontAtlas;

  static font Create(render_group* RenderGroup, const char* FontFilePath);

  r32 GetScale(r32 PixelSize);
  r32 GetLineSpacing(r32 PixelSize);
  v2  GetTextSize(r32 PixelSize, utf8_byte const * Text);
  r32 GetLineSpacingCanonicalSpace(r32 PixelSize);
  r32 GetCanonicalFontDescenOffset(r32 PixelSize);

  v2 GetTextSizeCanonicalSpace(r32 PixelSize, utf8_byte const * Text);

  b32 GetCharsCountToFit(    r32 PixelSize, r32 MaxWidthPixelSpace,     utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet);
  b32 GetCharsCountToFitCanonicalSpace(r32 PixelSize, r32 MaxWidthCanonicalSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet);

  void DrawTextPixelSpace(v2 PixelPos, rect2f PixelClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color);
  void DrawTextCanonicalSpace(v2 CanonicalPos, rect2f CanonicalClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color);

  void DrawTextPixelSpace(v2 PixelPos, r32 PixelSize, utf8_byte const * Text);
  void DrawTextCanonicalSpace(v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color = V4(1,1,1,1));
};





} // namespace render