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


void DrawVector(application_render_commands* RenderCommands, v3 From, v3 Direction, v3 LightDirection, v3 Color, m4 P, m4 V, r32 scale)
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

v2 GetTextureCoordinateFromUnitSphereCoordinate(v3 UnitSphere, opengl_bitmap* Bitmap)
{
  r32 AbsX = Abs(UnitSphere.X);
  r32 AbsY = Abs(UnitSphere.Y);
  r32 AbsZ = Abs(UnitSphere.Z);
  jwin_Assert(AbsX <= 1 && AbsY <= 1 && AbsZ <= 1)
  r32 Tx = 0;
  r32 Ty = 0;
  local_persist r32 PiOver4 = Pi32*0.25f;
  local_persist r32 SinCos45 = 2/sqrt(2);
  
#if 0

  v2 xv1 = Normalize(V2(UnitSphere.X, UnitSphere.Z));
  v2 xv2 = Normalize(V2(UnitSphere.X, UnitSphere.Y));
  r32 XX = ASin(xv1.Y);
  r32 XY = ASin(xv2.Y);

  v2 yv1 = Normalize(V2(UnitSphere.Y, UnitSphere.Z));
  v2 yv2 = Normalize(V2(UnitSphere.Y, UnitSphere.X));
  r32 YX = ASin(yv1.Y);
  r32 YY = ASin(yv2.Y);
  
  v2 zv1 = Normalize(V2(UnitSphere.Z, UnitSphere.Y));
  v2 zv2 = Normalize(V2(UnitSphere.Z, UnitSphere.X));
  r32 ZX = ASin(zv1.Y);
  r32 ZY = ASin(zv2.Y);
  
  if(XX > -CosSin45 && XX < CosSin45 && XY > -CosSin45 && XY < CosSin45 && UnitSphere.X > 0)
  //if(AbsX > AbsY && AbsX > AbsZ  && UnitSphere.X > 0) 
  {
    Tx = Clamp(LinearRemap(XX, -CosSin45, CosSin45, 2*Bitmap->Width / 3.f, Bitmap->Width), 2*Bitmap->Width / 3.f, Bitmap->Width-1);
    Ty = Clamp(LinearRemap(XY,  CosSin45,-CosSin45,                     0, Bitmap->Height / 2.f), 0, Bitmap->Height / 2.f-1);
    Platform.DEBUGPrint("+X");
  }else if(XX > -CosSin45 && XX < CosSin45 && XY > -CosSin45 && XY < CosSin45 && UnitSphere.X < 0) {
  //} else if(AbsX > AbsY && AbsX > AbsZ  && UnitSphere.X < 0) {
    Tx = Clamp(LinearRemap(XX, CosSin45, -CosSin45, 0, Bitmap->Width  / 3.f), 0, Bitmap->Width  / 3.f-1);
    Ty = Clamp(LinearRemap(XY, CosSin45, -CosSin45, 0, Bitmap->Height / 2.f), 0, Bitmap->Height / 2.f-1);
    Platform.DEBUGPrint("-X");
  }else if(YX > -CosSin45 && YX < CosSin45 && YY > -CosSin45 && YY < CosSin45 && UnitSphere.Y > 0 ){
    //  }else if(AbsY > AbsZ && AbsY > AbsX && UnitSphere.Y > 0) {
    Tx = Clamp(LinearRemap(YX, -CosSin45, CosSin45,                     0, Bitmap->Width/3.f),                    0, Bitmap->Width/3.f-1);
    Ty = Clamp(LinearRemap(YY, CosSin45, -CosSin45,  Bitmap->Height / 2.f, Bitmap->Height   ), Bitmap->Height / 2.f, Bitmap->Height-1   );
    Platform.DEBUGPrint("+Y");
  }else if(YX > -CosSin45 && YX < CosSin45 && YY > -CosSin45 && YY < CosSin45 && UnitSphere.Y < 0 ){
    //else if(AbsY > AbsZ && AbsY > AbsX && UnitSphere.Y < 0)
    Tx = Clamp(LinearRemap(YX, CosSin45, -CosSin45,   2*Bitmap->Width/3.f, Bitmap->Width ), 2*Bitmap->Width/3.f, Bitmap->Width-1 );
    Ty = Clamp(LinearRemap(YY, CosSin45, -CosSin45,  Bitmap->Height / 2.f, Bitmap->Height),  Bitmap->Height/2.f, Bitmap->Height-1);
    Platform.DEBUGPrint("-Y");
  }else if(ZX > -CosSin45 && ZX < CosSin45 && ZY > -CosSin45 && ZY < CosSin45 && UnitSphere.Z > 0 ){
    //} else if(AbsZ > AbsX && AbsZ > AbsY && UnitSphere.Z > 0) {
    Tx = Clamp(LinearRemap(ZX,  CosSin45, -CosSin45, Bitmap->Width/3, 2*Bitmap->Width/3.f), Bitmap->Width/3, 2*Bitmap->Width/3.f-1);
    Ty = Clamp(LinearRemap(ZY,  CosSin45, -CosSin45, Bitmap->Height / 2.f, Bitmap->Height), Bitmap->Height / 2.f, Bitmap->Height-1);
    Platform.DEBUGPrint("+Z");
  }else if(ZX > -CosSin45 && ZX < CosSin45 && ZY > -CosSin45 && ZY < CosSin45 && UnitSphere.Z < 0 ){
    //} else if(AbsZ > AbsX && AbsZ > AbsY && UnitSphere.Z < 0) {
    Tx = Clamp(LinearRemap(ZY,-CosSin45,  CosSin45, Bitmap->Width/3, 2*Bitmap->Width/3.f),Bitmap->Width/3, 2*Bitmap->Width/3.f-1);
    Ty = Clamp(LinearRemap(ZX, CosSin45, -CosSin45, 0, Bitmap->Height / 2.f),0, Bitmap->Height / 2.f-1);
    Platform.DEBUGPrint("-Z");
  } else {
    Platform.DEBUGPrint("!!\n");
  }

#endif

  if(AbsX > AbsY && AbsX > AbsZ ) 
  {

    v3 n = V3(1,0,0);
    v3 p = V3(0,0,0);

    v2 xv1 = Normalize(V2(UnitSphere.X, UnitSphere.Z));
    v2 xv2 = Normalize(V2(UnitSphere.X, UnitSphere.Y));
    r32 XX = ASin(xv1.Y);
    r32 XY = ASin(xv2.Y);

    if(UnitSphere.X > 0)
    {
      r32 Alpha = RayPlaneIntersection( V3(1,0,0), V3(1,0,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      Tx = Clamp(LinearRemap(PointOnPlane.Z, -1, 1, 2*Bitmap->Width / 3.f, Bitmap->Width), 2*Bitmap->Width / 3.f, Bitmap->Width-1);
      Ty = Clamp(LinearRemap(PointOnPlane.Y,  1,-1,                     0, Bitmap->Height / 2.f), 0, Bitmap->Height / 2.f-1);
      //Tx = Clamp(LinearRemap(XX, -PiOver4, PiOver4, 2*Bitmap->Width / 3.f, Bitmap->Width), 2*Bitmap->Width / 3.f, Bitmap->Width-1);
      //Ty = Clamp(LinearRemap(XY,  PiOver4,-PiOver4,                     0, Bitmap->Height / 2.f), 0, Bitmap->Height / 2.f-1);

      //Platform.DEBUGPrint("+X");
      //Platform.DEBUGPrint(" [%1.4f, %1.4f, %1.4f]\n", PointOnPlane.X, PointOnPlane.Y, PointOnPlane.Z);
    } else {
      r32 Alpha = RayPlaneIntersection( V3(-1,0,0), V3(-1,0,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      Tx = Clamp(LinearRemap(PointOnPlane.Z, 1, -1, 0, Bitmap->Width  / 3.f), 0, Bitmap->Width  / 3.f-1);
      Ty = Clamp(LinearRemap(PointOnPlane.Y, 1, -1, 0, Bitmap->Height / 2.f), 0, Bitmap->Height / 2.f-1);
      Platform.DEBUGPrint("-X");
    }
  } else if(AbsY > AbsZ && AbsY > AbsX) {
    
    v2 yv1 = Normalize(V2(UnitSphere.Y, UnitSphere.Z));
    v2 yv2 = Normalize(V2(UnitSphere.Y, UnitSphere.X));
    r32 YX = ASin(yv1.Y);
    r32 YY = ASin(yv2.Y);
    
    if(UnitSphere.Y > 0)
    {
      r32 Alpha = RayPlaneIntersection( V3(0,1,0), V3(0,1,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      Tx = Clamp(LinearRemap(PointOnPlane.Z, -1, 1,                     0, Bitmap->Width/3.f),                    0, Bitmap->Width/3.f-1);
      Ty = Clamp(LinearRemap(PointOnPlane.X,  1,-1,  Bitmap->Height / 2.f, Bitmap->Height   ), Bitmap->Height / 2.f, Bitmap->Height-1   );
      Platform.DEBUGPrint("+Y");
    }else{

      r32 Alpha = RayPlaneIntersection( V3(0,-1,0), V3(0,-1,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      Tx = Clamp(LinearRemap(PointOnPlane.Z, 1, -1,   2*Bitmap->Width/3.f, Bitmap->Width ), 2*Bitmap->Width/3.f, Bitmap->Width-1 );
      Ty = Clamp(LinearRemap(PointOnPlane.X, 1, -1,  Bitmap->Height / 2.f, Bitmap->Height),  Bitmap->Height/2.f, Bitmap->Height-1);
      Platform.DEBUGPrint("-Y");
    }
  } else if(AbsZ > AbsX && AbsZ > AbsY) {
    v2 zv1 = Normalize(V2(UnitSphere.Z, UnitSphere.Y));
    v2 zv2 = Normalize(V2(UnitSphere.Z, UnitSphere.X));
    r32 ZX = ASin(zv1.Y);
    r32 ZY = ASin(zv2.Y);

    if(UnitSphere.Z > 0) {
      r32 Alpha = RayPlaneIntersection( V3(0,0,1), V3(0,0,1), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      Tx = Clamp(LinearRemap(PointOnPlane.Y,  1, -1, Bitmap->Width/3, 2*Bitmap->Width/3.f), Bitmap->Width/3, 2*Bitmap->Width/3.f-1);
      Ty = Clamp(LinearRemap(PointOnPlane.X,  1, -1, Bitmap->Height / 2.f, Bitmap->Height), Bitmap->Height / 2.f, Bitmap->Height-1);
      Platform.DEBUGPrint("+Z");
    }else{
      r32 Alpha = RayPlaneIntersection( V3(0,0,-1), V3(0,0,-1), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      Tx = Clamp(LinearRemap(PointOnPlane.X,-1,  1, Bitmap->Width/3, 2*Bitmap->Width/3.f),Bitmap->Width/3, 2*Bitmap->Width/3.f-1);
      Ty = Clamp(LinearRemap(PointOnPlane.Y, 1, -1, 0, Bitmap->Height / 2.f),0, Bitmap->Height / 2.f-1);
      Platform.DEBUGPrint("-Z");  
    }
  } else {
    Platform.DEBUGPrint("!!\n");
  }
  //Platform.DEBUGPrint(" %1.4f [%1.4f,%1.4f,%1.4f] -> [%1.4f, %1.4f]\n",
  //  CosSin45,
  //  UnitSphere.X,UnitSphere.Y,UnitSphere.Z,
  //  Tx, Ty);
  //Platform.DEBUGPrint(" (%f %f) ", ACos(UnitSphere.X)* 180.0/Pi32 ,  ASin(UnitSphere.Y)* 180.0/Pi32 );
  //Platform.DEBUGPrint("-X, X %f %f, -Y, Y %f %f, -Z, Z %f %f ", AngleMX, AngleX,  AngleMY, AngleY,  AngleMZ, AngleZ);
  jwin_Assert(Tx >=0 && Tx < Bitmap->Width);
  jwin_Assert(Ty >=0 && Ty < Bitmap->Height);

  return {Tx, Ty};
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

void MapTgaSideToGlSide(
  u32 SrcWidth, u32 SrcHeight, u32* SrcPixels, 
  u32 SrcStartX, u32 SrcStartY, u32 SrcSideSize,
  u32 DstWidth, u32 DstHeight, u32* DstPixels,
  u32 DstStartX, u32 DstStartY, u32 DstSideSize,
  m2 Rotation)
{
  // Not implemented scaling for now
  jwin_Assert(SrcSideSize == DstSideSize); 
  for (int y = 0; y < SrcSideSize; ++y)
  {
    for (int x = 0; x < SrcSideSize; ++x)
    {
      u32 SrcPixelIdx = (SrcStartY+y) * SrcWidth + SrcStartX + x;

      r32 DstX = DstStartX + x;
      r32 DstY = DstStartY + y;

      DstX -= (DstStartX + DstSideSize/2);
      DstY -= (DstStartY + DstSideSize/2);
      jwin_Assert(DstX < DstSideSize/2.f && DstX >= -(r32)(DstSideSize/2.f));
      jwin_Assert(DstY < DstSideSize/2.f && DstY >= -(r32)(DstSideSize/2.f));
      DstX = Rotation.E[0] * DstX + Rotation.E[1] * DstY;
      DstY = Rotation.E[2] * DstX + Rotation.E[3] * DstY;
      jwin_Assert(DstX < DstSideSize/2.f && DstX >= -(r32)(DstSideSize/2.f));
      jwin_Assert(DstY < DstSideSize/2.f && DstY >= -(r32)(DstSideSize/2.f));
      DstX += DstStartX + DstSideSize/2;
      DstY += DstStartY + DstSideSize/2;
      u32 Rx = (u32)(DstX+0.5);
      u32 Ry = DstHeight - (u32)(DstY+0.5);
      jwin_Assert(Rx < DstWidth);
      jwin_Assert(Ry < DstHeight);
      u32 DstPixelIdx = Ry * DstWidth + Rx;
      DstPixels[DstPixelIdx] = SrcPixels[SrcPixelIdx];
    }
  }
}

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
      //RotateCamera(Camera, 2*Input->Mouse.dX, V3(0,-1,0) );
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

#define SKYBOXVERSION 0
#if SKYBOXVERSION == 0
  {
    local_persist opengl_bitmap SkyboxTexture = {};
    SkyboxTexture.BPP = 32;
    SkyboxTexture.Width = (u32) 256*3;
    SkyboxTexture.Height = 256*2;
    if(!SkyboxTexture.Pixels)
    {
      GlobalState->Skybox = GlLoadTexture(OpenGL, SkyboxTexture); // Has permanent data managed by the graphics-layer which we can update
      SkyboxTexture.Pixels = PushSize(GlobalPersistentArena, sizeof(u32) * SkyboxTexture.Width*SkyboxTexture.Height);
    }

    // Mouse picker
    v3 Up, Right, Forward;
    GetCameraDirections(Camera, &Up, &Right, &Forward);
    //Forward = -Normalize(V3(1.04*Cos(0.5*Input->Time),1.04*Sin(0.5*Input->Time),1));
    //Forward = -Normalize(V3(-1,-0.90,0.90));
    v2 tx = GetTextureCoordinateFromUnitSphereCoordinate(-Forward, &SkyboxTexture);
    //Platform.DEBUGPrint("[%1.4f,%1.4f,%1.4f] -> [%1.4f, %1.4f]\n",
    //-Forward.X,-Forward.Y,-Forward.Z,
    //tx.X, tx.Y);
    jwin_Assert(tx.X >=0 && tx.X < SkyboxTexture.Width);
    jwin_Assert(tx.Y >=0 && tx.Y < SkyboxTexture.Height);
    u32* Pixels = (u32*) SkyboxTexture.Pixels;
    u32 mx = (u32) Clamp(LinearRemap(Input->Mouse.X, 0, Camera->AspectRatio, 0, SkyboxTexture.Width), 0, SkyboxTexture.Width);
    u32 my = (u32) Clamp(Lerp(Input->Mouse.Y, 0, SkyboxTexture.Height),0,SkyboxTexture.Height);
    //Platform.DEBUGPrint("\n AA : [%1.2f, %1.2f] [%d %d] %f\n",Input->Mouse.X,Input->Mouse.Y,  mx,my, Camera->AspectRatio);
    //tx.X = 60 * Input->Time;
    //tx.Y = tx.X % SkyboxTexture.Width;
    //tx.X -= tx.Y * SkyboxTexture.Width;
    u32 TextureCoordIndex = (u32)( ((u32) tx.Y) * SkyboxTexture.Width + ((u32) tx.X));

    u32 TextureCoordIndex2 =(u32)( tx.Y * SkyboxTexture.Width +  tx.X);

    //Platform.DEBUGPrint("\n[%1.2f, %1.2f] [%d %d %d]\n",tx.X,tx.Y, TextureCoordIndex, TextureCoordIndex2, (s32)TextureCoordIndex - (s32)TextureCoordIndex2);
    //TextureCoordIndex =(u32)( my * SkyboxTexture.Width + mx);
    //TextureCoordIndex =(u32)( SkyboxTexture.Width * 60*Input->Time + 30);
    //jwin_Assert(TextureCoordIndex <= SkyboxTexture.Width * SkyboxTexture.Height);
    Pixels[TextureCoordIndex] = 0xffffffff;
    //Platform.DEBUGPrint("%d", TextureCoordIndex);
    GlUpdateTexture(OpenGL, GlobalState->Skybox, SkyboxTexture);
    //Platform.DEBUGPrint("%1.2f, %1.2f, %1.2f - %1.2f, %1.2f\n",
    //  -Forward.X,-Forward.Y,-Forward.Z, tx.X, tx.Y);
  }

#elif SKYBOXVERSION == 1
    if(!GlobalState->Skybox || 
      jwin::Pushed(Input->Keyboard.Key_L))
    {
      obj_bitmap* TgaSkybox = LoadTGA(GlobalTransientArena, "..\\data\\textures\\skybox3.tga");
      opengl_bitmap GlSkybox = MapObjBitmapToOpenGLBitmap(GlobalTransientArena, TgaSkybox);
      GlobalState->Skybox = GlLoadTexture(OpenGL, GlSkybox);


      // Given a skybox texture layout we need functions to map
      // For rendering:
      //  Pixel Coordinate to 3d Cube coordinate
      //  3D Cube coordinate to Pixel Coordinate
      // For drawing:
      //  Pixel Coordinate to 3d Sphere coordinate
      //  3d Sphere coordinate to Pixel Coordinate

      // In the program be able to draw or paste premade textures to a globe.
      // Project that globe on to a cube
      // Write it to file as a texture. Can be a bin-dump. Do a tga export if you are feeling fancy
      // and have the ability to further mod it in for example gimp.

      // Drawing:
      //   Given mouse screen coordinate and player orientation, Map that screen coordinate to a sphere centered on the player.
      //   Project a texture onto that sphere.
      //   Project that sphere onto a cube.
      //   Map that cube onto a texture.


      //  Need a convenient structure to do this mapping. Ie specify where the faces are in the texture
      //  that is easy to modify.
    }
#elif SKYBOXVERSION == 2 
    if(!GlobalState->Skybox || 
    jwin::Pushed(Input->Keyboard.Key_L))
    {
      obj_bitmap* TgaSkybox = LoadTGA(GlobalTransientArena, "..\\data\\textures\\skybox.tga");
      opengl_bitmap GlSkybox = {};
      GlSkybox.BPP = 32;
      GlSkybox.Width = (u32) 3*TgaSkybox->Width/4.f;
      GlSkybox.Height = TgaSkybox->Height;
      GlSkybox.Pixels = PushSize(GlobalTransientArena, sizeof(u32) * GlSkybox.Width*GlSkybox.Height);

      u32 SrcSideSize = TgaSkybox->Width/4;
      u32 DstSideSize = TgaSkybox->Width/4;

      u32 SrcStartX,SrcStartY,DstStartX,DstStartY;
      r32 Theta;
      for (int tgaSide = 0; tgaSide < 8; ++tgaSide)
      {
        // Rotation Matrix =
        // Cos(Theta), -Sin(Theta)
        // Sin(Theta),  Cos(Theta)
        switch(tgaSide)
        {

          // TGA is origin is lower left
          // GLSkybox is top Left
          case 0:
          case 3: continue;
          case 1: { // maps to Y  -> Lower Left
            SrcStartX = SrcSideSize;
            SrcStartY = SrcSideSize;
            DstStartX = 0;
            DstStartY = DstSideSize;
            Theta = -Pi32/2.f;
            Theta = 0;
          }break;
          case 2: { // maps to -Y -> Lower Right
            SrcStartX = 3*SrcSideSize;
            SrcStartY = 0;
            DstStartX = 2*DstSideSize;
            DstStartY = DstSideSize;
            Theta = Pi32/2.f;
            Theta = 0;
          }break;
          case 4: { // maps to -X -> Top Left
            SrcStartX = 0;
            SrcStartY = 0;
            DstStartX = 0;
            DstStartY = 0;
            Theta = 0;
          }break;
          case 5: { // maps to -Z -> Top Mid
            SrcStartX = SrcSideSize;
            SrcStartY = 0;
            DstStartX = DstSideSize;
            DstStartY = 0;
            Theta = 0;
          }break;
          case 6: { // maps to X -> Top Right
            SrcStartX = 2 * SrcSideSize;
            SrcStartY = 0;
            DstStartX = 2 * DstSideSize;
            DstStartY = 0;
            Theta = 0;
          }break;
          case 7: { // maps to -Z -> Bot Mid
            SrcStartX = 3 * SrcSideSize;
            SrcStartY = 0;
            DstStartX = DstSideSize;
            DstStartY = DstSideSize;
            Theta = 0;
          }break;
        }
        r32 Cos = cosf(Theta);
        r32 Sin = sinf(Theta);
        m2 Rotation = M2(
          Cos, -Sin,
          Sin,  Cos);
        MapTgaSideToGlSide(
          TgaSkybox->Width, TgaSkybox->Height, (u32*) TgaSkybox->Pixels, 
          SrcStartX, SrcStartY, SrcSideSize,
          GlSkybox.Width, GlSkybox.Height, (u32*) GlSkybox.Pixels,
          DstStartX, DstStartY, DstSideSize,
          Rotation);
      }
      GlobalState->Skybox = GlLoadTexture(OpenGL, GlSkybox);  
    }
#elif SKYBOXVERSION == 2
    if(!GlobalState->Skybox || 
    jwin::Pushed(Input->Keyboard.Key_L))
    {
      u32 Sides = 6;
      u32 SideSize = 256;
      void* Pixels = PushSize(GlobalTransientArena, sizeof(u32) * Sides*SideSize*SideSize);
      opengl_bitmap Skybox = {};
      Skybox.BPP = 32;
      Skybox.Width = SideSize*Sides/2;
      Skybox.Height = SideSize*Sides/3;
      Skybox.Pixels = Pixels;

      skybox_params Params = {};
      Params.SideSize = SideSize;
      Params.TextureWidth = Skybox.Width;
      Params.TextureHeight = Skybox.Height;
      // SideMapping: (Mapping done in graphics layer when setting up texture coordinates,
      //               At some point move that part here since theyre linked)
      //  Number is the index in the Colors array, x,y,z is the direction of the cube face normal
      //  ___________________
      //  |     |     |     |
      //  |0,-x |1,-z |2,+x |
      //  |_____|_____|_____|
      //  |     |     |     |
      //  |3,+y |4,+z |5,-y |
      //  |_____|_____|_____|

      // We want to iterate over each pixel in the cube and on that poin sample the sphere.
      // Given a pixel value, we want to find the corresponding coordinate on the cube
      // from that cube coordinate get the sphere coordinate

      u32* Pixel = (u32*) Pixels;
      for (int y = 0; y < Params.TextureHeight; ++y)
      {
        for (int x = 0; x < Params.TextureWidth; ++x)
        {
          v3 CubeCoord = GetCubeCoordinateFromTexture(x, y, &Params);
          v3 SphereCoord = GetSphereCoordinateFromCube(CubeCoord);
          u32 CubeColor = GetColorFromUnitVector(CubeCoord / Sqrt(3));
          u32 SphereColor = GetColorFromUnitVector(SphereCoord);
          u32 idx = x + y*Params.TextureWidth;
          Pixel[idx] = SphereColor;
        }
      }
      GlobalState->Skybox = GlLoadTexture(OpenGL, Skybox);  
    }
    #endif

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