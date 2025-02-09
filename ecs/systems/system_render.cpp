#include "ecs/systems/system_render.h"
//#include "math/vector_math.h"
namespace ecs::render {

void PushStringToGpu(render_group* RenderGroup, render_object* RenderObject, jfont::sdf_font* Font, jfont::sdf_atlas* FontAtlas, r32 X0, r32 Y0, r32 RelativeScale, utf8_byte Text[])
{
  codepoint TextString[1024] = {};
  u32 UnicodeLen = ConvertToUnicode(Text, TextString);
  jfont::print_coordinates* TextPrintCoordinates = PushArray(GlobalTransientArena, UnicodeLen, jfont::print_coordinates);
  data::overlay_text* GlText = PushArray(&RenderGroup->Arena, UnicodeLen, data::overlay_text);
  GetTextPrintCoordinates(Font, FontAtlas, RelativeScale, X0, Y0, TextString, TextPrintCoordinates);

  for (int i = 0; i < UnicodeLen; ++i)
  {
    jfont::print_coordinates* tc = TextPrintCoordinates+i;
    data::overlay_text* Gltc = GlText+i;
    Gltc->TextCoord = V4(tc->u0, tc->v0, tc->u1, tc->v1);
    Gltc->ModelMatrix = M4Identity();
    Scale(V4(tc->sx, tc->sy,1,0), Gltc->ModelMatrix);
    Translate(V4(tc->x,tc->y, 0, 1), Gltc->ModelMatrix);
    Gltc->ModelMatrix = Transpose(Gltc->ModelMatrix);
  }

  PushInstanceData(RenderObject, UnicodeLen, UnicodeLen*sizeof(data::overlay_text), (void*) GlText);
}

// How I want it to work: 3 sceen space coordinate systems. Origin is always lower left.
//  - Pixel Space. (self explanatory)
//  - Canonical Space. Here the height goes from 0 to 1 and width from 0 to aspect ratio, Aspect ratio is Width / Height
//  - Real space or metric space. This is in cm or inches. This is dependant on screen resolution.
//  Things we need to know is. These should be available in the render commands struct and passed down to the render_push_buffer.
//  - Screen size in Real space, how many inches.
//  - Screen resolution, how many pixels per inch
//  - Render resolution. How big is the Opengl viewport.
//  - Window resolution. How big is the window we are rendering to?


inline r32 PixelToCanonicalWidth(system* System, r32 X)
{
  r32 Result = LinearRemap(X, 0, System->WindowSize.ApplicationWidth,  0, System->WindowSize.ApplicationAspectRatio);
  return Result;
}

inline r32 PixelToCanonicalHeight(system* System, r32 Y)
{
  r32 Result = LinearRemap(Y, 0, System->WindowSize.ApplicationHeight, 0, 1);
  return Result;
}

inline v2 PixelToCanonicalSpace(system* System, v2 PixelPos)
{
  v2 CanonicalPos = V2( PixelToCanonicalWidth(System, PixelPos.X),
                        PixelToCanonicalHeight(System, PixelPos.Y));
  return CanonicalPos;
}

inline r32 CanonicalToPixelWidth(system* System, r32 X)
{
  r32 Result = LinearRemap(X, 0, System->WindowSize.ApplicationAspectRatio,  0, System->WindowSize.ApplicationWidth);
  return Result;
}

inline r32 CanonicalToPixelHeight(system* System, r32 Y)
{
  r32 Result = LinearRemap(Y, 0, 1, 0, System->WindowSize.ApplicationHeight);
  return Result;
}

inline v2 CanonicalToPixelSpace(system* System, v2 CanPos)
{
  v2 PixelPos = V2( CanonicalToPixelWidth(System, CanPos.X),
                        CanonicalToPixelHeight(System, CanPos.Y));
  return PixelPos;
}

r32 GetLineSpacingPixelSpace(system* System, r32 PixelSize)
{
  r32 SizePixel = jfont::GetLineSpacingPixelSpace(&System->Font.Font, PixelSize);
  return SizePixel;
}

r32 GetLineSpacingCanonicalSpace(system* System, r32 PixelSize)
{
  r32 SizePixel = jfont::GetLineSpacingPixelSpace(&System->Font.Font, PixelSize);
  r32 Result = PixelToCanonicalHeight(System, SizePixel);
  return Result;
}

internal inline r32 GetScaleFromPixelSize(system* System, r32 PixelSize)
{
  r32 Result = jfont::GetScaleFromPixelSize(&System->Font.Font, PixelSize);
  return Result;
}

v2 GetTextSizePixelSpace(system* System, r32 PixelSize, utf8_byte const * Text)
{ 
  SCOPED_TRANSIENT_ARENA;
  r32 FontRelativeScale = GetScaleFromPixelSize(System, PixelSize);
  u32 Length = jstr::StringLength((const char*)Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, Length+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode((utf8_byte*) Text, CodePoints);
  v2 Result = {};
  jfont::GetTextDim(&System->Font.Font, FontRelativeScale, &Result.X, &Result.Y, CodePoints);
  return Result;
}

v2 GetTextSizeCanonicalSpace(system* System, r32 PixelSize, utf8_byte const * Text)
{
  v2 PixelPos = GetTextSizePixelSpace(System, PixelSize, Text);
  v2 CanonicalPos = PixelToCanonicalSpace(System, PixelPos);
  return CanonicalPos;
}

void DrawTextPixelSpace(system* System, v2 PixelPos, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  SCOPED_TRANSIENT_ARENA;
  u32 Length = jstr::StringLength((const char*) Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, Length+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode(Text, CodePoints);
  r32 RelativeScale =  GetScaleFromPixelSize(System, PixelSize);
  jfont::print_coordinates* TextPrintCoordinates = PushArray(GlobalTransientArena, UnicodeLen, jfont::print_coordinates);
  jfont::GetTextPrintCoordinates(&System->Font.Font, &System->Font.FontAtlas, RelativeScale, PixelPos.X, PixelPos.Y, CodePoints, TextPrintCoordinates);

  for (int i = 0; i < UnicodeLen; ++i)
  {
    jfont::print_coordinates* tc = TextPrintCoordinates+i;
    data::overlay_text OverlayText = {};
    OverlayText.TextCoord = V4(tc->u0, tc->v0, tc->u1, tc->v1);
    OverlayText.ModelMatrix = M4Identity();
    Scale(V4(tc->sx, tc->sy,1,0), OverlayText.ModelMatrix);
    Translate(V4(tc->x,tc->y, 0, 1), OverlayText.ModelMatrix);
    OverlayText.ModelMatrix = Transpose(OverlayText.ModelMatrix);
    OverlayText.Color = Color;
    Push(&System->Arena, &System->OverlayText, (bptr)&OverlayText);
  }
}

void DrawTextCanonicalSpace(system* System, v2 CanonicalPos, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  v2 PixelPos = CanonicalToPixelSpace(System, CanonicalPos);
  DrawTextPixelSpace(System, PixelPos, PixelSize, Text, Color);
}


void DrawOverlayQuadPixelSpace(system* System, rect2f PixelRect, v4 Color)
{
  m4 ModelMatrix = M4Identity();
  Scale(V4(0.5,0.5, 0, 1), ModelMatrix);
  Scale(V4(PixelRect.W,PixelRect.H,0,1), ModelMatrix);
  Translate(V4(PixelRect.X, PixelRect.Y, 0, 0), ModelMatrix);
  ModelMatrix = Transpose(ModelMatrix);

  data::overlay_quad Quad = {};
  Quad.Color = Color;
  Quad.ModelMatrix = ModelMatrix;
  
  Push(&System->Arena, &System->OverlayQuads, (bptr)&Quad);
}

void DrawOverlayQuadCanonicalSpace(system* System, rect2f CanonicalRect, v4 Color)
{
  rect2f PixelRect = Rect2f( CanonicalToPixelSpace(System, V2(CanonicalRect.X,CanonicalRect.Y)),
                             CanonicalToPixelSpace(System, V2(CanonicalRect.W,CanonicalRect.H)));

  DrawOverlayQuadPixelSpace(System, PixelRect, Color);
}

// Note: BinomialDepth must be even.
// CutOff must be less than half BinomialDepth
u32 GetGaussianKernel(u32 BinomialDepth, u32 CutOff, r32* OutOffset, r32* OutWeight)
{
  r32 CoefficientsA[1028] = {};
  r32 CoefficientsB[1028] = {};
  r32 Offset[1028] = {};
  r32* Current = CoefficientsA;
  r32* Previous = CoefficientsB;
  for (int i = 0; i <= BinomialDepth; ++i)
  {
    if(i > 0)
    {
      for (int j = 0; j <= i; ++j)
      {
        if(j == 0)
        {
          Current[0] = Previous[0];
        }else if (j == i)
        {
          Current[j] = Previous[i-1];
        }else{
          Current[j] = Previous[j] + Previous[j-1];
        }
      }  
    }else{
      Current[0] = 1;
    }
    r32* Tmp = Previous;
    Previous = Current;
    Current = Tmp;
  }
  
  for (int i = CutOff; i <= BinomialDepth-CutOff; ++i)
  {
    Current[i-CutOff] = Previous[i];
  }

  u32 ReducedSize = BinomialDepth-2*CutOff + 1;
  r32 Sum = 0;
  for (int i = 0; i < ReducedSize; ++i)
  {
    Sum += Current[i];
  }

  for (int i = 0; i < ReducedSize; ++i)
  {
    Current[i] /= Sum;
  }

  u32 ReducedHalfSize = ReducedSize / 2 + 1;

  
  r32* Tmp = Previous;
  Previous = Current;
  Current = Tmp;
  for (int i = 0; i < ReducedHalfSize; ++i)
  {
    Offset[i] = i;
    Current[ReducedHalfSize - 1 - i] = Previous[i];
  }

  u32 Size = ReducedHalfSize/2 + 1;
  Tmp = Previous;
  Previous = Current;
  Current = Tmp;
  OutWeight[0] = Previous[0];
  for (int i = 1; i < Size; ++i)
  {
    u32 idx = 2*i-1;
    OutWeight[i] = Previous[idx] + Previous[idx+1];
    OutOffset[i] = (Previous[idx] * Offset[idx] + Previous[idx+1] * Offset[idx+1]) / OutWeight[i];
  }

  return Size;
}


void PushRenderObject(render_group* RenderGroup, component* Render, u32 Program, u32 FrameBuffer, m4& ProjectionMatrix, m4& ViewMatrix,
  v3 LightDirection, v3 LightColor)
{
  entity_id EntityId = GetEntityIDFromComponent( (bptr) Render );
  ecs::position::component* Position =  GetPositionComponent(&EntityId);

  render_object* Object = PushNewRenderObject(RenderGroup);
  Object->ProgramHandle = Program;
  Object->FrameBufferHandle = FrameBuffer;
  Object->MeshHandle = Render->MeshHandle;
  Object->TextureCount = 1;
  Object->TextureHandles[0] = Render->TextureHandle;

  m4 Scale = GetScaleMatrix(V4(Render->Scale,1));
  m4 Rotation = GetRotationMatrix(GetAbsoluteRotation(Position), -V4(0,1,0,0));
  m4 Translation = GetTranslationMatrix(V4(GetAbsolutePosition(Position),1));
  m4 ModelMat =  Translation*Rotation*Scale;
  //Rotate( GetAbsoluteRotation(Position), -V4(0,1,0,0), ModelMat );

  m4 ModelView = ViewMatrix*ModelMat;
  m4 NormalView = Transpose(RigidInverse(ModelView));
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "ProjectionMat"), ProjectionMatrix);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "ModelView"), ModelView);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "NormalView"), NormalView);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "LightDirection"), LightDirection);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "LightColor"), LightColor);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialAmbient"), Render->Material.Ambient);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialDiffuse"), Render->Material.Diffuse);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialSpecular"), Render->Material.Specular);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "Shininess"), Render->Material.Shininess);

}

void Draw(entity_manager* EntityManager, system* RenderSystem, m4 ProjectionMatrix, m4 ViewMatrix)
{
  render_group* RenderGroup = RenderSystem->RenderGroup;

  v3 LightColor = V3(1,1,1);
  v3 LightPosition = V3(1,1,1);
  v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPosition,0));

  filtered_entity_iterator EntityIterator = GetComponentsOfType(EntityManager, flag::RENDER);

  chunk_list SolidObjects = NewChunkList(GlobalTransientArena, sizeof(component**), 32);
  chunk_list TransparentObjects = NewChunkList(GlobalTransientArena, sizeof(component**), 32);

  while(Next(&EntityIterator))
  {
    component* Component = GetRenderComponent(&EntityIterator);
    if(Component->Material.Ambient.W < 1)
    {
      Push(GlobalTransientArena, &TransparentObjects, (bptr) &Component);
    }else{
      Push(GlobalTransientArena, &SolidObjects, (bptr) &Component);
    }
  }

  // Some Gaussian Blur just cause I can
  r32* KernelOffset = PushArray(GlobalTransientArena, 64, r32);
  r32* KernelWeight = PushArray(GlobalTransientArena, 64, r32);
  u32 KernelSize = GetGaussianKernel(12, 2, KernelOffset, KernelWeight);

  window_size_pixel* Window = &RenderSystem->WindowSize;

  u32 MSAA = 4;
  render_state* DefaultState = PushNewState(RenderGroup);
  *DefaultState = DefaultRenderState3(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio);

  clear_operation* DefClearColor = PushNewClearOperation(RenderGroup);
  DefClearColor->BufferType = OPEN_GL_COLOR;
  DefClearColor->FrameBufferHandle = GlobalState->DefaultFrameBuffer;
  DefClearColor->TextureIndex = 0;
  DefClearColor->Color = V4(0,0,0,1);

  clear_operation* DefClearDepth = PushNewClearOperation(RenderGroup);
  DefClearDepth->BufferType = OPEN_GL_DEPTH;
  DefClearDepth->FrameBufferHandle = GlobalState->DefaultFrameBuffer;
  DefClearDepth->TextureIndex = 0;
  DefClearDepth->Depth = 1;

  clear_operation* ClearMSAAColor = PushNewClearOperation(RenderGroup);
  ClearMSAAColor->BufferType = OPEN_GL_COLOR;
  ClearMSAAColor->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
  ClearMSAAColor->TextureIndex = 0;
  ClearMSAAColor->Color = V4(0,0,0,1);

  clear_operation* ClearMSAADepth = PushNewClearOperation(RenderGroup);
  ClearMSAADepth->BufferType = OPEN_GL_DEPTH;
  ClearMSAADepth->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
  ClearMSAADepth->TextureIndex = 0;
  ClearMSAADepth->Depth = 1;

  clear_operation* TransparenClearOp0 = PushNewClearOperation(RenderGroup);
  TransparenClearOp0->BufferType = OPEN_GL_COLOR;
  TransparenClearOp0->FrameBufferHandle = GlobalState->TransparentFrameBuffer;
  TransparenClearOp0->TextureIndex = 0;
  TransparenClearOp0->Color = V4(0,0,0,0);

  clear_operation* TransparenClearOp1 = PushNewClearOperation(RenderGroup);
  TransparenClearOp1->BufferType = OPEN_GL_COLOR;
  TransparenClearOp1->FrameBufferHandle = GlobalState->TransparentFrameBuffer;
  TransparenClearOp1->TextureIndex = 1;
  TransparenClearOp1->Color = V4(1,0,0,0);

  // First draw solid objects
  chunk_list_iterator SolidIt = BeginIterator(&SolidObjects);
  while(Valid(&SolidIt)) {
    component** RenderPtr = (component**) Next(&SolidIt);
    PushRenderObject(RenderGroup, *RenderPtr, GlobalState->PhongProgram, GlobalState->MsaaFrameBuffer, ProjectionMatrix, ViewMatrix, LightDirection, LightColor);
  }


  // move to transparent drawing
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

  // Then draw Transparent objects
    // First draw solid objects
  chunk_list_iterator TransparentIt = BeginIterator(&TransparentObjects);
  while(Valid(&TransparentIt)) {
    component** RenderPtr = (component**) Next(&TransparentIt);
    PushRenderObject(RenderGroup, *RenderPtr, GlobalState->PhongProgramTransparent, GlobalState->TransparentFrameBuffer, ProjectionMatrix, ViewMatrix, LightDirection, LightColor);
  }

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
  render_object* CompositionObject = PushNewRenderObject(RenderGroup);
  CompositionObject->ProgramHandle = GlobalState->TransparentCompositionProgram;
  CompositionObject->MeshHandle = GlobalState->BlitPlane;
  CompositionObject->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
  CompositionObject->TextureHandles[0] = GlobalState->AccumTexture;
  CompositionObject->TextureHandles[1] = GlobalState->RevealTexture;
  CompositionObject->TextureCount = 2;

  PushUniform(CompositionObject, GetUniformHandle(RenderGroup, GlobalState->TransparentCompositionProgram,  "AccumTex"), (u32)0);
  PushUniform(CompositionObject, GetUniformHandle(RenderGroup, GlobalState->TransparentCompositionProgram , "RevealTex"), (u32)1);

  // Shrink to regular screeen sice
  render_state* ViewportAndBlend = PushNewState(RenderGroup);
  ///SetState(ViewportAndBlend, ViewportState(Window->ApplicationWidth, Window->ApplicationHeight, Window->ApplicationAspectRatio));
  SetState(ViewportAndBlend, ViewportState(Window->WindowWidth, Window->WindowHeight, Window->ApplicationAspectRatio));
  SetState(ViewportAndBlend, DefaultBlendState());

  // Gaussian blur

  blit_operation* BlitOperation = PushNewBlitOperation(RenderGroup);
  BlitOperation->ReadFrameBufferHandle = GlobalState->MsaaFrameBuffer;
  
#if 1
  BlitOperation->DrawFrameBufferHandle = GlobalState->DefaultFrameBuffer;
#else
  BlitOperation->DrawFrameBufferHandle = GlobalState->GaussianAFrameBuffer;

  for (int i = 0; i < 4; ++i)
  {
    render_object* GaussianBlurX = PushNewRenderObject(RenderGroup);
    GaussianBlurX->ProgramHandle = GlobalState->GaussianProgramX;
    GaussianBlurX->MeshHandle = GlobalState->BlitPlane;
    GaussianBlurX->FrameBufferHandle = GlobalState->GaussianBFrameBuffer;
    GaussianBlurX->TextureHandles[0] = GlobalState->GaussianATexture;
    GaussianBlurX->TextureCount = 1;

    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "offset"), UniformType::R32, KernelOffset, KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "weight"), UniformType::R32, KernelWeight, KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "kernerlSize"), KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "RenderedTexture"), (u32) 0);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "sideSize"), V2(Window->ApplicationWidth, Window->ApplicationHeight));

    render_object* GaussianBlurY = PushNewRenderObject(RenderGroup);
    GaussianBlurY->ProgramHandle = GlobalState->GaussianProgramY;
    GaussianBlurY->MeshHandle = GlobalState->BlitPlane;
    GaussianBlurY->FrameBufferHandle = GlobalState->GaussianAFrameBuffer;
    GaussianBlurY->TextureHandles[0] = GlobalState->GaussianBTexture;
    GaussianBlurY->TextureCount = 1;
    
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "offset"), UniformType::R32, KernelOffset, KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "weight"), UniformType::R32, KernelWeight, KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "kernerlSize"), KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "RenderedTexture"), (u32) 0);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "sideSize"), V2(Window->ApplicationWidth, Window->ApplicationHeight));
  }
  blit_operation* BlitOperation2 = PushNewBlitOperation(RenderGroup);
  BlitOperation2->ReadFrameBufferHandle = GlobalState->GaussianBFrameBuffer;
  BlitOperation2->DrawFrameBufferHandle = GlobalState->DefaultFrameBuffer;
#endif
  depth_state OverlayDepthState = {};
  OverlayDepthState.TestActive = false;
  OverlayDepthState.WriteActive = false;
  SetState(CompositState, OverlayDepthState);
  m4 OrthoProjectionMatrix = GetOrthographicProjection(-1, 1, Window->ApplicationWidth, 0, Window->ApplicationHeight, 0);

  chunk_list_iterator OverlayQuadsIT = BeginIterator(&RenderSystem->OverlayQuads);
  u32 QuadCount = GetBlockCount(&RenderSystem->OverlayQuads);
  if(QuadCount)
  {
    render_object* QuadObject = PushNewRenderObject(RenderGroup);
    QuadObject->ProgramHandle = GlobalState->ColoredSquareOverlayProgram;
    QuadObject->MeshHandle = GlobalState->BlitPlane;
    QuadObject->FrameBufferHandle = GlobalState->DefaultFrameBuffer;
    u32 i = 0;
    data::overlay_quad* QuadInstanceData = PushArray(GlobalTransientArena, QuadCount, data::overlay_quad);
    while(Valid(&OverlayQuadsIT)) {
      data::overlay_quad* OverlayQuad = (data::overlay_quad*) Next(&OverlayQuadsIT);

      QuadInstanceData[i++] = *OverlayQuad;
    }
    
    PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "Projection"), OrthoProjectionMatrix);
    PushInstanceData(QuadObject, QuadCount, QuadCount*sizeof(data::overlay_quad), QuadInstanceData);
    Clear(&RenderSystem->OverlayQuads);
  }
  

  // Overlay text
  u32 TextCount = GetBlockCount(&RenderSystem->OverlayText);
  if(TextCount)
  {
    render_object* OverlayTextProgram = PushNewRenderObject(RenderGroup);
    OverlayTextProgram->ProgramHandle = GlobalState->FontRenterProgram;
    OverlayTextProgram->MeshHandle = GlobalState->BlitPlane;
    OverlayTextProgram->FrameBufferHandle = GlobalState->DefaultFrameBuffer;
    OverlayTextProgram->TextureHandles[0] = RenderSystem->FontTextureHandle;
    OverlayTextProgram->TextureCount = 1;
    
    PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "Projection"), OrthoProjectionMatrix);
    PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "RenderedTexture"), (u32)0);
    PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "OnEdgeValue"), 128/255.f);
    PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "PixelDistanceScale"), 32/255.f);
    
    data::overlay_text* Text = PushArray(GlobalTransientArena, TextCount, data::overlay_text);
    chunk_list_iterator TextIt = BeginIterator(&RenderSystem->OverlayText);
    u32 i = 0;
    while(Valid(&TextIt)) {
      data::overlay_text* OverlayText = (data::overlay_text*) Next(&TextIt);
      Text[i] = *OverlayText;
      i++;
    }

    PushInstanceData(OverlayTextProgram, TextCount, TextCount*sizeof(data::overlay_text), (void*) Text);
    Clear(&RenderSystem->OverlayText);
  }

}


jfont::sdf_font LoadSDFFont(jfont::sdf_fontchar* CharMemory, s32 CharCount, c8 FontFilePath[], r32 TextPixelSize, s32 Padding, s32 OnedgeValue, 
  r32 PixelDistanceScale)
{
  debug_read_file_result TTFFile = Platform.DEBUGPlatformReadEntireFile(FontFilePath);
  Assert(TTFFile.Contents);

  jfont::sdf_font Font = jfont::LoadSDFFont(CharMemory, CharCount, TTFFile.Contents, TextPixelSize, Padding, OnedgeValue, PixelDistanceScale);

  // TODO: MaybeCopyData To GlobalTransientArena so we can free the file
  //Platform.DEBUGPlatformFreeFileMemory(TTFFile.Contents);
  return Font;
}

data::font CreateFont(memory_arena* Arena)
{
  u32 CharCount = 0x100;
  char FontPath[] = "C:\\Windows\\Fonts\\consola.ttf";
  data::font Font = {};
  Font.OnedgeValue = 128;  // "Brightness" of the sdf. Higher value makes the SDF bigger and brighter.
                                   // Has no impact on TextPixelSize since the char then is also bigger.
  Font.TextPixelSize = 64; // Size of the SDF
  Font.PixelDistanceScale = 32.0; // Smoothness of how fast the pixel-value goes to zero. Higher PixelDistanceScale, makes it go faster to 0;
                                  // Lower PixelDistanceScale and Higher OnedgeValue gives a 'sharper' sdf.
  Font.FontRelativeScale = 1.f;
  Font.Font = LoadSDFFont(PushArray(Arena, CharCount, jfont::sdf_fontchar),
    CharCount, FontPath, Font.TextPixelSize, 3, Font.OnedgeValue, Font.PixelDistanceScale);

  midx AtlasFileSize = jfont::SDFAtlasRequiredMemoryAmount(&Font.Font);
  u8* FontMemory = PushArray(Arena, AtlasFileSize, u8);
  Font.FontAtlas = jfont::CreateSDFAtlas(&Font.Font, FontMemory);



  return Font;
}

system* CreateRenderSystem(render_group* RenderGroup, r32 ApplicationWidth, r32 ApplicationHeight, application_render_commands* RenderCommands)
{
  system* Result = BootstrapPushStruct(system, Arena);
  //Result->SolidObjects = NewChunkList(&Result->Arena, sizeof(render::component), 64);
  //Result->TransparentObjects = NewChunkList(&Result->Arena, sizeof(render::component), 64);
  Result->OverlayText = NewChunkList(&Result->Arena, sizeof(data::overlay_text), 512);
  Result->OverlayQuads = NewChunkList(&Result->Arena, sizeof(data::overlay_quad), 512);
  Result->RenderGroup = RenderGroup;
  Result->Font = CreateFont(&Result->Arena);

  jfont::sdf_atlas* FontAtlas = &Result->Font.FontAtlas;

  texture_params FontTexParam = DefaultColorTextureParams();
  FontTexParam.TextureFormat = texture_format::R_8;
  FontTexParam.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  Result->FontTextureHandle = PushNewTexture(RenderGroup, FontAtlas->AtlasWidth, FontAtlas->AtlasHeight, FontTexParam, FontAtlas->AtlasPixels);

  Result->WindowSize.WindowWidth       = (r32) RenderCommands->WindowInfo.Width;
  Result->WindowSize.WindowHeight      = (r32) RenderCommands->WindowInfo.Height;
  Result->WindowSize.MonitorWidth      = (r32) RenderCommands->MonitorInfo.Width;
  Result->WindowSize.MonitorHeight     = (r32) RenderCommands->MonitorInfo.Height;
  Result->WindowSize.MonitorDPI        = (r32) RenderCommands->MonitorInfo.RawDPI;
  Result->WindowSize.EffectiveDPI      = (r32) RenderCommands->MonitorInfo.EffectiveDPI;
  Result->WindowSize.MSAA              = 4;
  Result->WindowSize.ApplicationWidth   = ApplicationWidth;
  Result->WindowSize.ApplicationHeight  = ApplicationHeight;
  Result->WindowSize.ApplicationAspectRatio = ApplicationWidth / ApplicationHeight;
  return Result;
}

}