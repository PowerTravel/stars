#include "stars.h"

#include "platform/jwin_platform_memory.cpp"
#include "platform/jwin_platform_input.h"
#include "platform/jfont.cpp"
#include "platform/obj_loader.cpp"
#include "renderer/software_render_functions.cpp"
#include "renderer/render_push_buffer.cpp"
#include "math/AABB.cpp"
#include "camera.cpp"

internal void BlendPixel(platform_offscreen_buffer* OffscreenBuffer, s32 x, s32 y, u8 Red, u8 Green, u8 Blue, u8 alpha)
{
  u32* Pixel = ((u32*) OffscreenBuffer->Memory) + x + y * OffscreenBuffer->Width;
  u32 Dest = *Pixel;

  r32 PreMul = 1/255.f;
  r32 NormAlpha   = alpha * PreMul;
  r32 NormRed     = Red * PreMul;
  r32 NormGreen   = Green * PreMul;
  r32 NormBlue    = Blue * PreMul;

  r32 DAlpha  = (r32) (( (Dest & 0xFF000000) >> 24 )) * PreMul;
  r32 DRed    = (r32) (( (Dest & 0x00FF0000) >> 16 )) * PreMul;
  r32 DGreen  = (r32) (( (Dest & 0x0000FF00) >> 8  )) * PreMul;
  r32 DBlue   = (r32) (( (Dest & 0x000000FF) >> 0  )) * PreMul;

  NormRed   = (1 - NormAlpha)*DRed   + NormAlpha*NormRed;
  NormGreen = (1 - NormAlpha)*DGreen + NormAlpha*NormGreen;
  NormBlue  = (1 - NormAlpha)*DBlue  + NormAlpha*NormBlue;

  s32 R = (s32)(NormRed   * 255 + 0.5);
  s32 G = (s32)(NormGreen * 255 + 0.5);
  s32 B = (s32)(NormBlue  * 255 + 0.5);

  *Pixel = (R << 16) | (G << 8) | (B << 0);
}




internal void DrawChar(int OnedgeValue, int PixelDistanceScale, int sdfWidth, int sdfHeight, u8* Data,
  r32 RelativeScale, platform_offscreen_buffer* OffscreenBuffer, r32 xPos, r32 yPos, r32 xOff, r32 yOff, b32 invert = false)
{
  r32 fx0 = xPos + xOff*RelativeScale;
  r32 fy0 = yPos + yOff*RelativeScale;
  if(invert)
  {
    fy0 = yPos - (sdfHeight + yOff)*RelativeScale;
  }
  r32 fx1 = fx0 + sdfWidth*RelativeScale;
  r32 fy1 = fy0 + sdfHeight*RelativeScale;
  s32 ix0 = (s32) Floor(fx0);
  s32 iy0 = (s32) Floor(fy0);
  s32 ix1 = (s32) Ciel(fx1);
  s32 iy1 = (s32) Ciel(fy1);
  // clamp to viewport
  if (ix0 < 0)
  {
    ix0 = 0;
  }
  if (iy0 < 0)
  {
    iy0 =  0;
  }
  if (ix1 > OffscreenBuffer->Width)
  {
    ix1 = OffscreenBuffer->Width;
  }
  if (iy1 > OffscreenBuffer->Height)
  {
    iy1 = OffscreenBuffer->Height;
  }

  for (s32 y = iy0; y < iy1; ++y)
  {
    for (s32 x = ix0; x < ix1; ++x)
    {
      // Map pixel to char-space
      r32 bmx = LinearRemap(x, fx0, fx1, 0, sdfWidth);
      // Invert Y-Axis when reading from the SDF since we use origin in bottom left and stb uses top left.
      r32 bmy = LinearRemap(y, fy0, fy1, 0, sdfHeight); 
      if(invert)
      {
        bmy = LinearRemap(y, fy1, fy0, 0, sdfHeight); 
      }

      // Get sample points for pixel in each corner of the pixel
      s32 sx0 = (int) bmx;
      s32 sx1 = sx0+1;
      s32 sy0 = (int) bmy;
      s32 sy1 = sy0+1;

      // compute lerp weights
      bmx = bmx - sx0;  // pixel diff in x-dirextion
      bmy = bmy - sy0;  // pixel diff in y-direction

      // clamp to edge
      sx0 = Clamp(sx0, 0, sdfWidth-1);
      sx1 = Clamp(sx1, 0, sdfWidth-1);
      sy0 = Clamp(sy0, 0, sdfHeight-1);
      sy1 = Clamp(sy1, 0, sdfHeight-1);

      // Bilinear texture sample
      s32 v00 = Data[sy0*sdfWidth+sx0]; // Bottom left corner
      s32 v01 = Data[sy0*sdfWidth+sx1]; // Bottom right corner
      s32 v10 = Data[sy1*sdfWidth+sx0]; // Top Left Corner
      s32 v11 = Data[sy1*sdfWidth+sx1]; // Top Right Corner

      r32 v0 = Lerp(bmx, v00, v01);
      r32 v1 = Lerp(bmx, v10, v11);
      // v is the approximated value within a sdf-pixel given the value at four corners of the pixel
      // 0 <= v <= 255
      r32 v  = Lerp(bmy, v0, v1);

      // Map SDF scale to 0,1 scale.
      //    If v is < OnedgeValue, sdf_dist is negative. 
      //    If v is > OnedgeValue + PixelDistanceScale, sdf_dist is larger than 1.
      //    If v is between, sdf_dist between 0 and 1.
      r32 sdf_dist = LinearRemap(v, OnedgeValue, OnedgeValue + PixelDistanceScale, 0, 1);
      // convert distance in SDF OffscreenBuffer to distance in output OffscreenBuffer
      r32 pix_dist = sdf_dist * RelativeScale;
      // anti-alias by mapping 1/2 pixel around contour from 0..1 alpha

      v = LinearRemap(pix_dist, -0.5f, 0.5f, 0, 1);
      
      if (v > 1)
      {
        v = 1;
      }
      if (v > 0)
      {
        BlendPixel(OffscreenBuffer, x, y, 0xd8, 0xde, 0xe9, v * 255);
      }
      r32 MinBorder = -1;
      r32 MaxBorder = 0.5;
      if (v <= MaxBorder && v > MinBorder)
      {
        v = LinearRemap(pix_dist, MinBorder, MaxBorder, 0, 125);
        BlendPixel(OffscreenBuffer, x, y, 0, 0, 0, v);
      }
    }
  }
}

internal void DrawChar(jfont::sdf_font* Font, jfont::sdf_fontchar* Char, platform_offscreen_buffer* OffscreenBuffer, r32 xPos, r32 yPos, r32 RelativeScale)
{
  DrawChar(Font->OnedgeValue, Font->PixelDistanceScale, Char->w, Char->h, Char->data, RelativeScale, OffscreenBuffer, xPos, yPos,
    Char->xoff, Char->yoff, true);
}


u32 PushUnique( u8* Array, const u32 ElementCount, const u32 ElementByteSize,
               u8* NewElement, b32 (*CompareFunction)(const u8* DataA, const u8* DataB))
{
  for( u32 i = 0; i < ElementCount; ++i )
  {
    if( CompareFunction(NewElement, Array) )
    {
      return i;
    }
    Array += ElementByteSize;
  }
  
  // If we didn't find the element we push it to the end
  utils::Copy(ElementByteSize, NewElement, Array);
  
  return ElementCount;
}


internal gl_vertex_buffer
CreateGLVertexBuffer( memory_arena* TemporaryMemory,
                     const u32 IndexCount,
                     const u32* VerticeIndeces, const u32* TextureIndeces, const u32* NormalIndeces,
                     const v3* VerticeData,     const v2* TextureData,     const v3* NormalData)
{
  jwin_Assert(VerticeIndeces && VerticeData);
  u32* GLVerticeIndexArray  = PushArray(TemporaryMemory, 3*IndexCount, u32);
  u32* GLIndexArray         = PushArray(TemporaryMemory, IndexCount, u32);
  
  u32 VerticeArrayCount = 0;
  for( u32 i = 0; i < IndexCount; ++i )
  {
    const u32 vidx = VerticeIndeces[i];
    const u32 tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const u32 nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    u32 NewElement[3] = {vidx, tidx, nidx};
    u32 Index = PushUnique((u8*)GLVerticeIndexArray, VerticeArrayCount, sizeof(NewElement), (u8*) NewElement,
                           [](const u8* DataA, const u8* DataB) {
                             u32* U32A = (u32*) DataA;
                             const u32 A1 = *(U32A+0);
                             const u32 A2 = *(U32A+1);
                             const u32 A3 = *(U32A+2);
                             u32* U32B = (u32*) DataB;
                             const u32 B1 = *(U32B+0);
                             const u32 B2 = *(U32B+1);
                             const u32 B3 = *(U32B+2);
                             b32 result = (A1 == B1) && (A2 == B2) && (A3 == B3);
                             return result;
                           });
    if(Index == VerticeArrayCount)
    {
      VerticeArrayCount++;
    }
    
    GLIndexArray[i] = Index;
  }
  
  opengl_vertex* VertexData = PushArray(TemporaryMemory, VerticeArrayCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  for( u32 i = 0; i < VerticeArrayCount; ++i )
  {
    const u32 vidx = *(GLVerticeIndexArray + 3 * i + 0);
    const u32 tidx = *(GLVerticeIndexArray + 3 * i + 1);
    const u32 nidx = *(GLVerticeIndexArray + 3 * i + 2);
    Vertice->v  = VerticeData[vidx];
    Vertice->vt = TextureData ? TextureData[tidx] : V2(0,0);
    Vertice->vn = NormalData  ? NormalData[nidx]  : V3(0,0,0);
    ++Vertice;
  }
  
  gl_vertex_buffer Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = GLIndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.VertexData = VertexData;
  return Result;
}


opengl_buffer_data MapObjToOpenGLMesh(memory_arena* Arena, obj_loaded_file* Obj)
{ 
  opengl_buffer_data Result = {};
  Result.BufferCount = Obj->ObjectCount;
  Result.BufferData = PushArray(Arena, Obj->ObjectCount,gl_vertex_buffer);
  obj_mesh_data* ObjMeshData = Obj->MeshData;
  jwin_Assert(ObjMeshData->v && ObjMeshData->vt && ObjMeshData->vn);
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    obj_mesh_indeces* ObjIndeces = Obj->ObjectGroups[i].Indeces;
    
    jwin_Assert(ObjIndeces->Count && ObjIndeces->vi && ObjIndeces->ti &&  ObjIndeces->ni);
    Result.BufferData[i] = CreateGLVertexBuffer(
        Arena,
        ObjIndeces->Count,
        ObjIndeces->vi,
        ObjIndeces->ti,
        ObjIndeces->ni,
        ObjMeshData->v,
        ObjMeshData->vt,
        ObjMeshData->vn
      );
  }

  return Result;
}

char** LoadShader(char* CodePath)
{
  char** Result = 0; 
  debug_read_file_result Shader = Platform.DEBUGPlatformReadEntireFile(CodePath);
  char* ShaderCode = 0;
  if(Shader.Contents)
  {
    ShaderCode = (char*) PushSize(GlobalTransientArena, Shader.ContentSize+2);
    utils::Copy(Shader.ContentSize, Shader.Contents, ShaderCode);
    ShaderCode[Shader.ContentSize+1] = '\n';
    Platform.DEBUGPlatformFreeFileMemory(Shader.Contents);
    Result = PushStruct(GlobalTransientArena, char*);
    *Result = ShaderCode;
  }
  return Result;
}


internal gl_shader_program* GetProgram(open_gl* OpenGL, u32 Handle)
{
  jwin_Assert(Handle < ArrayCount(OpenGL->Programs));
  gl_shader_program* Program = OpenGL->Programs + Handle;
  return Program;
}

void DeclareUniform(open_gl* OpenGL, u32 ProgramHandle, char* Name, GlUniformType Type)
{
  gl_shader_program* Program = GetProgram(OpenGL, ProgramHandle);
  u32 Handle = Program->UniformCount++;
  jwin_Assert(Handle < ArrayCount(Program->Uniforms));
  gl_uniform* Uniform = Program->Uniforms + Handle;
  Uniform->Handle = Handle;
  Uniform->Type = Type;
  jwin_Assert(jstr::StringLength(Name) < ArrayCount(Uniform->Name));
  jstr::CopyStringsUnchecked(Name, Uniform->Name);
}

u32 ReloadShaderProgram(open_gl* OpenGL, u32 ProgramHandle, u32 VertexCodeCount, char** VertexShader, u32 FragmentCodeCount, char** FragmentShader)
{
  gl_shader_program* Program = GetProgram(OpenGL, ProgramHandle);
  jwin_Assert(Program->Handle == ProgramHandle);
  Program->State = GlProgramState::NEW;
  Program->VertexCodeCount = VertexCodeCount;
  Program->VertexCode = VertexShader;
  Program->FragmentCodeCount = FragmentCodeCount;
  Program->FragmentCode = FragmentShader;
  return ProgramHandle;
}

u32 NewProgram(open_gl* OpenGL, c8* Name, u32 VertexCodeCount, char** VertexShader, u32 FragmentCodeCount, char** FragmentShader)
{
  u32 ProgramHandle = OpenGL->ProgramCount++;
  gl_shader_program* Result = GetProgram(OpenGL, ProgramHandle);
  *Result = {};
  Result->Handle = ProgramHandle;
  Result->State = GlProgramState::NEW;
  Result->VertexCodeCount = VertexCodeCount;
  Result->VertexCode = VertexShader;
  Result->FragmentCodeCount = FragmentCodeCount;
  Result->FragmentCode = FragmentShader;
  jwin_Assert(jstr::StringLength(Name) < ArrayCount(Result->Name));
  jstr::CopyStringsUnchecked(Name, Result->Name);
  return ProgramHandle;
}

u32 CreatePhongProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = NewProgram(OpenGL, "PhongShading",
      1, LoadShader("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadShader("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"));
  DeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "NormalView", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "LightDirection", GlUniformType::V3);
  DeclareUniform(OpenGL, ProgramHandle, "LightColor", GlUniformType::V3);
  DeclareUniform(OpenGL, ProgramHandle, "MaterialAmbient", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "MaterialDiffuse", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "MaterialSpecular", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "Shininess", GlUniformType::R32);
  return ProgramHandle;
}

u32 CreatePhongNoTexProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = NewProgram(OpenGL, "PhongShadingNoTex",
      1, LoadShader("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadShader("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));

  DeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "NormalView", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "LightDirection", GlUniformType::V3);
  DeclareUniform(OpenGL, ProgramHandle, "LightColor", GlUniformType::V3);
  DeclareUniform(OpenGL, ProgramHandle, "MaterialAmbient", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "MaterialDiffuse", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "MaterialSpecular", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "Shininess", GlUniformType::R32);
  return ProgramHandle;
}

u32 CreatePlaneStarProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = NewProgram(OpenGL, "PlaneStar",
      1, LoadShader("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadShader("..\\jwin\\shaders\\StarPlaneFragment.glsl"));
  DeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "ModelMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "ViewMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "Color", GlUniformType::V4);
  DeclareUniform(OpenGL, ProgramHandle, "Radius", GlUniformType::R32);
  DeclareUniform(OpenGL, ProgramHandle, "FadeDist", GlUniformType::R32);
  DeclareUniform(OpenGL, ProgramHandle, "Center", GlUniformType::V3);
  return ProgramHandle;
}

u32 CreateSphereStarProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = NewProgram(OpenGL, "SphereStar",
     1, LoadShader("..\\jwin\\shaders\\StarSphereVertex.glsl"),
     1, LoadShader("..\\jwin\\shaders\\StarSphereFragment.glsl"));
  DeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "Time", GlUniformType::R32);
  return ProgramHandle;
}

u32 CreateSolidSphereProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = NewProgram(OpenGL, "SolidSphere",
     1, LoadShader("..\\jwin\\shaders\\SolidSphereVertex.glsl"),
     1, LoadShader("..\\jwin\\shaders\\SolidSphereFragment.glsl"));
  DeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  DeclareUniform(OpenGL, ProgramHandle, "Color", GlUniformType::V4);
  return ProgramHandle;
}


opengl_bitmap MapObjBitmapToOpenGLBitmap(memory_arena* Arena, obj_bitmap* texture)
{
  opengl_bitmap Result = {};
  Result.BPP = texture->BPP;
  Result.Width = texture->Width;
  Result.Height = texture->Height;
  u32 ByteSize = (texture->BPP/8) * texture->Width * texture->Height;
  Result.Pixels = PushCopy(Arena, ByteSize, texture->Pixels);

  return Result;
}

u32 GetUniformHandle(open_gl* OpenGL, u32 ProgramHandle, c8* Name)
{
  gl_shader_program* Program = GetProgram(OpenGL, ProgramHandle);
  for (int i = 0; i < Program->UniformCount; ++i)
  {
    gl_uniform* Uniform = Program->Uniforms + i;
    if(jstr::Compare(Name,Uniform->Name) == 0)
    {
      return Uniform->Handle;
    }
  }
  // No Uniform found
  INVALID_CODE_PATH;
  return 0;
}

u32 PushMeshToOpenGl(open_gl* OpenGL, opengl_buffer_data BufferData)
{
  u32 BufferIndex = OpenGL->BufferKeeperCount++;
  jwin_Assert(BufferIndex < ArrayCount(OpenGL->BufferKeepers));
  buffer_keeper* NewKeeper = OpenGL->BufferKeepers + BufferIndex;
  NewKeeper->Handle = BufferIndex;
  NewKeeper->BufferData = BufferData;
  return BufferIndex;
}

u32 PushTextureToOpenGl(open_gl* OpenGL, opengl_bitmap TextureData)
{
  u32 TextureIndex = OpenGL->TextureKeeperCount++;
  jwin_Assert(TextureIndex < ArrayCount(OpenGL->TextureKeepers));
  texture_keeper* NewKeeper = OpenGL->TextureKeepers + TextureIndex;
  NewKeeper->Handle = TextureIndex;
  NewKeeper->TextureData = TextureData;
  return TextureIndex;
}

void PushVectorToGpu(application_render_commands* RenderCommands, v3 From, v3 Direction, v3 LightDirection, v3 Color, m4 P, m4 V, r32 scale)
{
  v4 Amb =  V4(Color.X * 0.05, Color.Y * 0.05, Color.Z * 0.05, 1.0);
  v4 Diff = V4(Color.X * 0.25, Color.Y * 0.25, Color.Z * 0.25, 1.0);
  v4 Spec = V4(1, 1, 1, 1.0);

  v4 RotQuat1 = GetRotation(V3(0,1,0), Normalize(Direction));
  m4 ModelMatVec = M4Identity();
  Scale(V4(0.1,0.5,0.1,0),ModelMatVec);
  Scale(V4(scale,scale,scale,0),ModelMatVec);
  Translate(V4(0,scale*0.5,0,0),ModelMatVec);

  ModelMatVec = GetRotationMatrix(RotQuat1) * ModelMatVec;
  Translate(V4(From),ModelMatVec);

  m4 ModelViewVec = V*ModelMatVec;
  m4 NormalViewVec = V*Transpose(RigidInverse(ModelMatVec));

  render_object* Vec = PushNewRenderObject(RenderCommands->RenderGroup);
  Vec->ProgramHandle = GlobalState->PhongProgramNoTex;
  Vec->MeshHandle = GlobalState->Cylinder;
  Vec->TextureHandle = U32Max;

  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "Shininess"), (r32) 20);

  m4 ModelMatVecTop = M4Identity();
  Scale(V4(0.2,0.2,0.2,0),ModelMatVecTop);
  Scale(V4(scale,scale,scale,0),ModelMatVecTop);
  Translate(V4(0,scale*1,0,0),ModelMatVecTop);


  v4 RotQuat2 = GetRotation(V3(0,1,0), Normalize(Direction));
  ModelMatVecTop = GetRotationMatrix(RotQuat2) * ModelMatVecTop;
  Translate(V4(From), ModelMatVecTop);

  m4 ModelViewVecTop = V*ModelMatVecTop;
  m4 NormalViewVecTop = V*Transpose(RigidInverse(ModelMatVecTop));
  render_object* VecTop = PushNewRenderObject(RenderCommands->RenderGroup);
  VecTop->ProgramHandle = GlobalState->PhongProgramNoTex;
  VecTop->MeshHandle = GlobalState->Cone;
  VecTop->TextureHandle = U32Max;

  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ModelView"), ModelViewVecTop);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "NormalView"), NormalViewVecTop);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(VecTop, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "Shininess"), (r32) 20);
}

void CastRay(application_render_commands* RenderCommands, camera* Camera, r32 Angle, r32 Width, r32 Length)
{
  v3 Forward, Up, Right;
  GetCameraDirections(Camera, &Up, &Right, &Forward);
  v3 CamPos = GetCameraPosition(Camera);
  m4 StaticAngle = GetRotationMatrix(Angle, V4(0,0,1,0));
  m4 BillboardRotation = CoordinateSystemTransform(CamPos,-CrossProduct(Right, Forward));
  m4 ModelMat = M4Identity();

  r32 Top = 1.161060;
  r32 Bot = 0.580535;
  r32 Middle = (Top + Bot) / 2.f;
  Translate(V4(0,Bot-Middle-Sqrt(3)/2.f,0,0), ModelMat);
  ModelMat = GetScaleMatrix(V4(Width,Length,1,1)) * ModelMat;
  ModelMat = StaticAngle*ModelMat;
  ModelMat = BillboardRotation*ModelMat;

  v4 A = BillboardRotation * V4(1,0,0,0);
  v4 B = BillboardRotation * V4(0,1,0,0);
  v4 C = BillboardRotation * V4(0,0,1,0);

  m4 ModelView = Camera->V*ModelMat;

  render_object* Ray = PushNewRenderObject(RenderCommands->RenderGroup);
  Ray->ProgramHandle = GlobalState->PlaneStarProgram;
  Ray->MeshHandle = GlobalState->Triangle;

  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);
  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ModelMat"), ModelMat);
  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "Color"), V4(254.0/255.0, 254.0/255.0, 255/255, 0.3));
  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "Radius"), 3.f);
  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "FadeDist"), 2.f);
  PushUniform(Ray, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "Center"), V3(0,0,0));
}

// void ApplicationUpdateAndRender(application_memory* Memory, application_render_commands* RenderCommands, jwin::device_input* Input)
extern "C" JWIN_UPDATE_AND_RENDER(ApplicationUpdateAndRender)
{
  GlobalState = JwinBeginFrameMemory(application_state);

  u32 CharCount = 0x100;
  int Padding = 3;
  char FontPath[] = "C:\\Windows\\Fonts\\consola.ttf";
  int OnedgeValue = 128;
  platform_offscreen_buffer* OffscreenBuffer = &RenderCommands->PlatformOffscreenBuffer;
  local_persist v3 LightPosition = V3(1,1,0);
  open_gl* OpenGL = &RenderCommands->OpenGL;
  if(!GlobalState->Initialized)
  {
    obj_loaded_file* plane = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\checker_plane_simple.obj");
    obj_loaded_file* billboard = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\plane.obj");
    obj_loaded_file* sphere = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\sphere.obj");
    obj_loaded_file* triangle = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\triangle.obj");
    obj_loaded_file* cone = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cone.obj");
    obj_loaded_file* cube = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\qube.obj");
    obj_loaded_file* cylinder = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cylinder.obj");
    obj_bitmap* texture = LoadTGA(GlobalPersistentArena, "..\\data\\textures\\brick_wall_base.tga");

    // This memory only needs to exist until the data is loaded to the GPU
    GlobalState->PlaneStarProgram = CreatePlaneStarProgram(OpenGL);
    GlobalState->PhongProgram = CreatePhongProgram(OpenGL);
    GlobalState->SphereStarProgram = CreateSphereStarProgram(OpenGL);
    GlobalState->SolidSphereProgram = CreateSolidSphereProgram(OpenGL);
    GlobalState->PhongProgramNoTex = CreatePhongNoTexProgram(OpenGL);
    GlobalState->Cube = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cube));
    GlobalState->Plane = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, plane));
    GlobalState->Sphere = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, sphere));
    GlobalState->Cone = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cone));
    GlobalState->Cylinder = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cylinder));
    GlobalState->Triangle = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, triangle));
    GlobalState->Billboard = PushMeshToOpenGl(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena,  billboard));
    GlobalState->PlaneTexture = PushTextureToOpenGl(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, plane->MaterialData->Materials[0].MapKd));
    GlobalState->SphereTexture = PushTextureToOpenGl(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, texture));
    GlobalState->OnedgeValue = 128;  // "Brightness" of the sdf. Higher value makes the SDF bigger and brighter.
                                     // Has no impact on TextPixelSize since the char then is also bigger.
    GlobalState->TextPixelSize = 64; // Size of the SDF
    GlobalState->PixelDistanceScale = 32.0; // Smoothness of how fast the pixel-value goes to zero. Higher PixelDistanceScale, makes it go faster to 0;
                                            // Lower PixelDistanceScale and Higher OnedgeValue gives a 'sharper' sdf.
    GlobalState->FontRelativeScale = 1.f;
    GlobalState->Font = jfont::LoadSDFFont(PushArray(GlobalPersistentArena, CharCount, jfont::sdf_fontchar),
      CharCount, FontPath, GlobalState->TextPixelSize, Padding, GlobalState->OnedgeValue, GlobalState->PixelDistanceScale);
    GlobalState->Initialized = true;

    GlobalState->Camera = {};
    r32 AspectRatio = RenderCommands->ScreenWidthPixels / (r32)RenderCommands->ScreenHeightPixels;
    InitiateCamera(&GlobalState->Camera, 70, AspectRatio, 0.001);
    LookAt(&GlobalState->Camera, V3(0,0,10), V3(0,0,0));
    RenderCommands->RenderGroup = InitiateRenderGroup();

    

  }
  r32 AspectRatio = RenderCommands->ScreenWidthPixels / (r32) RenderCommands->ScreenHeightPixels;
  ResetRenderGroup(RenderCommands->RenderGroup);



  camera* Camera = &GlobalState->Camera;
  local_persist r32 near = 0.001;
  if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
  {
    if(Pushed(Input->Keyboard.Key_UP))
    {
      m4 V = Camera->V;
      r32 AngleOfView = Camera->AngleOfView;
      r32 AspectRatio = Camera->AspectRatio;

      InitiateCamera(Camera, AngleOfView+1, AspectRatio, near);
      Camera->V = V;
      Platform.DEBUGPrint("AngleOfView: %f\n", AspectRatio*Camera->AngleOfView);
    }else if(Pushed(Input->Keyboard.Key_DOWN))
    {
      m4 V = Camera->V;
      r32 AngleOfView = Camera->AngleOfView;
      r32 AspectRatio = Camera->AspectRatio;

      InitiateCamera(Camera, AngleOfView-1, AspectRatio, near);
      Camera->V = V;

      Platform.DEBUGPrint("AngleOfView: %f\n", AspectRatio*Camera->AngleOfView);
    }
  }else{
    if(Pushed(Input->Keyboard.Key_UP))
    {
      m4 V = Camera->V;
      r32 AngleOfView = Camera->AngleOfView;
      r32 AspectRatio = Camera->AspectRatio;
      near = near*1.1;
      InitiateCamera(Camera, AngleOfView, AspectRatio, near);
      Camera->V = V;
      Platform.DEBUGPrint("Near: %f\n", near);
    }else if(Pushed(Input->Keyboard.Key_DOWN))
    {
      m4 V = Camera->V;
      r32 AngleOfView = Camera->AngleOfView;
      r32 AspectRatio = Camera->AspectRatio;
      near = near * 0.9;
      InitiateCamera(Camera, AngleOfView, AspectRatio, near);
      Camera->V = V;

      Platform.DEBUGPrint("Near: %f\n", near);
    }
  }

  r32 Len = 4;
  v3 Pos = V3(Len,0,0);
  if(!(jwin::Active(Input->Keyboard.Key_LALT) || jwin::Active(Input->Keyboard.Key_RALT)))
  {
    if(jwin::Pushed(Input->Keyboard.Key_X))
    {
      Pos = V3(Len,0,0);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LookAt(Camera, Pos, V3(0,0,0));
      }else{
        Pos = -Pos;
        LookAt(Camera, Pos, V3(0,0,0));
      }
      Platform.DEBUGPrint("CamPos: %f %f %f\n", Pos.X, Pos.Y, Pos.Z);
    }else if(jwin::Pushed(Input->Keyboard.Key_Y)){
      Pos = V3(0,Len,0);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LookAt(Camera, Pos, V3(0,0,0), V3(1,0,0));
      }else{
        Pos = -Pos;
        LookAt(Camera, Pos, V3(0,0,0),V3(1,0,0));
      }
      Platform.DEBUGPrint("CamPos: %f %f %f\n", Pos.X, Pos.Y, Pos.Z);
    }else if(jwin::Pushed(Input->Keyboard.Key_Z)){
      Pos = V3(0,0,Len);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LookAt(Camera, Pos, V3(0,0,0));
      }else{
        Pos = -Pos;
        LookAt(Camera, Pos, V3(0,0,0));
      }  
      Platform.DEBUGPrint("CamPos: %f %f %f\n", Pos.X, Pos.Y, Pos.Z);
    }else if(jwin::Pushed(Input->Keyboard.Key_Q)){
      Pos = V3(0,Len,Len);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LookAt(Camera, Pos, V3(0,0,0));
      }else{
        Pos = -Pos;
        LookAt(Camera, Pos, V3(0,0,0));
      }
      Platform.DEBUGPrint("CamPos: %f %f %f\n", Pos.X, Pos.Y, Pos.Z);
    }
  }else{
    if(jwin::Pushed(Input->Keyboard.Key_X))
    {
      Pos = V3(Len,0,0);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LightPosition = Pos;
      }else{
        LightPosition = -Pos;
      }
      Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
    }else if(jwin::Pushed(Input->Keyboard.Key_Y)){
      Pos = V3(0,Len,0);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LightPosition = Pos;
      }else{
        LightPosition = -Pos;
      }
      Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
    }else if(jwin::Pushed(Input->Keyboard.Key_Z)){
      Pos = V3(0,0,Len);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LightPosition = Pos;
      }else{
        LightPosition = -Pos;
      }
      Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
    }else if(jwin::Pushed(Input->Keyboard.Key_Q)){
      Pos = V3(0,Len,Len);
      if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        LightPosition = Pos;
      }else{
        LightPosition = -Pos;
      }
      Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
    }
  }
  
  r32 CamSpeed = 0.05;
  if(jwin::Active(Input->Keyboard.Key_W))
  {
    TranslateCamera(Camera, V3(0,0,-CamSpeed));
  }
  if(jwin::Active(Input->Keyboard.Key_S))
  {
    TranslateCamera(Camera, V3(0,0,CamSpeed));
  }
  if(jwin::Active(Input->Keyboard.Key_A))
  {
    TranslateCamera(Camera, V3(-CamSpeed,0,0));
  }
  if(jwin::Active(Input->Keyboard.Key_D))
  {
    TranslateCamera(Camera, V3(CamSpeed,0,0));
  }
  if(jwin::Active(Input->Keyboard.Key_R))
  {
    TranslateCamera(Camera, V3(0,CamSpeed,0));
  }
  if(jwin::Active(Input->Keyboard.Key_F))
  {
    TranslateCamera(Camera, V3(0,-CamSpeed,0));
  }
  if(!jwin::Active(Input->Mouse.Button[jwin::MouseButton_Middle]))
  {
    if(Input->Mouse.dX != 0)
    {
      RotateCameraAroundWorldAxis(Camera, -2*Input->Mouse.dX, V3(0,1,0) );
    }
    if(Input->Mouse.dY != 0)
    {
      RotateCamera(Camera, 2*Input->Mouse.dY, V3(1,0,0) );
    }
  }else{
    if(Input->Mouse.dX != 0)
    {
      v3 Up, Right, Forward;
      GetCameraDirections(Camera, &Up, &Right, &Forward);
      RotateAround(Camera, -5*Input->Mouse.dX, Up);
      char Buf[32] = {};
      jstr::ToString( Right.E, 2, ArrayCount(Buf), Buf );
      Platform.DEBUGPrint("Right   : %s\n", Buf);
    }
    if(Input->Mouse.dY != 0)
    {
      v3 Up, Right, Forward;
      GetCameraDirections(Camera, &Up, &Right, &Forward);
      RotateAround(Camera, -5*Input->Mouse.dY, -Right);
      char Buf[32] = {};
      jstr::ToString( Up.E, 2, ArrayCount(Buf), Buf );
      Platform.DEBUGPrint("Up: %s\n", Buf);
    }
  }

  if(jwin::Pushed(Input->Keyboard.Key_ENTER) || Input->ExecutableReloaded)
  {
    GlobalState->PhongProgram = ReloadShaderProgram(OpenGL, GlobalState->PhongProgram,
      1, LoadShader("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadShader("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"));
    GlobalState->PhongProgramNoTex = ReloadShaderProgram(OpenGL, GlobalState->PhongProgramNoTex,
      1, LoadShader("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadShader("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));
    GlobalState->PlaneStarProgram = ReloadShaderProgram(OpenGL, GlobalState->PlaneStarProgram,
      1, LoadShader("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadShader("..\\jwin\\shaders\\StarPlaneFragment.glsl"));
    GlobalState->SphereStarProgram = ReloadShaderProgram(OpenGL, GlobalState->SphereStarProgram,
      1, LoadShader("..\\jwin\\shaders\\StarSphereVertex.glsl"),
      1, LoadShader("..\\jwin\\shaders\\StarSphereFragment.glsl"));
    GlobalState->SolidSphereProgram = ReloadShaderProgram(OpenGL, GlobalState->SolidSphereProgram,
     1, LoadShader("..\\jwin\\shaders\\SolidSphereVertex.glsl"),
     1, LoadShader("..\\jwin\\shaders\\SolidSphereFragment.glsl"));
  }

  UpdateViewMatrix(Camera);


  // Model data
  local_persist b32 ModelLoaded = false;
  local_persist m4 Plane = M4Identity();
  local_persist m4 ModelMatSphere = M4Identity();
  if(!ModelLoaded)
  {
    // Plane
    Scale( V4(10,0.1,10,0), Plane );
    Translate( V4(0,-1,0,0), Plane);
    Translate( V4(-10,0,0,0), ModelMatSphere);
    ModelLoaded = true;
  }

  v3 LightDirection = V3(Transpose(RigidInverse(Camera->V)) * V4(LightPosition,0));

  // Floor
  m4 ModelViewPlane = Camera->V*Plane;
  m4 NormalViewPlane = Camera->V*Transpose(RigidInverse(Plane));

  render_object* Floor = PushNewRenderObject(RenderCommands->RenderGroup);
  Floor->ProgramHandle = GlobalState->PhongProgram;
  Floor->MeshHandle = GlobalState->Plane;
  Floor->TextureHandle = GlobalState->PlaneTexture;

  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "ProjectionMat"), Camera->P);
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "ModelView"), ModelViewPlane);
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "NormalView"), NormalViewPlane);
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightDirection"), LightDirection);
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightColor"), V3(1,1,1));
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialAmbient"), V4(0.4,0.4,0.4,1));
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialDiffuse"), V4(0.5,0.5,0.5,1));
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialSpecular"), V4(0.75,0.75,0.75,1));
  PushUniform(Floor, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "Shininess"), (r32) 20);

  // Textured Sphere
  Rotate( 1/120.f, -V4(0,1,0,0), ModelMatSphere );

  m4 ModelViewSphere = Camera->V*ModelMatSphere;
  m4 NormalViewSphere = Camera->V*Transpose(RigidInverse(ModelMatSphere));

  render_object* Sphere = PushNewRenderObject(RenderCommands->RenderGroup);
  Sphere->ProgramHandle = GlobalState->PhongProgram;
  Sphere->MeshHandle = GlobalState->Sphere;
  Sphere->TextureHandle = GlobalState->SphereTexture;

  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "ProjectionMat"), Camera->P);
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "ModelView"), ModelViewSphere);
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "NormalView"), NormalViewSphere);
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightDirection"), LightDirection);
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightColor"), V3(1,1,1));
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialAmbient"), V4(0.2,0.2,0.2,1));
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialDiffuse"), V4(0.5,0.5,0.5,1));
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialSpecular"), V4(0.75,0.75,0.75,1));
  PushUniform(Sphere, GetUniformHandle(OpenGL, GlobalState->PhongProgram, "Shininess"), (r32) 20);


  local_persist r32 Time = 0;
  local_persist r32 Angle2 = 0;

  // Ray
  r32 RayCount = 15;
  for (r32 i = 0; i < RayCount; ++i)
  {
    r32 RayAngle = 0.06*Input->Time + i*Tau32/RayCount;
    CastRay(RenderCommands, Camera, RayAngle , 0.3, 4);
  }
  RayCount = 7;
  for (r32 i = 0; i < RayCount; ++i)
  {
    r32 RayAngle = -0.04*Input->Time + i*Tau32/RayCount + Pi32/3.f + 0.03*Sin(Input->Time);
    CastRay(RenderCommands, Camera, RayAngle, 1, 4);
  }
  
  #if 1

  v3 Forward, Up, Right;
  GetCameraDirections(Camera, &Up, &Right, &Forward);
  v3 CamPos = GetCameraPosition(Camera);
  m4 BillboardRotation = CoordinateSystemTransform(CamPos,-CrossProduct(Right, Forward));
  m4 HaloModelMat = M4Identity();
  HaloModelMat = GetRotationMatrix(Pi32/2.f, V4(1,0,0,0))* HaloModelMat;
  HaloModelMat = GetScaleMatrix(V4(2,2,2,1)) * HaloModelMat;
  HaloModelMat = BillboardRotation*HaloModelMat;

  render_object* Halo = PushNewRenderObject(RenderCommands->RenderGroup);
  Halo->ProgramHandle = GlobalState->PlaneStarProgram;
  Halo->MeshHandle = GlobalState->Plane;

  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);
  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ModelMat"), HaloModelMat);
  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "Color"), V4(254.0/255.0, 254.0/255.0, 255/255, 0.3));
  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "Radius"), (r32)( 1.3f + 0.07 * Sin(Input->Time)));
  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "FadeDist"), 0.3f);
  PushUniform(Halo, GetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "Center"), V3(0,0,0));
#endif

#if 1
  render_object* FinalSphere = PushNewRenderObject(RenderCommands->RenderGroup);
  FinalSphere->ProgramHandle = GlobalState->SolidSphereProgram;
  FinalSphere->MeshHandle = GlobalState->Sphere;
  r32 FinalSize = 1;
  r32 FinalSizeOscillation = FinalSize * ( 1 + 0.01* Sin(0.05*Input->Time));
  m4 FMSS = GetScaleMatrix(V4(FinalSizeOscillation,FinalSizeOscillation,FinalSizeOscillation,1));

  PushUniform(FinalSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
  PushUniform(FinalSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*FMSS);
  PushUniform(FinalSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(45.0/255.0, 51.0/255, 197.0/255.0, 1));

  render_object* LargeSphere = PushNewRenderObject(RenderCommands->RenderGroup);
  LargeSphere->ProgramHandle = GlobalState->SolidSphereProgram;
  LargeSphere->MeshHandle = GlobalState->Sphere;
  r32 LargeSize = 0.95;
  r32 LargeSizeOscillation = LargeSize * ( 1 + 0.02* Sin(0.1 * Input->Time+ 1.1));
  m4 MSS = GetScaleMatrix(V4(LargeSizeOscillation,LargeSizeOscillation,LargeSizeOscillation,1));

  PushUniform(LargeSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
  PushUniform(LargeSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*MSS);
  PushUniform(LargeSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(56.0/255.0, 75.0/255, 220.0/255.0, 1));


  render_object* MediumSphere = PushNewRenderObject(RenderCommands->RenderGroup);
  MediumSphere->ProgramHandle = GlobalState->SolidSphereProgram;
  MediumSphere->MeshHandle = GlobalState->Sphere;
  r32 MediumSize = 0.85;
  r32 MediumScaleOccilation = MediumSize * ( 1 + 0.02* Sin(Input->Time+Pi32/4.f));
  MSS = GetScaleMatrix(V4(MediumScaleOccilation,MediumScaleOccilation,MediumScaleOccilation,1));

  PushUniform(MediumSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
  PushUniform(MediumSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*MSS);
  PushUniform(MediumSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(57.0/255.0, 110.0/255, 247.0/255.0, 1));


  render_object* SmallSphere = PushNewRenderObject(RenderCommands->RenderGroup);
  SmallSphere->ProgramHandle = GlobalState->SolidSphereProgram;
  SmallSphere->MeshHandle = GlobalState->Sphere;
  r32 SmallSize = 0.65;
  r32 SmallScaleOccilation = SmallSize * ( 1 + 0.03* Sin(Input->Time+3/4.f *Pi32));
  MSS = GetScaleMatrix(V4(SmallScaleOccilation,SmallScaleOccilation,SmallScaleOccilation,1));

  PushUniform(SmallSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
  PushUniform(SmallSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*MSS);
  PushUniform(SmallSphere, GetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(107.0/255.0, 196.0/255, 1, 1));


  // PlaneStar
  render_object* SphereStar = PushNewRenderObject(RenderCommands->RenderGroup);
  SphereStar->ProgramHandle = GlobalState->SphereStarProgram;
  SphereStar->MeshHandle = GlobalState->Sphere;


  PushUniform(SphereStar, GetUniformHandle(OpenGL, GlobalState->SphereStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(SphereStar, GetUniformHandle(OpenGL, GlobalState->SphereStarProgram, "ModelView"), Camera->V*FMSS);
  PushUniform(SphereStar, GetUniformHandle(OpenGL, GlobalState->SphereStarProgram, "Time"), Time);
  //PushUniform(SphereStar, GetUniformHandle(OpenGL, GlobalState->SphereStarProgram, "Time"), Time);
  #endif
  Time+=0.03*Input->deltaTime;
  if(Time > 1)
  {
    Time = -1;
  }
  Angle2 += Input->deltaTime;
  if(Angle2>Pi32)
  {
    Angle2 -= Tau32;
  }



  //PushVectorToGpu(RenderCommands, V3(0,0,0), ToProjZX, LightDirection, 0, Camera->P, Camera->V, 0.4);
  //PushVectorToGpu(RenderCommands, V3(0,0,0), To, LightDirection, 2, Camera->P, Camera->V, 0.4);

  //PushVectorToGpu(RenderCommands, V3(0,0,0), V3(A), LightDirection, V3(1,0,0), Camera->P, Camera->V, 0.4);
  //PushVectorToGpu(RenderCommands, V3(0,0,0), V3(B), LightDirection, V3(0,1,0), Camera->P, Camera->V, 0.4);
  //PushVectorToGpu(RenderCommands, V3(0,0,0), V3(C), LightDirection, V3(0,0,1), Camera->P, Camera->V, 0.4);

  
  //ModelViewPlaneStar1 = M4Identity();

    //PushVectorToGpu(RenderCommands, V3(0,0,0), V3(1,0,0), LightDirection, 0, Camera->P, Camera->V, 0.4);
    //PushVectorToGpu(RenderCommands, V3(0,0,0), V3(0,1,0), LightDirection, 1, Camera->P, Camera->V, 0.4);
    //PushVectorToGpu(RenderCommands, V3(0,0,0), V3(0,0,1), LightDirection, 2, Camera->P, Camera->V, 0.4);
}