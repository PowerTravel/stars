#pragma once

#include "ecs/components/component_render.h"
#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "containers/chunk_list.h"
#include "math/rect2f.h"
#include "containers/rb_tree.h"
#include "asset_manager/asset_manager.h"

struct opengl_buffer_data;

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

  struct line_3d {
    v3 P0;
    v3 P1;
    v4 Color;
    r32 Thickness;
  };

  struct textured_overlay_quad {
    v4 Color;
    v4 TexCoord;
    m4 ModelMatrix; // PixelSpace
  };

  struct render_level {
    chunk_list OverlayText;
    chunk_list OverlayQuads;
    chunk_list OverlayIcon;
    render_level* Next;
    render_level* Previous;
  };

  struct render_data {
    void (*RenderFunction)(m4 ProjectionMatrix, m4 ViewMatrix, void* Data);
    void* Data;
  };

  enum internal_textures {
    INT_TEX_MSAA_COLOR,
    INT_TEX_MSAA_DEPTH,
    INT_TEX_ACCUM,
    INT_TEX_REVEAL,
    INT_TEX_GAUSSIAN_A,
    INT_TEX_GAUSSIAN_B,
    INT_TEX_COUNT,
  };

  enum framebuffers{
    FRAMEBUFFER_DEFAULT,
    FRAMEBUFFER_MSAA,
    FRAMEBUFFER_TRANSPARENT,
    FRAMEBUFFER_GAUSSIAN_A,
    FRAMEBUFFER_GAUSSIAN_B,
    FRAMEBUFFER_COUNT
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

    chunk_list RenderHandles; // u32 // Global PErsistent Arena
    // Key is Asset Index
    // Value is u32, Handle from the render system
    rb_tree MeshHandleMap;
    rb_tree TextureHandleMap;
    
    u32* InternalTextures;
    u32* FrameBuffers;

    data::font Font;
    u32 BlitPlaneHandle;
    u32 FontTextureHandle;
    chunk_list SolidObjects;      
    chunk_list TransparentObjects;
    chunk_list OverlayRenders;     // render_data
    chunk_list LineObjects;
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
  void Begin();
  void DrawRenderObject(component* Component);
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
  
  b32 GetCharsCountToFitPixelSpace(system* System, r32 PixelSize, r32 MaxWidthPixelSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet);
  b32 GetCharsCountToFitCanonicalSpace(system* System, r32 PixelSize, r32 MaxWidthCanonicalSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet);


  rect2f RectCenterBotLeft(rect2f Rect)
  {
    rect2f Result = Rect2f(Rect.X + Rect.W*0.5f,Rect.Y + Rect.H*0.5f, Rect.W, Rect.H);
    return Result;
  }

  void DrawTextPixelSpace(system* System, v2 PixelPos, rect2f PixelClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color);
  void DrawTextCanonicalSpace(system* System, v2 CanonicalPos, rect2f CanonicalClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color);

  void DrawTextPixelSpace(system* System, v2 PixelPos, r32 PixelSize, utf8_byte const * Text);
  void DrawTextCanonicalSpace(system* System, v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color = V4(1,1,1,1));

  void DrawQuadPixelSpace(system* System, rect2f PixelRect, v4 Color);
  void DrawQuadCanonicalSpace(system* System, rect2f CanonicalRect, v4 Color);

  void DrawIconPixelSpace(system* System, rect2f PixelRect,  v4 TextureCoords, v4 Color);
  void DrawIconCanonicalSpace(system* System, rect2f CanonicalRect,  v4 TextureCoords, v4 Color);

  void DrawOverlay3DObject(void* Data, void (*RenderFunction)(m4 ProjectionMatrix, m4 ViewMatrix, void* Data));

  // When drawing with DrawTextPixelSpace or DrawTextCanonicalSpace, the position is the line someone would draw on in a note-book.
  // That is letters like 'g' dips under the line. If someone wants to draw text in a rect one maybe don't want the g to go outside the rect
  // Therefore this 'FontDescenOffset' is the offset needed such that the text origin is the lowest dip of the text.
  inline r32 GetCanonicalFontDescenOffset(system* System, r32 PixelSize);

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
  
  void DrawRenderTree(asset::render_tree_id RenderTreeId);

  // Loading assets to gpu
  u32 GetMeshHandle(const c8* Name);
  u32 GetMeshHandle(asset::key AssetKey);

  u32 Get32BitTextureHandle(const c8* Name);
  u32 Get32BitTextureHandle(asset::key AssetKey);

  u32 FrameBuffer(u32 Index);


  void Init(asset::key MeshKey, asset::key MaterialKey, component* Render);
  void Init2(asset::key RenderTreeHandle, component* Render);

  // Draw basic shapes
  void DrawLine3D(v3 Start, v3 End, v4 Color, r32 Thickness);
  void DrawAABB(aabb3f AABB);
}
}
