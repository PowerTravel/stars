#include "ecs/systems/system_render.h"
#include "asset_manager/gl_mapper.h"


extern ecs::render::system* GlobalRenderSystem;

namespace ecs{
namespace render {

enum class overlay_object_type {
  POSITION
};

struct overlay_object {
  overlay_object_type Type;
  v3 Position;
  quat Rotation;
};

u32 GetMeshHandle(u32 AssetKey)
{
  u32* Handle = (u32*) Find(&GlobalRenderSystem->MeshHandleMap, AssetKey);
  u32 Result = 0;
  if(Handle)
  {
    Result = *Handle;
  }else{
    asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, AssetKey);
    Assert(Mesh);
    
    // There is a one to many relationship between mesh-handles and the rendersystems ptimitive-handles which were not handling atm
    // This assert is here to catch the cases when we need to deal with that.
    Assert(Mesh->PrimitiveCount == 1);
    opengl_buffer_data glBufferData = asset::mapper::MeshToGlVertexBuffer(GlobalTransientArena, Mesh);
    Result = ecs::render::LoadMeshToGpu(AssetKey, &glBufferData);
  }
  return Result;
}

u32 GetMeshHandle(const c8* Name)
{
  u32 AssetKey = asset::ToKey(asset::type::RENDER_TREE, Name);
  asset::render_tree* Tree = (asset::render_tree*) asset::Find(asset::type::RENDER_TREE, Name);
  Assert(Tree->NodeCount == 1);
  u32 Handle = GetMeshHandle(Tree->Root->Mesh);
  return Handle;
}

u32 Get32BitTextureHandle(asset::key AssetKey)
{
  u32* Handle = (u32*) Find(&GlobalRenderSystem->TextureHandleMap, AssetKey);
  u32 Result = 0;
  if(Handle)
  {
    Result = *Handle;
  }else{
    asset::image* Texture = (asset::image*) asset::Find(asset::type::IMAGE, AssetKey);
    Result = ecs::render::LoadImageToGpu(AssetKey, Texture);
  }

  return Result;
}

u32 Get32BitTextureHandle(const c8* Name)
{
  asset::key AssetKey = asset::ToKey(asset::type::IMAGE, Name);
  u32 Handle = Get32BitTextureHandle(AssetKey);
  return Handle;
}

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

r32 GetScaleFromPixelSize(system* System, r32 PixelSize)
{
  r32 Result = jfont::GetScaleFromPixelSize(&System->Font.Font, PixelSize);
  return Result;
}

r32 GetCanonicalFontDescenOffset(system* System, r32 PixelSize)
{
  r32 FontDescent = -System->Font.Font.Descent;
  r32 DecentCan = PixelToCanonicalHeight(GetRenderSystem(), FontDescent);
  r32 Scale = ecs::render::GetScaleFromPixelSize(GetRenderSystem(), PixelSize);
  r32 Result = Scale*DecentCan;
  return Result;
}

v2 GetTextSizePixelSpace(system* System, r32 PixelSize, utf8_byte const * Text)
{ 
  if(!Text) return {};

  SCOPED_TRANSIENT_ARENA;
  r32 FontRelativeScale = GetScaleFromPixelSize(System, PixelSize);
  u32 Length = jstr::StringLength((const char*)Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, Length+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode((utf8_byte*) Text, CodePoints);
  v2 Result = {};
  jfont::GetTextDim(&System->Font.Font, FontRelativeScale, &Result.X, &Result.Y, CodePoints);
  return Result;
}

// Calculates the number of characters to fit within a given MaxWidthPixelSpace leaving space for a suffix.
// The current usecase is if we have the string 
//    "Hello world"
// We may want to print
//    "Hello w..."
// if the last 'd' does not fit. Result would be 7.
// If suffix is null or the empty string the number result would be 10. (Hello worl)

// Sets CharCountRet with the number of chars in Text that will fit for the suffix to also have space if Text is longer than MaxWidthPixelSpace,
// Returns true if all of Text fits, otherwise false.
b32 GetCharsCountToFitPixelSpace(system* System, r32 PixelSize, r32 MaxWidthPixelSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet)
{ 
  SCOPED_TRANSIENT_ARENA;
  r32 FontRelativeScale = GetScaleFromPixelSize(System, PixelSize);

  r32 TextWidth = GetTextSizePixelSpace(System, PixelSize, Text).X;
  b32 Result = true;
  if(TextWidth > MaxWidthPixelSpace)
  {
    r32 SuffixWidth = GetTextSizePixelSpace(System, PixelSize, Suffix).X;
    MaxWidthPixelSpace = Maximum(MaxWidthPixelSpace - SuffixWidth, 0);
    Result = false;
  }
  
  codepoint* TextCodePoints = PushArray(GlobalTransientArena, jstr::StringLength((const char*)Text)+1, codepoint);
  ConvertToUnicode((utf8_byte*) Text, TextCodePoints);
  *CharCountRet = jfont::GetUnicodeCharCountThatFitsInSize(&System->Font.Font, FontRelativeScale, MaxWidthPixelSpace, TextCodePoints);

  return Result;
}

b32 GetCharsCountToFitCanonicalSpace(system* System, r32 PixelSize, r32 MaxWidthCanonicalSpace, utf8_byte const * Text, utf8_byte const * Suffix, size_t* CharCountRet)
{
  r32 MaxWidthPixelSpace = CanonicalToPixelSpace(System, V2(MaxWidthCanonicalSpace,0)).X;
  b32 Result = GetCharsCountToFitPixelSpace(System, PixelSize, MaxWidthPixelSpace, Text, Suffix, CharCountRet);
  return Result;
}

v2 GetTextSizeCanonicalSpace(system* System, r32 PixelSize, utf8_byte const * Text)
{
  v2 PixelPos = GetTextSizePixelSpace(System, PixelSize, Text);
  v2 CanonicalPos = PixelToCanonicalSpace(System, PixelPos);
  return CanonicalPos;
}

data::render_level* GetTopRenderLevel(system* System)
{
  if(ListEmpty(&System->RenderSentinel))
  {
    NewRenderLevel(System);
  }
  return System->RenderSentinel.Previous;
}

data::render_level* GetBotRenderLevel(system* System)
{
  if(ListEmpty(&System->RenderSentinel))
  {
    NewRenderLevel(System);
  }
  return System->RenderSentinel.Next;
}

inline file_local chunk_list* GetOverlayText(system* System, data::render_level* RenderLevel)
{
  if(!IsInitiated(&RenderLevel->OverlayText))
  {
    RenderLevel->OverlayText = NewChunkList(&System->Arena, sizeof(data::overlay_text), 512);
  }
  return &RenderLevel->OverlayText;
}

inline file_local chunk_list* GetOverlayQuads(system* System, data::render_level* RenderLevel)
{
  if(!IsInitiated(&RenderLevel->OverlayQuads))
  {
    RenderLevel->OverlayQuads = NewChunkList(&System->Arena, sizeof(data::overlay_quad), 512);
  }
  return &RenderLevel->OverlayQuads;
}

inline file_local chunk_list* GetOverlayIcon(system* System, data::render_level* RenderLevel)
{
  if(!IsInitiated(&RenderLevel->OverlayIcon))
  {
    RenderLevel->OverlayIcon = NewChunkList(&System->Arena, sizeof(data::textured_overlay_quad), 512);
  }
  return &RenderLevel->OverlayIcon;
}
  
inline file_local chunk_list* GetSolidObjects()
{
  if(!IsInitiated(&GlobalRenderSystem->SolidObjects))
  {
    GlobalRenderSystem->SolidObjects = NewChunkList(&GlobalRenderSystem->Arena, sizeof(component**), 32);
  }
  return &GlobalRenderSystem->SolidObjects;
}

inline file_local chunk_list* GetTransparentObjects()
{
  if(!IsInitiated(&GlobalRenderSystem->TransparentObjects))
  {
    GlobalRenderSystem->TransparentObjects = NewChunkList(&GlobalRenderSystem->Arena, sizeof(component**), 32);
  }
  return &GlobalRenderSystem->TransparentObjects;
}

inline file_local chunk_list* GetOverlayRenders()
{
  if(!IsInitiated(&GlobalRenderSystem->OverlayRenders))
  {
    GlobalRenderSystem->OverlayRenders = NewChunkList(&GlobalRenderSystem->Arena, sizeof(data::render_data), 32);
  }
  return &GlobalRenderSystem->OverlayRenders;
}

inline file_local chunk_list* GetLineObjects()
{
  if(!IsInitiated(&GlobalRenderSystem->LineObjects))
  {
    GlobalRenderSystem->LineObjects = NewChunkList(&GlobalRenderSystem->Arena, sizeof(data::line_3d), 32);
  }
  return &GlobalRenderSystem->LineObjects;
}

void DrawTextPixelSpace(system* System, v2 PixelPos, rect2f PixelClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  SCOPED_TRANSIENT_ARENA;
  u32 Length = jstr::StringLength((const char*) Text);
  codepoint* CodePoints = PushArray(GlobalTransientArena, Length+1, codepoint);
  u32 UnicodeLen = ConvertToUnicode(Text, CodePoints);
  r32 RelativeScale =  GetScaleFromPixelSize(System, PixelSize);
  jfont::print_coordinates* TextPrintCoordinates = PushArray(GlobalTransientArena, UnicodeLen, jfont::print_coordinates);
  jfont::GetTextPrintCoordinates(&System->Font.Font, &System->Font.FontAtlas, RelativeScale, PixelPos.X, PixelPos.Y, PixelClipRect, CodePoints, TextPrintCoordinates);

  data::render_level* RenderLevel = GetTopRenderLevel(System);
  chunk_list* TextBuffer = GetOverlayText(System, RenderLevel);
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
    Push(&System->Arena, &RenderLevel->OverlayText, (bptr)&OverlayText);
  }
}

void DrawTextCanonicalSpace(system* System, v2 CanonicalPos,  rect2f CanonicalClipRect, r32 PixelSize, utf8_byte const * Text, v4 Color)
{
  v2 PixelPos = CanonicalToPixelSpace(System, CanonicalPos);
  v2 PixelClipPos = CanonicalToPixelSpace(System, V2(CanonicalClipRect.X, CanonicalClipRect.Y));
  v2 PixelClipSize = CanonicalToPixelSpace(System, V2(CanonicalClipRect.W, CanonicalClipRect.H));
  DrawTextPixelSpace(System, PixelPos, Rect2f(PixelClipPos, PixelClipSize), PixelSize, Text, Color);
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

  data::render_level* RenderLevel = GetTopRenderLevel(System);
  chunk_list* TextBuffer = GetOverlayText(System, RenderLevel);

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
    Push(&System->Arena, &RenderLevel->OverlayText, (bptr)&OverlayText);
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
 
  data::render_level* RenderLevel = GetTopRenderLevel(System); 
  chunk_list* QuadBuffer = GetOverlayQuads(System, RenderLevel);
  Push(&System->Arena, QuadBuffer, (bptr)&Quad);
}

void DrawRenderObject(component* Component)
{
  if(!Component) return;

  
  r32 Transparancy = 1;
  if(Component->PhongMaterialHandle)
  {
    asset::phong_material* PhongMaterial = (asset::phong_material*) asset::Find(asset::type::PHONG_MATERIAL, Component->PhongMaterialHandle);
    Assert(PhongMaterial->Ks);
    Transparancy = PhongMaterial->Ks->W;
  }

  if(Transparancy < 1)
  {
    Push(&GlobalRenderSystem->Arena,  GetTransparentObjects(), (bptr) &Component);
  }else{
    Push(&GlobalRenderSystem->Arena, GetSolidObjects(), (bptr) &Component);
  }
}
/*
  ecs::position::component* Position = GetPositionComponent(&EntityIterator);
  if(Position)
  {
    DrawOverlayDot3D(System, Position->RelativePosition);
  }
*/



void DrawOverlay3DObject(void* Data, void (*RenderFunction)(m4 ProjectionMatrix, m4 ViewMatrix, void* Data)){
  data::render_data RenderData = {};
  RenderData.Data = Data;
  RenderData.RenderFunction = RenderFunction;
  Push(&GlobalRenderSystem->Arena, GetOverlayRenders(), (bptr) &RenderData);
} 

void DrawOverlayQuadCanonicalSpace(system* System, rect2f CanonicalRect, v4 Color)
{
  rect2f PixelRect = Rect2f( CanonicalToPixelSpace(System, V2(CanonicalRect.X,CanonicalRect.Y)),
                             CanonicalToPixelSpace(System, V2(CanonicalRect.W,CanonicalRect.H)));

  DrawOverlayQuadPixelSpace(System, PixelRect, Color);
}

void DrawIconPixelSpace(system* System, rect2f PixelRect, v4 TextureCoords, v4 Color)
{
  m4 ModelMatrix = M4Identity();
  Scale(V4(0.5,0.5, 0, 1), ModelMatrix);
  Scale(V4(PixelRect.W, PixelRect.H, 0, 1), ModelMatrix);
  Translate(V4(PixelRect.X, PixelRect.Y, 0, 0), ModelMatrix);
  ModelMatrix = Transpose(ModelMatrix);

  data::textured_overlay_quad Quad = {};
  Quad.TexCoord = TextureCoords;
  Quad.Color = Color;
  Quad.ModelMatrix = ModelMatrix;
 
  data::render_level* RenderLevel = GetTopRenderLevel(System); 
  chunk_list* QuadBuffer = GetOverlayIcon(System, RenderLevel);
  Push(&System->Arena, QuadBuffer, (bptr)&Quad);
}

void DrawIconCanonicalSpace(system* System, rect2f CanonicalRect, v4 TextureCoords, v4 Color)
{
  rect2f PixelRect = Rect2f( CanonicalToPixelSpace(System, V2(CanonicalRect.X,CanonicalRect.Y)),
                             CanonicalToPixelSpace(System, V2(CanonicalRect.W,CanonicalRect.H)));
  DrawIconPixelSpace(System, PixelRect, TextureCoords, Color);
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

void PushRenderObjectWithoutEntity(render_group* RenderGroup, u32 MeshHandle, u32 Program, u32 FrameBuffer, m4& ProjectionMatrix, m4& ViewMatrix,
  v3 LightDirection, v3 LightColor, v3 Pos, quat Rot, v3 Scal)
{
  render_object* Object = PushNewRenderObject(RenderGroup);
  Object->ProgramHandle = Program;
  Object->FrameBufferHandle = FrameBuffer;
  Object->MeshHandle = MeshHandle;
  Object->TextureCount = 0;

  m4 Scale = GetScaleMatrix(V4(Scal,1));
  m4 Rotation = GetRotationMatrix(Rot);
  m4 Translation = GetTranslationMatrix(V4(Pos,1));
  m4 ModelMat = Translation*Rotation*Scale;

  asset::phong_material* Material = (asset::phong_material*) asset::Find(asset::type::PHONG_MATERIAL, asset::ToKey(asset::type::PHONG_MATERIAL, "red_rubber"));
  v4 Ambient = {};
  if(Material->Ka){
    Ambient = *Material->Ka;
  }
  v4 Diffuse = {};
  if(Material->Kd){
    Diffuse = *Material->Kd;
  }
  v4 Specular = {};
  if(Material->Ks){
    Specular = *Material->Ks;
  }
  r32 Shininess = {};
  if(Material->Ns){
    Shininess = *Material->Ns;
  }

  m4 ModelView = ViewMatrix*ModelMat;
  m4 NormalView = Transpose(RigidInverse(ModelView));
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "ProjectionMat"), ProjectionMatrix);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "ModelView"), ModelView);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "NormalView"), NormalView);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "LightDirection"), LightDirection);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "LightColor"), LightColor);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialAmbient"), Ambient);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialDiffuse"), Diffuse);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialSpecular"), Specular);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "Shininess"), Shininess);
}

static void PushRenderObject(render_group* RenderGroup, component* Render, u32 Program, u32 FrameBuffer, m4& ProjectionMatrix, m4& ViewMatrix,
  v3 LightDirection, v3 LightColor)
{
  entity_id EntityId = GetEntityIDFromComponent( (bptr) Render );
  ecs::position::component* Position =  GetPositionComponent(&EntityId);

  render_object* Object = PushNewRenderObject(RenderGroup);
  Object->ProgramHandle = Program;
  Object->FrameBufferHandle = FrameBuffer;
  Object->MeshHandle = Render->MeshHandle;


  m4 ModelMat = GetModelMatrix(Position);

  m4 ModelView = ViewMatrix*ModelMat;
  m4 NormalView = Transpose(RigidInverse(ModelView));

  v4 Ambient = V4(0.2,0.2,0.2,1);
  v4 Diffuse = V4(0.6,0.6,0.6,1);
  v4 Specular = V4(1,1,1,1);
  r32 Shininess = 16;
  
  if(Render->PhongMaterialHandle)
  {
    asset::phong_material* PhongMaterial = (asset::phong_material*) asset::Find(asset::type::PHONG_MATERIAL, Render->PhongMaterialHandle);
    if(PhongMaterial->Ka){
      Ambient   = *PhongMaterial->Ka;
    }
    if(PhongMaterial->Kd){
      Diffuse   = *PhongMaterial->Kd;
    }
    if(PhongMaterial->Ks){
      Specular  = *PhongMaterial->Ks;
    }
    if(PhongMaterial->Ns){
      Shininess = *PhongMaterial->Ns;
    }

    if(PhongMaterial->HasDiffuseTexture)
    {
      asset::key ImageHandle = PhongMaterial->DiffuseTexture.Image;
      asset::image* Image = (asset::image*) Find(asset::type::IMAGE, PhongMaterial->DiffuseTexture.Image);
      u32 TexHandle = Get32BitTextureHandle(ImageHandle);

      Object->TextureCount = 1;
      Object->TextureHandles[0] = TexHandle;
    }
  } 
#if 0
  else if (Render->PbrMaterialHandle)
  {
    asset::pbr_material* PbrMaterial = (asset::pbr_material*) asset::Find(asset::type::PBR_MATERIAL, Render->PbrMaterialHandle);
    Assert(PbrMaterial->HasMetallicRoughness);
    Diffuse =  PbrMaterial->MetallicRoughness.BaseColorFactor;
  }
#endif

  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "ProjectionMat"),    ProjectionMatrix);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "ModelView"),        ModelView);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "NormalView"),       NormalView);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "LightDirection"),   LightDirection);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "LightColor"),       LightColor);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialAmbient"),  Ambient);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialDiffuse"),  Diffuse);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "MaterialSpecular"), Specular);
  PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->PhongProgram, "Shininess"),        Shininess);
}

void DEBUGPrintMatrix(m4 Matrix)
{
  Platform.DEBUGPrint("| %1.2f, %1.2f, %1.2f, %1.2f |\n", Matrix.r0.X, Matrix.r0.Y,Matrix.r0.Z,Matrix.r0.W);
  Platform.DEBUGPrint("| %1.2f, %1.2f, %1.2f, %1.2f |\n", Matrix.r1.X, Matrix.r1.Y,Matrix.r1.Z,Matrix.r1.W);
  Platform.DEBUGPrint("| %1.2f, %1.2f, %1.2f, %1.2f |\n", Matrix.r2.X, Matrix.r2.Y,Matrix.r2.Z,Matrix.r2.W);
  Platform.DEBUGPrint("| %1.2f, %1.2f, %1.2f, %1.2f |\n", Matrix.r3.X, Matrix.r3.Y,Matrix.r3.Z,Matrix.r3.W);
}

inline u32 InternalTexture(u32 Index)
{
  u32 Result = GlobalRenderSystem->InternalTextures[Index];
  return Result;
}

inline u32 FrameBuffer(u32 Index)
{
  u32 Result = GlobalRenderSystem->FrameBuffers[Index];
  return Result;
}

void Draw(entity_manager* EntityManager, system* RenderSystem, m4 ProjectionMatrix, m4 ViewMatrix)
{
  render_group* RenderGroup = RenderSystem->RenderGroup;

  //DrawOverlayDot3D(RenderSystem, V3(0,3,0));

  v3 LightColor = V3(1,1,1);
  v3 LightPosition = V3(1,1,1);
  v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPosition,0));

  // Some Gaussian Blur just cause I can
  r32* KernelOffset = PushArray(GlobalTransientArena, 64, r32);
  r32* KernelWeight = PushArray(GlobalTransientArena, 64, r32);
  u32 KernelSize = GetGaussianKernel(12, 2, KernelOffset, KernelWeight);

  window_size_pixel* Window = &RenderSystem->WindowSize;

  // ClearRenderState
  {
    render_state* DefaultState = PushNewState(RenderGroup);
    *DefaultState = DefaultRenderState3(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio);

    // Clear default color frame buffer
    clear_operation* DefClearColor = PushNewClearOperation(RenderGroup);
    DefClearColor->BufferType = OPEN_GL_COLOR;
    DefClearColor->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
    DefClearColor->TextureIndex = 0;
    DefClearColor->Color = V4(0,0,0,1);

    // Clear default depth frame buffer
    clear_operation* DefClearDepth = PushNewClearOperation(RenderGroup);
    DefClearDepth->BufferType = OPEN_GL_DEPTH;
    DefClearDepth->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
    DefClearDepth->TextureIndex = 0;
    DefClearDepth->Depth = 1;

    // Clear MSAA Color frame buffer
    clear_operation* ClearMSAAColor = PushNewClearOperation(RenderGroup);
    ClearMSAAColor->BufferType = OPEN_GL_COLOR;
    ClearMSAAColor->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_MSAA);
    ClearMSAAColor->TextureIndex = 0;
    ClearMSAAColor->Color = V4(0,0,0,1);

    // Clear MSAA Depth frame buffer
    clear_operation* ClearMSAADepth = PushNewClearOperation(RenderGroup);
    ClearMSAADepth->BufferType = OPEN_GL_DEPTH;
    ClearMSAADepth->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_MSAA);
    ClearMSAADepth->TextureIndex = 0;
    ClearMSAADepth->Depth = 1;

    // Clear Transparent calculation frame buffer 0
    clear_operation* TransparenClearOp0 = PushNewClearOperation(RenderGroup);
    TransparenClearOp0->BufferType = OPEN_GL_COLOR;
    TransparenClearOp0->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_TRANSPARENT);
    TransparenClearOp0->TextureIndex = 0;
    TransparenClearOp0->Color = V4(0,0,0,0);

    // Clear Transparent calculation frame buffer 1
    clear_operation* TransparenClearOp1 = PushNewClearOperation(RenderGroup);
    TransparenClearOp1->BufferType = OPEN_GL_COLOR;
    TransparenClearOp1->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_TRANSPARENT);
    TransparenClearOp1->TextureIndex = 1;
    TransparenClearOp1->Color = V4(1,0,0,0);
  }
  

  render_state* DefaultState2 = PushNewState(RenderGroup);
  *DefaultState2 = DefaultRenderState3(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio);

  chunk_list* SolidObjects = GetSolidObjects();
  chunk_list* TransparentObjects = GetTransparentObjects();
  chunk_list* OverlayRenders = GetOverlayRenders();
  chunk_list* LineObjects = GetLineObjects();
  if(GetBlockCount(SolidObjects) > 0 || GetBlockCount(TransparentObjects) > 0)
  {
//      render_state* MSAAViewport = PushNewState(RenderGroup);
//      SetState(MSAAViewport, ViewportState(Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, Window->ApplicationAspectRatio));
    
    // First draw solid objects
    if(GetBlockCount(SolidObjects) > 0)
    {
      chunk_list_iterator SolidIt = BeginIterator(SolidObjects);
      while(Valid(&SolidIt)) {
        component** RenderPtr = (component**) Next(&SolidIt);
        PushRenderObject(RenderGroup, *RenderPtr, GlobalState->PhongProgram, FrameBuffer(data::FRAMEBUFFER_MSAA), ProjectionMatrix, ViewMatrix, LightDirection, LightColor);
      }
      Clear(SolidObjects);
    }

   { 
      // Solid Lines
      u32 LineCount = GetBlockCount(LineObjects);
      if(LineCount)
      {
        render_object* Object = PushNewRenderObject(RenderGroup);
        Object->ProgramHandle = GlobalState->LineRenderProgram;
        Object->MeshHandle = RenderSystem->BlitPlaneHandle;
        Object->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_MSAA);
        
        PushUniform(Object, GetUniformHandle(RenderGroup, GlobalState->LineRenderProgram, "ProjectionMat"), ProjectionMatrix);

        data::line_3d* Lines = (data::line_3d*) Copy(GlobalTransientArena, LineObjects);
        
        PushInstanceData(Object, LineCount, LineCount*sizeof(data::line_3d), (void*) Lines);
        Clear(LineObjects);
      }

    }


    if(GetBlockCount(TransparentObjects) > 0)
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

      chunk_list_iterator TransparentIt = BeginIterator(TransparentObjects);
      while(Valid(&TransparentIt)) {
        component** RenderPtr = (component**) Next(&TransparentIt);
        PushRenderObject(RenderGroup, *RenderPtr, GlobalState->PhongProgramTransparent, FrameBuffer(data::FRAMEBUFFER_TRANSPARENT), ProjectionMatrix, ViewMatrix, LightDirection, LightColor);
      }
      Clear(TransparentObjects);

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
      CompositionObject->MeshHandle =  RenderSystem->BlitPlaneHandle;
      CompositionObject->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_MSAA);
      CompositionObject->TextureHandles[0] = InternalTexture(data::INT_TEX_ACCUM);
      CompositionObject->TextureHandles[1] = InternalTexture(data::INT_TEX_REVEAL);
      CompositionObject->TextureCount = 2;

      PushUniform(CompositionObject, GetUniformHandle(RenderGroup, GlobalState->TransparentCompositionProgram,  "AccumTex"), (u32)0);
      PushUniform(CompositionObject, GetUniformHandle(RenderGroup, GlobalState->TransparentCompositionProgram , "RevealTex"), (u32)1);

    }

    { 
      // Render overlay doodads.
      render_state* BlendAndDepth = PushNewState(RenderGroup);
      SetState(BlendAndDepth, DefaultBlendState());
      depth_state DepthState = {};
      DepthState.TestActive = true;
      DepthState.WriteActive = true; 
      SetState(BlendAndDepth,DepthState);
     
      clear_operation* ClearMsaaDepthBuffer = PushNewClearOperation(RenderGroup);
      ClearMsaaDepthBuffer->BufferType = OPEN_GL_DEPTH;
      ClearMsaaDepthBuffer->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_MSAA);
      ClearMsaaDepthBuffer->TextureIndex = 0;
      ClearMsaaDepthBuffer->Depth = 1;
      
      if(GetBlockCount(OverlayRenders) > 0)
      {
        chunk_list_iterator OverlayIt = BeginIterator(OverlayRenders);
        while(Valid(&OverlayIt)) {
          data::render_data* RenderData = (data::render_data*) Next(&OverlayIt);
          RenderData->RenderFunction(ProjectionMatrix, ViewMatrix, RenderData->Data);
        }
        Clear(OverlayRenders);
      }
    }

    // Shrink to regular screeen sice
    render_state* ViewportAndBlend = PushNewState(RenderGroup);
    ///SetState(ViewportAndBlend, ViewportState(Window->ApplicationWidth, Window->ApplicationHeight, Window->ApplicationAspectRatio));
    SetState(ViewportAndBlend, ViewportState(Window->WindowWidth, Window->WindowHeight, Window->ApplicationAspectRatio));
    SetState(ViewportAndBlend, DefaultBlendState());

    blit_operation* BlitOperation = PushNewBlitOperation(RenderGroup);
    BlitOperation->ReadFrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_MSAA);
  
#if 1
    BlitOperation->DrawFrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
    BlitOperation->DrawRegionUnitCoord = RenderSystem->UnitDrawRegion;
#else
  // Gaussian blur
  BlitOperation->DrawFrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_GAUSSIAN_A);

  for (int i = 0; i < 4; ++i)
  {
    render_object* GaussianBlurX = PushNewRenderObject(RenderGroup);
    GaussianBlurX->ProgramHandle = GlobalState->GaussianProgramX;
    GaussianBlurX->MeshHandle =  RenderSystem->BlitPlaneHandle;
    GaussianBlurX->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_GAUSSIAN_B);
    GaussianBlurX->TextureHandles[0] = InternalTexture(data::INT_TEX_GAUSSIAN_A);
    GaussianBlurX->TextureCount = 1;

    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "offset"), UniformType::R32, KernelOffset, KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "weight"), UniformType::R32, KernelWeight, KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "kernerlSize"), KernelSize);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "RenderedTexture"), (u32) 0);
    PushUniform(GaussianBlurX, GetUniformHandle(RenderGroup, GaussianBlurX->ProgramHandle, "sideSize"), V2(Window->ApplicationWidth, Window->ApplicationHeight));

    render_object* GaussianBlurY = PushNewRenderObject(RenderGroup);
    GaussianBlurY->ProgramHandle = GlobalState->GaussianProgramY;
    GaussianBlurY->MeshHandle = RenderSystem->BlitPlaneHandle;
    GaussianBlurY->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_GAUSSIAN_A);
    GaussianBlurY->TextureHandles[0] = InternalTexture(data::INT_TEX_GAUSSIAN_B);
    GaussianBlurY->TextureCount = 1;
    
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "offset"), UniformType::R32, KernelOffset, KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "weight"), UniformType::R32, KernelWeight, KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "kernerlSize"), KernelSize);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "RenderedTexture"), (u32) 0);
    PushUniform(GaussianBlurY, GetUniformHandle(RenderGroup, GaussianBlurY->ProgramHandle, "sideSize"), V2(Window->ApplicationWidth, Window->ApplicationHeight));
  }
  blit_operation* BlitOperation2 = PushNewBlitOperation(RenderGroup);
  BlitOperation2->ReadFrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_GAUSSIAN_B);
  BlitOperation2->DrawFrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
#endif

  }else{
    // Shrink to regular screeen sice
    render_state* ViewportAndBlend = PushNewState(RenderGroup);
    ///SetState(ViewportAndBlend, ViewportState(Window->ApplicationWidth, Window->ApplicationHeight, Window->ApplicationAspectRatio));
    SetState(ViewportAndBlend, ViewportState(Window->WindowWidth, Window->WindowHeight, Window->ApplicationAspectRatio));
    SetState(ViewportAndBlend, DefaultBlendState());
  }
    




  data::render_level* RenderLevel = GetBotRenderLevel(RenderSystem);
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
      QuadObject->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
      data::overlay_quad* QuadInstanceData = (data::overlay_quad*) Copy(GlobalTransientArena, OverlayQuads);

      PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "Projection"), OrthoProjectionMatrix);
      PushInstanceData(QuadObject, QuadCount, QuadCount*sizeof(data::overlay_quad), QuadInstanceData);
      Clear(OverlayQuads);
    }
    
    chunk_list* OverlayIcon = &RenderLevel->OverlayIcon;
    u32 IconCount = GetBlockCount(OverlayIcon);
    if(IconCount)
    {
      render_object* QuadObject = PushNewRenderObject(RenderGroup);
      QuadObject->ProgramHandle = GlobalState->TexturedSquareOverlayProgram;
      QuadObject->MeshHandle = RenderSystem->BlitPlaneHandle;
      QuadObject->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
      QuadObject->TextureHandles[0] = GlobalState->ImguiContext.Icons.Atlas;
      QuadObject->TextureCount = 1;
      data::textured_overlay_quad* QuadInstanceData = (data::textured_overlay_quad*) Copy(GlobalTransientArena, OverlayIcon);
      
      PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "Projection"), OrthoProjectionMatrix);
      PushUniform(QuadObject, GetUniformHandle(RenderGroup, QuadObject->ProgramHandle, "RenderedTexture"), (u32)0);
      PushInstanceData(QuadObject, IconCount, IconCount*sizeof(data::textured_overlay_quad), QuadInstanceData);
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
      OverlayTextProgram->FrameBufferHandle = FrameBuffer(data::FRAMEBUFFER_DEFAULT);
      OverlayTextProgram->TextureHandles[0] = RenderSystem->FontTextureHandle;
      OverlayTextProgram->TextureCount = 1;
      
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "Projection"), OrthoProjectionMatrix);
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "RenderedTexture"), (u32)0);
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "OnEdgeValue"), 128/255.f);
      PushUniform(OverlayTextProgram, GetUniformHandle(RenderGroup, OverlayTextProgram->ProgramHandle, "PixelDistanceScale"), 32/255.f);
      
      u32 i = 0;
      data::overlay_text* Text = PushArray(GlobalTransientArena, TextCount, data::overlay_text);
      chunk_list_iterator TextIt = BeginIterator(OverlayText);
      while(Valid(&TextIt)) {
        data::overlay_text* OverlayText = (data::overlay_text*) Next(&TextIt);
        Text[i] = *OverlayText;
        i++;
      }

      PushInstanceData(OverlayTextProgram, TextCount, TextCount*sizeof(data::overlay_text), (void*) Text);
      Clear(OverlayText);
    }

    RenderLevel = RenderLevel->Next;
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


opengl_buffer_data GetBlitPlane()
{
  // Define and Load Blitplane into asset manager.
  int VerticeIndex[] = {
    0,1,2,
    2,1,3
  };

  v3 Vertices[] = {
    {-1.0f, -1.0f, 0.0f},
    { 1.0f, -1.0f, 0.0f},
    {-1.0f,  1.0f, 0.0f},
    { 1.0f,  1.0f, 0.0f}
  };
  v2 TextureVertices[] = {
    {0,0},
    {1,0},
    {0,1},
    {1,1}
  };
  v2* TextureVerticesArr[] = {
    TextureVertices
  };
  
  int TextureVerticesCounts[] = {ArrayCount(TextureVertices)};

  asset::mesh Mesh = {};
  asset::mesh::primitive Primitive = {};
  Primitive.IndexCount = ArrayCount(VerticeIndex);
  Primitive.Indeces = VerticeIndex;
  Primitive.VertexCount = ArrayCount(Vertices);
  Primitive.Vertex = Vertices;
  Primitive.TextureVertexSetCount = ArrayCount(TextureVerticesCounts);
  Primitive.TextureVertices = TextureVerticesArr;
  Primitive.Topology = asset::mesh::primitive::topology::TRIANGLES;
  Primitive.AABB = AABB3f(V3(-1,-1,0), V3(1,1,0));
  Mesh.Primitives = &Primitive;
  Mesh.PrimitiveCount = 1;

  Assert(! asset::Find(asset::type::MESH, "BlitPlane"));

  asset::mesh* LoadedMesh = asset::LoadMesh("BlitPlane", &Mesh);
  opengl_buffer_data Result = asset::mapper::MeshToGlVertexBuffer(GlobalTransientArena, LoadedMesh);
  return Result;
}

u32 PushBlitPlaneMesh(system* RenderSystem, render_group* RenderGroup)
{
  // Define and Load Blitplane into asset manager.
  asset::key AssetKey  = 0;
  // Send BlitPlane to the render-system
  opengl_buffer_data GlBufferData = GetBlitPlane();
  Assert(GlBufferData.BufferCount == 1);
  u32 MeshHandle = PushNewMesh(RenderGroup, GlBufferData.BufferData->VertexCount, GlBufferData.BufferData->VertexData);
  u32 IndexHandle = PushNewMeshIndices(RenderGroup, MeshHandle, GlBufferData.BufferData->IndexCount, GlBufferData.BufferData->Indeces);
  
  // Create a mapping between the render-handle and the asset-handle
  {
    u32* StoredRenderHandle = (u32*) GetNewBlock(GlobalPersistentArena, &RenderSystem->RenderHandles);
    *StoredRenderHandle = IndexHandle;
    Insert(&RenderSystem->MeshHandleMap, AssetKey, StoredRenderHandle);
  }

  return IndexHandle;
}

u32* CreateInternalTextures(render_group* RenderGroup, window_size_pixel* Window)
{    
  texture_params DefaultColor = DefaultColorTextureParams();
  texture_params DefaultDepth = DefaultDepthTextureParams();
  texture_params RevealTexParam = DefaultColorTextureParams();
  RevealTexParam.TextureFormat = texture_format::R_8;

  u32* Result = (u32*) PushArray(GlobalPersistentArena, data::INT_TEX_COUNT, u32);
  Result[data::INT_TEX_MSAA_COLOR] = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultColor, 0);
  Result[data::INT_TEX_MSAA_DEPTH] = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultDepth, 0);
  Result[data::INT_TEX_ACCUM]      = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultColor, 0);
  Result[data::INT_TEX_REVEAL]     = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, RevealTexParam, 0);
  Result[data::INT_TEX_GAUSSIAN_A] = PushNewTexture(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
  Result[data::INT_TEX_GAUSSIAN_B] = PushNewTexture(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
  return Result;
}

u32* CreateFrameBuffers(render_group* RenderGroup, window_size_pixel* Window, u32* Textures )
{
  u32* Result = (u32*) PushArray(GlobalPersistentArena, data::FRAMEBUFFER_COUNT, u32);
  u32 TransparentColorTexture[]         = {Textures[data::INT_TEX_ACCUM], Textures[data::INT_TEX_REVEAL]};
  Result[data::FRAMEBUFFER_DEFAULT]     = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 0, 0, 0, 0);
  Result[data::FRAMEBUFFER_MSAA]        = PushNewFrameBuffer(RenderGroup,  Window->MSAA * Window->ApplicationWidth,  Window->MSAA * Window->ApplicationHeight, 1, &Textures[data::INT_TEX_MSAA_COLOR], Textures[data::INT_TEX_MSAA_DEPTH], 0);
  Result[data::FRAMEBUFFER_TRANSPARENT] = PushNewFrameBuffer(RenderGroup,  Window->MSAA * Window->ApplicationWidth,  Window->MSAA * Window->ApplicationHeight, ArrayCount(TransparentColorTexture), TransparentColorTexture, Textures[data::INT_TEX_MSAA_DEPTH], 0);
  Result[data::FRAMEBUFFER_GAUSSIAN_A]  = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 1, &Textures[data::INT_TEX_GAUSSIAN_A], 0, 0);
  Result[data::FRAMEBUFFER_GAUSSIAN_B]  = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth, Window->ApplicationHeight, 1, &Textures[data::INT_TEX_GAUSSIAN_B], 0, 0);
  return Result;
}

system* CreateRenderSystem(render_group* RenderGroup, r32 ApplicationWidth, r32 ApplicationHeight, application_render_commands* RenderCommands)
{
  system* Result = BootstrapPushStruct(system, Arena);
  ListInitiate(&Result->RenderSentinel);
  Result->RenderGroup = RenderGroup;
  Result->Font = CreateFont(&Result->Arena);

  Result->RenderHandles    = NewChunkList(GlobalPersistentArena, sizeof(u32), 128);
  Result->MeshHandleMap    = NewRBTree(GlobalPersistentArena, 64, 64);
  Result->TextureHandleMap = NewRBTree(GlobalPersistentArena, 64, 64);

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

  Result->InternalTextures  = CreateInternalTextures(RenderGroup, &Result->WindowSize);
  Result->FrameBuffers      = CreateFrameBuffers(RenderGroup, &Result->WindowSize, Result->InternalTextures);
  Result->BlitPlaneHandle   = PushBlitPlaneMesh(Result, RenderGroup);

  Result->TempMem = BeginTemporaryMemory(&Result->Arena);

  return Result;
}

void Begin()
{
  EndTemporaryMemory( GlobalRenderSystem->TempMem );
  GlobalRenderSystem->TempMem = BeginTemporaryMemory(&GlobalRenderSystem->Arena);
  ListInitiate(&GlobalRenderSystem->RenderSentinel);
  GlobalRenderSystem->TransparentObjects = {};
  GlobalRenderSystem->SolidObjects = {};
  GlobalRenderSystem->OverlayRenders = {};
  GlobalRenderSystem->LineObjects = {};
}

file_local void SetHandle(rb_tree* HandleTree, u32 Key, u32 Handle){
  u32* HandleMem = (u32*) GetNewBlock(GlobalPersistentArena, &GlobalRenderSystem->RenderHandles);
  *HandleMem = Handle;
  Insert(HandleTree, Key, (void*) HandleMem);
}

u32 LoadMeshToGpu(u32 AssetKey, opengl_buffer_data* BufferDataPtr) {
  Assert(BufferDataPtr->BufferCount == 1);
  gl_vertex_buffer* VertexBuffer = BufferDataPtr->BufferData;
  u32 MeshHandle = PushNewMesh(GlobalRenderCommands->RenderGroup, VertexBuffer->VertexCount, VertexBuffer->VertexData);
  u32 IndexHandle = PushNewMeshIndices(GlobalRenderCommands->RenderGroup, MeshHandle, VertexBuffer->IndexCount, VertexBuffer->Indeces);
  SetHandle(&GlobalRenderSystem->MeshHandleMap, AssetKey, IndexHandle);
  return IndexHandle;
}

u32 LoadImageToGpu(u32 AssetKey, asset::image* Image) {
  Assert(Image->Channels == 4);
  texture_params Params = DefaultColorTextureParams();
  Params.TextureFormat = texture_format::RGBA_U8;
  Params.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  u32 Handle = PushNewTexture(GlobalRenderCommands->RenderGroup, Image->Width, Image->Height, Params, Image->Pixels);
  SetHandle(&GlobalRenderSystem->TextureHandleMap, AssetKey, Handle);
  return Handle;
}

void ecs::render::Init(asset::key MeshKey, asset::key MaterialKey, component* Render)
{
  asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, MeshKey);
  Render->MeshHandle = ecs::render::GetMeshHandle(MeshKey);
  Assert(Mesh->PrimitiveCount == 1); // We don't support multi primitive mesh rendering (yet)
  Render->PhongMaterialHandle = MaterialKey;
}

void DrawLine3D(v3 Start, v3 End, v4 Color, r32 Thickness) {
  data::line_3d Line = {};
  Line.P0 = Start;
  Line.P1 = End;
  Line.Color = Color;
  Line.Thickness = Thickness;
  Push(&GlobalRenderSystem->Arena, GetLineObjects(), (bptr) &Line);
}

void DrawAABB(aabb3f AABB) {
  v4 Color = V4(0,1,0,1);
  r32 Thickness = 1;
  v3 AABBVertices[8] = {};
  GetAABBVertices(&AABB, AABBVertices);

  DrawLine3D(AABBVertices[0], AABBVertices[1], Color, Thickness);
  DrawLine3D(AABBVertices[1], AABBVertices[2], Color, Thickness);
  DrawLine3D(AABBVertices[2], AABBVertices[3], Color, Thickness);
  DrawLine3D(AABBVertices[3], AABBVertices[0], Color, Thickness);
  DrawLine3D(AABBVertices[4], AABBVertices[5], Color, Thickness);
  DrawLine3D(AABBVertices[5], AABBVertices[6], Color, Thickness);
  DrawLine3D(AABBVertices[6], AABBVertices[7], Color, Thickness);
  DrawLine3D(AABBVertices[7], AABBVertices[4], Color, Thickness);
  DrawLine3D(AABBVertices[0], AABBVertices[4], Color, Thickness);
  DrawLine3D(AABBVertices[1], AABBVertices[5], Color, Thickness);
  DrawLine3D(AABBVertices[2], AABBVertices[6], Color, Thickness);
  DrawLine3D(AABBVertices[3], AABBVertices[7], Color, Thickness);
}

} // render
} // ecs

