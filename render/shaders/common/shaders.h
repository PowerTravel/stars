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

}//namespace common_shaders
}//namespace render