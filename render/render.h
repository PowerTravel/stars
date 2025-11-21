#pragma once

// Easy to use interface between the platform renderer
// Keeps track of assets loaded to the GPU 
// Holds the rendering pipeline

#include "asset_manager/asset_types.h"
#include "renderer/render_push_buffer/application_render_push_buffer.h"

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
    bool Transparent;
    shader_type ShaderType;
    asset::mesh::primitive* Primitive;
    union{
      asset::pbr_material* PbrMaterial;
      asset::phong_material* PhongMaterial;
    };
    m4* Transform;
  };

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

  struct gl_vertex_buffer
  {
    u32 IndexCount;
    u32* Indeces;
    u32 VertexCount;
    opengl_vertex* VertexData;
  };

  struct opengl_buffer_data{
    u32 BufferCount;
    gl_vertex_buffer* BufferData;
  };
  
  struct asset_render_object {
    asset::mesh::primitive* Primitive;
    shader_type ShaderType;
    union {
      asset::phong_material* PhongMaterial;
      asset::pbr_material* PbrMaterial;
    };
    m4 Transform;
    ecs::entity_id EntityID;
  };

  typedef cmn::list<asset_render_object> render_list;
  typedef render_list::element render_list_element;


  struct renderer {
    chunk_list RenderHandles; // u32 // Global Persistent Arena
    rb_tree LoadedTextures;
    rb_tree LoadedPrograms;
    rb_tree LoadedPrimitives;

    render_group* RenderGroup;

    render_list RenderList;
    
    window_size_pixel WindowSize;
    u32* InternalTextures;
    u32* FrameBuffers;
    u32* BasicShapes;
    u32* InternalShaders;
  };

  void SetWindowSize(application_render_commands* RenderCommands);

  renderer Create(render_group* RenderGroup, r32 ApplicationWidth, r32 ApplicationHeight, application_render_commands* RenderCommands);
  void Begin();

  void DrawAssetRenderObject(asset::mesh::primitive* Primitive, asset::phong_material* Material, m4 Transform, ecs::entity_id EntityID);
  void DrawAssetRenderObject(asset::mesh::primitive* Primitive, asset::pbr_material* Material, m4 Transform, ecs::entity_id EntityID);
  void DrawRenderTree(asset::render_tree_id ID, ecs::entity_id EntityID);
  void DrawMesh(asset::mesh_id ID, const m4& Transform, ecs::entity_id EntityID);

  u32 LoadMeshToGPU(gl_vertex_buffer VertexBuffer);
  u32 LoadMeshPrimitiveToGPU(asset::mesh::primitive* AssetPrimitive);
  u32 GetOrCreateTexture(asset::texture* Texture);
  cmn::vector<primitive>& GetOrCreateMeshHandle(render_group* RenderGroup, asset::mesh_id MeshID);

  void RenderScene(renderer* Renderer, m4 ProjectionMatrix, m4 ViewMatrix);
  void RecompileAllPrograms();
} // namespace render
