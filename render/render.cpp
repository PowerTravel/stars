#include "render.h"
#include "asset_manager/asset_manager.h"

#include "renderer/render_push_buffer/application_render_push_buffer.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

#include "shaders/pbr/pbr.h"
#include "shaders/phong/phong.h"
#include "shaders/common/shaders.h"
#include "shaders/common/gaussian_blur.h"

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
  INTERNAL_SHADER_OVERLAY_SDF,
  INTERNAL_SHADER_OVERLAY_SPRITE_RGBA,
  INTERNAL_SHADER_OVERLAY_SPRITE_RGB,
  INTERNAL_SHADER_OVERLAY_SPRITE_A,
  INTERNAL_SHADER_OVERLAY_QUAD,
  INTERNAL_SHADER_COUNT
};

char** LoadFileFromDisk(const char* CodePath)
{
  Assert(GlobalRenderer);
  char** Result = 0; 
  debug_read_file_result Shader = Platform.DEBUGPlatformReadEntireFile(CodePath);
  char* ShaderCode = 0;
  if(Shader.Contents)
  {
    ShaderCode = (char*) PushSize(&GlobalRenderer->RenderTransientArena, Shader.ContentSize+2);
    utils::Copy(Shader.ContentSize, Shader.Contents, ShaderCode);
    ShaderCode[Shader.ContentSize+1] = '\n';
    Platform.DEBUGPlatformFreeFileMemory(Shader.Contents);
    Result = PushStruct(&GlobalRenderer->RenderTransientArena, char*);
    *Result = ShaderCode;
  }else{
    INVALID_CODE_PATH
  }
  return Result;
}

struct vertex_data
{
  u32 IndexCount;
  u32* Indeces;
  u32 VertexCount;
  opengl_vertex* VertexData;
};


vertex_data CreateGLVertexBuffer(memory_arena* Arena,
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
  
  vertex_data Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces =  (u32*) GLIndexArray;
  Result.VertexCount = VertexCount;
  Result.VertexData = VertexData;
  return Result;
}

file_local void GeometryToGlVertexData(memory_arena* Arena, const asset::geometry * Geometry, vertex_data* Result)
{
  Assert(Geometry->IndexCount && Geometry->Indeces && Geometry->VertexCount && Geometry->Vertex);
  // We are only handling 1 set of texture vertices atm. Increase if we find the need
  Assert(Geometry->TextureVertexSetCount== 0 || Geometry->TextureVertexSetCount ==1);
  *Result = CreateGLVertexBuffer(
      Arena,
      Geometry->IndexCount,
      Geometry->Indeces,
      Geometry->VertexCount,
      Geometry->Vertex,
      Geometry->VertexNormal,
      Geometry->TextureVertices ? Geometry->TextureVertices[0] : 0
    );
}

file_local u32* CreateInternalTextures(render_group* RenderGroup, r32 MSAA)
{    
  texture_params DefaultColor = DefaultColorTextureParams();
  texture_params DefaultDepth = DefaultDepthTextureParams();
  texture_params RevealTexParam = DefaultColorTextureParams();
  RevealTexParam.TextureFormat = texture_format::R_8;

  window_size_pixel* Window = &GlobalWindowSize;

  u32* Result = (u32*) PushArray(GlobalPersistentArena, INT_TEX_COUNT, u32);
  Result[INT_TEX_MSAA_COLOR] = PushNewTexture2D(RenderGroup, MSAA * Window->ApplicationWidth, MSAA * Window->ApplicationHeight, DefaultColor, 0);
  Result[INT_TEX_MSAA_DEPTH] = PushNewTexture2D(RenderGroup, MSAA * Window->ApplicationWidth, MSAA * Window->ApplicationHeight, DefaultDepth, 0);
  Result[INT_TEX_ACCUM]      = PushNewTexture2D(RenderGroup, MSAA * Window->ApplicationWidth, MSAA * Window->ApplicationHeight, DefaultColor, 0);
  Result[INT_TEX_REVEAL]     = PushNewTexture2D(RenderGroup, MSAA * Window->ApplicationWidth, MSAA * Window->ApplicationHeight, RevealTexParam, 0);
  Result[INT_TEX_GAUSSIAN_A] = PushNewTexture2D(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
  Result[INT_TEX_GAUSSIAN_B] = PushNewTexture2D(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
  return Result;
}

file_local u32* CreateFrameBuffers(render_group* RenderGroup, u32* Textures,  r32 MSAA )
{
  window_size_pixel* Window = &GlobalWindowSize;
  u32* Result = (u32*) PushArray(GlobalPersistentArena, FRAMEBUFFER_COUNT, u32);
  u32 TransparentColorTexture[]         = {Textures[INT_TEX_ACCUM], Textures[INT_TEX_REVEAL]};
  Result[FRAMEBUFFER_DEFAULT]     = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 0, 0, 0, 0);
  Result[FRAMEBUFFER_MSAA]        = PushNewFrameBuffer(RenderGroup,  MSAA * Window->ApplicationWidth,  MSAA * Window->ApplicationHeight, 1, &Textures[INT_TEX_MSAA_COLOR], Textures[INT_TEX_MSAA_DEPTH], 0);
  Result[FRAMEBUFFER_TRANSPARENT] = PushNewFrameBuffer(RenderGroup,  MSAA * Window->ApplicationWidth,  MSAA * Window->ApplicationHeight, ArrayCount(TransparentColorTexture), TransparentColorTexture, Textures[INT_TEX_MSAA_DEPTH], 0);
  Result[FRAMEBUFFER_GAUSSIAN_A]  = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 1, &Textures[INT_TEX_GAUSSIAN_A], 0, 0);
  Result[FRAMEBUFFER_GAUSSIAN_B]  = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 1, &Textures[INT_TEX_GAUSSIAN_B], 0, 0);
  return Result;
}

u32 LoadVertexdataToGPU(render_group* RenderGroup, u32 IndexCount, u32* Indeces, u32 VertexCount, opengl_vertex* VertexData){
  u32 MeshHandle  = PushNewMesh(RenderGroup, VertexCount, VertexData);
  u32 IndexHandle = PushNewMeshIndices(RenderGroup, MeshHandle, IndexCount, Indeces);
  return IndexHandle;
}

file_local u32 CreateBlitPlane(render_group* RenderGroup)
{
  u32 Indeces[] = {
    0,1,2,
    2,1,3
  };
  opengl_vertex VertexData[4] =
  {
     // v                  vn       vt
    {{-1.0f, -1.0f, 0.0f}, {0,0,1}, {0,0}},
    {{ 1.0f, -1.0f, 0.0f}, {0,0,1}, {1,0}},
    {{-1.0f,  1.0f, 0.0f}, {0,0,1}, {0,1}},
    {{ 1.0f,  1.0f, 0.0f}, {0,0,1}, {1,1}}
  };

  u32 ResultHandle = LoadVertexdataToGPU(RenderGroup, 
    ArrayCount(Indeces),              (u32*) PushCopy(GlobalTransientArena, sizeof(Indeces),    Indeces),
    ArrayCount(VertexData), (opengl_vertex*) PushCopy(GlobalTransientArena, sizeof(VertexData), VertexData));
  
  return ResultHandle;
}

file_local u32* CreateBasicShapes(render_group* RenderGroup)
{
  u32* Result = PushArray(GlobalPersistentArena, BASIC_SHAPE_COUNT, u32);
  Result[BASIC_SHAPE_BLIT_PLANE] = CreateBlitPlane(RenderGroup);
  return Result;
}


file_local u32* CreateInternalShaders(render_group* RenderGroup)
{
  u32* Result = PushArray(GlobalPersistentArena, INTERNAL_SHADER_COUNT, u32);
  Result[INTERNAL_SHADER_TRANSPARENT_COMPOSITION] = common_shaders::CreateTransparentCompositionProgram(RenderGroup);
  Result[INTERNAL_SHADER_GAUSSIAN_BLUR_X]         = common_shaders::CreateGaussianBlurProgramX(RenderGroup);
  Result[INTERNAL_SHADER_GAUSSIAN_BLUR_Y]         = common_shaders::CreateGaussianBlurProgramY(RenderGroup);
  Result[INTERNAL_SHADER_OVERLAY_SDF]             = common_shaders::CreateSDFRenderProgram(RenderGroup);
  Result[INTERNAL_SHADER_OVERLAY_SPRITE_RGBA]     = common_shaders::CreateSpriteRenderProgram(RenderGroup, 4);
  Result[INTERNAL_SHADER_OVERLAY_SPRITE_RGB]      = common_shaders::CreateSpriteRenderProgram(RenderGroup, 3);
  Result[INTERNAL_SHADER_OVERLAY_SPRITE_A]        = common_shaders::CreateSpriteRenderProgram(RenderGroup, 1);
  Result[INTERNAL_SHADER_OVERLAY_QUAD]            = common_shaders::CreateSpriteRenderProgram(RenderGroup, 0);
  return Result;
}

CMN_MALLOC_FUNCTION(RenderTransientMalloc){
  return PushSize(&GlobalRenderer->RenderTransientArena, sz);
}
CMN_FREE_FUNCTION(RenderTransientFree){
}

renderer* CreateRenderer(render_group* RenderGroup, r32 ApplicationWidth, r32 ApplicationHeight, application_render_commands* RenderCommands)
{
  renderer* Result         = BootstrapPushStruct(renderer, RenderTransientArena);
  Result->RenderGroup      = RenderGroup;
  GlobalRenderer           = Result;

  Result->RenderHandles    = NewChunkList(GlobalPersistentArena, sizeof(u32), 128);
  Result->LoadedTextures   = NewRBTree(GlobalPersistentArena, 64, 64);
  Result->LoadedPrograms   = NewRBTree(GlobalPersistentArena, 64, 64);
  Result->LoadedPrimitives = NewRBTree(GlobalPersistentArena, 64, 64);
  Result->Cameras          = cmn::hash_map<camera>::Create(8);

  Result->MSAA = 4; 
  Result->InternalTextures  = CreateInternalTextures(RenderGroup, Result->MSAA);
  Result->FrameBuffers      = CreateFrameBuffers(RenderGroup, Result->InternalTextures, Result->MSAA);
  Result->BasicShapes       = CreateBasicShapes(RenderGroup);
  Result->InternalShaders   = CreateInternalShaders(RenderGroup);
  Result->Font              = font::Create(RenderGroup, "C:\\Windows\\Fonts\\consola.ttf");



  Result->TempMem          = BeginTemporaryMemory(&Result->RenderTransientArena);
  Result->RenderList       = render_list::Create(RenderTransientMalloc, RenderTransientFree);
  Result->OverlayLevels    = cmn::list<overlay_level>::Create(RenderTransientMalloc, RenderTransientFree);
  return Result;
}

void Begin()
{
  Assert(GlobalRenderer);
  EndTemporaryMemory( GlobalRenderer->TempMem );
  GlobalRenderer->TempMem = BeginTemporaryMemory(&GlobalRenderer->RenderTransientArena);

  GlobalRenderer->RenderList = render_list::Create(RenderTransientMalloc, RenderTransientFree);
  GlobalRenderer->OverlayLevels = cmn::list<overlay_level>::Create(RenderTransientMalloc, RenderTransientFree);
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
  window_size_pixel* Window = &GlobalWindowSize;

  render_state* DefaultState = PushNewState(RenderGroup);
  *DefaultState = DefaultRenderState3(Renderer->MSAA * Window->ApplicationWidth, Renderer->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio);

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
}


u32 LoadGeometryToGPU(asset::geometry* Geometry)
{  
  vertex_data VertexData = {};
  GeometryToGlVertexData(&GlobalRenderer->RenderTransientArena, Geometry, &VertexData);
  u32 IndexHandle = LoadVertexdataToGPU(GlobalRenderCommands->RenderGroup, VertexData.IndexCount, VertexData.Indeces,
  VertexData.VertexCount, VertexData.VertexData );
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

file_local void SetHandle(rb_tree* HandleTree, size_t Key, u32 Handle){
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
      //Platform.DEBUGPrint("Program Hash: %u, Dfac = %s, Tras = %s\n",
      //  ProgramHash,
      //  Definition.HasDiffuseTexture ? "True" : "False",
      //  Definition.Transparent       ? "True" : "False");
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

local_persist u32 GetOrCreateGeometryID(render_group* RenderGroup, asset::geometry* AssetGeometry)
{
  size_t AssetGeometryID = (size_t) AssetGeometry;
  u32* PrimitiveHandlePtr = (u32*) Find(&GlobalRenderer->LoadedPrimitives, AssetGeometryID);
  u32 GeometryID = 0;
  if(!PrimitiveHandlePtr)
  {
    GeometryID  = LoadGeometryToGPU(AssetGeometry);
    SetHandle(&GlobalRenderer->LoadedPrimitives, AssetGeometryID, GeometryID);
  }else{
    GeometryID = *PrimitiveHandlePtr;
  }

  return GeometryID;
}

file_local void ActivateMSAAFrameBuffer(render_group* RenderGroup)
{
  render_state* MSAAViewport = PushNewState(RenderGroup);
  SetState(MSAAViewport, ViewportState(GlobalRenderer->MSAA * GlobalWindowSize.ApplicationWidth, GlobalRenderer->MSAA * GlobalWindowSize.ApplicationHeight, GlobalWindowSize.ApplicationAspectRatio));
}

file_local void TurnOffZBuffer(render_group* RenderGroup)
{
  render_state* TransparentState = PushNewState(RenderGroup);
  depth_state DepthState = {};
  SetState(TransparentState, DepthState);
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
      pbr::SetMaterialUniforms(RenderGroup, Object, Primitive->PbrMaterial);
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
      phong::SetMaterialUniforms(RenderGroup, Object, Primitive->PhongMaterial);
      int a = 10;
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
  BlitOperation->ReadFrameBufferHandle = SrcBuffer;
  BlitOperation->DrawFrameBufferHandle = DstBuffer;
  BlitOperation->DrawRegionUnitCoord = DrawRegion;
}

file_local void GaussianBlur(render_group* RenderGroup, u32 BlurCount, u32 SrcBuffer, u32 DstBuffer, u32 ApplicationWidth, u32 ApplicationHeight)
{
  r32* KernelOffset = PushArray(&GlobalRenderer->RenderTransientArena, 64, r32);
  r32* KernelWeight = PushArray(&GlobalRenderer->RenderTransientArena, 64, r32);
  u32 KernelSize    = gaussian_blur::Kernel(12, 2, KernelOffset, KernelWeight);
  BlitBuffers(RenderGroup, SrcBuffer, FrameBuffer(FRAMEBUFFER_GAUSSIAN_A), Rect2f(0,0,1,1));
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
  BlitBuffers(RenderGroup, FrameBuffer(FRAMEBUFFER_GAUSSIAN_B), DstBuffer,  Rect2f(0,0,1,1));
}


file_local void DrawSprites(render_group* RenderGroup, u32 ProgramHandle, u32 TextureHandle, m4& OrthoProjectionMatrix, u32 SpriteCount, common_shaders::sprite_varying* SpriteVec)
{
  render_object* SpriteObject       = PushNewRenderObject(RenderGroup);
  SpriteObject->ProgramHandle       = ProgramHandle;
  SpriteObject->MeshHandle          = BasicShape(BASIC_SHAPE_BLIT_PLANE);
  SpriteObject->FrameBufferHandle   = FrameBuffer(FRAMEBUFFER_DEFAULT);
  SpriteObject->TextureHandles[0]   = GlobalState->ImguiContext.Icons.Atlas; // Todo: Make dynamic
  SpriteObject->TextureCount        = 1;

  PushUniform(SpriteObject, GetUniformHandle(RenderGroup, SpriteObject->ProgramHandle, "Projection"), OrthoProjectionMatrix);
  PushUniform(SpriteObject, GetUniformHandle(RenderGroup, SpriteObject->ProgramHandle, "SpriteMap"), (u32)0);
  PushInstanceData(SpriteObject, SpriteCount, SpriteCount*sizeof(common_shaders::sprite_varying), (void*) SpriteVec);
}

#define CMN_LIST_FOR_EACH( _ListName, _ElementName ) for( auto* _ElementName = _ListName.First(); !_ListName.IsEnd(_ElementName); _ElementName = _ElementName->Next)
file_local void DrawOverlaySprites(render_group* RenderGroup, overlay_level* OverlayLevel, m4& OrthoProjectionMatrix)
{
  cmn::list<overlay_sprite>& OverlaySprites = OverlayLevel->OverlaySprite;

  cmn::vector<common_shaders::sprite_varying> SolidQuads  = cmn::vector<common_shaders::sprite_varying>::CreateTransient(OverlayLevel->SolidQuad);
  cmn::vector<common_shaders::sprite_varying> SpriteAs    = cmn::vector<common_shaders::sprite_varying>::CreateTransient(OverlayLevel->SpriteA);
  cmn::vector<common_shaders::sprite_varying> SpriteRGBs  = cmn::vector<common_shaders::sprite_varying>::CreateTransient(OverlayLevel->SpriteRGB);
  cmn::vector<common_shaders::sprite_varying> SpriteRGBAs = cmn::vector<common_shaders::sprite_varying>::CreateTransient(OverlayLevel->SpriteRGBA);
  CMN_LIST_FOR_EACH(OverlaySprites, Element)
  {
    overlay_sprite* Sprite = Element->GetPtr();
    m4 ModelMatrix = M4Identity();
    Scale(V4(0.5,0.5, 0, 1), ModelMatrix);
    Scale(V4(Sprite->Rect.W, Sprite->Rect.H, 0, 1), ModelMatrix);
    Translate(V4(Sprite->Rect.X, Sprite->Rect.Y, 0, 0), ModelMatrix);
    ModelMatrix = Transpose(ModelMatrix);

    common_shaders::sprite_varying Varying = {};
    Varying.Color = Sprite->Color;
    Varying.TexCoord = Sprite->TexCoord;
    Varying.TexDepth = Sprite->SpriteDepth;
    Varying.ModelMatrix = ModelMatrix;

    switch(Sprite->SpriteColorCount)
    {
      case 0: {
        Assert(SolidQuads.Size()<OverlayLevel->SolidQuad);
        SolidQuads.PushBack(Varying);
      } break;
      case 1: {
        Assert(SpriteAs.Size()<OverlayLevel->SpriteA);
        SpriteAs.PushBack(Varying);
      } break;
      case 3: {
        Assert(SpriteRGBs.Size()<OverlayLevel->SpriteRGB);
        SpriteRGBs.PushBack(Varying);
      } break;
      case 4: {
        Assert(SpriteRGBAs.Size()<OverlayLevel->SpriteRGBA);
        SpriteRGBAs.PushBack(Varying);
      } break;
    }
  }

  if(SolidQuads.Size()){
    DrawSprites(RenderGroup, InternalShader(INTERNAL_SHADER_OVERLAY_QUAD), OverlayLevel->SpriteHandle, OrthoProjectionMatrix, SolidQuads.Size(), SolidQuads.m_data);
  }
  if(SpriteAs.Size()){
    DrawSprites(RenderGroup, InternalShader(INTERNAL_SHADER_OVERLAY_SPRITE_A), OverlayLevel->SpriteHandle, OrthoProjectionMatrix, SpriteAs.Size(), SpriteAs.m_data);
  }
  if(SpriteRGBs.Size()){
    DrawSprites(RenderGroup, InternalShader(INTERNAL_SHADER_OVERLAY_SPRITE_RGB), OverlayLevel->SpriteHandle, OrthoProjectionMatrix, SpriteRGBs.Size(), SpriteRGBs.m_data);
  }
  if(SpriteRGBAs.Size()){
    DrawSprites(RenderGroup, InternalShader(INTERNAL_SHADER_OVERLAY_SPRITE_RGBA), OverlayLevel->SpriteHandle, OrthoProjectionMatrix, SpriteRGBAs.Size(), SpriteRGBAs.m_data);
  }
}

file_local void DrawSDF(render_group* RenderGroup, cmn::list<overlay_sdf>& OverlaySDF, m4& OrthoProjectionMatrix)
{
  render_object* OverlaySDFProgram     = PushNewRenderObject(RenderGroup);
  OverlaySDFProgram->ProgramHandle     = InternalShader(INTERNAL_SHADER_OVERLAY_SDF);
  OverlaySDFProgram->MeshHandle        = BasicShape(BASIC_SHAPE_BLIT_PLANE);
  OverlaySDFProgram->FrameBufferHandle = FrameBuffer(FRAMEBUFFER_DEFAULT);
  OverlaySDFProgram->TextureHandles[0] = GlobalRenderer->Font.FontMapHandle;
  OverlaySDFProgram->TextureCount      = 1;
  
  PushUniform(OverlaySDFProgram, GetUniformHandle(RenderGroup, OverlaySDFProgram->ProgramHandle, "Projection"),         OrthoProjectionMatrix);
  PushUniform(OverlaySDFProgram, GetUniformHandle(RenderGroup, OverlaySDFProgram->ProgramHandle, "SDFMap"),             (u32) 0);
  PushUniform(OverlaySDFProgram, GetUniformHandle(RenderGroup, OverlaySDFProgram->ProgramHandle, "OnEdgeValue"),        (r32) 128/255.f);
  PushUniform(OverlaySDFProgram, GetUniformHandle(RenderGroup, OverlaySDFProgram->ProgramHandle, "PixelDistanceScale"), (r32) 32/255.f);
  
  size_t SDFCount = OverlaySDF.Size();
  common_shaders::sdf_varying* SDF = PushArray(&GlobalRenderer->RenderTransientArena, SDFCount, common_shaders::sdf_varying);
  int i = 0;
  CMN_LIST_FOR_EACH(OverlaySDF, SDFElement)
  { 
    overlay_sdf* OverlaySDF = SDFElement->GetPtr();
    common_shaders::sdf_varying SDFVarying = {};
    SDFVarying.Color = OverlaySDF->Color;
    SDFVarying.TextCoord = OverlaySDF->TextCoord;
    SDFVarying.ModelMatrix = OverlaySDF->ModelMatrix;

    SDF[i++] = SDFVarying;
  }
  PushInstanceData(OverlaySDFProgram, SDFCount, SDFCount*sizeof(common_shaders::sdf_varying), (void*) SDF);
}

void RenderScene(m4 ProjectionMatrix, m4 ViewMatrix)
{
  SCOPED_TRANSIENT_ARENA;
  renderer* Renderer = GlobalRenderer;
  render_group* RenderGroup = Renderer->RenderGroup;

  v3 LightColor     = V3(1,1,1);
  v3 LightPosition  = V3(1,1,1);
  v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPosition,0));

  window_size_pixel* Window = &GlobalWindowSize;

  // Wipes all internal textures and resets to default render state
  ClearRenderState(Renderer);

  r32 InitTime = Platform.DEBUGGetTime();
  cmn::list<primitive> SolidMesh = cmn::list<primitive>::CreateTransient();
  cmn::list<primitive> TransparentMesh = cmn::list<primitive>::CreateTransient();
  render_list_element* Element = Renderer->RenderList.First();
  r32 ElementCount = Renderer->RenderList.Size();
  while (!Renderer->RenderList.IsEnd(Element))
  {
    asset_render_object* AssetRenderObject = Element->Data;
    
    primitive LoadedPrimitive  = {};
    LoadedPrimitive.GeometryID  = GetOrCreateGeometryID(RenderGroup, AssetRenderObject->Geometry);
    LoadedPrimitive.ShaderType  = AssetRenderObject->ShaderType;
    LoadedPrimitive.Transparent = IsTransparent(AssetRenderObject);
    LoadedPrimitive.ProgramID   = GetOrCreateProgram(RenderGroup, AssetRenderObject);
    LoadedPrimitive.Geometry   = AssetRenderObject->Geometry;
    if(LoadedPrimitive.ShaderType == shader_type::PBR){
      LoadedPrimitive.PbrMaterial = AssetRenderObject->PbrMaterial;
    }else if(LoadedPrimitive.ShaderType == shader_type::PHONG){
      LoadedPrimitive.PhongMaterial = AssetRenderObject->PhongMaterial;
    }
    LoadedPrimitive.Transform   = &AssetRenderObject->Transform;
    if(LoadedPrimitive.Transparent)
    {
      TransparentMesh.PushBack(LoadedPrimitive);
    }else{
      SolidMesh.PushBack(LoadedPrimitive);
    }

    Element = Element->Next;
  }
  //Platform.DEBUGPrint("%f Meshes in %f sec\n", ElementCount, Platform.DEBUGGetTime() - InitTime);
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

  TurnOffZBuffer(RenderGroup);
  //render_state* ScaleViewport = PushNewState(RenderGroup);
  m4 OrthoProjectionMatrix = GetOrthographicProjection(-1, 1, Window->ApplicationWidth, 0, Window->ApplicationHeight, 0);
  cmn::list<overlay_level>& OverlayLevels = Renderer->OverlayLevels;
  CMN_LIST_FOR_EACH(OverlayLevels,LevelElement)
  {
    overlay_level* OverlayLevel = LevelElement->GetPtr();
    DrawOverlaySprites(RenderGroup, OverlayLevel, OrthoProjectionMatrix);
    DrawSDF (RenderGroup,  OverlayLevel->OverlaySDF,    OrthoProjectionMatrix);
  }
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

u32 LoadImageToGpu(asset::image* Image, texture_params TextureParams) {
  Assert(Image);
  u32 Handle = PushNewTexture2D(GlobalRenderCommands->RenderGroup, Image->Width, Image->Height, TextureParams, Image->Pixels);
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

void DrawAssetRenderObject(asset::geometry* Geometry, asset::phong_material* Material, m4 Transform)
{
  asset_render_object Object = {};
  Object.Geometry = Geometry;
  Object.ShaderType = shader_type::PHONG;
  Object.PhongMaterial = Material;
  Object.Transform = Transform;
  GlobalRenderer->RenderList.PushBack(Object);
}

void DrawAssetRenderObject(asset::geometry* Geometry, asset::pbr_material* Material, m4 Transform)
{
  asset_render_object Object = {};
  Object.Geometry = Geometry;
  Object.ShaderType = shader_type::PBR;
  Object.PbrMaterial = Material;
  Object.Transform = Transform;
  GlobalRenderer->RenderList.PushBack(Object);
}

void DrawRenderComponent(ecs::render::component* RenderComponent, m4& Transform)
{
  if(RenderComponent->PbrMaterial){
    DrawAssetRenderObject(RenderComponent->Geometry, RenderComponent->PbrMaterial, Transform);
  }else if(RenderComponent->PhongMaterial){
    DrawAssetRenderObject(RenderComponent->Geometry, RenderComponent->PhongMaterial, Transform);
  }
}

void DrawMesh( asset::mesh_id ID, const m4& Transform)
{
  asset::mesh* Mesh = (asset::mesh*) Find(asset::type::MESH, ID);
  for (int i = 0; i < Mesh->PrimitiveCount; ++i)
  {
    asset::mesh::primitive* Primitive = &Mesh->Primitives[i];
    asset::geometry* Geometry = (asset::geometry*) Find(asset::type::GEOMETRY, Primitive->Geometry);
    if(Primitive->PbrMaterial)
    {
      asset::pbr_material* Material = (asset::pbr_material*) Find(asset::type::PBR_MATERIAL, Primitive->PbrMaterial);
      DrawAssetRenderObject(Geometry, Material, Transform);
    }else if(Primitive->PhongMaterial){
      asset::phong_material* Material = (asset::phong_material*) Find(asset::type::PHONG_MATERIAL, Primitive->PhongMaterial);
      DrawAssetRenderObject(Geometry, Material, Transform);
    }
  }
}

void UseCamera(asset::camera_id CameraID, m4& Transform)
{
  camera* Camera = GlobalRenderer->Cameras.FindVal(CameraID);
  if(!Camera)
  {
    asset::camera* AssetCamera = (asset::camera*) asset::Find(asset::type::CAMERA, CameraID);
    Assert(AssetCamera);
    Camera = GlobalRenderer->Cameras.AtVal(CameraID);
    *Camera = FromAssetCamera(AssetCamera, Transform);
  }
  GlobalRenderer->ActiveCamera = Camera;
}

void DrawRenderTree(asset::render_tree_id ID) {

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
        m4 PreviousTransform = TransformVec[Index-1];
        if(Data->HasTransform)
        {
          CurrentTransform = PreviousTransform*Data->Transform;
        }else{
          CurrentTransform = PreviousTransform;
        }
      }
      if(Data->Camera)
      {
        UseCamera(Data->Camera, CurrentTransform);
      }
      if(Data->Mesh)
      {
        DrawMesh(Data->Mesh, CurrentTransform);
      }
    }
  }
}

void RecompileAllPrograms()
{
  
}

file_local overlay_level* GetTopOverlayLevel()
{
  cmn::list<overlay_level>& OverlayLevels = GlobalRenderer->OverlayLevels;
  if(OverlayLevels.IsEnd(OverlayLevels.Last()))
  {
    OverlayLevels.PushBack({});
  }
  return OverlayLevels.Last()->Data;
}

file_local cmn::list<overlay_sdf>& GetOverlaySDF(overlay_level* OverlayLevel){
  if(!OverlayLevel->OverlaySDF.Initiated())
  {
    OverlayLevel->OverlaySDF = cmn::list<overlay_sdf>::Create(RenderTransientMalloc, RenderTransientFree);
  }
  return OverlayLevel->OverlaySDF;
}

inline file_local m4 ModelMatrixFromRect(v2 Pos, v2 Size){
  m4 Result = M4Identity();
  Scale(V4(Size.X,Size.Y,1,0), Result); 
  Translate(V4(Pos.X, Pos.Y, 0, 1), Result);
  Result = Transpose(Result);
  return Result;
}

inline overlay_sdf OverlaySDFFromPrintCoordinate(jfont::print_coordinates* tc, v4 Color, r32 OnEdgeValue, r32 PixelDistanceScale)
{
  overlay_sdf Result = {};
  Result.Color = Color;
  Result.TextCoord = V4(tc->u0, tc->v0, tc->u1, tc->v1);
  Result.ModelMatrix = ModelMatrixFromRect(V2(tc->x, tc->y), V2(tc->sx,tc->sy));
  Result.OnEdgeValue =  OnEdgeValue;
  Result.PixelDistanceScale = PixelDistanceScale;


  m4 M = M4Identity();
  Scale(V4(tc->sx, tc->sy,1,0), M);
  Translate(V4(tc->x,tc->y, 0, 1), M);
  M = Transpose(M);

  m4 MM = M - Result.ModelMatrix;

  return Result;
}

void DrawTextPixelSpace(v2 PixelPos, rect2f PixelClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  SCOPED_TRANSIENT_ARENA;
  renderer* Renderer = GlobalRenderer;
  font& Font = Renderer->Font;

  u32 BuffLen = jstr::StringLength((const char*) Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, BuffLen+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode(Text, CodePoints);
  r32 RelativeScale = Font.GetScale(PixelSize);
  jfont::print_coordinates* TextPrintCoordinates = PushArray(GlobalTransientArena, UnicodeLen, jfont::print_coordinates);
  jfont::GetTextPrintCoordinates(&Font.Font, &Font.FontAtlas, RelativeScale, PixelPos.X, PixelPos.Y, PixelClipRect, CodePoints, TextPrintCoordinates);

  overlay_level* OverlayLevel = GetTopOverlayLevel();
  OverlayLevel->SDFHandle = GlobalRenderer->Font.FontMapHandle; // TODO: Handle SDF handles more dynamically
  cmn::list<overlay_sdf>& OverlaySDFList = GetOverlaySDF(OverlayLevel);
  for (int i = 0; i < UnicodeLen; ++i){
    OverlaySDFList.PushBack( OverlaySDFFromPrintCoordinate(TextPrintCoordinates+i, Color, 128/255.f, 32/255.f));

#if 0
  m4 Result = M4Identity();
  Scale(V4(Size,1,0), Result);
  Translate(V4(Pos, 0, 1), Result);
  Result = Transpose(Result);
#endif



  }
}

void DrawTextCanonicalSpace(v2 CanonicalPos,  rect2f CanonicalClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  v2 PixelPos = CanonicalToPixelSpace(CanonicalPos);
  v2 PixelClipPos = CanonicalToPixelSpace(V2(CanonicalClipRect.X, CanonicalClipRect.Y));
  v2 PixelClipSize = CanonicalToPixelSpace(V2(CanonicalClipRect.W, CanonicalClipRect.H));
  DrawTextPixelSpace(PixelPos, Rect2f(PixelClipPos, PixelClipSize), PixelSize, Text, Color);
}


void DrawTextPixelSpace(v2 PixelPos, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  SCOPED_TRANSIENT_ARENA;
  renderer* Renderer = GlobalRenderer;
  font& Font = Renderer->Font;

  u32 Length = jstr::StringLength((const char*) Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, Length+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode(Text, CodePoints);
  r32 RelativeScale = Renderer->Font.GetScale(PixelSize);
  jfont::print_coordinates* TextPrintCoordinates = PushArray(GlobalTransientArena, UnicodeLen, jfont::print_coordinates);
  jfont::GetTextPrintCoordinates(&Font.Font, &Font.FontAtlas, RelativeScale, PixelPos.X, PixelPos.Y, CodePoints, TextPrintCoordinates);

  overlay_level* OverlayLevel = GetTopOverlayLevel();
  OverlayLevel->SDFHandle = GlobalRenderer->Font.FontMapHandle; // TODO: Handle SDF handles more dynamically
  cmn::list<overlay_sdf>& OverlaySDFList = GetOverlaySDF(OverlayLevel);
  for (int i = 0; i < UnicodeLen; ++i){
    OverlaySDFList.PushBack( OverlaySDFFromPrintCoordinate(TextPrintCoordinates+i, Color, 128/255.f, 32/255.f));
  }
}


void DrawTextCanonicalSpace(v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  v2 PixelPos = CanonicalToPixelSpace(CanonicalPos);
  DrawTextPixelSpace(PixelPos, PixelSize, Text, Color);
}


file_local cmn::list<overlay_sprite>& GetOverlaySprite(overlay_level* OverlayLevel){
  if(!OverlayLevel->OverlaySprite.Initiated())
  {
    OverlayLevel->OverlaySprite = cmn::list<overlay_sprite>::Create(RenderTransientMalloc, RenderTransientFree);
  }
  return OverlayLevel->OverlaySprite;
}


void DrawOverlaySprite(rect2f PixelRect, v4 TextureCoords, v4 Color, u32 SpriteColorCount)
{
  overlay_sprite Sprite = {};
  Sprite.Rect = PixelRect;
  Sprite.TexCoord = TextureCoords;
  Sprite.Color = Color;
  Sprite.SpriteDepth = 0; // For now we arent using TextureArrays to hold spritemaps.
  Sprite.SpriteColorCount = SpriteColorCount;

  overlay_level* OverlayLevel = GetTopOverlayLevel();
  OverlayLevel->SpriteHandle = GlobalState->ImguiContext.Icons.Atlas; // Handle more dynamically
  switch(SpriteColorCount)
  {
    case 0: {OverlayLevel->SolidQuad++;   } break;
    case 1: {OverlayLevel->SpriteA++;     } break;
    case 3: {OverlayLevel->SpriteRGB++;   } break;
    case 4: {OverlayLevel->SpriteRGBA++;  } break;
    default: INVALID_CODE_PATH;      
  }
  cmn::list<overlay_sprite>& OverlaySpriteList = GetOverlaySprite(OverlayLevel);
  OverlaySpriteList.PushBack(Sprite);
}

void DrawOverlayQuadPixelSpace(rect2f PixelRect, v4 Color) { 
  DrawOverlaySprite(PixelRect, {}, Color, 0);
}

void DrawOverlayQuadCanonicalSpace(rect2f CanonicalRect, v4 Color)
{
  rect2f PixelRect = Rect2f( CanonicalToPixelSpace(V2(CanonicalRect.X, CanonicalRect.Y)),
                             CanonicalToPixelSpace(V2(CanonicalRect.W, CanonicalRect.H)));
  DrawOverlaySprite(PixelRect, {}, Color, 0);
}

void DrawIconPixelSpace(rect2f PixelRect, v4 TextureCoords, v4 Color)
{
  // Temp Hack solution.
  DrawOverlaySprite(PixelRect, TextureCoords, Color, 1);
}

void DrawIconCanonicalSpace(rect2f CanonicalRect, v4 TextureCoords, v4 Color)
{
  rect2f PixelRect = Rect2f( CanonicalToPixelSpace(V2(CanonicalRect.X,CanonicalRect.Y)),
                             CanonicalToPixelSpace(V2(CanonicalRect.W,CanonicalRect.H)));
  DrawOverlaySprite(PixelRect, TextureCoords, Color, 1);
}

void NewOverlayLevel() {
  GlobalRenderer->OverlayLevels.PushBack({});
}
  

} // namespace render 