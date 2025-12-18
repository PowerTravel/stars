#pragma once

// Easy to use interface between the platform renderer
// Keeps track of assets loaded to the GPU 
// Holds the rendering pipeline

#include "window_size.h"
#include "asset_manager/asset_types.h"
#include "renderer/render_push_buffer/application_render_push_buffer.h"
#include "cmn/hash_map.h"

#include "font.h"

namespace render {

  enum class shader_type {
    PHONG,
    PBR
  };

  // Data about how to render a certain primitive
  struct primitive {
    u32 GeometryID;     // The ID to send to the platform renderer
    u32 ProgramID;      // The ID of the program to use.
    //u32 ShaderDefinitionHash;
    b32 Transparent;
    shader_type ShaderType;
    asset::geometry* Geometry;
    union{
      asset::pbr_material* PbrMaterial;
      asset::phong_material* PhongMaterial;
    };
    m4* Transform;
  };

  struct asset_render_object {
    asset::geometry* Geometry;
    shader_type ShaderType;
    union {
      asset::phong_material* PhongMaterial;
      asset::pbr_material* PbrMaterial;
    };
    m4 Transform;
  };

  typedef cmn::list<asset_render_object> render_list;
  typedef render_list::element render_list_element;

  struct overlay_sdf {
    v4 Color;
    v4 TextCoord;
    r32 OnEdgeValue;
    r32 PixelDistanceScale;
    m4 ModelMatrix; // PixelSpace
  };

  struct overlay_sprite {
    rect2f Rect;
    v4 TexCoord; // u0,v0,u1,v1
    v4 Color;
    u32 SpriteDepth;      // Depth within the 2dArray
    u32 SpriteColorCount; // 0,1,3 or 4
  };

  struct overlay_level {
    u32 SDFHandle;
    cmn::list<overlay_sdf> OverlaySDF;
    
    u32 SolidQuad;
    u32 SpriteA;
    u32 SpriteRGB;
    u32 SpriteRGBA;
    u32 SpriteHandle; // GPU-Handle of a 2dArrayTexture
    cmn::list<overlay_sprite> OverlaySprite;
  };

  struct renderer {
    memory_arena RenderTransientArena;
    temporary_memory TempMem;
    chunk_list RenderHandles; // u32 // Global Persistent Arena
    rb_tree LoadedTextures;
    rb_tree LoadedPrograms;
    rb_tree LoadedPrimitives;

    camera* ActiveCamera;
    cmn::hash_map<camera> Cameras;

    r32 MSAA;

    render_group* RenderGroup;

    render_list RenderList;

    cmn::list<overlay_level> OverlayLevels;

    u32* InternalTextures;
    u32* FrameBuffers;
    u32* BasicShapes;
    u32* InternalShaders;

    font Font;
  };

  char** LoadFileFromDisk(const char* CodePath);

  void SetWindowSize(application_render_commands* RenderCommands);

  renderer* CreateRenderer(render_group* RenderGroup, r32 ApplicationWidth, r32 ApplicationHeight, application_render_commands* RenderCommands);
  void Begin();

  void DrawAssetRenderObject(asset::geometry* Geometry, asset::phong_material* Material, m4 Transform);
  void DrawAssetRenderObject(asset::geometry* Geometry, asset::pbr_material* Material, m4 Transform);
  void DrawRenderTree(asset::render_tree_id ID);
  void DrawMesh(asset::mesh_id ID, const m4& Transform);

  u32 LoadVertexdataToGPU(render_group* RenderGroup, u32 IndexCount, u32* Indeces, u32 VertexCount, opengl_vertex* VertexData);
  u32 LoadGeometryToGPU(asset::mesh::primitive* AssetPrimitive);
  u32 LoadImageToGpu(asset::image* Image, texture_params TextureParams);
  u32 GetOrCreateTexture(asset::texture* Texture);
  cmn::vector<primitive>& GetOrCreateMeshHandle(render_group* RenderGroup, asset::mesh_id MeshID);

  void RenderScene(m4 ProjectionMatrix, m4 ViewMatrix);
  void RecompileAllPrograms();



  void DrawTextPixelSpace(v2 PixelPos, rect2f PixelClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color);
  void DrawTextCanonicalSpace(v2 CanonicalPos,  rect2f CanonicalClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color);
  void DrawTextPixelSpace(v2 PixelPos, r32 PixelSize, utf8_byte const * Text, v4 Color);
  void DrawTextCanonicalSpace(v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color);

  // Note: Merge this into maybe DrawSprite. Somehting like
  // void DrawSprite(v2 PixelPos, rect2f PixelClipRect, v4 TextureCoords, asset::texture* Texture);
  // Food for thought


  void DrawOverlayQuadPixelSpace(rect2f PixelRect, v4 Color);
  void DrawOverlayQuadCanonicalSpace(rect2f CanonicalRect, v4 Color);
  void DrawIconPixelSpace(rect2f PixelRect, v4 TextureCoords, v4 Color);
  void DrawIconCanonicalSpace(rect2f CanonicalRect, v4 TextureCoords, v4 Color);
  void DrawIconPixelSpace(rect2f PixelRect, rect2f PixelClipRect, v4 TexCoords, v4 Color);
  void DrawIconCanonicalSpace(rect2f CanonicalRect, rect2f CanonicalClipRect, v4 TexCoords, v4 Color);

  void DrawIconPixelSpace2(rect2f PixelRect, rect2f PixelClipRect, v4 TexCoords, v4 Color);
  void DrawIconCanonicalSpace2(rect2f CanonicalRect, rect2f CanonicalClipRect, v4 TexCoords, v4 Color);

  void NewOverlayLevel();

} // namespace render
