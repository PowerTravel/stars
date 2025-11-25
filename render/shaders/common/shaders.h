#pragma once

namespace render {

namespace common_shaders {

u32 CreateTransparentCompositionProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup,
    "TransparentCompositionProgram");

  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "AccumTex");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "RevealTex");
  CompileShader(RenderGroup, ProgramHandle,  
    1, LoadFileFromDisk("..\\render\\shaders\\common\\blit_plane_vertex.glsl"),
    1, LoadFileFromDisk("..\\render\\shaders\\common\\transparent_composition_fragment.glsl"));

  return ProgramHandle;
}

u32 CreateGaussianBlurProgramY(render_group* RenderGroup)
{
  u32 ProgramHandleY = NewShaderProgram(RenderGroup, "GaussianYProgram");

  AddUniform(RenderGroup, UniformType::U32, ProgramHandleY, "RenderedTexture");
  AddUniform(RenderGroup, UniformType::V2,  ProgramHandleY, "sideSize");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleY, "offset");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleY, "weight");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandleY, "kernerlSize");
  CompileShader(RenderGroup, ProgramHandleY,
    1, LoadFileFromDisk("..\\render\\shaders\\common\\blit_plane_vertex.glsl"),
    1, LoadFileFromDisk("..\\render\\shaders\\common\\gaussian_fragment_y.glsl"));
  return ProgramHandleY;
}

u32 CreateGaussianBlurProgramX(render_group* RenderGroup)
{
  u32 ProgramHandleX = NewShaderProgram(RenderGroup,"GaussianXProgram");

  AddUniform(RenderGroup, UniformType::U32, ProgramHandleX, "RenderedTexture");
  AddUniform(RenderGroup, UniformType::V2,  ProgramHandleX, "sideSize");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleX, "offset");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleX, "weight");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandleX, "kernerlSize");
  CompileShader(RenderGroup, ProgramHandleX,
    1, LoadFileFromDisk("..\\render\\shaders\\common\\blit_plane_vertex.glsl"),
    1, LoadFileFromDisk("..\\render\\shaders\\common\\gaussian_fragment_x.glsl"));
  return ProgramHandleX;
}

struct sdf_varying {
  v4 Color;
  v4 TextCoord;
  m4 ModelMatrix; // PixelSpace
};

u32 CreateSDFRenderProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "OverlaySDFRenderProgram");

  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "Projection");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "SDFMap");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "OnEdgeValue");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "PixelDistanceScale");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "TextColor_in");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "TexCoord_in");
  AddVarying(RenderGroup, UniformType::M4,  ProgramHandle, "Model");
  CompileShader(RenderGroup, ProgramHandle, 
     1, LoadFileFromDisk("..\\render\\shaders\\common\\sdf_vertex.glsl"),
     1, LoadFileFromDisk("..\\render\\shaders\\common\\sdf_fragment.glsl"));
  return ProgramHandle;
}

struct sprite_varying {
  v4 Color;
  v4 TexCoord;
  u32 TexDepth;
  m4 ModelMatrix; // PixelSpace
};

u32 CreateSpriteRenderProgram(render_group* RenderGroup, u32 TextureComponentCount)
{
  char ProgramNameBuf[1024] = {};  
  FormatString(ProgramNameBuf, sizeof(ProgramNameBuf), "SpriteRenderProgram_%d", TextureComponentCount);

  u32 ProgramHandle = NewShaderProgram(RenderGroup, ProgramNameBuf);
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "Projection");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "SpriteMap");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "Color_in");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "TexCoord");
  AddVarying(RenderGroup, UniformType::U32, ProgramHandle, "SpriteDepth");
  AddVarying(RenderGroup, UniformType::M4,  ProgramHandle, "Model");
  
 char* Defines = PushArray(&GlobalRenderer->RenderTransientArena, 1024, char);
    FormatString(Defines, 1024*sizeof(char), 
      "#version 330 core\n"
      "#define TEXTURE_COMPONENT %d\n",
      TextureComponentCount);

  char** VertexShaderCode = PushArray(&GlobalRenderer->RenderTransientArena, 3, char*);
  VertexShaderCode[0] = Defines;
  VertexShaderCode[1] = *LoadFileFromDisk("..\\render\\shaders\\common\\sprite_vertex.glsl");

  char** FragmentShaderCode = PushArray(&GlobalRenderer->RenderTransientArena, 3, char*);
  FragmentShaderCode[0] = Defines;
  FragmentShaderCode[1] = *LoadFileFromDisk("..\\render\\shaders\\common\\sprite_fragment.glsl");

  CompileShader(RenderGroup, ProgramHandle,
     2,  VertexShaderCode,
     2,  FragmentShaderCode);
  return ProgramHandle;
}

}//namespace common_shaders
}//namespace render