#pragma once

#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "containers/chunk_list.h"
#include "math/rect2f.h"

namespace ecs {

namespace render {

  namespace data {

  struct font
  {
    int TextPixelSize;
    int OnedgeValue;
    float PixelDistanceScale;
    float FontRelativeScale;
    jfont::sdf_font Font;
    jfont::sdf_atlas FontAtlas;
  };

  struct overlay_text
  {
    v4 Color;
    v4 TextCoord;
    m4 ModelMatrix;
  };

  struct overlay_quad {
    v4 Color;
    m4 ModelMatrix; // PixelSpace
  };

  struct render_level {
    chunk_list SolidObjects;
    chunk_list TransparentObjects;
    chunk_list OverlayText;
    chunk_list OverlayQuads;
    render_level* Next;
    render_level* Previous;
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
    data::font Font;
    u32 FontTextureHandle;
    data::render_level RenderSentinel;
    rect2f UnitDrawRegion; // UnitCoordinate [0,0,1,1], Percentage of applicationWidth/Height
    window_size_pixel WindowSize;
    temporary_memory TempMem;
  };

  void SetWindowSize(system* System, application_render_commands* RenderCommands)
  {
    System->WindowSize.WindowWidth       = (r32) RenderCommands->WindowInfo.Width;
    System->WindowSize.WindowHeight      = (r32) RenderCommands->WindowInfo.Height;
  }

  system* CreateRenderSystem(render_group* RenderGroup, r32 ResolutionWidth, r32 ResolutionHeight, application_render_commands* RenderCommands);
  void BeginRender(system* System)
  {
    EndTemporaryMemory( System->TempMem );
    System->TempMem = BeginTemporaryMemory(&System->Arena);
    ListInitiate(&System->RenderSentinel);
  }
  void DrawScene(system* System, ecs::entity_manager* EntityManager);
  void Draw(entity_manager* EntityManager, system* RenderSystem, m4 ProjectionMatrix, m4 ViewMatrix);
  void DrawOverlayText(system* RenderSystem, utf8_byte* Text, u32 X0, u32 Y0, r32 RelativeScale);

  window_size_pixel GetWindowSize(system* System)
  {
    return System->WindowSize;
  }
  
  r32 PixelToCanonicalWidth(system* System, r32 X);
  r32 PixelToCanonicalHeight(system* System, r32 Y);
  v2 PixelToCanonicalSpace(system* System, v2 PixelPos);

  r32 CanonicalToPixelWidth(system* System, r32 X);
  r32 CanonicalToPixelHeight(system* System, r32 Y);
  v2 CanonicalToPixelSpace(system* System, v2 CanPos);

  r32 GetLineSpacingPixelSpace(system* System, r32 PixelSize);
  r32 GetLineSpacingCanonicalSpace(system* System, r32 PixelSize);

  v2 GetTextSizePixelSpace(system* System, r32 PixelSize, utf8_byte const * Text);
  v2 GetTextSizeCanonicalSpace(system* System, r32 PixelSize, utf8_byte const * Text);

  rect2f RectCenterBotLeft(rect2f Rect)
  {
    rect2f Result = Rect2f(Rect.X + Rect.W*0.5f,Rect.Y + Rect.H*0.5f, Rect.W, Rect.H);
    return Result;
  }


  void DrawTextPixelSpace(system* System, v2 PixelPos, r32 PixelSize, utf8_byte const * Text);
  void DrawTextCanonicalSpace(system* System, v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color = V4(1,1,1,1));

  void DrawOverlayQuadPixelSpace(system* System, rect2f PixelRect, v4 Color);
  void DrawOverlayQuadCanonicalSpace(system* System, rect2f CanonicalRect, v4 Color);

  void SetDrawWindow(system* System, rect2f DrawRegion){
    System->UnitDrawRegion = DrawRegion;
  }

  void SetDrawWindowCanCord(system* System, rect2f DrawRegion){
    System->UnitDrawRegion = Rect2f(
      DrawRegion.X / System->WindowSize.ApplicationAspectRatio,
      DrawRegion.Y,
      DrawRegion.W / System->WindowSize.ApplicationAspectRatio,
      DrawRegion.H);
  }

  void NewRenderLevel(system* System){
    data::render_level* RenderLevel = PushStruct(&System->Arena, data::render_level);
    ListInsertBefore(&System->RenderSentinel, RenderLevel);
  }

  void DrawTexturedOverlayQuadCanonicalSpace(system* System, rect2f CanonicalRect, rect2f TextureCoordinates, u32 TextureHandle)
  {
    Platform.DEBUGPrint("Warning: DrawTexturedOverlayQuadCanonicalSpace has no implementation\n");
  }
}
}
