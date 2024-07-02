#include "stars.h"

#include "platform/jwin_platform_memory.cpp"
#include "platform/jwin_platform_input.h"
#include "platform/jfont.cpp"
#include "platform/obj_loader.cpp"
#include "renderer/software_render_functions.cpp"
#include "renderer/render_push_buffer.cpp"
#include "math/AABB.cpp"
#include "camera.cpp"

#define SPOTCOUNT 120

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

u32 CreatePlaneStarProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneFragment.glsl"),
       "PlaneStar");
  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ViewMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "AccumTex",  GlUniformType::U32);
  GlDeclareUniform(OpenGL, ProgramHandle, "RevealTex",  GlUniformType::U32);
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::M4,   "ModelMat");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V4,   "Color");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32,  "Radius");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32,  "FadeDist");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V3,   "Center");
  return ProgramHandle;
}

u32 CreateSphereStarProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\StarSphereVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\StarSphereFragment.glsl"),
     "SphereStar");
  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32,  "Time");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::R32,  "Radius");
  GlDeclareInstanceVarying(OpenGL, ProgramHandle, GlUniformType::V2,   "StarPos");
  return ProgramHandle;
}

u32 CreateSolidSphereProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidSphereVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidSphereFragment.glsl"),
     "SolidSphere");
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
  NewKeeper->Handle = TextureIndex;
  NewKeeper->TextureData = TextureData;
  return TextureIndex;
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

ray_cast CastRay(camera* Camera, r32 Angle, r32 Width, r32 Length, v4 Color)
{
  ray_cast Result = {};
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
  Result.ModelMat = ModelMat;
  Result.Color = Color;
  Result.Radius = 3.f;
  Result.FaceDist = 2.f;
  Result.Center = V3(0,0,0);
  return Result;
}

void CastConeRays(application_render_commands* RenderCommands, jwin::device_input* Input, camera* Camera, v3 PointOnUitSphere, r32 AngleOnSphere, r32 MaxAngleOnSphere)
{
  ray_cast* Ray = PushStruct(GlobalTransientArena, ray_cast);

  m4 StaticAngle = QuaternionAsMatrix(GetRotation(V3(0,-1,0), PointOnUitSphere));

  m4 ModelMat = M4Identity();
  Translate(V4(0,-1,0,0), ModelMat);
  ModelMat = GetScaleMatrix(V4(Sin(AngleOnSphere)*1.01,1,Sin(AngleOnSphere+0.01)*1.01,1)) * ModelMat;
  ModelMat = StaticAngle*ModelMat;
  Ray->ModelMat = ModelMat;
  Ray->Color = V4(1, 1, 1, LinearRemap(MaxAngleOnSphere-AngleOnSphere, 0, MaxAngleOnSphere, 0.26,0.5));
  Ray->Radius = 2.f;
  Ray->FaceDist = 2.f;
  Ray->Center = V3(0,0,0);

  render_object* RayObj = PushNewRenderObject(RenderCommands->RenderGroup);
  RayObj->ProgramHandle = GlobalState->PlaneStarProgram;
  RayObj->MeshHandle = GlobalState->Cone;

  PushUniform(RayObj, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(RayObj, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);
  PushUniform(RayObj, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "AccumTex"), (u32)2);
  PushUniform(RayObj, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "RevealTex"), (u32)3);

  PushInstanceData(RayObj, 1, sizeof(ray_cast), (void*) Ray);
  PushRenderState(RayObj, DepthTestNoCulling);
  RayObj->Transparent = true;
}

void CastRays(application_render_commands* RenderCommands, jwin::device_input* Input, camera* Camera)
{
  r32 ThinRayCount = 15;
  r32 ThickRayCount = 7;
  ray_cast* Rays = PushArray(GlobalTransientArena, ThinRayCount + ThickRayCount, ray_cast);
  for (r32 i = 0; i < ThinRayCount; ++i)
  {
    r32 RayAngle = 0.06f*Input->Time + i*Tau32 / ThinRayCount;
    //Rays[(u32)i] = CastRay(Camera, RayAngle, 0.3, 4, V4(1-i/ThinRayCount, i/ThinRayCount, 0.81, 0.6));
    Rays[(u32)i] = CastRay(Camera, RayAngle, 0.3, 4, V4(1, 1, 1, 0.5));
  }
  for (r32 i = 0; i < ThickRayCount; ++i)
  {
    r32 RayAngle = -0.04*Input->Time + i*Tau32/ThickRayCount + Pi32/3.f + 0.03*Sin(Input->Time);
    //Rays[(u32)(i + ThinRayCount)] = CastRay(Camera, RayAngle, 1, 4, V4(i/ThickRayCount, 1-i/ThickRayCount, 0.81, 0.6));
    Rays[(u32)(i + ThinRayCount)] = CastRay(Camera, RayAngle, 1, 4, V4(1, 1, 1, 0.5));
  }

  render_object* Ray = PushNewRenderObject(RenderCommands->RenderGroup);
  Ray->ProgramHandle = GlobalState->PlaneStarProgram;
  Ray->MeshHandle = GlobalState->Triangle;
  Ray->Transparent = true;

  PushUniform(Ray, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(Ray, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);  
  PushUniform(Ray, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "AccumTex"), (u32)2);
  PushUniform(Ray, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "RevealTex"), (u32)3);
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

void DrawEruptionBands(application_render_commands* RenderCommands, jwin::device_input* Input, u32 ParamCounts, eruption_params* Params, m4 StarModelMat) {

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
        2*EruptionBands[BandCount-1].InnerRadii, EruptionSize);
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
    Param->RadiiIncrements[i] = Param->RadiiIncrements[i-1] + 2*GetRandomRealNorm(Generator);
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

// void ApplicationUpdateAndRender(application_memory* Memory, application_render_commands* RenderCommands, jwin::device_input* Input)
extern "C" JWIN_UPDATE_AND_RENDER(ApplicationUpdateAndRender)
{
  GlobalState = JwinBeginFrameMemory(application_state);

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
    GlobalState->EruptionBandProgram = CreateEruptionBandProgram(OpenGL);
    GlobalState->Cube = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cube));
    GlobalState->Plane = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, plane));
    GlobalState->Sphere = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, sphere));
    GlobalState->Cone = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cone));
    GlobalState->Cylinder = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cylinder));
    GlobalState->Triangle = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, triangle));
    GlobalState->Billboard = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena,  billboard));
    GlobalState->PlaneTexture = GlLoadTexture(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, plane->MaterialData->Materials[0].MapKd));
    GlobalState->SphereTexture = GlLoadTexture(OpenGL, MapObjBitmapToOpenGLBitmap(GlobalTransientArena, texture));
    

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
    GlobalState->PhongProgram = GlReloadProgram(OpenGL, GlobalState->PhongProgram,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"));
    GlobalState->PhongProgramNoTex = GlReloadProgram(OpenGL, GlobalState->PhongProgramNoTex,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));
    GlobalState->PlaneStarProgram = GlReloadProgram(OpenGL, GlobalState->PlaneStarProgram,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarPlaneFragment.glsl"));
    GlobalState->SphereStarProgram = GlReloadProgram(OpenGL, GlobalState->SphereStarProgram,
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarSphereVertex.glsl"),
      1, LoadShaderFromDisk("..\\jwin\\shaders\\StarSphereFragment.glsl"));
    GlobalState->SolidSphereProgram = GlReloadProgram(OpenGL, GlobalState->SolidSphereProgram,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidSphereVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\SolidSphereFragment.glsl"));
    GlobalState->EruptionBandProgram = GlReloadProgram(OpenGL, GlobalState->EruptionBandProgram,
     1, LoadShaderFromDisk("..\\jwin\\shaders\\EruptionBandVertex.glsl"),
     1, LoadShaderFromDisk("..\\jwin\\shaders\\EruptionBandFragment.glsl"));
  }

  UpdateViewMatrix(Camera);

  

  // Model data
  local_persist b32 ModelLoaded = false;
  local_persist m4 Plane = M4Identity();
  local_persist m4 ModelMatSphere = M4Identity();
  local_persist m4 ModelMatSphere2 = M4Identity();
  if(!ModelLoaded)
  {
    // Plane
    Scale( V4(10,0.1,10,0), Plane );
    Translate( V4(0,-1,0,0), Plane);
    Translate( V4(-9,-1,0,0), ModelMatSphere);
    Translate( V4(-9,1,0,0), ModelMatSphere2);
    ModelLoaded = true;
  }

  v3 LightDirection = V3(Transpose(RigidInverse(Camera->V)) * V4(LightPosition,0));

#if 1
  // Floor
  {
    m4 ModelViewPlane = Camera->V*Plane;
    m4 NormalViewPlane = Camera->V*Transpose(RigidInverse(Plane));

    render_object* Floor = PushNewRenderObject(RenderCommands->RenderGroup);
    Floor->ProgramHandle = GlobalState->PhongProgram;
    Floor->MeshHandle = GlobalState->Plane;
    Floor->TextureHandle = GlobalState->PlaneTexture;

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
#endif
  // Textured Sphere
  {
    Rotate( 1/120.f, -V4(0,1,0,0), ModelMatSphere );

    m4 ModelViewSphere = Camera->V*ModelMatSphere;
    m4 NormalViewSphere = Camera->V*Transpose(RigidInverse(ModelMatSphere));

    render_object* Sphere = PushNewRenderObject(RenderCommands->RenderGroup);
    Sphere->ProgramHandle = GlobalState->PhongProgram;
    Sphere->MeshHandle = GlobalState->Sphere;
    Sphere->TextureHandle = GlobalState->SphereTexture;

    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ModelView"), ModelViewSphere);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "NormalView"), NormalViewSphere);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightDirection"), LightDirection);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightColor"), V3(1,1,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialAmbient"), V4(0.2,0.2,0.2,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialDiffuse"), V4(0.5,0.5,0.5,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialSpecular"), V4(0.75,0.75,0.75,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "Shininess"), (r32) 20);
    PushRenderState(Sphere, DepthTestCulling);
  }

  // Textured Sphere
  {
    Rotate( 1/120.f, V4(0,1,0,0), ModelMatSphere2 );

    m4 ModelViewSphere = Camera->V*ModelMatSphere2;
    m4 NormalViewSphere = Camera->V*Transpose(RigidInverse(ModelMatSphere2));

    render_object* Sphere = PushNewRenderObject(RenderCommands->RenderGroup);
    Sphere->ProgramHandle = GlobalState->PhongProgram;
    Sphere->MeshHandle = GlobalState->Sphere;
    Sphere->TextureHandle = GlobalState->SphereTexture;

    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "ModelView"), ModelViewSphere);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "NormalView"), NormalViewSphere);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightDirection"), LightDirection);
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "LightColor"), V3(1,1,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialAmbient"), V4(0.2,0.2,0.2,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialDiffuse"), V4(0.5,0.5,0.5,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "MaterialSpecular"), V4(0.75,0.75,0.75,1));
    PushUniform(Sphere, GlGetUniformHandle(OpenGL, GlobalState->PhongProgram, "Shininess"), (r32) 20);
    PushRenderState(Sphere, DepthTestCulling);
  }

  local_persist r32 Time = 0;
  local_persist r32 Angle2 = 0;


  // Bigest Sphere
  m4 FMSS = {};
  {
    render_object* FinalSphere = PushNewRenderObject(RenderCommands->RenderGroup);
    FinalSphere->ProgramHandle = GlobalState->SolidSphereProgram;
    FinalSphere->MeshHandle = GlobalState->Sphere;
    r32 FinalSize = 1;
    r32 FinalSizeOscillation = FinalSize * ( 1 + 0.01* Sin(0.05*Input->Time));
    FMSS = GetScaleMatrix(V4(FinalSizeOscillation,FinalSizeOscillation,FinalSizeOscillation,1));

    PushUniform(FinalSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
    PushUniform(FinalSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*FMSS);
    PushUniform(FinalSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(45.0/255.0, 51.0/255, 197.0/255.0, 1));
    PushRenderState(FinalSphere, DepthTestCulling);
  }

  // Second Largest Sphere
  m4 MSS = {};
  {
    render_object* LargeSphere = PushNewRenderObject(RenderCommands->RenderGroup);
    LargeSphere->ProgramHandle = GlobalState->SolidSphereProgram;
    LargeSphere->MeshHandle = GlobalState->Sphere;
    r32 LargeSize = 0.95;
    r32 LargeSizeOscillation = LargeSize * ( 1 + 0.02* Sin(0.1 * Input->Time+ 1.1));
    MSS = GetScaleMatrix(V4(LargeSizeOscillation,LargeSizeOscillation,LargeSizeOscillation,1));

    PushUniform(LargeSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
    PushUniform(LargeSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*MSS);
    PushUniform(LargeSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(56.0/255.0, 75.0/255, 220.0/255.0, 1));
    PushRenderState(LargeSphere, DepthTestCulling);
  }

  {
    render_object* MediumSphere = PushNewRenderObject(RenderCommands->RenderGroup);
    MediumSphere->ProgramHandle = GlobalState->SolidSphereProgram;
    MediumSphere->MeshHandle = GlobalState->Sphere;
    r32 MediumSize = 0.85;
    r32 MediumScaleOccilation = MediumSize * ( 1 + 0.02* Sin(Input->Time+Pi32/4.f));
    MSS = GetScaleMatrix(V4(MediumScaleOccilation,MediumScaleOccilation,MediumScaleOccilation,1));

    PushUniform(MediumSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
    PushUniform(MediumSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*MSS);
    PushUniform(MediumSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(57.0/255.0, 110.0/255, 247.0/255.0, 1));
    PushRenderState(MediumSphere, NoDepthTestCulling);
  }

  // Smallest Sphere
  {
    render_object* SmallSphere = PushNewRenderObject(RenderCommands->RenderGroup);
    SmallSphere->ProgramHandle = GlobalState->SolidSphereProgram;
    SmallSphere->MeshHandle = GlobalState->Sphere;
    r32 SmallSize = 0.65;
    r32 SmallScaleOccilation = SmallSize * ( 1 + 0.03* Sin(Input->Time+3/4.f *Pi32));
    MSS = GetScaleMatrix(V4(SmallScaleOccilation,SmallScaleOccilation,SmallScaleOccilation,1));

    PushUniform(SmallSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ProjectionMat"), Camera->P);
    PushUniform(SmallSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "ModelView"), Camera->V*MSS);
    PushUniform(SmallSphere, GlGetUniformHandle(OpenGL, GlobalState->SolidSphereProgram, "Color"), V4(107.0/255.0, 196.0/255, 1, 1));
    PushRenderState(SmallSphere, NoDepthTestCulling);
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

      r32 Theta = GetRandomReal(&GlobalState->RandomGenerator, AngleSpans[EmptiestRegionIndex][0], AngleSpans[EmptiestRegionIndex][2]);
      r32 Phi   = GetRandomReal(&GlobalState->RandomGenerator, AngleSpans[EmptiestRegionIndex][1], AngleSpans[EmptiestRegionIndex][3]);
      if(GetRandomRealNorm(&GlobalState->RandomGenerator) < 0.7) {
        InitializeEruption(Param, &GlobalState->RandomGenerator, ArrayCount(Colors1), Colors1,0.1, 0.275, 0.7, Theta, Phi);
      }else{
        InitializeEruption(Params + i, &GlobalState->RandomGenerator, ArrayCount(Colors2), Colors2, 0.05, 0.14, 0, Theta, Phi);
      }
      Param->Time     = GetRandomReal(&GlobalState->RandomGenerator, 0, Param->Duration);
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
      r32 Theta = GetRandomReal(&GlobalState->RandomGenerator, AngleSpans[EmptiestRegionIndex][0], AngleSpans[EmptiestRegionIndex][2]);
      r32 Phi   = GetRandomReal(&GlobalState->RandomGenerator, AngleSpans[EmptiestRegionIndex][1], AngleSpans[EmptiestRegionIndex][3]);
      eruption_params* Param = Params + i;
      if(GetRandomRealNorm(&GlobalState->RandomGenerator) < 0.7)
      {
        InitializeEruption(Param, &GlobalState->RandomGenerator, ArrayCount(Colors1), Colors1,  0.1, 0.275, 0.7, Theta, Phi);
      }else{
        InitializeEruption(Param, &GlobalState->RandomGenerator, ArrayCount(Colors2), Colors2, 0.05, 0.14, 0, Theta, Phi);
      }
      FillRates[EmptiestRegionIndex] += Param->EruptionSize;
      Param->RegionIndex = EmptiestRegionIndex;
    }
  }

  for (int i = 0; i < ArrayCount(FillRates); ++i)
  {
    Platform.DEBUGPrint("%1.2f ", FillRates[i]);
  }
  Platform.DEBUGPrint("\n");
  DrawEruptionBands(RenderCommands, Input,SPOTCOUNT, Params, FMSS);
  
  // Ray
  CastRays(RenderCommands, Input, Camera);

  // Halo
  {
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
    Halo->Transparent = true;

    PushUniform(Halo, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
    PushUniform(Halo, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);
    PushUniform(Halo, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "AccumTex"), (u32)2);
    PushUniform(Halo, GlGetUniformHandle(&RenderCommands->OpenGL, GlobalState->PlaneStarProgram, "RevealTex"), (u32)3);
    ray_cast* HaloRay = PushStruct(GlobalTransientArena, ray_cast);
    HaloRay->ModelMat = HaloModelMat;
    HaloRay->Color = V4(254.0/255.0, 254.0/255.0, 255/255, 0.3);
    HaloRay->Radius = (r32)( 1.3f + 0.07 * Sin(Input->Time));
    HaloRay->FaceDist =  0.3f;
    HaloRay->Center = V3(0,0,0);
    PushInstanceData(Halo, 1, sizeof(ray_cast), (void*) HaloRay);
    PushRenderState(Halo, DepthTestCulling);
  }

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