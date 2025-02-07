#pragma once

#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "containers/chunk_list.h"
#include "math/rect2f.h"

namespace ecs {

namespace render {

  namespace data {
  enum class programs{
    PHONG,
    TRANSPARENT_PHONG,
    TRANSPARENT_COMPOSITION
  };

  struct font
  {
    int TextPixelSize;
    int OnedgeValue;
    float PixelDistanceScale;
    float FontRelativeScale;
    jfont::sdf_font Font;
    jfont::sdf_atlas FontAtlas;
  };

  

  }// namespace data

  struct window_size_pixel {
    r32 WindowWidth;
    r32 WindowHeight;
    r32 MonitorWidth;
    r32 MonitorHeight;
    r32 MonitorDPI;
    r32 EffectiveDPI;
    r32 ApplicationAspectRatio;
    r32 ApplicationWidth;
    r32 ApplicationHeight;
    r32 MSAA;
  };


  struct system {
    memory_arena Arena;
    render_group* RenderGroup;
//    chunk_list SolidObjects;
//    chunk_list TransparentObjects;
    chunk_list OverlayText;
    data::font Font;
    u32 FontTextureHandle;

    window_size_pixel WindowSize;
  };

  void SetWindowSize(system* System, application_render_commands* RenderCommands)
  {
    System->WindowSize.WindowWidth       = (r32) RenderCommands->WindowInfo.Width;
    System->WindowSize.WindowHeight      = (r32) RenderCommands->WindowInfo.Height;
  }
  system* CreateRenderSystem(render_group* RenderGroup, r32 ResolutionWidth, r32 ResolutionHeight, application_render_commands* RenderCommands);
  void Draw(entity_manager* EntityManager, system* RenderSystem, m4 ProjectionMatrix, m4 ViewMatrix);
  void DrawOverlayText(system* RenderSystem, utf8_byte* Text, u32 X0, u32 Y0, r32 RelativeScale);

  window_size_pixel GetWindowSize(system* System)
  {
    return System->WindowSize;
  }


  v2 PixelToCanonicalSpace(system* System, v2 PixelPos);
  v2 GetTextSizePixelSpace(system* System, r32 PixelSize, utf8_byte const * Text);
  v2 GetTextSizeCanonicalSpace(system* System, r32 PixelSize, utf8_byte const * Text);

  void DrawTextPixelSpace(system* System, v2 PixelPos, r32 PixelSize, utf8_byte const * Text);
  void DrawTextCanonicalSpace(system* System, v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color = V4(1,1,1,1));

  void PushOverlayQuad(system* System, rect2f Rect, v4 Color){}
  void PushTexturedOverlayQuad(system* System, rect2f Rect, rect2f TextureCoords, u32 TextureHandle){};

  

  r32 GetLineSpacingPixelSpace(system* System, r32 PixelSize)
  {
    r32 SizePixel = jfont::GetLineSpacingPixelSpace(&System->Font.Font, PixelSize);
    return SizePixel;
  }
  r32 GetCanonicalFontHeightFromPixelSize(system* System, r32 PixelSize)
  {
    r32 SizePixel =  GetLineSpacingPixelSpace(System, PixelSize);
    v2 Result = PixelToCanonicalSpace(System, V2(0, SizePixel));
    return Result.Y;
  }

  r32 GetScaleFromPixelSize(system* System, r32 PixelSize)
  {
    r32 Result = jfont::GetScaleFromPixelSize(&System->Font.Font, PixelSize);
    return Result;
  }
}
}
