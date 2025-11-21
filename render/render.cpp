#include "render.h"
#include "asset_manager/asset_manager.h"
#include "shaders/post_processing/gaussian_blur.h"


#include "renderer/render_push_buffer/application_render_push_buffer.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

#include "shaders/pbr/pbr.h"
#include "shaders/phong/phong.h"
#include "shaders/common/shaders.h"

extern render::renderer* GlobalRenderer;

namespace render {

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

enum basic_shapes {
  BASIC_SHAPE_BLIT_PLANE,
  BASIC_SHAPE_COUNT
};

enum internal_shaders {
  INTERNAL_SHADER_TRANSPARENT_COMPOSITION,
  INTERNAL_SHADER_GAUSSIAN_BLUR_X,
  INTERNAL_SHADER_GAUSSIAN_BLUR_Y,
  INTERNAL_SHADER_COUNT
};

gl_vertex_buffer CreateGLVertexBuffer(memory_arena* Arena,
                     const int IndexCount, const int* Indeces, const int VertexCount,
                     const v3* VerticeData, const v3* NormalData, const v2* TextureData)
{
  int* GLIndexArray         = PushArray(Arena, IndexCount, int);
  opengl_vertex* VertexData = PushArray(Arena, VertexCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  utils::Copy(IndexCount*sizeof(int), (void*) Indeces, (void*)GLIndexArray);

  for (int i = 0; i < VertexCount; ++i)
  {
    opengl_vertex* GlVertice = &VertexData[i];
    Vertice->v  = VerticeData[i];
    Vertice->vt = TextureData ? TextureData[i] : V2(0,0);
    Vertice->vn = NormalData  ? NormalData[i]  : V3(0,0,0);
    ++Vertice;
  }
  
  render::gl_vertex_buffer Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces =  (u32*) GLIndexArray;
  Result.VertexCount = VertexCount;
  Result.VertexData = VertexData;
  return Result;
}

file_local void PrimitiveToGlVertexBuffer(memory_arena* Arena, const asset::mesh::primitive * Primitive, render::gl_vertex_buffer* Result)
{
  Assert(Primitive->IndexCount && Primitive->Indeces && Primitive->VertexCount && Primitive->Vertex);
  // We are only handling 1 set of texture vertices atm. Increase if we find the need
  Assert(Primitive->TextureVertexSetCount== 0 || Primitive->TextureVertexSetCount ==1);
  *Result = CreateGLVertexBuffer(
      Arena,
      Primitive->IndexCount,
      Primitive->Indeces,
      Primitive->VertexCount,
      Primitive->Vertex,
      Primitive->VertexNormal,
      Primitive->TextureVertices ? Primitive->TextureVertices[0] : 0
    );
}

gl_vertex_buffer PrimitiveToGlVertexBuffer(memory_arena* Arena, const asset::mesh::primitive * Primitive)
{
  render::gl_vertex_buffer Result = {};
  PrimitiveToGlVertexBuffer(Arena,Primitive, &Result);
  return Result;
}

opengl_buffer_data MeshToGlVertexBuffer(memory_arena* Arena, const asset::mesh * Mesh)
{ 
  render::opengl_buffer_data Result = {};
  Assert(Mesh->PrimitiveCount == 1); // Deal wiht several primitives per mesh when we run into them.
  Result.BufferCount = Mesh->PrimitiveCount;
  Result.BufferData  = PushArray(Arena, Result.BufferCount, render::gl_vertex_buffer);
  for (int i = 0; i < Result.BufferCount; ++i)
  {
    PrimitiveToGlVertexBuffer(Arena, &Mesh->Primitives[i], &Result.BufferData[i]);
  }

  return Result;
}


file_local u32* CreateInternalTextures(render_group* RenderGroup, window_size_pixel* Window)
{    
  texture_params DefaultColor = DefaultColorTextureParams();
  texture_params DefaultDepth = DefaultDepthTextureParams();
  texture_params RevealTexParam = DefaultColorTextureParams();
  RevealTexParam.TextureFormat = texture_format::R_8;

  u32* Result = (u32*) PushArray(GlobalPersistentArena, INT_TEX_COUNT, u32);
  Result[INT_TEX_MSAA_COLOR] = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultColor, 0);
  Result[INT_TEX_MSAA_DEPTH] = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultDepth, 0);
  Result[INT_TEX_ACCUM]      = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultColor, 0);
  Result[INT_TEX_REVEAL]     = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, RevealTexParam, 0);
  Result[INT_TEX_GAUSSIAN_A] = PushNewTexture(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
  Result[INT_TEX_GAUSSIAN_B] = PushNewTexture(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
  return Result;
}

file_local u32* CreateFrameBuffers(render_group* RenderGroup, window_size_pixel* Window, u32* Textures )
{
  u32* Result = (u32*) PushArray(GlobalPersistentArena, FRAMEBUFFER_COUNT, u32);
  u32 TransparentColorTexture[]         = {Textures[INT_TEX_ACCUM], Textures[INT_TEX_REVEAL]};
  Result[FRAMEBUFFER_DEFAULT]     = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 0, 0, 0, 0);
  Result[FRAMEBUFFER_MSAA]        = PushNewFrameBuffer(RenderGroup,  Window->MSAA * Window->ApplicationWidth,  Window->MSAA * Window->ApplicationHeight, 1, &Textures[INT_TEX_MSAA_COLOR], Textures[INT_TEX_MSAA_DEPTH], 0);
  Result[FRAMEBUFFER_TRANSPARENT] = PushNewFrameBuffer(RenderGroup,  Window->MSAA * Window->ApplicationWidth,  Window->MSAA * Window->ApplicationHeight, ArrayCount(TransparentColorTexture), TransparentColorTexture, Textures[INT_TEX_MSAA_DEPTH], 0);
  Result[FRAMEBUFFER_GAUSSIAN_A]  = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 1, &Textures[INT_TEX_GAUSSIAN_A], 0, 0);
  Result[FRAMEBUFFER_GAUSSIAN_B]  = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 1, &Textures[INT_TEX_GAUSSIAN_B], 0, 0);
  return Result;
}

file_local u32 LoadMeshToGPU(render_group* RenderGroup, gl_vertex_buffer VertexBuffer)
{
  u32 MeshHandle  = PushNewMesh(RenderGroup, VertexBuffer.VertexCount, VertexBuffer.VertexData);
  u32 IndexHandle = PushNewMeshIndices(RenderGroup, MeshHandle, VertexBuffer.IndexCount, VertexBuffer.Indeces);
  return IndexHandle;
}

gl_vertex_buffer CreateBlitPlane()
{
  u32 VerticeIndex[] = {
    0,1,2,
    2,1,3
  };
  opengl_vertex GlVertex[4] =
  {
     // v                  vn       vt
    {{-1.0f, -1.0f, 0.0f}, {0,0,1}, {0,0}},
    {{ 1.0f, -1.0f, 0.0f}, {0,0,1}, {1,0}},
    {{-1.0f,  1.0f, 0.0f}, {0,0,1}, {0,1}},
    {{ 1.0f,  1.0f, 0.0f}, {0,0,1}, {1,1}}
  };

  gl_vertex_buffer VertexBuffer = {};
  VertexBuffer.IndexCount  = ArrayCount(VerticeIndex);
  VertexBuffer.Indeces     = (u32*) PushCopy(GlobalTransientArena, sizeof(VerticeIndex), VerticeIndex);
  VertexBuffer.VertexCount = ArrayCount(GlVertex);
  VertexBuffer.VertexData  = (opengl_vertex*) PushCopy(GlobalTransientArena, sizeof(GlVertex), GlVertex);

  return VertexBuffer;
}

file_local u32* CreateBasicShapes(render_group* RenderGroup)
{
  u32* Result = PushArray(GlobalPersistentArena, BASIC_SHAPE_COUNT, u32);
  Result[BASIC_SHAPE_BLIT_PLANE] = LoadMeshToGPU(RenderGroup, CreateBlitPlane());
  return Result;
}

file_local u32* CreateInternalShaders(render_group* RenderGroup)
{
  u32* Result = PushArray(GlobalPersistentArena, INTERNAL_SHADER_COUNT, u32);
  Result[INTERNAL_SHADER_TRANSPARENT_COMPOSITION] = common_shaders::CreateTransparentCompositionProgram(RenderGroup);
  Result[INTERNAL_SHADER_GAUSSIAN_BLUR_X] = common_shaders::CreateGaussianBlurProgramX(RenderGroup);
  Result[INTERNAL_SHADER_GAUSSIAN_BLUR_Y] = common_shaders::CreateGaussianBlurProgramY(RenderGroup);
  return Result;
}

renderer Create(render_group* RenderGroup, r32 ApplicationWidth, r32 ApplicationHeight, application_render_commands* RenderCommands)
{
  renderer Result = {};
  Result.RenderGroup = RenderGroup;
  Result.RenderHandles    = NewChunkList(GlobalPersistentArena, sizeof(u32), 128);
  Result.LoadedTextures = NewRBTree(GlobalPersistentArena, 64, 64);
  Result.LoadedPrograms = NewRBTree(GlobalPersistentArena, 64, 64);
  Result.LoadedPrimitives = NewRBTree(GlobalPersistentArena, 64, 64);
  Result.RenderList = render_list::CreateTransient();
  Result.RenderList2 = render_list_2::CreateTransient();


  Result.WindowSize.WindowWidth       = (r32) RenderCommands->WindowInfo.Width;
  Result.WindowSize.WindowHeight      = (r32) RenderCommands->WindowInfo.Height;
  Result.WindowSize.MonitorWidth      = (r32) RenderCommands->MonitorInfo.Width;
  Result.WindowSize.MonitorHeight     = (r32) RenderCommands->MonitorInfo.Height;
  Result.WindowSize.MonitorDPI        = (r32) RenderCommands->MonitorInfo.RawDPI;
  Result.WindowSize.EffectiveDPI      = (r32) RenderCommands->MonitorInfo.EffectiveDPI;
  Result.WindowSize.MSAA              = 4;
  Result.WindowSize.ApplicationWidth   = ApplicationWidth;
  Result.WindowSize.ApplicationHeight  = ApplicationHeight;
  Result.WindowSize.ApplicationAspectRatio = ApplicationWidth / ApplicationHeight;

  Result.InternalTextures  = CreateInternalTextures(RenderGroup, &Result.WindowSize);
  Result.FrameBuffers      = CreateFrameBuffers(RenderGroup, &Result.WindowSize, Result.InternalTextures);
  Result.BasicShapes       = CreateBasicShapes(RenderGroup);
  Result.InternalShaders   = CreateInternalShaders(RenderGroup);

  return Result;
}

void Begin()
{
  Assert(GlobalRenderer);
  GlobalRenderer->RenderList = render_list::CreateTransient();
  GlobalRenderer->RenderList2 = render_list_2::CreateTransient();
}

void SetWindowSize(application_render_commands* RenderCommands)
{
  GlobalRenderer->WindowSize.WindowWidth       = (r32) RenderCommands->WindowInfo.Width;
  GlobalRenderer->WindowSize.WindowHeight      = (r32) RenderCommands->WindowInfo.Height;
}

file_local inline u32 InternalTexture(u32 Index)
{
  u32 Result = GlobalRenderer->InternalTextures[Index];
  return Result;
}

file_local inline u32 FrameBuffer(u32 Index)
{
  u32 Result = GlobalRenderer->FrameBuffers[Index];
  return Result;
}

file_local inline u32 BasicShape(u32 Index)
{
  u32 Result = GlobalRenderer->BasicShapes[Index];
  return Result;
}

file_local inline u32 InternalShader(u32 Index)
{
  u32 Result = GlobalRenderer->InternalShaders[Index];
  return Result;
}

file_local inline void ClearRenderState(renderer* Renderer) {
  render_group* RenderGroup = Renderer->RenderGroup;
  window_size_pixel* Window = &Renderer->WindowSize;

  render_state* DefaultState = PushNewState(RenderGroup);
  *DefaultState = DefaultRenderState3(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio);

  // Clear default color frame buffer
  clear_operation* DefClearColor = PushNewClearOperation(RenderGroup);
  DefClearColor->BufferType = OPEN_GL_COLOR;
  DefClearColor->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
  DefClearColor->TextureIndex = 0;
  DefClearColor->Color = V4(0,0,0,1);

  // Clear default depth frame buffer
  clear_operation* DefClearDepth = PushNewClearOperation(RenderGroup);
  DefClearDepth->BufferType = OPEN_GL_DEPTH;
  DefClearDepth->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
  DefClearDepth->TextureIndex = 0;
  DefClearDepth->Depth = 1;

  // Clear MSAA Color frame buffer
  clear_operation* ClearMSAAColor = PushNewClearOperation(RenderGroup);
  ClearMSAAColor->BufferType = OPEN_GL_COLOR;
  ClearMSAAColor->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_MSAA);
  ClearMSAAColor->TextureIndex = 0;
  ClearMSAAColor->Color = V4(0,0,0,1);

  // Clear MSAA Depth frame buffer
  clear_operation* ClearMSAADepth = PushNewClearOperation(RenderGroup);
  ClearMSAADepth->BufferType = OPEN_GL_DEPTH;
  ClearMSAADepth->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_MSAA);
  ClearMSAADepth->TextureIndex = 0;
  ClearMSAADepth->Depth = 1;

  // Clear Transparent calculation frame buffer 0
  clear_operation* TransparenClearOp0 = PushNewClearOperation(RenderGroup);
  TransparenClearOp0->BufferType = OPEN_GL_COLOR;
  TransparenClearOp0->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_TRANSPARENT);
  TransparenClearOp0->TextureIndex = 0;
  TransparenClearOp0->Color = V4(0,0,0,0);

  // Clear Transparent calculation frame buffer 1
  clear_operation* TransparenClearOp1 = PushNewClearOperation(RenderGroup);
  TransparenClearOp1->BufferType = OPEN_GL_COLOR;
  TransparenClearOp1->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_TRANSPARENT);
  TransparenClearOp1->TextureIndex = 1;
  TransparenClearOp1->Color = V4(1,0,0,0);


  //render_state* DefaultState = PushNewState(RenderGroup);
  //*DefaultState = DefaultRenderState3(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio);
}


u32 LoadMeshPrimitiveToGPU(asset::mesh::primitive* AssetPrimitive)
{  
  gl_vertex_buffer VertexBuffer = PrimitiveToGlVertexBuffer(GlobalTransientArena, AssetPrimitive);
  u32 IndexHandle = LoadMeshToGPU(GlobalRenderCommands->RenderGroup, VertexBuffer);
  return IndexHandle;
}

file_local inline bool IsTransparent(asset_render_object* AssetRenderObject)
{
  switch(AssetRenderObject->ShaderType)
  {
    case shader_type::PBR: {
      asset::pbr_material* PbrMaterial = AssetRenderObject->PbrMaterial;
      if( PbrMaterial->HasMetallicRoughness && 
         !PbrMaterial->MetallicRoughness.HasBaseColorTexture &&
         PbrMaterial->MetallicRoughness.BaseColorFactor.W < 1)
      {
        // Note: I don't think we support loading of transparent surfaces in PBR. IE BaseColorFactor.W is always 1.
        // 
        // But if/when we do we handle it here
        return true;
      }
      return false;
    } break;
    case shader_type::PHONG: {
      asset::phong_material* PhongMaterial = AssetRenderObject->PhongMaterial;
      if(PhongMaterial->Ka && PhongMaterial->Ka->W < 1)
      {
        return true;
      }
      return false;
    } break;
  }
  return false;
}

file_local void SetHandle(rb_tree* HandleTree, u32 Key, u32 Handle){
  u32* HandleMem = (u32*) GetNewBlock(GlobalPersistentArena, &GlobalRenderer->RenderHandles);
  *HandleMem = Handle;
  Insert(HandleTree, Key, (void*) HandleMem);
}

#define MAX_DEF_SIZE 32
struct program_definition_hash_helper
{
  shader_type Type;
  char Defintion[MAX_DEF_SIZE];
};

file_local u32 GetProgramHash(shader_type ShaderType, size_t DefinitionSize, char* Definition)
{
  Assert(DefinitionSize < MAX_DEF_SIZE);

  program_definition_hash_helper Hashdef = {};
  Hashdef.Type = ShaderType;
  utils::Copy(DefinitionSize, Definition, Hashdef.Defintion);
  u32 ProgramHash = cmn::utils::SuperFastHash( (char*) &Hashdef, sizeof(shader_type) + DefinitionSize);
  return ProgramHash;
}

file_local u32 GetOrCreateProgram(render_group* RenderGroup, asset_render_object* AssetRenderObject)
{
  u32 ProgramHandle = 0;
  shader_type ShaderType = AssetRenderObject->ShaderType;
  switch(ShaderType)
  {
    case shader_type::PHONG: {
      asset::phong_material* Material = AssetRenderObject->PhongMaterial;
      phong::definition Definition = phong::GetProgramDefinition(Material);
      u32 ProgramHash = GetProgramHash(ShaderType, sizeof(phong::definition), (char*) &Definition);
      Platform.DEBUGPrint("Program Hash: %u, Dfac = %s, Tras = %s\n",
        ProgramHash,
        Definition.HasDiffuseTexture ? "True" : "False",
        Definition.Transparent       ? "True" : "False");
      u32* ProgramHandlePtr = (u32*) Find(&GlobalRenderer->LoadedPrograms, ProgramHash);
      if(!ProgramHandlePtr)
      {
        ProgramHandle = phong::CreateProgram(RenderGroup, Definition);
        SetHandle(&GlobalRenderer->LoadedPrograms, ProgramHash, ProgramHandle);
      }else{
        ProgramHandle = *ProgramHandlePtr;
      }
    } break;
    case shader_type::PBR: {
      asset::pbr_material* Material = AssetRenderObject->PbrMaterial;
      pbr::definition Definition = pbr::GetProgramDefinition(Material);
      u32 ProgramHash = GetProgramHash(ShaderType, sizeof(pbr::definition), (char*) &Definition);
      u32* ProgramHandlePtr = (u32*) Find(&GlobalRenderer->LoadedPrograms, ProgramHash);
      if(!ProgramHandlePtr)
      {
        ProgramHandle = pbr::CreateProgram(RenderGroup, Definition);
        SetHandle(&GlobalRenderer->LoadedPrograms, ProgramHash, ProgramHandle);
      }else{
        ProgramHandle = *ProgramHandlePtr;
      }
    } break;
  }
  
  Assert(ProgramHandle);
  return ProgramHandle;
}

u32 GetOrCreateGeometryID(render_group* RenderGroup, asset::mesh::primitive* MeshPrimitive)
{
  size_t MeshPrimitiveID = (size_t) MeshPrimitive;
  u32* PrimitiveHandlePtr = (u32*) Find(&GlobalRenderer->LoadedPrimitives, MeshPrimitiveID);
  u32 GeometryID = 0;
  if(!PrimitiveHandlePtr)
  {
    GeometryID  = LoadMeshPrimitiveToGPU(MeshPrimitive);
    SetHandle(&GlobalRenderer->LoadedPrimitives, MeshPrimitiveID, GeometryID);
  }else{
    GeometryID = *PrimitiveHandlePtr;
  }

  return GeometryID;
}

file_local void ActivateMSAAFrameBuffer(render_group* RenderGroup)
{
  window_size_pixel* Window = &GlobalRenderer->WindowSize;
  render_state* MSAAViewport = PushNewState(RenderGroup);
  SetState(MSAAViewport, ViewportState(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio));
}


file_local void PrepareOrderIndependentTransparentRendering(render_group* RenderGroup)
{
  render_state* TransparentState = PushNewState(RenderGroup);
  depth_state DepthState = {};
  DepthState.TestActive = true;
  DepthState.WriteActive = false;
  SetState(TransparentState, DepthState);

  blend_state BlendState = {};
  BlendState.Active = true;
  BlendState.TextureCount = 2;
  BlendState.TextureBlendStates[0].TextureIndex = 0;
  BlendState.TextureBlendStates[0].SrcFactor = OPEN_GL_ONE;
  BlendState.TextureBlendStates[0].DstFactor = OPEN_GL_ONE;
  BlendState.TextureBlendStates[1].TextureIndex = 1;
  BlendState.TextureBlendStates[1].SrcFactor = OPEN_GL_ZERO;
  BlendState.TextureBlendStates[1].DstFactor = OPEN_GL_ONE_MINUS_SRC_ALPHA;
  SetState(TransparentState, BlendState);
}

file_local void FinalizeOrderIndependentTransparentRendering(render_group* RenderGroup) {
  render_state* CompositState = PushNewState(RenderGroup);
  blend_state CompositBlend = {};
  CompositBlend.Active = true;
  CompositBlend.TextureCount = 1;
  CompositBlend.TextureBlendStates[0].TextureIndex = 0;
  CompositBlend.TextureBlendStates[0].SrcFactor = OPEN_GL_ONE_MINUS_SRC_ALPHA;
  CompositBlend.TextureBlendStates[0].DstFactor = OPEN_GL_SRC_ALPHA;
  SetState(CompositState, CompositBlend);

  depth_state CompositDepth = {};
  CompositDepth.TestActive = false;
  CompositDepth.WriteActive = false;
  SetState(CompositState, CompositDepth);

  // Then composit the solid and transparent objects into a single image
  render_object* CompositionObject     = PushNewRenderObject(RenderGroup);
  CompositionObject->ProgramHandle     = InternalShader(INTERNAL_SHADER_TRANSPARENT_COMPOSITION);
  CompositionObject->MeshHandle        = BasicShape(BASIC_SHAPE_BLIT_PLANE);
  CompositionObject->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_MSAA);
  CompositionObject->TextureHandles[0] = InternalTexture(INT_TEX_ACCUM);
  CompositionObject->TextureHandles[1] = InternalTexture(INT_TEX_REVEAL);
  CompositionObject->TextureCount = 2;

  PushUniform(CompositionObject, GetUniformHandle(RenderGroup, CompositionObject->ProgramHandle,  "AccumTex"), (u32)0);
  PushUniform(CompositionObject, GetUniformHandle(RenderGroup, CompositionObject->ProgramHandle , "RevealTex"), (u32)1);

  render_state* ResetBlendBlend = PushNewState(RenderGroup);
  SetState(ResetBlendBlend, DefaultBlendState());
}


file_local void DrawPrimitive(render_group* RenderGroup, primitive* Primitive, m4& ProjectionMatrix, m4& ViewMatrix)
{
  v3 LightPos = V3(10,10,0);
  m4 CamToWorld = RigidInverse(ViewMatrix);
  v3 CamPos = V3(Column(CamToWorld,3));
  v3 LightColor = V3(1,1,1);

  switch(Primitive->ShaderType) {

    case shader_type::PBR:{
      render_object* Object = PushNewRenderObject(RenderGroup);
      Object->ProgramHandle = Primitive->ProgramID;
      Object->FrameBufferHandle = Primitive->Transparent ? FrameBuffer(FRAMEBUFFER_TRANSPARENT) : FrameBuffer(FRAMEBUFFER_MSAA);
      Object->MeshHandle = Primitive->GeometryID;

      m4& ModelMat = *Primitive->Transform;
      m4 NormalModel = Transpose(RigidInverse(ModelMat));

      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "ProjectionMat"), ProjectionMatrix);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "View"),          ViewMatrix);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Model"),         ModelMat);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "NormalModel"),   NormalModel);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "CamPos"),        CamPos);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "LightPos"),      LightPos);
      asset::pbr_material* PbrMaterial = (asset::pbr_material*) Find(asset::type::PBR_MATERIAL,Primitive->Primitive->PbrMaterial);
      pbr::SetMaterialUniforms(RenderGroup, Object, PbrMaterial);
    }break;
    case shader_type::PHONG: {
      v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPos,0));
      m4& ModelMat = *Primitive->Transform;

      m4 ModelView = ViewMatrix*ModelMat;
      m4 NormalView = Transpose(RigidInverse(ModelView));

      render_object* Object = PushNewRenderObject(RenderGroup);
      Object->ProgramHandle = Primitive->ProgramID;
      Object->FrameBufferHandle = Primitive->Transparent ? FrameBuffer(FRAMEBUFFER_TRANSPARENT) : FrameBuffer(FRAMEBUFFER_MSAA);
      Object->MeshHandle = Primitive->GeometryID;

      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "ProjectionMat"),  ProjectionMatrix);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "ModelView"),      ModelView);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "NormalView"),     NormalView);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "LightDirection"), LightDirection);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "LightColor"),     LightColor);
      asset::phong_material* PhongMaterial = (asset::phong_material*) Find(asset::type::PHONG_MATERIAL, Primitive->Primitive->PhongMaterial);
      phong::SetMaterialUniforms(RenderGroup, Object, PhongMaterial);
    } break;
  }
}

file_local void ScaleViewport(render_group* RenderGroup, u32 Width, u32 Height, r32 AspectRatio)
{
  render_state* ScaleViewport = PushNewState(RenderGroup);
  SetState(ScaleViewport, ViewportState(Width, Height, AspectRatio));
}

file_local void BlitBuffers(render_group* RenderGroup, u32 SrcBuffer, u32 DstBuffer, rect2f DrawRegion)
{
  blit_operation* BlitOperation = PushNewBlitOperation(RenderGroup);
  BlitOperation->ReadFrameBufferHandle = FrameBuffer(FRAMEBUFFER_MSAA);
  BlitOperation->DrawFrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
  BlitOperation->DrawRegionUnitCoord = DrawRegion;
}

file_local void GaussianBlur(render_group* RenderGroup, u32 BlurCount, u32 SrcBuffer, u32 DstBuffer, u32 ApplicationWidth, u32 ApplicationHeight)
{
  r32* KernelOffset = PushArray(GlobalTransientArena, 64, r32);
  r32* KernelWeight = PushArray(GlobalTransientArena, 64, r32);
  u32 KernelSize    = gaussian_blur::Kernel(12, 2, KernelOffset, KernelWeight);
  BlitBuffers(RenderGroup, FrameBuffer(SrcBuffer), FrameBuffer(FRAMEBUFFER_GAUSSIAN_A), Rect2f(0,0,1,1));
  v2 SideSize = V2(ApplicationWidth, ApplicationHeight);
  for (int i = 0; i < BlurCount; ++i)
  {
    render_object* GaussianBlurX     = PushNewRenderObject(RenderGroup);
    GaussianBlurX->ProgramHandle     = InternalShader(INTERNAL_SHADER_GAUSSIAN_BLUR_X);
    GaussianBlurX->MeshHandle        = BasicShape(BASIC_SHAPE_BLIT_PLANE);
    GaussianBlurX->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_GAUSSIAN_B);
    GaussianBlurX->TextureHandles[0] = InternalTexture(INT_TEX_GAUSSIAN_A);
    GaussianBlurX->TextureCount      = 1;

    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "offset"), UniformType::R32, KernelOffset, KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "weight"), UniformType::R32, KernelWeight, KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "kernerlSize"), KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "RenderedTexture"), (u32) 0);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "sideSize"), SideSize);

    render_object* GaussianBlurY     = PushNewRenderObject(RenderGroup);
    GaussianBlurY->ProgramHandle     = InternalShader(INTERNAL_SHADER_GAUSSIAN_BLUR_Y);
    GaussianBlurY->MeshHandle        = BasicShape(BASIC_SHAPE_BLIT_PLANE);
    GaussianBlurY->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_GAUSSIAN_A);
    GaussianBlurY->TextureHandles[0] = InternalTexture(INT_TEX_GAUSSIAN_B);
    GaussianBlurY->TextureCount      = 1;
    
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "offset"), UniformType::R32, KernelOffset, KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "weight"), UniformType::R32, KernelWeight, KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "kernerlSize"), KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "RenderedTexture"), (u32) 0);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "sideSize"), SideSize);
  }
  BlitBuffers(RenderGroup, FrameBuffer(FRAMEBUFFER_GAUSSIAN_B), FrameBuffer(DstBuffer), {});
}

void RenderScene(renderer* Renderer, m4 ProjectionMatrix, m4 ViewMatrix)
{
  render_group* RenderGroup = Renderer->RenderGroup;

  v3 LightColor     = V3(1,1,1);
  v3 LightPosition  = V3(1,1,1);
  v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPosition,0));

  window_size_pixel* Window = &Renderer->WindowSize;

  // Wipes all internal textures and resets to default render state
  ClearRenderState(Renderer);

  cmn::list<primitive> SolidMesh = cmn::list<primitive>::CreateTransient();
  cmn::list<primitive> TransparentMesh = cmn::list<primitive>::CreateTransient();
  render_list_element_2* Element = Renderer->RenderList2.First();
  while (!Renderer->RenderList2.IsEnd(Element))
  {
    asset_render_object* AssetRenderObject = Element->Data;

    Platform.DEBUGPrint("Rendering Entity: %s\n",  ecs::GetName(GlobalEntityManager, &AssetRenderObject->EntityID));

    
    primitive LoadedPrimitive  = {};
    LoadedPrimitive.GeometryID  = GetOrCreateGeometryID(RenderGroup, AssetRenderObject->Primitive);
    LoadedPrimitive.ShaderType  = AssetRenderObject->ShaderType;
    LoadedPrimitive.Transparent = IsTransparent(AssetRenderObject);
    LoadedPrimitive.ProgramID   = GetOrCreateProgram(RenderGroup, AssetRenderObject);
    LoadedPrimitive.Primitive   = AssetRenderObject->Primitive;
    LoadedPrimitive.Transform   = &AssetRenderObject->Transform;
    if(LoadedPrimitive.Transparent)
    {
      TransparentMesh.PushBack(LoadedPrimitive);
    }else{
      SolidMesh.PushBack(LoadedPrimitive);
    }

    Element = Element->Next;
  }

  ActivateMSAAFrameBuffer(RenderGroup);
  
  for(cmn::list<primitive>::element* SolidElement = SolidMesh.First(); !SolidMesh.IsEnd(SolidElement); SolidElement = SolidElement->Next)
  {
    DrawPrimitive(RenderGroup, SolidElement->Data, ProjectionMatrix, ViewMatrix);
  }

  if(TransparentMesh.Size())
  {
    PrepareOrderIndependentTransparentRendering(RenderGroup);
    for(cmn::list<primitive>::element* TransparentElement = TransparentMesh.First(); !TransparentMesh.IsEnd(TransparentElement); TransparentElement = TransparentElement->Next)
    {
      DrawPrimitive(RenderGroup, TransparentElement->Data, ProjectionMatrix, ViewMatrix);
    }
    FinalizeOrderIndependentTransparentRendering(RenderGroup);
  }

  // Shrink to regular screeen sice
  ScaleViewport(RenderGroup, Window->WindowWidth, Window->WindowHeight, Window->ApplicationAspectRatio);
  
  #if 1
  BlitBuffers(RenderGroup, FrameBuffer(FRAMEBUFFER_MSAA), FrameBuffer(FRAMEBUFFER_DEFAULT), Rect2f(0,0,1,1));
  #else
  GaussianBlur(RenderGroup, 4, FrameBuffer(FRAMEBUFFER_MSAA), FrameBuffer(FRAMEBUFFER_DEFAULT), Window->ApplicationWidth, Window->ApplicationHeight);
  #endif
  
  #if 0

  render_level* RenderLevel = GetBotRenderLevel(RenderSystem);
  while(!ListEnd(&RenderSystem->RenderSentinel, RenderLevel))
  {
    render_state* OverlayState = PushNewState(RenderGroup);
    depth_state OverlayDepthState = {};
    OverlayDepthState.TestActive = false;
    OverlayDepthState.WriteActive = false;
    SetState(OverlayState, OverlayDepthState);
    m4 OrthoProjectionMatrix = GetOrthographicProjection(-1, 1, Window->ApplicationWidth, 0, Window->ApplicationHeight, 0);

    chunk_list* OverlayQuads = &RenderLevel->OverlayQuads;
    u32 QuadCount = GetBlockCount(OverlayQuads);
    if(QuadCount)
    {
      render_object* QuadObject = PushNewRenderObject(RenderGroup);
      QuadObject->ProgramHandle = GlobalState->ColoredSquareOverlayProgram;
      QuadObject->MeshHandle = RenderSystem->BlitPlaneHandle;
      QuadObject->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
      overlay_quad* QuadInstanceData = (overlay_quad*) Copy(GlobalTransientArena, OverlayQuads);

      PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "Projection"), OrthoProjectionMatrix);
      PushInstanceData(QuadObject, QuadCount, QuadCount*sizeof(overlay_quad), QuadInstanceData);
      Clear(OverlayQuads);
    }
    
    chunk_list* OverlayIcon = &RenderLevel->OverlayIcon;
    u32 IconCount = GetBlockCount(OverlayIcon);
    if(IconCount)
    {
      render_object* QuadObject = PushNewRenderObject(RenderGroup);
      QuadObject->ProgramHandle = GlobalState->TexturedSquareOverlayProgram;
      QuadObject->MeshHandle = RenderSystem->BlitPlaneHandle;
      QuadObject->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
      QuadObject->TextureHandles[0] = GlobalState->ImguiContext.Icons.Atlas;
      QuadObject->TextureCount = 1;
      textured_overlay_quad* QuadInstanceData = (textured_overlay_quad*) Copy(GlobalTransientArena, OverlayIcon);
      
      PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "Projection"), OrthoProjectionMatrix);
      PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "RenderedTexture"), (u32)0);
      PushInstanceData(QuadObject, IconCount, IconCount*sizeof(textured_overlay_quad), QuadInstanceData);
      Clear(OverlayIcon);
    }

    // Overlay text
    chunk_list* OverlayText = &RenderLevel->OverlayText;
    u32 TextCount = GetBlockCount(OverlayText);
    if(TextCount)
    {
      render_object* OverlayTextProgram = PushNewRenderObject(RenderGroup);
      OverlayTextProgram->ProgramHandle = GlobalState->FontRenterProgram;
      OverlayTextProgram->MeshHandle = RenderSystem->BlitPlaneHandle;
      OverlayTextProgram->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
      OverlayTextProgram->TextureHandles[0] = RenderSystem->FontTextureHandle;
      OverlayTextProgram->TextureCount = 1;
      
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "Projection"), OrthoProjectionMatrix);
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "RenderedTexture"), (u32)0);
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "OnEdgeValue"), 128/255.f);
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "PixelDistanceScale"), 32/255.f);
      
      u32 i = 0;
      overlay_text* Text = PushArray(GlobalTransientArena, TextCount, overlay_text);
      chunk_list_iterator TextIt = BeginIterator(OverlayText);
      while(Valid(&TextIt)) {
        overlay_text* OverlayText = (overlay_text*) Next(&TextIt);
        Text[i] = *OverlayText;
        i++;
      }

      PushInstanceData(OverlayTextProgram, TextCount, TextCount*sizeof(overlay_text), (void*) Text);
      Clear(OverlayText);
    }

    RenderLevel = RenderLevel->Next;
  }
#endif
  int _a = 10;
}


file_local u32 MapTextureFilter(asset::texture::filter Filter)
{
  switch(Filter)
  {
    case asset::texture::filter::NEAREST: return OPEN_GL_NEAREST;
    case asset::texture::filter::LINEAR: return OPEN_GL_LINEAR;
    case asset::texture::filter::NEAREST_MIPMAP_NEAREST: return OPEN_GL_NEAREST_MIPMAP_NEAREST;
    case asset::texture::filter::LINEAR_MIPMAP_NEAREST: return OPEN_GL_LINEAR_MIPMAP_NEAREST;
    case asset::texture::filter::NEAREST_MIPMAP_LINEAR: return OPEN_GL_NEAREST_MIPMAP_LINEAR;
    case asset::texture::filter::LINEAR_MIPMAP_LINEAR: return OPEN_GL_LINEAR_MIPMAP_LINEAR;
  }
  return OPEN_GL_LINEAR;
}

file_local u32 MapTextureWrap(asset::texture::wrap Wrap)
{
  switch(Wrap)
  {
    case asset::texture::wrap::CLAMP_TO_EDGE: return OPEN_GL_CLAMP_TO_EDGE;
    case asset::texture::wrap::MIRRORED_REPEAT: return OPEN_GL_MIRRORED_REPEAT;
    case asset::texture::wrap::REPEAT: return OPEN_GL_REPEAT;
  }
  return OPEN_GL_LINEAR;
}

inline file_local texture_params GetTextureParams(asset::texture* Texture){
  texture_params Result = {};
  Result.TextureFormat = texture_format::RGBA_F32;
  Result.InputDataType = OPEN_GL_FLOAT;
  SetParam(&Result, OPEN_GL_TEXTURE_MAG_FILTER, MapTextureFilter(Texture->MagFilter));
  SetParam(&Result, OPEN_GL_TEXTURE_MIN_FILTER, MapTextureFilter(Texture->MinFilter));
  SetParam(&Result, OPEN_GL_TEXTURE_WRAP_S,     MapTextureWrap(Texture->WrapS));
  SetParam(&Result, OPEN_GL_TEXTURE_WRAP_T,     MapTextureWrap(Texture->WrapT));
  return Result;
}

file_local u32 LoadImageToGpu(asset::image* Image, texture_params TextureParams) {
  Assert(Image);
  Assert(Image->Channels == 4);
  
  // TODO: Set params based on texture
  texture_params Params = DefaultColorTextureParams();
  Params.TextureFormat = texture_format::RGBA_U8;
  Params.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  u32 Handle = PushNewTexture(GlobalRenderCommands->RenderGroup, Image->Width, Image->Height, Params, Image->Pixels);

  return Handle;
}

file_local u32 LoadTextureToGpuAndSetHandle(asset::texture* Texture) {

  #if 0
  texture_params Params = GetTextureParams(Texture);
  #else
  texture_params Params = DefaultColorTextureParams();
  Params.TextureFormat = texture_format::RGBA_U8;
  Params.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  #endif

  asset::image* Image = (asset::image*) asset::Find(asset::type::IMAGE, Texture->Image);
  
  u32 Handle = LoadImageToGpu(Image, Params);
  SetHandle(&GlobalRenderer->LoadedTextures, Texture->Image, Handle);
  return Handle;
}

u32 GetOrCreateTexture(asset::texture* Texture)
{
  u32* Handle = (u32*) Find(&GlobalRenderer->LoadedTextures, Texture->Image);
  u32 Result = 0;
  if(Handle)
  {
    Result = *Handle;
  }else{
    Result = LoadTextureToGpuAndSetHandle(Texture);
  }

  return Result;
}

// Note:: EntityID is for debug purposes, remove later
void DrawAssetRenderObject(asset::mesh::primitive* Primitive, asset::phong_material* Material, m4 Transform, ecs::entity_id EntityID)
{
  asset_render_object Object = {};
  Object.Primitive = Primitive;
  Object.ShaderType = shader_type::PHONG;
  Object.PhongMaterial = Material;
  Object.Transform = Transform;
  Object.EntityID = EntityID;
  Platform.DEBUGPrint("Drawing Phong Entity: %s\n",  ecs::GetName(GlobalEntityManager, &EntityID));
  GlobalRenderer->RenderList2.PushBack(Object);
}
// Note:: EntityID is for debug purposes, remove later
void DrawAssetRenderObject(asset::mesh::primitive* Primitive, asset::pbr_material* Material, m4 Transform, ecs::entity_id EntityID)
{
  asset_render_object Object = {};
  Object.Primitive = Primitive;
  Object.ShaderType = shader_type::PBR;
  Object.PbrMaterial = Material;
  Object.Transform = Transform;
  Object.EntityID = EntityID;
  Platform.DEBUGPrint("Drawing PBR Entity: %s\n",  ecs::GetName(GlobalEntityManager, &EntityID));
  GlobalRenderer->RenderList2.PushBack(Object);
}


// TODO: Remove these. Should only use DrawAssetRenderObject
void DrawMesh( asset::mesh_id ID, const m4& Transform, ecs::entity_id EntityID)
{
  GlobalRenderer->RenderList.PushBack({ID, Transform, EntityID});
}

void DrawRenderTree(asset::render_tree_id ID, ecs::entity_id EntityID) {

  asset::render_tree* RenderTree = (asset::render_tree*) asset::Find(asset::type::RENDER_TREE, ID);
  if(RenderTree)
  {
    cmn::vector<m4> TransformVec = cmn::vector<m4>::CreateTransient(RenderTree->MaxDepth());
    cmn::n_tree_pre_order_it<asset::render_tree_data> It = RenderTree->PreOrderIterator();
    while(cmn::n_tree_node<asset::render_tree_data>* Node = It.Next())
    {
      int Index = It.Depth() - 1;
      m4& CurrentTransform = TransformVec[Index];
      asset::render_tree_data* Data = Node->Data;
      if(Index == 0)
      {
        CurrentTransform = Data->HasTransform ? Data->Transform : M4Identity();
      }else{
        CurrentTransform = Data->HasTransform ? CurrentTransform * TransformVec[Index-1] : TransformVec[Index-1];
      }
      if(Data->Mesh)
      {
        DrawMesh(Data->Mesh, CurrentTransform, EntityID);
      }
    }
  }
}


} // namespace render 