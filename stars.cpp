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

void LoadShaders(application_render_commands* RenderCommands)
{
  debug_read_file_result phongVertex = Platform.DEBUGPlatformReadEntireFile("..\\jwin\\shaders\\PhongVertexCameraView.glsl");
  debug_read_file_result phongFragment = Platform.DEBUGPlatformReadEntireFile("..\\jwin\\shaders\\PhongFragmentCameraView.glsl");
  
  if(phongVertex.Contents && phongFragment.Contents)
  {
    RenderCommands->OpenGL.PhongVertexShaderCode = (char*) PushCopy(GlobalTransientArena, phongVertex.ContentSize, phongVertex.Contents);
    RenderCommands->OpenGL.PhongFragmentShaderCode = (char*) PushCopy(GlobalTransientArena, phongFragment.ContentSize, phongFragment.Contents);
  }

  if(phongVertex.Contents)
  {
    Platform.DEBUGPlatformFreeFileMemory(phongVertex.Contents);
  }
  if(phongFragment.Contents)
  {
    Platform.DEBUGPlatformFreeFileMemory(phongFragment.Contents);
  }

  debug_read_file_result starPlaneVertex = Platform.DEBUGPlatformReadEntireFile("..\\jwin\\shaders\\StarPlaneVertex.glsl");
  debug_read_file_result starPlaneFragment = Platform.DEBUGPlatformReadEntireFile("..\\jwin\\shaders\\StarPlaneFragment.glsl");
  if(starPlaneVertex.Contents && starPlaneFragment.Contents)
  {
    RenderCommands->OpenGL.StarPlaneVertexShaderCode = (char*) PushCopy(GlobalTransientArena, starPlaneVertex.ContentSize, starPlaneVertex.Contents);
    RenderCommands->OpenGL.StarPlaneFragmentShaderCode = (char*) PushCopy(GlobalTransientArena, starPlaneFragment.ContentSize, starPlaneFragment.Contents);
  }

  if(starPlaneVertex.Contents)
  {
    Platform.DEBUGPlatformFreeFileMemory(starPlaneVertex.Contents);
  }
  if(starPlaneFragment.Contents)
  {
    Platform.DEBUGPlatformFreeFileMemory(starPlaneFragment.Contents);
  }

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
  if(!GlobalState->Initialized)
  {
    obj_loaded_file* obj1 = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\checker_plane_simple.obj");
    obj_loaded_file* obj2 = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\sphere.obj");
    obj_bitmap* texture = LoadTGA(GlobalPersistentArena, "..\\data\\textures\\brick_wall_base.tga");

    // This memory only needs to exist until the data is loaded to the GPU
    RenderCommands->OpenGL.Plane  = MapObjToOpenGLMesh(GlobalTransientArena, obj1);
    RenderCommands->OpenGL.Sphere = MapObjToOpenGLMesh(GlobalTransientArena, obj2);
    RenderCommands->OpenGL.BaseColorTexturePlane = MapObjBitmapToOpenGLBitmap(GlobalTransientArena, obj1->MaterialData->Materials[0].MapKd);
    RenderCommands->OpenGL.BaseColorTextureSphere = MapObjBitmapToOpenGLBitmap(GlobalTransientArena, texture);

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

    LoadShaders(RenderCommands);

    RenderCommands->RenderGroup->LightPosition = V3(1, 1, 0);

  }
  r32 AspectRatio = RenderCommands->ScreenWidthPixels / (r32) RenderCommands->ScreenHeightPixels;
  ResetRenderGroup(RenderCommands->RenderGroup);
  r32 Angle = 22.5f*Sin(Input->Time) + 45 + 22.5;
  Angle = 90;

  r32 Ar = 0.5f*Sin(Input->Time) + 1;
  //LookAt(&GlobalState->Camera, V3(-1,2,3), V3(-1,1,0));


  //RenderCommands->RenderGroup->LightPosition = V3(0, 0, 11);
  RenderCommands->RenderGroup->LightColor = V3(1, 1, 1);
  camera* Camera = &GlobalState->Camera;
  //Platform.DEBUGPrint("%f %f %f\n", Sin(Input->Time) ,Cos(Input->Time), AspectRatio);
  local_persist r32 near = 1;
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
  

  if(jwin::Active(Input->Keyboard.Key_W))
  {
    TranslateCamera(Camera, V3(0,0,-0.1));
  }
  if(jwin::Active(Input->Keyboard.Key_S))
  {
    TranslateCamera(Camera, V3(0,0,0.1));
  }
  if(jwin::Active(Input->Keyboard.Key_A))
  {
    TranslateCamera(Camera, V3(-0.1,0,0));
  }
  if(jwin::Active(Input->Keyboard.Key_D))
  {
    TranslateCamera(Camera, V3(0.1,0,0));
  }
  if(jwin::Active(Input->Keyboard.Key_R))
  {
    TranslateCamera(Camera, V3(0,0.1,0));
  }
  if(jwin::Active(Input->Keyboard.Key_F))
  {
    TranslateCamera(Camera, V3(0,-0.1,0));
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

  if(jwin::Pushed(Input->Keyboard.Key_ENTER))
  {
    LoadShaders(RenderCommands);
  }

  UpdateViewMatrix(Camera);
  RenderCommands->RenderGroup->ProjectionMatrix = Camera->P;
  RenderCommands->RenderGroup->ViewMatrix = Camera->V;
  RenderCommands->RenderGroup->LightPosition = LightPosition;

  
}