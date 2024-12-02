#include "stars.h"

#include "platform/jwin_platform_memory.cpp"
#include "platform/jwin_platform_input.h"
#include "platform/jfont.cpp"
#include "platform/obj_loader.cpp"
#include "renderer/software_render_functions.cpp"
#include "renderer/render_push_buffer.cpp"
#include "math/AABB.cpp"
#include "camera.cpp"

#define SPOTCOUNT 200

local_persist render_state NoDepthTestNoCulling = {false,false};
local_persist render_state DepthTestNoCulling = {true,false};
local_persist render_state NoDepthTestCulling = {false,true};
local_persist render_state DepthTestCulling = {true,true};

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

char** LoadShaderFromDisk(char* CodePath)
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
  }else{
    INVALID_CODE_PATH
  }
  return Result;
}


internal gl_shader_program* GlGetProgram(open_gl* OpenGL, u32 Handle)
{
  jwin_Assert(Handle < ArrayCount(OpenGL->Programs));
  gl_shader_program* Program = OpenGL->Programs + Handle;
  return Program;
}

void GlDeclareUniform(open_gl* OpenGL, u32 ProgramHandle, char* Name, GlUniformType Type)
{
  gl_shader_program* Program = GlGetProgram(OpenGL, ProgramHandle);
  u32 Handle = Program->UniformCount++;
  jwin_Assert(Handle < ArrayCount(Program->Uniforms));
  gl_uniform* Uniform = Program->Uniforms + Handle;
  Uniform->Handle = Handle;
  Uniform->Type = Type;
  jwin_Assert(jstr::StringLength(Name) < ArrayCount(Uniform->Name));
  jstr::CopyStringsUnchecked(Name, Uniform->Name);
}


// When declaring Layots:
//    The stride is the size of the sum of all declared layouts.
//    The index in the shader is the same as the order of the declared layouts.
//      Layout 0,1,2 are reserved for Vertex, Vertex normal and texture vertex.
//      So Instance layouts start at 3 and forward.
//    Declaration is done at program creation and cannot be changed.
//    The following instance data is declared as follows
//    struct{
//      v3 Center,
//      v3 Color,
//      r32 Speed,
//      r32 Time
//    };
//
// Order of declaration _must be the same_ as layout order in the shader and as input data.
//
// GlDeclareInstanceLayout(OpenGL, ProgramHandle, "Center", GlUniformType::V3) // Gets index: 3, Size: v3,  Offset in struct: 0,         Stride: v3 + v3 + r32 + r32
// GlDeclareInstanceLayout(OpenGL, ProgramHandle, "Color", GlUniformType::V3)  // Gets index: 4, Size: v3,  Offset in struct: v3,        Stride: v3 + v3 + r32 + r32
// GlDeclareInstanceLayout(OpenGL, ProgramHandle, "Speed", GlUniformType::R32) // Gets index: 5, Size: r32, Offset in struct: v3+v3,     Stride: v3 + v3 + r32 + r32
// GlDeclareInstanceLayout(OpenGL, ProgramHandle, "Time", GlUniformType::R32)  // Gets index: 6, Size: r32, Offset in struct: v3+v3+r32, Stride: v3 + v3 + r32 + r32

void GlDeclareInstanceVarying(open_gl* OpenGL, u32 ProgramHandle, GlUniformType Type, c8* DebugName = 0)
{
  gl_shader_program* Program = GlGetProgram(OpenGL, ProgramHandle);
  u32 VaryingIndex = Program->InstanceVaryingCount++;
  jwin_Assert(VaryingIndex < ArrayCount(Program->InstanceVaryings)); // Todo: Query at OpenGl startup the max number of layouts supported on GPU
  gl_instance_varying* Varying = Program->InstanceVaryings + VaryingIndex;
  Varying->AttribArrayIndex = VaryingIndex + 3;
  Varying->Type = Type;
  if(DebugName)
  {
    jwin_Assert(jstr::StringLength(DebugName) < ArrayCount(Varying->DebugName));
    jstr::CopyStringsUnchecked(DebugName, Varying->DebugName);
  }
}

u32 GlReloadProgram(open_gl* OpenGL, u32 ProgramHandle, u32 VertexCodeCount, char** VertexShader, u32 FragmentCodeCount, char** FragmentShader)
{
  gl_shader_program* Program = GlGetProgram(OpenGL, ProgramHandle);
  jwin_Assert(Program->Handle == ProgramHandle);
  Program->State = GlProgramState::RELOAD;
  Program->VertexCodeCount = VertexCodeCount;
  Program->VertexCode = VertexShader;
  Program->FragmentCodeCount = FragmentCodeCount;
  Program->FragmentCode = FragmentShader;
  return ProgramHandle;
}

u32 GlNewProgram(open_gl* OpenGL, u32 VertexCodeCount, char** VertexShader, u32 FragmentCodeCount, char** FragmentShader,  c8* DebugName = 0)
{
  u32 ProgramHandle = OpenGL->ProgramCount++;
  gl_shader_program* Result = GlGetProgram(OpenGL, ProgramHandle);
  *Result = {};
  Result->Handle = ProgramHandle;
  Result->State = GlProgramState::NEW;
  Result->VertexCodeCount = VertexCodeCount;
  Result->VertexCode = VertexShader;
  Result->FragmentCodeCount = FragmentCodeCount;
  Result->FragmentCode = FragmentShader;
  if(DebugName)
  {
    jwin_Assert(jstr::StringLength(DebugName) < ArrayCount(Result->DebugName));
    jstr::CopyStringsUnchecked(DebugName, Result->DebugName);
  }
  return ProgramHandle;
}

u32 CreatePhongProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"),
       "PhongShading");
  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "NormalView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightDirection", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightColor", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialAmbient", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialDiffuse", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialSpecular", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "Shininess", GlUniformType::R32);
  return ProgramHandle;
}

u32 CreatePhongNoTexProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"),
      "PhongShadingNoTex");

  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "NormalView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightDirection", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightColor", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialAmbient", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialDiffuse", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialSpecular", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "Shininess", GlUniformType::R32);
  return ProgramHandle;
}

u32 CreatePhongTransparentProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewTransparent.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewTransparent.glsl"),
      "PhongShadingTransparent");

  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "NormalView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightDirection", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightColor", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialAmbient", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialDiffuse", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialSpecular", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "Shininess", GlUniformType::R32);
  return ProgramHandle;
}

u32 CreatePlaneStarProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneFragment.glsl"),
       "StarPlane");
  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ViewMat", GlUniformType::M4);
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::M4,   "ModelMat");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V4,   "Color");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32,  "Radius");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32,  "FadeDist");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V3,   "Center");
  return ProgramHandle;
}

u32 CreateSolidColorProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidColorVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidColorFragment.glsl"),
     "SolidColor");
  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "Color", GlUniformType::V4);
  return ProgramHandle;
}

struct eurption_band{
  v4 Color;
  v3 Center;
  r32 InnerRadii;
  r32 OuterRadii;
};

u32 CreateEruptionBandProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\EruptionBandVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\EruptionBandFragment.glsl"),
     "EruptionBand");
  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V4,  "Color");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V3,  "Center");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32, "InnerRadii");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32, "OuterRadii");
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

u32 GlGetUniformHandle(open_gl* OpenGL, u32 ProgramHandle, c8* Name)
{
  gl_shader_program* Program = GlGetProgram(OpenGL, ProgramHandle);
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

u32 GlLoadMesh(open_gl* OpenGL, opengl_buffer_data BufferData)
{
  u32 BufferIndex = OpenGL->BufferKeeperCount++;
  jwin_Assert(BufferIndex < ArrayCount(OpenGL->BufferKeepers));
  buffer_keeper* NewKeeper = OpenGL->BufferKeepers + BufferIndex;
  NewKeeper->Handle = BufferIndex;
  NewKeeper->BufferData = BufferData;
  return BufferIndex;
}

u32 GlLoadTexture(open_gl* OpenGL, opengl_bitmap TextureData)
{
  u32 TextureIndex = OpenGL->TextureKeeperCount++;
  jwin_Assert(TextureIndex < ArrayCount(OpenGL->TextureKeepers));
  texture_keeper* NewKeeper = OpenGL->TextureKeepers + TextureIndex;
  NewKeeper->Loaded = false;
  NewKeeper->Handle = TextureIndex;
  NewKeeper->TextureData = TextureData;
  return TextureIndex;
}

void GlUpdateTexture(open_gl* OpenGL, u32 TextureIndex, opengl_bitmap TextureData)
{
  jwin_Assert(TextureIndex < ArrayCount(OpenGL->TextureKeepers));
  texture_keeper* Keeper = OpenGL->TextureKeepers + TextureIndex;
  jwin_Assert(Keeper->Handle == TextureIndex);
  Keeper->Loaded = false;
  Keeper->TextureData = TextureData;
}

void DrawDot(application_render_commands* RenderCommands, v3 Pos, v3 LightDirection, v3 Color, m4 P, m4 V, r32 scale)
{
  v4 Amb =  V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Diff = V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Spec = V4(1, 1, 1, 1.0);

  m4 ModelMat = M4Identity();
  Scale(V4(scale,scale,scale,0),ModelMat);
  Translate(V4(Pos),ModelMat);

  m4 ModelView = V*ModelMat;
  m4 NormalView = V*Transpose(RigidInverse(ModelMat));

  render_object* Sphere = PushNewRenderObject(RenderCommands->RenderGroup);
  Sphere->ProgramHandle = GlobalState->PhongProgramNoTex;
  Sphere->MeshHandle = GlobalState->Sphere;
  Sphere->TextureHandle = U32Max;

  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ModelView"), ModelView);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "NormalView"), NormalView);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(Sphere, DepthTestCulling);
}

void DrawLine(application_render_commands* RenderCommands, v3 LineStart, v3 LineEnd, v3 LightDirection, v3 Color, m4 P, m4 V, r32 scale)
{
  v4 Amb =  V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Diff = V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Spec = V4(1, 1, 1, 1.0);

  v4 RotQuat1 = GetRotation(V3(0,1,0), Normalize(LineEnd-LineStart));
  m4 ModelMatVec = M4Identity();
  r32 Length = Norm(LineEnd - LineStart);
  Scale(V4(0.1,0.5,0.1,0),ModelMatVec);
  Scale(V4(scale,Length,scale,0),ModelMatVec);
  Translate(V4(0,Length*0.5,0,0),ModelMatVec);

  ModelMatVec = GetRotationMatrix(RotQuat1) * ModelMatVec;
  Translate(V4(LineStart),ModelMatVec);

  m4 ModelViewVec = V*ModelMatVec;
  m4 NormalViewVec = V*Transpose(RigidInverse(ModelMatVec));

  render_object* Vec = PushNewRenderObject(RenderCommands->RenderGroup);
  Vec->ProgramHandle = GlobalState->PhongProgramNoTex;
  Vec->MeshHandle = GlobalState->Cylinder;
  Vec->TextureHandle = U32Max;

  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(Vec, DepthTestCulling);

}

void DrawVector(application_render_commands* RenderCommands, v3 From, v3 Direction, v3 LightDirection, v3 Color, m4 P, m4 V, r32 scale)
{
  //v4 Amb =  V4(Color.X * 0.05, Color.Y * 0.05, Color.Z * 0.05, 1.0);
  //v4 Diff = V4(Color.X * 0.25, Color.Y * 0.25, Color.Z * 0.25, 1.0);
  //v4 Spec = V4(1, 1, 1, 1.0);
  v4 Amb =  V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Diff = V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
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

  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(Vec, DepthTestCulling);

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

  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "ModelView"), ModelViewVecTop);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "NormalView"), NormalViewVecTop);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(VecTop, DepthTestCulling);
}

struct ray_cast
{
  m4 ModelMat;
  v4 Color;
  r32 Radius;
  r32 FaceDist;
  v3 Center;
};

ray_cast CastRay(camera* Camera, r32 Angle, r32 Width, r32 Length, v4 Color, v3 Position)
{
  ray_cast Result = {};
  v3 Forward, Up, Right;
  GetCameraDirections(Camera, &Up, &Right, &Forward);
  v3 Direction = GetCameraPosition(Camera) - Position;
  m4 StaticAngle = GetRotationMatrix(Angle, V4(0,0,1,0));
  m4 BillboardRotation = CoordinateSystemTransform(Direction,-CrossProduct(Right, Forward));
  m4 ModelMat = M4Identity();

  r32 Top = 1.161060;
  r32 Bot = 0.580535;
  r32 Middle = (Top + Bot) / 2.f;
  Translate(V4(0,Bot-Middle-Sqrt(3)/2.f,0,0), ModelMat);
  ModelMat = GetScaleMatrix(V4(Width,Length,1,1)) * ModelMat;
  ModelMat = StaticAngle*ModelMat;
  ModelMat = BillboardRotation*ModelMat;
  Translate(V4(Position,1),ModelMat);
  Result.ModelMat = ModelMat;
  Result.Color = Color;
  Result.Radius = 2.f;
  Result.FaceDist = 1.f;
  Result.Center = Position;
  return Result;
}

void CastConeRays(application_render_commands* RenderCommands, jwin::device_input* Input, camera* Camera, v3 PointOnUitSphere, r32 AngleOnSphere, r32 MaxAngleOnSphere, v4 Translation, m4 RotationMatrix)
{
  ray_cast* Ray = PushStruct(GlobalTransientArena, ray_cast);

  m4 StaticAngle = QuaternionAsMatrix(GetRotation(V3(0,-1,0), PointOnUitSphere));

  m4 ModelMat = M4Identity();
  Translate(V4(0,-1,0,0), ModelMat);
  ModelMat = GetScaleMatrix(V4(Sin(AngleOnSphere)*1.01,1,Sin(AngleOnSphere+0.01)*1.01,1)) * ModelMat;
  m4 TransMat = GetTranslationMatrix(Translation);
  ModelMat =  TransMat * RotationMatrix * StaticAngle * ModelMat;
  Ray->ModelMat = ModelMat;
  Ray->Color = V4(1, 1, 1, LinearRemap(MaxAngleOnSphere-AngleOnSphere, 0, MaxAngleOnSphere, 0.5,1));
  Ray->Radius = 2.f;
  Ray->FaceDist = 1.f;
  Ray->Center = V3(Translation);

  render_object* RayObj = PushNewRenderObject(RenderCommands->RenderGroup);
  RayObj->ProgramHandle = GlobalState->PlaneStarProgram;
  RayObj->MeshHandle = GlobalState->Cone;

  PushUniform(RayObj, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(RayObj, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);

  PushInstanceData(RayObj, 1, sizeof(ray_cast), (void*) Ray);
  PushRenderState(RayObj, DepthTestNoCulling);
  RayObj->Transparent = true;
}

void CastRays(application_render_commands* RenderCommands, jwin::device_input* Input, camera* Camera, v3 Position)
{
  r32 ThinRayCount = 15;
  r32 ThickRayCount = 7;
  ray_cast* Rays = PushArray(GlobalTransientArena, ThinRayCount + ThickRayCount, ray_cast);
  for (r32 i = 0; i < ThinRayCount; ++i)
  {
    r32 RayAngle = 0.06f*Input->Time + i*Tau32 / ThinRayCount;
    //Rays[(u32)i] = CastRay(Camera, RayAngle, 0.3, 4, V4(1-i/ThinRayCount, i/ThinRayCount, 0.81, 0.6));
    Rays[(u32)i] = CastRay(Camera, RayAngle, 0.3, 4, V4(1, 1, 1, 0.5),Position);
  }
  for (r32 i = 0; i < ThickRayCount; ++i)
  {
    r32 RayAngle = -0.04*Input->Time + i*Tau32/ThickRayCount + Pi32/3.f + 0.03*Sin(Input->Time);
    //Rays[(u32)(i + ThinRayCount)] = CastRay(Camera, RayAngle, 1, 4, V4(i/ThickRayCount, 1-i/ThickRayCount, 0.81, 0.6));
    Rays[(u32)(i + ThinRayCount)] = CastRay(Camera, RayAngle, 1, 4, V4(1, 1, 1, 0.5),Position);
  }

  render_object* Ray = PushNewRenderObject(RenderCommands->RenderGroup);
  Ray->ProgramHandle = GlobalState->PlaneStarProgram;
  Ray->MeshHandle = GlobalState->Triangle;
  Ray->Transparent = true;

  PushUniform(Ray, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(Ray, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);
  u32 InstanceCount = ThinRayCount + ThickRayCount;
  PushInstanceData(Ray, InstanceCount, InstanceCount * sizeof(ray_cast), (void*) Rays);
  PushRenderState(Ray, DepthTestCulling);
}


v4 LerpColor(r32 t, v4 StartColor, v4 EndColor)
{
  v4 Result = t * (EndColor - StartColor) + StartColor;
  return Result;
}

struct eruption_params {
  r32 EruptionSize;
  v3 PointOnUnitSphere;
  r32 PopTime;
  u32 MaxEruptionBandCount;
  r32 RadiiIncrements[4];
  v4 Colors[4];
  b32 HasRayCone;
  r32 Duration;
  r32 Time;
  u32 RegionIndex;
};

void DrawEruptionBands(application_render_commands* RenderCommands, jwin::device_input* Input, u32 ParamCounts, eruption_params* Params, m4 StarModelMat, v4 Translation, m4 RotationMatrix) {

  u32 MaxBandCount = 0;
  for (int ParamIndex = 0; ParamIndex < ParamCounts; ++ParamIndex)
  {
    MaxBandCount+=Params[ParamIndex].MaxEruptionBandCount;
  }

  eurption_band* EruptionBands = PushArray(GlobalTransientArena, MaxBandCount, eurption_band);
  u32 BandCount = 0;
  for (int ParamIndex = 0; ParamIndex < ParamCounts; ++ParamIndex)
  {
    eruption_params* Param = Params + ParamIndex;
    r32 EruptionSize = Param->PopTime * Param->EruptionSize;
    r32 TimeParameter = Unlerp(Param->Time, 0, Param->Duration);
    r32 OuterBandRadii = Lerp(TimeParameter, 0, EruptionSize);
    r32 InnerBandRadii = BranchlessArithmatic( TimeParameter < Param->PopTime,
      0,
      LinearRemap(TimeParameter, Param->PopTime, 1, 0, EruptionSize));

    u32 ActiveBandCount = 0;
    r32 IncrementSum = 0;
    u32 Index = 0;
    do
    {
      ActiveBandCount++;
      IncrementSum+=Param->RadiiIncrements[Index++] * Param->PopTime;
    }while(ActiveBandCount<Param->MaxEruptionBandCount && TimeParameter > IncrementSum);

    r32 Radii = OuterBandRadii;
    for (int BandIndex = 0; BandIndex < ActiveBandCount; ++BandIndex)
    {
      eurption_band* EruptionBand = EruptionBands + BandIndex + BandCount;
      EruptionBand->Color = Param->Colors[BandIndex];
      EruptionBand->Center = Param->PointOnUnitSphere;
      EruptionBand->OuterRadii = Radii;
      EruptionBand->InnerRadii = BranchlessArithmatic(TimeParameter < Param->PopTime,
        Maximum(0, Radii - Param->RadiiIncrements[BandIndex] * EruptionSize*Param->PopTime),
        Radii - (OuterBandRadii-InnerBandRadii) * Param->RadiiIncrements[BandIndex]);
      Radii = EruptionBand->InnerRadii;
      EruptionBand->Color.W = Minimum(1, LinearRemap(Param->Time, Param->Duration-0.3f, Param->Duration, 1,0));
    }
    BandCount += ActiveBandCount;
    if(Param->HasRayCone && ActiveBandCount == Param->MaxEruptionBandCount && EruptionBands[BandCount-1].InnerRadii > 0)
    {
      CastConeRays(RenderCommands, Input, &GlobalState->Camera, Param->PointOnUnitSphere,
        2*EruptionBands[BandCount-1].InnerRadii, EruptionSize, Translation, RotationMatrix);
    }
  }

  render_object* Eruptions = PushNewRenderObject(RenderCommands->RenderGroup);
  Eruptions->ProgramHandle = GlobalState->EruptionBandProgram;
  Eruptions->MeshHandle = GlobalState->Sphere;
  PushUniform(Eruptions, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->EruptionBandProgram, "ProjectionMat"), GlobalState->Camera.P);
  PushUniform(Eruptions, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->EruptionBandProgram, "ModelView"), GlobalState->Camera.V*StarModelMat);
  PushInstanceData(Eruptions, BandCount, BandCount * sizeof(eurption_band), (void*) EruptionBands);
  PushRenderState(Eruptions, NoDepthTestCulling);
}


void InitializeEruption(eruption_params* Param, random_generator* Generator, u32 BandCount, v4* Colors, r32 MinEruptionSize, r32 MaxEruptionSize, r32 MaxRayProbability, r32 Theta, r32 Phi)
{
  Param->EruptionSize             = GetRandomReal(Generator, MinEruptionSize, MaxEruptionSize);
  Param->Duration                 = LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 10, 30);
  Param->Time                     = 0;
  Param->PointOnUnitSphere        = V3(Cos(Theta)*Sin(Phi), Sin(Theta)*Sin(Phi), Cos(Phi));
  Param->PopTime                  = LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 0.7, 0.5);
  Param->MaxEruptionBandCount     = BandCount;
  Param->HasRayCone               = GetRandomRealNorm(Generator) < LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 0,MaxRayProbability);

  Param->RadiiIncrements[0] = GetRandomRealNorm(Generator);
  r32 TotSum = Param->RadiiIncrements[0];
  for (int i = 1; i < Param->MaxEruptionBandCount-1; ++i)
  {
    Param->RadiiIncrements[i] = Param->RadiiIncrements[i-1] + GetRandomRealNorm(Generator);
    TotSum += Param->RadiiIncrements[i];
  }
  for (int i = 0; i < Param->MaxEruptionBandCount-1; ++i)
  {
    Param->RadiiIncrements[i] /= TotSum;
  }
  Param->RadiiIncrements[Param->MaxEruptionBandCount-1] = LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 1, 3);
  for (int i = 0; i < Param->MaxEruptionBandCount; ++i)
  {
    Param->Colors[i] = Colors[i];
  }
  
}


void RenderStar(application_state* GameState, application_render_commands* RenderCommands, jwin::device_input* Input, v3 Position)
{
  r32 StarSize = 1;
  camera* Camera = &GameState->Camera;
  open_gl* OpenGL = &RenderCommands->OpenGL;
  m4 Sphere1ModelMat = {};
  m4 Sphere1RotationMatrix = GetRotationMatrix(Input->Time/20.f, V4(0,1,0,0));
  {
    render_object* Sphere1 = PushNewRenderObject(RenderCommands->RenderGroup);
    Sphere1->ProgramHandle = GlobalState->SolidColorProgram;
    Sphere1->MeshHandle = GlobalState->Sphere;
    r32 FinalSizeOscillation = StarSize * ( 1 + 0.01* Sin(0.05*Input->Time));
    Sphere1ModelMat = GetTranslationMatrix(V4(Position, 1))*  Sphere1RotationMatrix * GetScaleMatrix(V4(FinalSizeOscillation,FinalSizeOscillation,FinalSizeOscillation,1));

    PushUniform(Sphere1, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere1, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere1ModelMat);
    PushUniform(Sphere1, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "Color"), V4(45.0/255.0, 51.0/255, 197.0/255.0, 1));
    PushRenderState(Sphere1, DepthTestCulling);
  }

  // Second Largest Sphere
  {
    render_object* Sphere2 = PushNewRenderObject(RenderCommands->RenderGroup);
    Sphere2->ProgramHandle = GameState->SolidColorProgram;
    Sphere2->MeshHandle = GameState->Sphere;
    r32 LargeSize = 0.95 * StarSize;
    r32 LargeSizeOscillation = LargeSize * ( 1 + 0.02* Sin(0.1 * Input->Time+ 1.1));
    m4 Sphere2ModelMat = GetTranslationMatrix(V4(Position, 1)) * GetScaleMatrix(V4(LargeSizeOscillation,LargeSizeOscillation,LargeSizeOscillation,1));

    PushUniform(Sphere2, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere2, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere2ModelMat);
    PushUniform(Sphere2, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "Color"), V4(56.0/255.0, 75.0/255, 220.0/255.0, 1));
    PushRenderState(Sphere2, DepthTestCulling);
  }

  {
    render_object* Sphere3 = PushNewRenderObject(RenderCommands->RenderGroup);
    Sphere3->ProgramHandle = GameState->SolidColorProgram;
    Sphere3->MeshHandle = GameState->Sphere;
    r32 MediumSize = 0.85 * StarSize;
    r32 MediumScaleOccilation = MediumSize * ( 1 + 0.02* Sin(Input->Time+Pi32/4.f));
    m4 Sphere3ModelMat = GetTranslationMatrix(V4(Position, 1)) * GetScaleMatrix(V4(MediumScaleOccilation,MediumScaleOccilation,MediumScaleOccilation,1));

    PushUniform(Sphere3, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere3, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere3ModelMat);
    PushUniform(Sphere3, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "Color"), V4(57.0/255.0, 110.0/255, 247.0/255.0, 1));
    PushRenderState(Sphere3, NoDepthTestCulling);
  }

  // Smallest Sphere
  {
    render_object* Sphere4 = PushNewRenderObject(RenderCommands->RenderGroup);
    Sphere4->ProgramHandle = GameState->SolidColorProgram;
    Sphere4->MeshHandle = GameState->Sphere;
    r32 SmallSize = 0.65;
    r32 SmallScaleOccilation = SmallSize * ( 1 + 0.03* Sin(Input->Time+3/4.f *Pi32));
    m4 Sphere4ModelMat = GetTranslationMatrix(V4(Position, 1)) * GetScaleMatrix(V4(SmallScaleOccilation,SmallScaleOccilation,SmallScaleOccilation,1));

    PushUniform(Sphere4, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere4, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere4ModelMat);
    PushUniform(Sphere4, GlGetUniformHandle(OpenGL, GameState->SolidColorProgram, "Color"), V4(107.0/255.0, 196.0/255, 1, 1));
    PushRenderState(Sphere4, NoDepthTestCulling);
  }


  local_persist b32 RandomInit = false;
  local_persist eruption_params Params[SPOTCOUNT] = {};
  local_persist v4 Colors1[] = {
    V4(153.0/255.0, 173.0/255, 254.0/255.0, 1),
    V4(191.0/255.0, 238.0/255, 254.0/255.0, 1),
    V4(254.0/255.0, 254.0/255.0, 255/255, 1)
  };
  local_persist v4 Colors2[] = {
    V4(79/255.f, 187/255.f, 255/255.f, 1),
    V4(50/255.f, 156/255.f, 185/255.f, 1),
    V4(48.f/255.f, 51/255.f, 211/255.f, 1)
  };

  local_persist r32 FillRates[8] = {};
  local_persist r32 AngleSpans[8][4]
  {
    // Min Theta, Min Phi, Max Theta, Max Phi
    {0 * Tau32/4.f,        0, 1 * Tau32/4.f, Pi32/2.f},
    {1 * Tau32/4.f,        0, 2 * Tau32/4.f, Pi32/2.f},
    {2 * Tau32/4.f,        0, 3 * Tau32/4.f, Pi32/2.f},
    {3 * Tau32/4.f,        0, 4 * Tau32/4.f, Pi32/2.f},
    {0 * Tau32/4.f, Pi32/2.f, 1 * Tau32/4.f, Pi32},
    {1 * Tau32/4.f, Pi32/2.f, 2 * Tau32/4.f, Pi32},
    {2 * Tau32/4.f, Pi32/2.f, 3 * Tau32/4.f, Pi32},
    {3 * Tau32/4.f, Pi32/2.f, 4 * Tau32/4.f, Pi32}
  };

  if(!RandomInit)
  {
    for (int i = 0; i < SPOTCOUNT; ++i)
    {
      eruption_params* Param = Params + i;

      u32 EmptiestRegionIndex = 0;
      r32 EmptiestRegionFillRate = R32Max;
      for (int i = 0; i < ArrayCount(FillRates); ++i)
      {
        if(FillRates[i] < EmptiestRegionFillRate)
        {
          EmptiestRegionFillRate = FillRates[i];
          EmptiestRegionIndex = i;
        }
      }

      r32 Theta = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][0], AngleSpans[EmptiestRegionIndex][2]);
      r32 Phi   = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][1], AngleSpans[EmptiestRegionIndex][3]);
      if(GetRandomRealNorm(&GameState->RandomGenerator) < 0.7) {
        InitializeEruption(Param, &GameState->RandomGenerator, ArrayCount(Colors1), Colors1,0.1, 0.275, 0.7, Theta, Phi);
      }else{
        InitializeEruption(Params + i, &GameState->RandomGenerator, ArrayCount(Colors2), Colors2, 0.05, 0.14, 0, Theta, Phi);
      }
      Param->Time     = GetRandomReal(&GameState->RandomGenerator, 0, Param->Duration);
      FillRates[EmptiestRegionIndex] += Param->EruptionSize;
      Param->RegionIndex = EmptiestRegionIndex;
    }
    RandomInit = true;
  }
  
  for (int i = 0; i < SPOTCOUNT; ++i)
  {
    eruption_params* Param = Params + i;
    Param->Time += Input->deltaTime;
    if(Param->Time > Param->Duration)
    {
      u32 EmptiestRegionIndex = 0;
      r32 EmptiestRegionFillRate = R32Max;
      FillRates[Param->RegionIndex] -= Param->EruptionSize;
      for (int i = 0; i < ArrayCount(FillRates); ++i)
      {
        if(FillRates[i] < EmptiestRegionFillRate)
        {
          EmptiestRegionFillRate = FillRates[i];
          EmptiestRegionIndex = i;
        }
      }
      r32 Theta = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][0], AngleSpans[EmptiestRegionIndex][2]);
      r32 Phi   = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][1], AngleSpans[EmptiestRegionIndex][3]);
      eruption_params* Param = Params + i;
      if(GetRandomRealNorm(&GameState->RandomGenerator) < 0.7)
      {
        InitializeEruption(Param, &GameState->RandomGenerator, ArrayCount(Colors1), Colors1,  0.1, 0.275, 0.7, Theta, Phi);
      }else{
        InitializeEruption(Param, &GameState->RandomGenerator, ArrayCount(Colors2), Colors2, 0.05, 0.14, 0, Theta, Phi);
      }
      FillRates[EmptiestRegionIndex] += Param->EruptionSize;
      Param->RegionIndex = EmptiestRegionIndex;
    }
  }

  DrawEruptionBands(RenderCommands, Input, SPOTCOUNT, Params, Sphere1ModelMat, V4(Position,1), Sphere1RotationMatrix);
  
  // Ray
  CastRays(RenderCommands, Input, Camera, Position);

  // Halo
  {
    v3 Forward, Up, Right;
    GetCameraDirections(Camera, &Up, &Right, &Forward);
    v3 Direction = GetCameraPosition(Camera) - Position;
    m4 BillboardRotation = CoordinateSystemTransform(Direction,-CrossProduct(Right, Forward));
    m4 HaloModelMat = M4Identity();
    HaloModelMat = GetRotationMatrix(Pi32/2.f, V4(1,0,0,0))* HaloModelMat;
    HaloModelMat = GetScaleMatrix(V4(2,2,2,1)) * HaloModelMat;
    HaloModelMat = GetTranslationMatrix(V4(Position,0)) * BillboardRotation*HaloModelMat;

    render_object* Halo = PushNewRenderObject(RenderCommands->RenderGroup);
    Halo->ProgramHandle = GameState->PlaneStarProgram;
    Halo->MeshHandle = GameState->Plane;
    Halo->Transparent = true;

    PushUniform(Halo, GlGetUniformHandle(&RenderCommands->OpenGL, GameState->PlaneStarProgram, "ProjectionMat"), Camera->P);
    PushUniform(Halo, GlGetUniformHandle(&RenderCommands->OpenGL, GameState->PlaneStarProgram, "ViewMat"), Camera->V);
    ray_cast* HaloRay = PushStruct(GlobalTransientArena, ray_cast);
    HaloRay->ModelMat = HaloModelMat;
    HaloRay->Color = V4(254.0/255.0, 254.0/255.0, 255/255, 0.3);
    HaloRay->Radius = (r32)( 1.3f + 0.07 * Sin(Input->Time));
    HaloRay->FaceDist =  0.3f;
    HaloRay->Center = Position;
    PushInstanceData(Halo, 1, sizeof(ray_cast), (void*) HaloRay);
    PushRenderState(Halo, DepthTestCulling);
  }
}


struct skybox_params
{
  u32 TextureHeight; // Textures pixel size height
  u32 TextureWidth; // Textures pixel size width
  u32 SideSize; // Textures pixel size one side of the of skybox
};

v3 GetCubeCoordinateFromTexture(u32 PixelX, u32 PixelY, skybox_params* Params)
{
  u32 TextureGridX = PixelX % Params->SideSize;
  u32 TextureGridY = PixelY % Params->SideSize;
  u32 GridX = PixelX / Params->SideSize;
  u32 GridY = PixelY / Params->SideSize;
  u32 GridIndex = GridY * 3 + GridX;
  r32 X = (2.f*TextureGridX - Params->SideSize) / ((r32)Params->SideSize);
  r32 Y = (2.f*TextureGridY - Params->SideSize) / ((r32)Params->SideSize);

  // Maps the direction of the plane in world-space to the direction of the texture
  // 0,1,2 is the top half of the texture, 
  //  It starts at face -X and wraps around to +X via -Z with top of the texture being in +Y dir.
  // 3,4,5 is the bot half of the texture,
  // It starts at face +Y and wraps around to -Y via +Z with top of texture being in +X dir.
  switch(GridIndex)
  {
    case 0: {return V3(-1.0f,   -Y,   -X);} break; // Cube Normal -X, TextureY -> -Y
    case 1: {return V3(    X,   -Y,-1.0f);} break; // Cube Normal -Z, TextureY -> -Y
    case 2: {return V3( 1.0f,   -Y,    X);} break; // Cube Normal +X, TextureY -> -Y
    case 3: {return V3(   -Y, 1.0f,    X);} break; // Cube Normal +Y, TextureY -> -X
    case 4: {return V3(   -Y,   -X, 1.0f);} break; // Cube Normal +Z, TextureY -> -X
    case 5: {return V3(   -Y,-1.0f,   -X);} break; // Cube Normal -Y, TextureY -> -X
  }
  INVALID_CODE_PATH;
  return {};
}

r32 RayPlaneIntersection( v3 PlaneNormal, v3 PointOnPlane, v3 Ray, v3 RayOrigin)
{
  r32 Lambda = PlaneNormal * (PointOnPlane - RayOrigin) / (PlaneNormal * Ray);
  return Lambda;
}

enum class skybox_side{
  X_MINUS,
  Z_MINUS,
  X_PLUS,
  Y_PLUS,
  Z_PLUS,
  Y_MINUS,
  SIDE_COUNT
};


struct sky_vectors {
  v3 TopLeft;
  skybox_side TopLeftSide;
  v3 TopRight;
  skybox_side TopRightSide;
  v3 BotLeft;
  skybox_side BotLeftSide;
  v3 BotRight;
  skybox_side BotRightSide;
};

skybox_side GetSkyboxSide(v3 Direction)
{
  r32 AbsX = Abs(Direction.X);
  r32 AbsY = Abs(Direction.Y);
  r32 AbsZ = Abs(Direction.Z);
  if(AbsX >= AbsY && AbsX >= AbsZ ) 
  {
    return Direction.X > 0 ? skybox_side::X_PLUS : skybox_side::X_MINUS;
  } else if(AbsY >= AbsZ && AbsY > AbsX) {
    
    return Direction.Y > 0 ? skybox_side::Y_PLUS : skybox_side::Y_MINUS;
    
  } else if(AbsZ > AbsX && AbsZ > AbsY) {

    return Direction.Z > 0 ? skybox_side::Z_PLUS : skybox_side::Z_MINUS;
  }

  // Not specifically a bug to end up here,
  // Just not knowing when it could happen
  INVALID_CODE_PATH
  return skybox_side::SIDE_COUNT;
}

v3 GetSkyNormal(skybox_side SkyboxSide)
{
  switch(SkyboxSide)
  {
    case skybox_side::X_MINUS: return V3(-1, 0, 0); break;
    case skybox_side::Z_MINUS: return V3( 0, 0,-1); break;
    case skybox_side::X_PLUS:  return V3( 1, 0, 0); break;
    case skybox_side::Y_PLUS:  return V3( 0, 1, 0); break;
    case skybox_side::Z_PLUS:  return V3( 0, 0, 1); break;
    case skybox_side::Y_MINUS: return V3( 0,-1, 0); break;
    default: INVALID_CODE_PATH;
  }

  return {};
}

sky_vectors GetSkyVectors(camera* Camera, r32 SkyAngle)
{
  v3 CamUp, CamRight, CamForward;
  GetCameraDirections(Camera, &CamUp, &CamRight, &CamForward);
  v3 TexForward = -CamForward;
  v3 TexRight = -CamRight;
  v3 TexUp = CamUp;

  r32 TexWidth = Sin(SkyAngle);
  r32 TexHeight = Sin(SkyAngle);
  sky_vectors Result = {};
  Result.TopLeft   = Normalize( TexForward - TexRight * TexWidth + TexUp * TexHeight);
  Result.TopRight  = Normalize( TexForward + TexRight * TexWidth + TexUp * TexHeight);
  Result.BotLeft   = Normalize( TexForward - TexRight * TexWidth - TexUp * TexHeight);
  Result.BotRight  = Normalize( TexForward + TexRight * TexWidth - TexUp * TexHeight);
  Result.TopLeftSide = GetSkyboxSide(Result.TopLeft);
  Result.TopRightSide = GetSkyboxSide(Result.TopRight);
  Result.BotLeftSide = GetSkyboxSide(Result.BotLeft);
  Result.BotRightSide = GetSkyboxSide(Result.BotRight);
  return Result;
}


v2 GetTextureCoordinateFromUnitSphereCoordinate(v3 UnitSphere, skybox_side Side, opengl_bitmap* Bitmap)
{
  jwin_Assert(Bitmap->Width / 3.f == Bitmap->Height / 2.f);

  u32 TextureStartX = 0;
  u32 TextureStartY = 0;
  u32 TextureSideSize = Bitmap->Height / 2.f;
  v2 PlaneCoordinate = V2(0,0);
  r32 Alpha = 0;

  r32 AbsX = Abs(UnitSphere.X);
  r32 AbsY = Abs(UnitSphere.Y);
  r32 AbsZ = Abs(UnitSphere.Z);
  jwin_Assert(AbsX <= 1 && AbsY <= 1 && AbsZ <= 1);

  switch(Side)
  {
    case skybox_side::X_MINUS:
    {
      Alpha = RayPlaneIntersection( V3(-1,0,0), V3(-1,0,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = 0;
      TextureStartY = 0;
      PlaneCoordinate.X = -PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.Y;
    }break;
    case skybox_side::Z_MINUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,0,-1), V3(0,0,-1), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      
      TextureStartX = Bitmap->Width/3;
      TextureStartY = 0;
      PlaneCoordinate.X =  PointOnPlane.X;
      PlaneCoordinate.Y = -PointOnPlane.Y;
    }break;
    case skybox_side::X_PLUS:
    {
      TextureStartX = 2*Bitmap->Width / 3.f;
      TextureStartY = 0;
      Alpha = RayPlaneIntersection( V3(1,0,0), V3(1,0,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      PlaneCoordinate.X = PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.Y;
    }break;
    case skybox_side::Y_PLUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,1,0), V3(0,1,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = 0;
      TextureStartY = Bitmap->Height / 2.f;
      PlaneCoordinate.X =  PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.X;
    }break;
    case skybox_side::Z_PLUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,0,1), V3(0,0,1), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = Bitmap->Width/3;
      TextureStartY = Bitmap->Height / 2.f;
      PlaneCoordinate.X = -PointOnPlane.Y;
      PlaneCoordinate.Y = -PointOnPlane.X;
    }break;
    case skybox_side::Y_MINUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,-1,0), V3(0,-1,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = 2*Bitmap->Width/3.f;
      TextureStartY = Bitmap->Height / 2.f;
      PlaneCoordinate.X = -PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.X;
    }break;
  }

  r32 Tx = Clamp(LinearRemap(PlaneCoordinate.X, -1, 1, TextureStartX, TextureStartX+TextureSideSize), TextureStartX, TextureStartX+TextureSideSize-1);
  r32 Ty = Clamp(LinearRemap(PlaneCoordinate.Y, -1, 1, TextureStartY, TextureStartY+TextureSideSize), TextureStartY, TextureStartY+TextureSideSize-1);

  return {Tx, Ty};
}


v2 GetTextureCoordinateFromUnitSphereCoordinate(v3 UnitSphere, opengl_bitmap* Bitmap)
{
  return GetTextureCoordinateFromUnitSphereCoordinate(UnitSphere, GetSkyboxSide(UnitSphere), Bitmap);
}

v3 GetSphereCoordinateFromCube(v3 PointOnUnitCube)
{
  // (x*r)^2 + (y*r)^2 + (z*r)^2 = 1
  // x^2*r^2 + y^2*r^2 + z^2*r^2 = 1
  // (x^2 + y^2 + z^2)*r^2 = 1
  // r^2 = 1 / (x^2 + y^2 + z^2)
  // r = sqrt(1/(x^2 + y^2 + z^2))
  r32 r = sqrt(1.f/(PointOnUnitCube.X*PointOnUnitCube.X +
                    PointOnUnitCube.Y*PointOnUnitCube.Y +
                    PointOnUnitCube.Z*PointOnUnitCube.Z));
  return r * PointOnUnitCube;
}

u32 GetColorFromUnitVector(v3 UnitVec)
{
  v3 P = (V3(1,1,1) + UnitVec) / 2;
  //P.X = Clamp(LinearRemap(UnitVec.X, -1,1, -1,1),0,1);
  //P.Y = Clamp(LinearRemap(UnitVec.Y, -1,1, -1,1),0,1);
  //P.Z = Clamp(LinearRemap(UnitVec.Z, -1,1, -1,1),0,1);
  //jwin_Assert(P.X >= 0);
  //P.X = 0;
  //P.Y = 0;
  //P.Z = 0;
  return 0xFF << 24 | 
         ((u32)(P.X * 0xFF)) << 16 |
         ((u32)(P.Y * 0xFF)) <<  8 |
         ((u32)(P.Z * 0xFF)) <<  0;
}


// Read: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
// If Edgefunction returns:
//   > 0 : the point P is to the right side of the line a->b
//   = 0 : the point P is on the line a->b
//   < 0 : the point P is to the left side of the line a->b
// The edgefunction is equivalent to the magnitude of the cross product between the vector b-a and p-a
r32 EdgeFunction( v2& a, v2& b, v2& p )
{
  r32 Result = (a.X-p.X) * (b.Y-a.Y) - (b.X-a.X) * (a.Y-p.Y);
  return(Result);
}

// Read: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
// If Edgefunction returns:
//   > 0 : the point P is to the right side of the line a->b
//   = 0 : the point P is on the line a->b
//   < 0 : the point P is to the left side of the line a->b
// The edgefunction is equivalent to the magnitude of the cross product between the vector b-a and p-a
r32 EdgeFunction( v3& a, v3& b, v3& p )
{
  r32 Result = Determinant(a,b,p);
  return(Result);
}

struct skybox_vertice {
  v3 P;
  v2 Tex;
};

skybox_vertice SkyboxVertice(r32 X, r32 Y, r32 Z, r32 Tx, r32 Ty)
{
  skybox_vertice Result = {};
  Result.P = V3(X,Y,Z);
  Result.Tex = V2(Tx,Ty);
  return Result;
}

struct skybox_quad {
  skybox_vertice A;
  skybox_vertice B;
  skybox_vertice C;
  skybox_vertice D;
};


struct skybox_triangle {
  skybox_vertice A;
  skybox_vertice B;
  skybox_vertice C;
};

skybox_quad SkyboxQuad(skybox_vertice A, skybox_vertice B, skybox_vertice C, skybox_vertice D) {
  skybox_quad Result = {};
  Result.A = A;
  Result.B = B;
  Result.C = C;
  Result.D = D;
  return Result;
}

// SkyboxQuad texture mapping: (Mapping done in graphics layer when setting up texture coordinates,
//  At some point move that part here since theyre linked)
//  Number is the index in the Colors array, x,y,z is the direction of the cube face normal
//  ___________________
//  |     |     |     |
//  |0,-x |1,-z |2,+x |  
//  |_____|_____|_____|
//  |     |     |     |
//  |3,+y |4,+z |5,-y |
//  |_____|_____|_____|
//
//  Top Left maps to -x plane, Top Middle maps to -z plane etc
//  The lower row mapping +y,+z-y wraps around in a way such that the 
//  left edge of +y attaches to the top of -z
//  Unwrapped with connecting edges the texture would look like this:
//         _____
//        |     |
//        |5,-y |
//        |_____|
//        |     |
//        |4,+z |
//        |_____|
//        |     |      
//        |3,+y |
//   _____|_____|_____
//  |     |     |     |
//  |0,-x |1,-z |2,+x |  
//  |_____|_____|_____|
//
//  When folded to a cube it folds such that the 
//  normals point inwards

skybox_quad GetSkyboxQuad(skybox_side Side)
{
    // T1 = (A,C,D), T2 = (A B C)
    local_persist skybox_quad Skybox_XMinus = SkyboxQuad(
    SkyboxVertice(-1.0f, 1.0f,-1.0f, 1.0f/3.0f, 0.0f/2.0f),  // A
    SkyboxVertice(-1.0f,-1.0f,-1.0f, 1.0f/3.0f, 1.0f/2.0f),  // B
    SkyboxVertice(-1.0f,-1.0f, 1.0f, 0.0f/3.0f, 1.0f/2.0f),  // C
    SkyboxVertice(-1.0f, 1.0f, 1.0f, 0.0f/3.0f, 0.0f/2.0f)); // D

    local_persist skybox_quad Skybox_ZMinus = SkyboxQuad(
    // T1 = (A,C,D), T2 = (A B C)
    SkyboxVertice( 1.0f, 1.0f,-1.0f, 2.0f/3.0f, 0.0f/2.0f),  // A
    SkyboxVertice( 1.0f,-1.0f,-1.0f, 2.0f/3.0f, 1.0f/2.0f),  // B
    SkyboxVertice(-1.0f,-1.0f,-1.0f, 1.0f/3.0f, 1.0f/2.0f),  // C
    SkyboxVertice(-1.0f, 1.0f,-1.0f, 1.0f/3.0f, 0.0f/2.0f)); // D

    local_persist skybox_quad Skybox_XPlus = SkyboxQuad(
    // T1 = (A,C,D), T2 = (A B C)
    SkyboxVertice(1.0f, 1.0f, 1.0f, 3.0f/3.0f, 0.0f/2.0f),   // A
    SkyboxVertice(1.0f,-1.0f, 1.0f, 3.0f/3.0f, 1.0f/2.0f),   // B
    SkyboxVertice(1.0f,-1.0f,-1.0f, 2.0f/3.0f, 1.0f/2.0f),   // C
    SkyboxVertice(1.0f, 1.0f,-1.0f, 2.0f/3.0f, 0.0f/2.0f));  // D

    local_persist skybox_quad Skybox_YPlus = SkyboxQuad(
    // T1 = (A,C,D), T2 = (A B C)
    SkyboxVertice( 1.0f, 1.0f, 1.0f, 1.0f/3.0f, 1.0f/2.0f),  // A
    SkyboxVertice( 1.0f, 1.0f,-1.0f, 0.0f/3.0f, 1.0f/2.0f),  // B
    SkyboxVertice(-1.0f, 1.0f,-1.0f, 0.0f/3.0f, 2.0f/2.0f),  // C
    SkyboxVertice(-1.0f, 1.0f, 1.0f, 1.0f/3.0f, 2.0f/2.0f)); // D

    local_persist skybox_quad Skybox_ZPlus = SkyboxQuad(
    // T1 = (A,C,D), T2 = (A B C)
    SkyboxVertice( 1.0f,-1.0f, 1.0f, 2.0f/3.0f, 1.0f/2.0f),  // A
    SkyboxVertice( 1.0f, 1.0f, 1.0f, 1.0f/3.0f, 1.0f/2.0f),  // B 
    SkyboxVertice(-1.0f, 1.0f, 1.0f, 1.0f/3.0f, 2.0f/2.0f),  // C
    SkyboxVertice(-1.0f,-1.0f, 1.0f, 2.0f/3.0f, 2.0f/2.0f)); // D

    local_persist skybox_quad Skybox_YMinus = SkyboxQuad(
    // T1 = (A,C,D), T2 = (A B C)
    SkyboxVertice( 1.0f,-1.0f,-1.0f, 3.0f/3.0f, 1.0f/2.0f),  // A
    SkyboxVertice( 1.0f,-1.0f, 1.0f, 2.0f/3.0f, 1.0f/2.0f),  // B
    SkyboxVertice(-1.0f,-1.0f, 1.0f, 2.0f/3.0f, 2.0f/2.0f),  // C
    SkyboxVertice(-1.0f,-1.0f,-1.0f, 3.0f/3.0f, 2.0f/2.0f)); // D

    switch (Side)
    {
      case skybox_side::X_MINUS: return Skybox_XMinus;
      case skybox_side::Z_MINUS: return Skybox_ZMinus;
      case skybox_side::X_PLUS:  return Skybox_XPlus;
      case skybox_side::Y_PLUS:  return Skybox_YPlus;
      case skybox_side::Z_PLUS:  return Skybox_ZPlus;
      case skybox_side::Y_MINUS: return Skybox_YMinus;
    }
    return {};
}

skybox_triangle GetBotLeftTriangle(skybox_quad* Quad) {
  return {Quad->A, Quad->C, Quad->D};
}
skybox_triangle GetTopRightTriangle(skybox_quad* Quad) {
  return {Quad->A, Quad->B, Quad->C};
}

void DrawPixels(v2* DstPixels, v2* SrcPixels, opengl_bitmap* DstBitmap, opengl_bitmap* SrcBitmap){

  // Bottom left triangle made up by right handed pointing vectors
  r32 Area2Inverse = 1/EdgeFunction(DstPixels[0], DstPixels[1], DstPixels[2]);
  
  v2 DstPixel0 = DstPixels[0];
  v2 DstPixel1 = DstPixels[1];
  v2 DstPixel2 = DstPixels[2];


  v2 SrcPixel0 = SrcPixels[0];
  v2 SrcPixel1 = SrcPixels[1];
  v2 SrcPixel2 = SrcPixels[2];

  if(Area2Inverse < 0)
  {
    DstPixel1 = DstPixels[2];
    DstPixel2 = DstPixels[1];
    SrcPixel1 = SrcPixels[2];
    SrcPixel2 = SrcPixels[1];
    Area2Inverse = 1/EdgeFunction(DstPixel0, DstPixel1, DstPixel2);
  }

  v4 BoundingBox = {};
  BoundingBox.X = Minimum(DstPixel0.X, Minimum(DstPixel1.X, DstPixel2.X)); // Min X
  BoundingBox.Y = Minimum(DstPixel0.Y, Minimum(DstPixel1.Y, DstPixel2.Y)); // Min Y
  BoundingBox.Z = Maximum(DstPixel0.X, Maximum(DstPixel1.X, DstPixel2.X)); // Max X
  BoundingBox.W = Maximum(DstPixel0.Y, Maximum(DstPixel1.Y, DstPixel2.Y)); // Max Y

  for(s32 Y = Round( BoundingBox.Y ); Y < BoundingBox.W; ++Y)
  {
    for(s32 X = Round( BoundingBox.X ); X < BoundingBox.Z; ++X)
    {
      v2 p = V2( (r32) X, (r32) Y);
      r32 PixelInRange0 = EdgeFunction( DstPixel0, DstPixel1, p);
      r32 PixelInRange1 = EdgeFunction( DstPixel1, DstPixel2, p);
      r32 PixelInRange2 = EdgeFunction( DstPixel2, DstPixel0, p);

      if( ( PixelInRange0 >= 0 ) && 
          ( PixelInRange1 >= 0 ) &&
          ( PixelInRange2 >= 0 ) )
      {
        r32 Lambda0 = PixelInRange1 * Area2Inverse;
        r32 Lambda1 = PixelInRange2 * Area2Inverse;
        r32 Lambda2 = PixelInRange0 * Area2Inverse;

        v2 InterpolatedTextureCoord = Lambda0 * SrcPixel0 + Lambda1 * SrcPixel1 + Lambda2 * SrcPixel2;
        u32 SrxPixelX = TruncateReal32ToInt32(Lerp(InterpolatedTextureCoord.X, 0, SrcBitmap->Width-1));
        u32 SrxPixelY = TruncateReal32ToInt32(Lerp(InterpolatedTextureCoord.Y, 0, SrcBitmap->Height-1));
        u32 SrcPixelIndex = SrcBitmap->Width *  SrxPixelY + SrxPixelX;
        jwin_Assert(SrcPixelIndex <  SrcBitmap->Width * SrcBitmap->Height);
        u32* SrcPixels = (u32*) SrcBitmap->Pixels;
        u32 TextureValue = SrcPixels[SrcPixelIndex];
        u32* DstPixels = (u32*) DstBitmap->Pixels;
        u32 TextureCoordIndex = Y * DstBitmap->Width + X;
        jwin_Assert(TextureCoordIndex < (DstBitmap->Width * DstBitmap->Height));
        DstPixels[TextureCoordIndex] = TextureValue;
      }
    }
  }
}


b32 LineLineIntersection(v3 A_Start, v3 A_End, v3 B_Start, v3 B_End, v3* ResultPoint_a, v3* ResultPoint_b) {


  v3 a = A_Start;
  v3 b = A_End;
  v3 c = B_Start;
  v3 d = B_End;

  r32 Sdenom_xy = Determinant(V2(b.X - a.X, b.Y - a.Y), V2(d.X - c.X, d.Y - c.Y));
  r32 Sdenom_xz = Determinant(V2(b.X - a.X, b.Z - a.Z), V2(d.X - c.X, d.Z - c.Z));
  r32 Sdenom_yz = Determinant(V2(b.Y - a.Y, b.Z - a.Z), V2(d.Y - c.Y, d.Z - c.Z));

  r32 s = 0;
  r32 sDenom = 0;
  if(Abs(Sdenom_xy) > 0)
  {
    r32 Senum = Determinant(V2(b.X - a.X, b.Y - a.Y), V2(a.X - c.X, a.Y - c.Y));
    s = Senum/Sdenom_xy;
    sDenom = Sdenom_xy;
  }
  else if(Abs(Sdenom_xz) > 0)
  {
    r32 Senum = Determinant(V2(b.X - a.X, b.Z - a.Z), V2(a.X - c.X, a.Z - c.Z));
    s = Senum/Sdenom_xz;
    sDenom = Sdenom_xz;
  }
  else if(Abs(Sdenom_yz) > 0)
  {
    r32 Senum = Determinant(V2(b.Y - a.Y, b.Z - a.Z), V2(a.Y - c.Y, a.Z - c.Z));
    s = Senum/Sdenom_yz;
    sDenom = Sdenom_yz;
  }

  r32 Epsilon = 1E-2;
  if (Abs(sDenom) < Epsilon) return false;

  r32 t = ( s * (d.X - c.X) - (a.X - c.X)) / (b.X - a.X);

  v3 Result_a = a + t * (b-a);
  v3 Result_b = c + s * (d-c);
  
  // Note: Blir ibland små numeriska fel då man gör dot-produkt då en av vektorkomponenterna är jättesmå men inte 0
  // Vilket får Result_a och Result_b att diffa lite. Därför använder vi inte IsPointOnLinesegment som är ganska numeriskt strikt
  //  - 1 Man skulle kunna söka efter vilken av linjerna som har minst numeriska fel i sig och använda den men vill inte lägga
  //      tid på å hitta något bra sätt att göra det.
  //  - 2 Man skulle även alltid kunna ta Result_b som vi vet är en numeriskt stabil linje för att den tillhör skybox-boxen,
  //      men så kanske det inte kommer vara om vi vill återanvända denna funktion någon annan stans.
  // Så enklast för vårat syfte är nog att returnera båda resultaten och låta användaren bestämma vilken som bör vara stabilast. 
  if( Abs(Result_a.X - Result_b.X) > Epsilon || 
      Abs(Result_a.Y - Result_b.Y) > Epsilon || 
      Abs(Result_a.Z - Result_b.Z) > Epsilon) 
  {
    // LOGGA Fel?
    int a = 10; // Plats för breakpoint om vi upptäcker skumt beteende
  }

  if(ResultPoint_a != 0)
  {
    *ResultPoint_a = Result_a;
  }
  if(ResultPoint_b != 0)
  {
    *ResultPoint_b = Result_b;
  }

  return t >= 0 && t <= 1 && s >= 0 && s <= 1;
}

struct skybox_point_list
{
  v3 Point;
  skybox_point_list* Previous;
  skybox_point_list* Next;
};

// void ApplicationUpdateAndRender(application_memory* Memory, application_render_commands* RenderCommands, jwin::device_input* Input)
extern "C" JWIN_UPDATE_AND_RENDER(ApplicationUpdateAndRender)
{
  GlobalState = JwinBeginFrameMemory(application_state);

  platform_offscreen_buffer* OffscreenBuffer = &RenderCommands->PlatformOffscreenBuffer;
  local_persist v3 LightPosition = V3(0,3,0);
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
    obj_bitmap* BrickWallTexture = LoadTGA(GlobalTransientArena, "..\\data\\textures\\brick_wall_base.tga");
    obj_bitmap* FadedRayTexture = LoadTGA(GlobalTransientArena, "..\\data\\textures\\faded_ray.tga");
    obj_bitmap* EarthTexture = LoadTGA(GlobalTransientArena, "..\\data\\textures\\8081_earthmap4k.tga");

    // This memory only needs to exist until the data is loaded to the GPU
    GlobalState->PhongProgram = CreatePhongProgram(OpenGL);
    GlobalState->PhongProgramNoTex = CreatePhongNoTexProgram(OpenGL);
    GlobalState->PhongProgramTransparent = CreatePhongTransparentProgram(OpenGL);
    GlobalState->PlaneStarProgram = CreatePlaneStarProgram(OpenGL);
    GlobalState->SolidColorProgram = CreateSolidColorProgram(OpenGL);
    GlobalState->EruptionBandProgram = CreateEruptionBandProgram(OpenGL);
    GlobalState->Cube = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cube));
    GlobalState->Plane = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, plane));
    GlobalState->Sphere = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, sphere));
    GlobalState->Cone = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cone));
    GlobalState->Cylinder = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cylinder));
    GlobalState->Triangle = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, triangle));
    GlobalState->Billboard = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena,  billboard));
    GlobalState->CheckerBoardTexture = GlLoadTexture(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, plane->MaterialData->Materials[0].MapKd));
    GlobalState->BrickWallTexture = GlLoadTexture(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, BrickWallTexture));
    GlobalState->FadedRayTexture = GlLoadTexture(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, FadedRayTexture));
    GlobalState->EarthTexture = GlLoadTexture(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, EarthTexture));
    
    u8 WhitePixel[4] = {255,255,255,255};
    opengl_bitmap WhitePixelBitmap = {};
    WhitePixelBitmap.BPP = 32;
    WhitePixelBitmap.Width = 1;
    WhitePixelBitmap.Height = 1;
    u32 ByteSize = (WhitePixelBitmap.BPP/8) * WhitePixelBitmap.Width * WhitePixelBitmap.Height;
    WhitePixelBitmap.Pixels = PushCopy(GlobalTransientArena, ByteSize, (void*)WhitePixel);
    GlobalState->WhitePixelTexture = GlLoadTexture(OpenGL, WhitePixelBitmap);


    u32 CharCount = 0x100;
    char FontPath[] = "C:\\Windows\\Fonts\\consola.ttf";
    GlobalState->OnedgeValue = 128;  // "Brightness" of the sdf. Higher value makes the SDF bigger and brighter.
                                     // Has no impact on TextPixelSize since the char then is also bigger.
    GlobalState->TextPixelSize = 64; // Size of the SDF
    GlobalState->PixelDistanceScale = 32.0; // Smoothness of how fast the pixel-value goes to zero. Higher PixelDistanceScale, makes it go faster to 0;
                                            // Lower PixelDistanceScale and Higher OnedgeValue gives a 'sharper' sdf.
    GlobalState->FontRelativeScale = 1.f;
    GlobalState->Font = jfont::LoadSDFFont(PushArray(GlobalPersistentArena, CharCount, jfont::sdf_fontchar),
      CharCount, FontPath, GlobalState->TextPixelSize, 3, GlobalState->OnedgeValue, GlobalState->PixelDistanceScale);
    GlobalState->Initialized = true;

    GlobalState->Camera = {};
    r32 AspectRatio = RenderCommands->ScreenWidthPixels / (r32)RenderCommands->ScreenHeightPixels;
    InitiateCamera(&GlobalState->Camera, 70, AspectRatio, 0.1);
    LookAt(&GlobalState->Camera, V3(0,0,10), V3(0,0,0));
    RenderCommands->RenderGroup = InitiateRenderGroup();
    RenderCommands->LoadDebugCode = true;
    

    GlobalState->RandomGenerator = RandomGenerator(Input->RandomSeed);
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

  r32 Len = 0;
  v3 Pos = V3(Len,0,0);
  v3 At = V3(0,0,0);
  v3 Up = V3(0,1,0);
  b32 UpdateCamera = false;
  if(!(jwin::Active(Input->Keyboard.Key_LALT) || jwin::Active(Input->Keyboard.Key_RALT)))
  {
    if(jwin::Pushed(Input->Keyboard.Key_X))
    {
      UpdateCamera = true;
      Pos = V3(Len,0,0);
      At = V3(Len+1,0,0);
      if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT))) 
      {
        Pos = -Pos;
        At = At = V3(Len-1,0,0);
      }
    }
    else if(jwin::Pushed(Input->Keyboard.Key_Y))
    {
      UpdateCamera = true;
      Pos = V3(0,Len,0);
      At = V3(0,Len + 1,0);
      Up = V3(1,0,0);
      if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        Pos = -Pos;
        At = V3(0,Len - 1,0);
      }
    }
    else if(jwin::Pushed(Input->Keyboard.Key_Z))
    {
      UpdateCamera = true;
      Pos = V3(0,0,Len);
      At = V3(0,0,Len + 1);
      if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        Pos = -Pos;
        At = V3(0,0,Len - 1);
      }  
    }
    else if(jwin::Pushed(Input->Keyboard.Key_Q))
    {
      UpdateCamera = true;
      Pos = V3(0,Len,Len);
      if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
      {
        Pos = -Pos;
        At = V3(0,0,0);
      }
    }

    if(UpdateCamera)
    {
      LookAt(Camera, Pos, At, Up);
      v3 Up, Right, Forward;
      GetCameraDirections(Camera, &Up, &Right, &Forward);
      v3 CamPos = GetCameraPosition(Camera);
      Platform.DEBUGPrint("CamPos: (%1.2f %1.2f %1.2f) CamForward (%1.2f %1.2f %1.2f)\n",
        CamPos.X, CamPos.Y, CamPos.Z, -Forward.X, -Forward.Y, -Forward.Z);
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
      //RotateCamera(Camera, 2*Input->Mouse.dX, V3(0,-1,0) );
    }
    if(Input->Mouse.dY != 0)
    {
      RotateCamera(Camera, 2*Input->Mouse.dY, V3(1,0,0) );
      v3 Up, Right, Forward;
      GetCameraDirections(Camera, &Up, &Right, &Forward);
      v3 CamPos = GetCameraPosition(Camera);
      Platform.DEBUGPrint("CamPos: (%1.2f %1.2f %1.2f) CamForward (%1.2f %1.2f %1.2f)\n",
        CamPos.X, CamPos.Y, CamPos.Z, -Forward.X, -Forward.Y, -Forward.Z);
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
    Platform.DEBUGPrint("We should reload debug code\n");
    RenderCommands->LoadDebugCode = true;
    GlobalState->PhongProgram = GlReloadProgram(OpenGL, GlobalState->PhongProgram,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"));
    GlobalState->PhongProgramNoTex = GlReloadProgram(OpenGL, GlobalState->PhongProgramNoTex,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));
    GlobalState->PhongProgramTransparent = GlReloadProgram(OpenGL, GlobalState->PhongProgramTransparent,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewTransparent.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewTransparent.glsl"));
    GlobalState->PlaneStarProgram = GlReloadProgram(OpenGL, GlobalState->PlaneStarProgram,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneFragment.glsl"));
    GlobalState->SolidColorProgram = GlReloadProgram(OpenGL, GlobalState->SolidColorProgram,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidColorVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidColorFragment.glsl"));
    GlobalState->EruptionBandProgram = GlReloadProgram(OpenGL, GlobalState->EruptionBandProgram,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\EruptionBandVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\EruptionBandFragment.glsl"));
  }

  UpdateViewMatrix(Camera);

  v3 LightDirection = V3(Transpose(RigidInverse(Camera->V)) * V4(LightPosition,0));
  RenderStar(GlobalState, RenderCommands, Input, V3(0,10,0));

#if 0
  // Ray
  {
    m4 ModelViewPlane = Camera->V* GetTranslationMatrix(V4(0,-1,0,0)) * GetScaleMatrix(V4(10,0,10,1));
    m4 NormalViewPlane = Transpose(RigidInverse(ModelViewPlane));

    render_object* Ray = PushNewRenderObject(RenderCommands->RenderGroup);
    Ray->ProgramHandle = GlobalState->PhongProgramTransparent;
    Ray->MeshHandle = GlobalState->Plane;
    Ray->TextureHandle = GlobalState->FadedRayTexture;
    Ray->Transparent = true;
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ProjectionMat"), Camera->P);
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ModelView"), ModelViewPlane);
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "NormalView"), NormalViewPlane);
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightDirection"), LightDirection);
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightColor"), V3(1,1,1));
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialAmbient"), V4(0.4,0.4,0.4,1));
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialDiffuse"), V4(0.5,0.5,0.5,1));
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialSpecular"), V4(0.75,0.75,0.75,1));
    PushUniform(Ray, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "Shininess"), (r32) 20);
    PushRenderState(Ray, DepthTestCulling);

  }
#endif

#if 0
  // Floor
  {
    m4 ModelMatPlane = GetTranslationMatrix( V4(0,-1.1,0,0)) * GetScaleMatrix( V4(10,0.1,10,0));
    m4 ModelViewPlane = Camera->V*ModelMatPlane;
    m4 NormalViewPlane = Transpose(RigidInverse(ModelViewPlane));

    render_object* Floor = PushNewRenderObject(RenderCommands->RenderGroup);
    Floor->ProgramHandle = GlobalState->PhongProgram;
    Floor->MeshHandle = GlobalState->Plane;
    Floor->TextureHandle = GlobalState->CheckerBoardTexture;

    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ProjectionMat"), Camera->P);
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ModelView"), ModelViewPlane);
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "NormalView"), NormalViewPlane);
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightDirection"), LightDirection);
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightColor"), V3(1,1,1));
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialAmbient"), V4(0.4,0.4,0.4,1));
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialDiffuse"), V4(0.5,0.5,0.5,1));
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialSpecular"), V4(0.75,0.75,0.75,1));
    PushUniform(Floor, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "Shininess"), (r32) 20);
    PushRenderState(Floor, DepthTestCulling);
  }

  r32 Alpha1 = 0.3;
  r32 Alpha2 = 0.5;
  r32 Alpha3 = 0.8;

  {
    // Sphere
    m4 ModelMat = GetTranslationMatrix( V4(0,0,2,0));
    Rotate( 1/120.f, -V4(0,1,0,0), ModelMat );

    m4 ModelView = Camera->V*ModelMat;
    m4 NormalView = Transpose(RigidInverse(ModelView));


    render_object* Object = PushNewRenderObject(RenderCommands->RenderGroup);
    Object->ProgramHandle = GlobalState->PhongProgramTransparent;
    Object->MeshHandle = GlobalState->Sphere;
    Object->TextureHandle = GlobalState->WhitePixelTexture;
    Object->Transparent = true;
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ProjectionMat"), Camera->P);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ModelView"), ModelView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "NormalView"), NormalView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightDirection"), LightDirection);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightColor"), V3(1,1,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialAmbient"), V4(0.0,  0.0, 0.01, Alpha1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialDiffuse"), V4(0.0,  0.0, 0.25, Alpha1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialSpecular"), V4(1.0, 1.0, 1.0,  Alpha1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "Shininess"), (r32) 20);
    PushRenderState(Object, DepthTestCulling);
  }

  {
    // Cone
    m4 ModelMat = GetTranslationMatrix( V4(0,0,0,0));
    Rotate( 1/120.f, -V4(0,1,0,0), ModelMat );

    m4 ModelView = Camera->V*ModelMat;
    m4 NormalView = Transpose(RigidInverse(ModelView));

    render_object* Object = PushNewRenderObject(RenderCommands->RenderGroup);
    Object->ProgramHandle = GlobalState->PhongProgramTransparent;
    Object->MeshHandle = GlobalState->Cone;
    Object->TextureHandle = GlobalState->WhitePixelTexture;
    Object->Transparent = true;
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ProjectionMat"), Camera->P);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ModelView"), ModelView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "NormalView"), NormalView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightDirection"), LightDirection);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightColor"), V3(1,1,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialAmbient"), V4(0.01, 0.0, 0.0, Alpha2));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialDiffuse"), V4(0.25, 0.0, 0.0, Alpha2));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialSpecular"), V4(1.0, 1.0, 1.0, Alpha2));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "Shininess"), (r32) 20);
    PushRenderState(Object, DepthTestCulling);
  }
  {
    // Cube
    m4 ModelMat = GetTranslationMatrix( V4(2,0,2,0));
    Rotate( 1/120.f, -V4(0,1,0,0), ModelMat );

    m4 ModelView = Camera->V*ModelMat;
    m4 NormalView = Transpose(RigidInverse(ModelView));

    render_object* Object = PushNewRenderObject(RenderCommands->RenderGroup);
    Object->ProgramHandle = GlobalState->PhongProgramTransparent;
    Object->MeshHandle = GlobalState->Cube;
    Object->TextureHandle = GlobalState->WhitePixelTexture;
    Object->Transparent = true;
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ProjectionMat"), Camera->P);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "ModelView"), ModelView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "NormalView"), NormalView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightDirection"), LightDirection);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "LightColor"), V3(1,1,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialAmbient"), V4(0.0, 0.01, 0.0, Alpha3));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialDiffuse"), V4(0.0, 0.25, 0.0, Alpha3));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "MaterialSpecular"), V4(1.0, 1.0, 1.0, Alpha3));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgramTransparent, "Shininess"), (r32) 20);
    PushRenderState(Object, DepthTestCulling);
  }

  // Solid Cone
  {
    m4 ModelMat = GetTranslationMatrix(V4(2,0,0,0));
    Rotate( 1/120.f, V4(0,1,0,0), ModelMat );

    m4 ModelView = Camera->V*ModelMat;
    m4 NormalView = Transpose(RigidInverse(ModelView));

    render_object* Object = PushNewRenderObject(RenderCommands->RenderGroup);
    Object->ProgramHandle = GlobalState->PhongProgram;
    Object->MeshHandle = GlobalState->Cone;
    Object->TextureHandle = GlobalState->WhitePixelTexture;

    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ProjectionMat"), Camera->P);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ModelView"), ModelView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "NormalView"), NormalView);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightDirection"), LightDirection);
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightColor"), V3(1,1,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialAmbient"), V4(0.01,0.01,0.01,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialDiffuse"), V4(0.25,0.25,0.25,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialSpecular"), V4(1,1,1,1));
    PushUniform(Object, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "Shininess"), (r32) 20);
    PushRenderState(Object, DepthTestCulling);
  }
#endif


  {

  {
    // Skybox

    local_persist opengl_bitmap SkyboxTexture = {};
    u32 SideSize = 1024;
    SkyboxTexture.BPP = 32;
    SkyboxTexture.Width = (u32) SideSize*3;
    SkyboxTexture.Height = SideSize*2;
    local_persist opengl_bitmap TgaBitmap = {};
    if(!SkyboxTexture.Pixels)
    {
      GlobalState->Skybox = GlLoadTexture(OpenGL, SkyboxTexture); // Has permanent data managed by the graphics-layer which we can update
      SkyboxTexture.Pixels = PushSize(GlobalPersistentArena, sizeof(u32) * SkyboxTexture.Width*SkyboxTexture.Height);
      TgaBitmap = MapObjBitmapToOpenGLBitmap(GlobalPersistentArena, LoadTGA(GlobalTransientArena, "..\\data\\textures\\background_stars_spritesheet_20x20.tga"));
    }

    local_persist r32 SkyAngle = 0.1f;

    if(Input->Mouse.dZ != 0)
    {
      SkyAngle *= (Input->Mouse.dZ > 0) ? 0.85 : 1.1;
    }

    local_persist u32 StarIndex = 0;
    u32 StarTextureGrid = 20;
    u32 StarXRowCount = 5;
    u32 StarYRowCount = 5;
    u32 TotalStarCount = StarXRowCount * StarYRowCount;

    u32* SrcPixels = (u32*) TgaBitmap.Pixels;

    sky_vectors SkyVectors = GetSkyVectors(Camera, SkyAngle);

    v3 P_xm = V3(-1,0,0);
    v3 P_xp = V3( 1,0,0);
    v3 P_ym = V3(0,-1,0);
    v3 P_yp = V3(0, 1,0);
    v3 P_zm = V3(0,0,-1);
    v3 P_zp = V3(0,0, 1);

    v3 BoxPoints[] = {
      P_xm,
      P_xp,
      P_ym,
      P_yp,
      P_zm,
      P_zp
    };
    v3 BoxPointNormals[] = {
      P_xm,
      P_xp,
      P_ym,
      P_yp,
      P_zm,
      P_zp
    };

    v3 P_xm_ym_zm = V3(-1,-1,-1);
    v3 P_xp_ym_zm = V3( 1,-1,-1);
    v3 P_xm_yp_zm = V3(-1, 1,-1);
    v3 P_xp_yp_zm = V3( 1, 1,-1);
    v3 P_xm_ym_zp = V3(-1,-1, 1);
    v3 P_xp_ym_zp = V3( 1,-1, 1);
    v3 P_xm_yp_zp = V3(-1, 1, 1);
    v3 P_xp_yp_zp = V3( 1, 1, 1);
    v3 SquarePoints[] = 
    {
      P_xm_ym_zm,
      P_xp_ym_zm,
      P_xm_yp_zm,
      P_xp_yp_zm,
      P_xm_ym_zp,
      P_xp_ym_zp,
      P_xm_yp_zp,
      P_xp_yp_zp
    };
    v3 L_x_ym_zm = P_xp_ym_zm - P_xm_ym_zm;
    v3 L_x_yp_zm = P_xp_yp_zm - P_xm_yp_zm;
    v3 L_x_ym_zp = P_xp_ym_zp - P_xm_ym_zp;
    v3 L_x_yp_zp = P_xp_yp_zp - P_xm_yp_zp;
    v3 L_xm_y_zm = P_xm_yp_zm - P_xm_ym_zm;
    v3 L_xp_y_zm = P_xp_yp_zm - P_xp_ym_zm;
    v3 L_xm_y_zp = P_xm_yp_zp - P_xm_ym_zp;
    v3 L_xp_y_zp = P_xp_yp_zp - P_xp_ym_zp;
    v3 L_xm_ym_z = P_xm_ym_zp - P_xm_ym_zm;
    v3 L_xp_ym_z = P_xp_ym_zp - P_xp_ym_zm;
    v3 L_xm_yp_z = P_xm_yp_zp - P_xm_yp_zm;
    v3 L_xp_yp_z = P_xp_yp_zp - P_xp_yp_zm;
    v3 SquareLines[] =
    {
      L_x_ym_zm,
      L_x_yp_zm,
      L_x_ym_zp,
      L_x_yp_zp,
      L_xm_y_zm,
      L_xp_y_zm,
      L_xm_y_zp,
      L_xp_y_zp,
      L_xm_ym_z,
      L_xp_ym_z,
      L_xm_yp_z,
      L_xp_yp_z
    };
    v3 SkyboxLineOrigins[] = 
    {
      P_xm_ym_zm,
      P_xm_yp_zm,
      P_xm_ym_zp,
      P_xm_yp_zp,
      P_xm_ym_zm,
      P_xp_ym_zm,
      P_xm_ym_zp,
      P_xp_ym_zp,
      P_xm_ym_zm,
      P_xp_ym_zm,
      P_xm_yp_zm,
      P_xp_yp_zm
    };
    v3 SkyboxLineNormals[] = 
    {
      Normalize(V3( 0,-1,-1)), //L_x_ym_zm,
      Normalize(V3( 0, 1,-1)), //L_x_yp_zm,
      Normalize(V3( 0,-1, 1)), //L_x_ym_zp,
      Normalize(V3( 0, 1, 1)), //L_x_yp_zp,
      Normalize(V3(-1, 0,-1)), //L_xm_y_zm,
      Normalize(V3( 1, 0,-1)), //L_xp_y_zm,
      Normalize(V3(-1, 0, 1)), //L_xm_y_zp,
      Normalize(V3( 1, 0, 1)), //L_xp_y_zp
      Normalize(V3(-1,-1, 0)), //L_xm_ym_z,
      Normalize(V3( 1,-1, 0)), //L_xp_ym_z,
      Normalize(V3(-1, 1, 0)), //L_xm_yp_z,
      Normalize(V3( 1, 1, 0))  //L_xp_yp_z
    };
    
    v3 BotLeftDstTriangle[] = {SkyVectors.BotLeft, SkyVectors.BotRight, SkyVectors.TopLeft};
    v3 BotLeftDstNormals[] = {
      GetSkyNormal(SkyVectors.BotLeftSide),
      GetSkyNormal(SkyVectors.BotRightSide),
      GetSkyNormal(SkyVectors.TopLeftSide)
    };
    v3 TopRightDstTriangle[] = {SkyVectors.TopLeft, SkyVectors.BotRight, SkyVectors.TopRight};
    v3 TopRightDstNormals[] = {
      GetSkyNormal(SkyVectors.TopLeftSide),
      GetSkyNormal(SkyVectors.BotRightSide),
      GetSkyNormal(SkyVectors.TopRightSide)
    };

    r32 Area1 = EdgeFunction(BotLeftDstTriangle[0], BotLeftDstTriangle[1], BotLeftDstTriangle[2]);
    if(Area1 < 0)
    {
      v3 Tmp = BotLeftDstTriangle[1];
      BotLeftDstTriangle[1] = BotLeftDstTriangle[2];
      BotLeftDstTriangle[2] = Tmp;
      Tmp = BotLeftDstNormals[1];
      BotLeftDstNormals[1] = BotLeftDstNormals[2];
      BotLeftDstNormals[2] = Tmp;
    }
    r32 Area2 = EdgeFunction(TopRightDstTriangle[0], TopRightDstTriangle[1], TopRightDstTriangle[2]);
    if(Area2 < 0)
    {
      v3 Tmp = TopRightDstTriangle[1];
      TopRightDstTriangle[1] = TopRightDstTriangle[2];
      TopRightDstTriangle[2] = Tmp;
      Tmp = TopRightDstNormals[1];
      TopRightDstNormals[1] = TopRightDstNormals[2];
      TopRightDstNormals[2] = Tmp;
    }

    v3 CamUp, CamRight, CamForward;
    GetCameraDirections(Camera, &CamUp, &CamRight, &CamForward);
    v3 PlaneNormal = -CamForward;
    v3 TexRight = -CamRight;
    v3 TexUp = CamUp;
    v3 IntersectionPoints2[ArrayCount(SquareLines)][3] = {};

    v3 BotLeftDstTriangleProjected[3] = {
      BotLeftDstTriangle[0] * RayPlaneIntersection(BotLeftDstNormals[0], BotLeftDstNormals[0], BotLeftDstTriangle[0], V3(0,0,0)),
      BotLeftDstTriangle[1] * RayPlaneIntersection(BotLeftDstNormals[1], BotLeftDstNormals[1], BotLeftDstTriangle[1], V3(0,0,0)),
      BotLeftDstTriangle[2] * RayPlaneIntersection(BotLeftDstNormals[2], BotLeftDstNormals[2], BotLeftDstTriangle[2], V3(0,0,0))
    };

    skybox_point_list SkyboxPointsSentinel = {};
    ListInitiate(&SkyboxPointsSentinel);
    skybox_point_list* OriginalPoints = PushArray(GlobalTransientArena, 3, skybox_point_list);
    OriginalPoints[0].Point = BotLeftDstTriangleProjected[0];
    OriginalPoints[1].Point = BotLeftDstTriangleProjected[1];
    OriginalPoints[2].Point = BotLeftDstTriangleProjected[2];
    ListInsertBefore(&SkyboxPointsSentinel, &OriginalPoints[0]);
    ListInsertBefore(&SkyboxPointsSentinel, &OriginalPoints[1]);
    ListInsertBefore(&SkyboxPointsSentinel, &OriginalPoints[2]);

    DrawVector(RenderCommands, BotLeftDstTriangleProjected[0], BotLeftDstTriangleProjected[1]-BotLeftDstTriangleProjected[0], V3(0,0,0), V3(1,0,0), Camera->P, Camera->V, 0.05);
    DrawVector(RenderCommands, BotLeftDstTriangleProjected[1], BotLeftDstTriangleProjected[2]-BotLeftDstTriangleProjected[1], V3(0,0,0), V3(0,1,0), Camera->P, Camera->V, 0.05);
    DrawVector(RenderCommands, BotLeftDstTriangleProjected[2], BotLeftDstTriangleProjected[0]-BotLeftDstTriangleProjected[2], V3(0,0,0), V3(0,0,1), Camera->P, Camera->V, 0.05);

    v3 TriangleCenter = (BotLeftDstTriangle[0] + BotLeftDstTriangle[1] + BotLeftDstTriangle[2]) /3.f;
    v3 SquareCenter = Normalize(SkyVectors.BotLeft + SkyVectors.BotRight + SkyVectors.TopLeft + SkyVectors.TopRight);
    r32 PointInFront000 = ACos(Normalize(SquareCenter) * Normalize(SkyVectors.BotLeft));
    r32 PointInFront111 = ACos(Normalize(SquareCenter) * Normalize(SkyVectors.BotRight));
    r32 PointInFront222 = ACos(Normalize(SquareCenter) * Normalize(SkyVectors.TopLeft));
    r32 PointInFront333 = ACos(Normalize(SquareCenter) * Normalize(SkyVectors.TopRight));
    for (int i = 0; i < ArrayCount(SquareLines); ++i)
    {
      v3 LineOrigin = SkyboxLineOrigins[i];
      v3 LineEnd = SkyboxLineOrigins[i] + SquareLines[i];
      r32 PointInFront1 = TriangleCenter * LineOrigin;
      r32 PointInFront2 = SquareCenter * LineEnd;
      if(PointInFront1 <= 0 && PointInFront2 <=0)
      {
        continue;
      }

      DrawLine(RenderCommands, LineOrigin, LineEnd, V3(0,0,0), V3(1,0,0), Camera->P, Camera->V, 0.1);
      DrawVector(RenderCommands, LineOrigin + SquareLines[i]/2, SkyboxLineNormals[i], V3(0,0,0), V3(1,1,0), Camera->P, Camera->V, 0.1);

      r32 lambda0 = RayPlaneIntersection( SkyboxLineNormals[i], LineOrigin+SquareLines[i]/2, BotLeftDstTriangle[0], V3(0,0,0));
      r32 lambda1 = RayPlaneIntersection( SkyboxLineNormals[i], LineOrigin+SquareLines[i]/2, BotLeftDstTriangle[1], V3(0,0,0));
      r32 lambda2 = RayPlaneIntersection( SkyboxLineNormals[i], LineOrigin+SquareLines[i]/2, BotLeftDstTriangle[2], V3(0,0,0));
      v3 ProjectedPoint0 = lambda0 * BotLeftDstTriangle[0];
      v3 ProjectedPoint1 = lambda1 * BotLeftDstTriangle[1];
      v3 ProjectedPoint2 = lambda2 * BotLeftDstTriangle[2];
      r32 PointInRange0 = EdgeFunction(ProjectedPoint0, ProjectedPoint1, LineOrigin);
      r32 PointInRange1 = EdgeFunction(ProjectedPoint1, ProjectedPoint2, LineOrigin);
      r32 PointInRange2 = EdgeFunction(ProjectedPoint2, ProjectedPoint0, LineOrigin);

      v3 IntersectionPoint[3] = {};
      b32 IntersectsLine0 = LineLineIntersection(ProjectedPoint0, ProjectedPoint1, LineOrigin, LineEnd, NULL, &IntersectionPoint[0]);
      b32 IntersectsLine1 = LineLineIntersection(ProjectedPoint1, ProjectedPoint2, LineOrigin, LineEnd, NULL, &IntersectionPoint[1]);
      b32 IntersectsLine2 = LineLineIntersection(ProjectedPoint2, ProjectedPoint0, LineOrigin, LineEnd, NULL, &IntersectionPoint[2]);

      v3 ColorPoint = V3(1,1,1);
      if(Abs(SkyboxLineNormals[i] * Normalize(ProjectedPoint2) -2)< 0.001 )
      {
        ColorPoint = V3(1,0,1);
      }

      r32 PointInFront00 = ACos(Normalize(SquareCenter) * Normalize(IntersectionPoint[0]));
      if(IntersectsLine0 && PointInFront00 <= PointInFront000)
      {
        IntersectionPoints2[i][0] = IntersectionPoint[0];
        skybox_point_list* Point = PushStruct(GlobalTransientArena, skybox_point_list);
        Point->Point = IntersectionPoint[0];
        ListInsertAfter(&OriginalPoints[0], Point);
      }
      r32 PointInFront11 = ACos(Normalize(SquareCenter) * Normalize(IntersectionPoint[1]));
      if(IntersectsLine1 && PointInFront11 <= PointInFront000)
      {
        IntersectionPoints2[i][1] = IntersectionPoint[1];
        skybox_point_list* Point = PushStruct(GlobalTransientArena, skybox_point_list);
        Point->Point = IntersectionPoint[1];
        ListInsertAfter(&OriginalPoints[1], Point);
      }
      r32 PointInFront22 = ACos(Normalize(SquareCenter) * Normalize(IntersectionPoint[2]));
      if(IntersectsLine2 && PointInFront22 <= PointInFront000)
      {
        IntersectionPoints2[i][2] = IntersectionPoint[2];
        skybox_point_list* Point = PushStruct(GlobalTransientArena, skybox_point_list);
        Point->Point = IntersectionPoint[2];
        ListInsertAfter(&OriginalPoints[2], Point);
      }
      
    }

    for (int i = 0; i < ArrayCount(SquarePoints); ++i)
    {
      v3 SkyboxCornerPoint = SquarePoints[i];
      r32 lambda0 = RayPlaneIntersection( PlaneNormal, SkyboxCornerPoint, BotLeftDstTriangle[0], V3(0,0,0));
      r32 lambda1 = RayPlaneIntersection( PlaneNormal, SkyboxCornerPoint, BotLeftDstTriangle[1], V3(0,0,0));
      r32 lambda2 = RayPlaneIntersection( PlaneNormal, SkyboxCornerPoint, BotLeftDstTriangle[2], V3(0,0,0));
      v3 ProjectedPoint0 = lambda0 * BotLeftDstTriangle[0];
      v3 ProjectedPoint1 = lambda1 * BotLeftDstTriangle[1];
      v3 ProjectedPoint2 = lambda2 * BotLeftDstTriangle[2];
      r32 PointInRange0 = EdgeFunction(ProjectedPoint0, ProjectedPoint1, SkyboxCornerPoint);
      r32 PointInRange1 = EdgeFunction(ProjectedPoint1, ProjectedPoint2, SkyboxCornerPoint);
      r32 PointInRange2 = EdgeFunction(ProjectedPoint2, ProjectedPoint0, SkyboxCornerPoint);
      v3 TriangleCenter = (ProjectedPoint0 + ProjectedPoint1 + ProjectedPoint2) /3.f;
      r32 PointInFront = TriangleCenter * SkyboxCornerPoint;
      r32 PointInRange = PointInFront > 0 && PointInRange0 >= 0 && PointInRange1 >= 0 && PointInRange2 >= 0;
      if(PointInRange)
      {      
        skybox_point_list* Point = PushStruct(GlobalTransientArena, skybox_point_list);
        Point->Point = SkyboxCornerPoint;
        ListInsertAfter(&SkyboxPointsSentinel, Point);
      }
    }

    for(skybox_point_list* PointElement = SkyboxPointsSentinel.Next;
      PointElement != &SkyboxPointsSentinel;
      PointElement = PointElement->Next)
    {
      DrawDot(RenderCommands, PointElement->Point, V3(0,0,0), V3(1,1,1), Camera->P, Camera->V, 0.022);
    }




    if(jwin::Active(Input->Mouse.Button[jwin::MouseButton_Left]))
    {


      if(SkyVectors.TopLeftSide == SkyVectors.TopRightSide &&
         SkyVectors.TopLeftSide == SkyVectors.BotLeftSide &&
         SkyVectors.TopLeftSide == SkyVectors.BotRightSide)
      {
        v2 DstPixelTopLeft  = GetTextureCoordinateFromUnitSphereCoordinate(SkyVectors.TopLeft, SkyVectors.TopLeftSide, &SkyboxTexture);
        v2 DstPixelTopRight = GetTextureCoordinateFromUnitSphereCoordinate(SkyVectors.TopRight, SkyVectors.TopRightSide, &SkyboxTexture);
        v2 DstPixelBotLeft  = GetTextureCoordinateFromUnitSphereCoordinate(SkyVectors.BotLeft, SkyVectors.BotLeftSide, &SkyboxTexture);
        v2 DstPixelBotRight = GetTextureCoordinateFromUnitSphereCoordinate(SkyVectors.BotRight, SkyVectors.BotRightSide, &SkyboxTexture);
        
        v2 SrcPixelTopLeft = V2(0,0);
        v2 SrcPixelTopRight = V2(1,0);
        v2 SrcPixelBotLeft = V2(0,1);
        v2 SrcPixelBotRight = V2(1,1);

        v2 BotLeftDstTriangle[] = {DstPixelBotLeft, DstPixelBotRight, DstPixelTopLeft};
        v2 BotLeftSrcTriangle[] =  {SrcPixelBotLeft, SrcPixelBotRight, SrcPixelTopLeft};
        DrawPixels(BotLeftDstTriangle, BotLeftSrcTriangle, &SkyboxTexture, &TgaBitmap);

        v2 TopRightDstTriangle[] = {DstPixelTopLeft, DstPixelBotRight, DstPixelTopRight};
        v2 TopRightSrcTriangle[] =  {SrcPixelTopLeft, SrcPixelBotRight, SrcPixelTopRight};
        DrawPixels(TopRightDstTriangle, TopRightSrcTriangle, &SkyboxTexture, &TgaBitmap);

      }else{

/*
        r32 xAngleTopLeft = GetAngleBetweenVectors(V3(1,0,0), SkyVectors.TopLeft);
        r32 yAngleTopLeft = GetAngleBetweenVectors(V3(0,1,0), SkyVectors.TopLeft);
        r32 zAngleTopLeft = GetAngleBetweenVectors(V3(0,0,1), SkyVectors.TopLeft);
        
        r32 xAngleTopTopRight = GetAngleBetweenVectors(V3(1,0,0), SkyVectors.TopTopRight);
        r32 yAngleTopTopRight = GetAngleBetweenVectors(V3(0,1,0), SkyVectors.TopTopRight);
        r32 zAngleTopTopRight = GetAngleBetweenVectors(V3(0,0,1), SkyVectors.TopTopRight);

        r32 xAngleBotLeft = GetAngleBetweenVectors(V3(1,0,0), SkyVectors.BotLeft);
        r32 yAngleBotLeft = GetAngleBetweenVectors(V3(0,1,0), SkyVectors.BotLeft);
        r32 zAngleBotLeft = GetAngleBetweenVectors(V3(0,0,1), SkyVectors.BotLeft);

        r32 xAngleBotRight = GetAngleBetweenVectors(V3(1,0,0), SkyVectors.BotRight);
        r32 yAngleBotRight = GetAngleBetweenVectors(V3(0,1,0), SkyVectors.BotRight);
        r32 zAngleBotRight = GetAngleBetweenVectors(V3(0,0,1), SkyVectors.BotRight);
*/




        //Platform.DEBUGPrint("%1.2f %1.2f %1.2f\n",xAngle, yAngle, zAngle);

        if(SkyVectors.TopLeftSide != SkyVectors.TopRightSide)
        {
         
        }

        if(SkyVectors.BotLeftSide != SkyVectors.BotRightSide)
        {
          
        }

        if(SkyVectors.TopLeftSide != SkyVectors.BotLeftSide)
        {
          
        }

        if(SkyVectors.TopRightSide != SkyVectors.BotRightSide)
        {
          
        }


        v3 P_xm_ym_zm = V3(-1,-1,-1);
        v3 P_xp_ym_zm = V3( 1,-1,-1);
        v3 P_xm_yp_zm = V3(-1, 1,-1);
        v3 P_xp_yp_zm = V3( 1, 1,-1);
        v3 P_xm_ym_zp = V3(-1,-1, 1);
        v3 P_xp_ym_zp = V3( 1,-1, 1);
        v3 P_xm_yp_zp = V3(-1, 1, 1);
        v3 P_xp_yp_zp = V3( 1, 1, 1);




      }

      StarIndex = (StarIndex + 1) % TotalStarCount;

    }

    // Need system to update only parts of a texture.
    GlUpdateTexture(OpenGL, GlobalState->Skybox, SkyboxTexture);
    }
    skybox* SkyBox = PushNewSkybox(RenderCommands->RenderGroup);
    SkyBox->SkyboxTexture = GlobalState->Skybox;
    SkyBox->ProjectionMat = Camera->P;
    SkyBox->ViewMat = M4(
      V4(V3(Camera->V.r0),0),
      V4(V3(Camera->V.r1),0),
      V4(V3(Camera->V.r1),0),
      V4(V3(Camera->V.r2),1));
    m4 ModelView = SkyBox->ViewMat = Camera->V * GetScaleMatrix(V4(1,1,1,1));// * GetRotationMatrix(Pi32, V4(0,1,0,0)) * GetRotationMatrix(-Pi32/2, V4(0,0,1,0));
    ModelView.E[3] = 0;
    ModelView.E[7] = 0;
    ModelView.E[11] = 0;
    SkyBox->ViewMat = ModelView;
  }

  //DrawVector(RenderCommands, V3(0,0,0), ToProjZX, LightDirection, 0, Camera->P, Camera->V, 0.4);
  //DrawVector(RenderCommands, V3(0,0,0), To, LightDirection, 2, Camera->P, Camera->V, 0.4);

  //DrawVector(RenderCommands, V3(0,0,0), V3(A), LightDirection, V3(1,0,0), Camera->P, Camera->V, 0.4);
  //DrawVector(RenderCommands, V3(0,0,0), V3(B), LightDirection, V3(0,1,0), Camera->P, Camera->V, 0.4);
  //DrawVector(RenderCommands, V3(0,0,0), V3(C), LightDirection, V3(0,0,1), Camera->P, Camera->V, 0.4);

  
  //ModelViewPlaneStar1 = M4Identity();

    //DrawVector(RenderCommands, V3(0,0,0), V3(1,0,0), LightDirection, 0, Camera->P, Camera->V, 0.4);
    //DrawVector(RenderCommands, V3(0,0,0), V3(0,1,0), LightDirection, 1, Camera->P, Camera->V, 0.4);
    //DrawVector(RenderCommands, V3(0,0,0), V3(0,0,1), LightDirection, 2, Camera->P, Camera->V, 0.4);

}