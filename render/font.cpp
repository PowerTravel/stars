#pragma once

#include "font.h"
#include "platform/jfont.h"

namespace render {



font font::Create(render_group* RenderGroup, const char* FontFilePath)
{
  //char FontPath[] = "C:\\Windows\\Fonts\\consola.ttf";
  debug_read_file_result TTFFile = Platform.DEBUGPlatformReadEntireFile(FontFilePath);
  Assert(TTFFile.Contents);

  font Result = {};
  Result.CharCount = 0x100;          // The number of chars to load, alot of them are empty 
  Result.TextPixelSize = 64;         // Size of each SDF-Codepoint in pixels
  Result.OnedgeValue = 128;          // "Brightness" of the sdf. Higher value makes the SDF bigger and brighter.
                                     // Has no impact on TextPixelSize since the char then is also bigger.
  Result.Padding = 3;                // 3 Pixels between Codepoints
  Result.PixelDistanceScale = 32.0;  // Smoothness of how fast the pixel-value goes to zero. Higher PixelDistanceScale, makes it go faster to 0;
                                     // Lower PixelDistanceScale and Higher OnedgeValue gives a 'sharper' sdf.
  Result.FontRelativeScale = 1.f;    // Not sure if this is needed. This is a paramteter sent when rendering

  Result.Font = jfont::LoadSDFFont(PushArray(GlobalPersistentArena, Result.CharCount, jfont::sdf_fontchar),
    Result.CharCount, TTFFile.Contents, Result.TextPixelSize, Result.Padding, Result.OnedgeValue, Result.PixelDistanceScale);

  midx AtlasFileSize = jfont::SDFAtlasRequiredMemoryAmount(&Result.Font);
  Result.FontAtlas = jfont::CreateSDFAtlas(&Result.Font, PushArray(GlobalPersistentArena, AtlasFileSize, u8));


  texture_params FontTexParam = DefaultColorTextureParams();
  FontTexParam.TextureFormat = texture_format::R_8;
  FontTexParam.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  Result.FontMapHandle = PushNewTexture2D(RenderGroup, Result.FontAtlas.AtlasWidth, Result.FontAtlas.AtlasHeight, FontTexParam, Result.FontAtlas.AtlasPixels);
  Platform.DEBUGPlatformFreeFileMemory(TTFFile.Contents);
  return Result;
}


r32 font::GetScale(r32 PixelSize)
{
  r32 Result = jfont::GetScaleFromPixelSize(&Font, PixelSize);
  return Result;
}

r32 font::GetLineSpacing(r32 PixelSize)
{
  r32 SizePixel = jfont::GetLineSpacingPixelSpace(&Font, PixelSize);
  return SizePixel;
}

r32 font::GetLineSpacingCanonicalSpace(r32 PixelSize)
{
  r32 SizePixel = jfont::GetLineSpacingPixelSpace(&Font, PixelSize);
  r32 Result = PixelToCanonicalHeight(SizePixel);
  return Result;
}

r32 font::GetCanonicalFontDescenOffset(r32 PixelSize)
{
  r32 FontDescent = -Font.Descent;
  r32 DecentCan = PixelToCanonicalHeight(FontDescent);
  r32 Scale =   GetScale(PixelSize);
  r32 Result = Scale*DecentCan;
  return Result;
}

// Calculates the number of characters to fit within a given MaxWidthPixelSpace leaving space for a suffix.
// The current usecase is if we have the string 
//    "Hello world"
// We may want to print
//    "Hello w..."
// if the last 'd' does not fit. Result would be 7.
// If suffix is null or the empty string the number result would be 10. (Hello worl)

// Sets CharCountRet with the number of chars in Text that will fit for the suffix to also have space if Text is longer than MaxWidthPixelSpace,
// Returns true if all of Text fits, otherwise false.
b32 font::GetCharsCountToFit(r32 PixelSize, r32 MaxWidthPixelSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet)
{ 
  SCOPED_TRANSIENT_ARENA;
  r32 FontRelativeScale = GetScale(PixelSize);

  r32 TextWidth = GetTextSize(PixelSize, Text).X;
  b32 Result = true;
  if(TextWidth > MaxWidthPixelSpace)
  {
    r32 SuffixWidth = GetTextSize(PixelSize, Suffix).X;
    MaxWidthPixelSpace = Maximum(MaxWidthPixelSpace - SuffixWidth, 0);
    Result = false;
  }
  
  codepoint* TextCodePoints = PushArray(GlobalTransientArena, jstr::StringLength((const char*)Text)+1, codepoint);
  ConvertToUnicode((utf8_byte*) Text, TextCodePoints);
  *CharCountRet = jfont::GetUnicodeCharCountThatFitsInSize(&Font, FontRelativeScale, MaxWidthPixelSpace, TextCodePoints);

  return Result;
}

b32 font::GetCharsCountToFitCanonicalSpace(r32 PixelSize, r32 MaxWidthCanonicalSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet)
{
  r32 MaxWidthPixelSpace = CanonicalToPixelSpace(V2(MaxWidthCanonicalSpace,0)).X;
  b32 Result = GetCharsCountToFit(PixelSize, MaxWidthPixelSpace, Text, Suffix, CharCountRet);
  return Result;
}


v2 font::GetTextSize(r32 PixelSize, utf8_byte const * Text)
{ 
  if(!Text) return {};

  SCOPED_TRANSIENT_ARENA;
  r32 FontRelativeScale = GetScale(PixelSize);
  u32 Length = jstr::StringLength((const char*)Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, Length+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode((utf8_byte*) Text, CodePoints);
  v2 Result = {};
  jfont::GetTextDim(&Font, FontRelativeScale, &Result.X, &Result.Y, CodePoints);
  return Result;
}


v2 font::GetTextSizeCanonicalSpace(r32 PixelSize, utf8_byte const * Text)
{
  v2 PixelPos = GetTextSize(PixelSize, Text);
  v2 CanonicalPos = PixelToCanonicalSpace(PixelPos);
  return CanonicalPos;
}


} // namespace render