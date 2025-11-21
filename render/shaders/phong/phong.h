#pragma once
#include "asset_manager/asset_types.h"

namespace render {
namespace phong {

struct definition {
  bool HasDiffuseTexture;
  bool Transparent;
};

int GetProgramName(definition Definition, size_t BufferSize, char* Buffer)
{
  int CharCount = FormatString(Buffer, BufferSize, "PHONG_%s_%s",
    Definition.HasDiffuseTexture ? "DTEX" : "DFAC",
    Definition.Transparent ? "TRANSPARENT" : "SOLID");
  return CharCount;
}

u32 CreateProgram(render_group* RenderGroup, definition Definition)
{ 
  char* Defines = PushArray(GlobalTransientArena, 1024, char);
  FormatString(Defines, 1024*sizeof(char), 
    "#version 330 core\n"
    "#define DIFFUSE_TEXTURE %d\n"
    "#define TRANSPARENT %d\n",
    Definition.HasDiffuseTexture,
    Definition.Transparent);

  char* ProgramName = PushArray(GlobalTransientArena, 1024, char);
  GetProgramName(Definition, 1024*sizeof(char), ProgramName);
  u32 ProgramHandle = NewShaderProgram(RenderGroup, ProgramName);
  Platform.DEBUGPrint("Creating Program %d '%s'\n",ProgramHandle, ProgramName);
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ModelView");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "NormalView");
  AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "LightDirection");
  AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "LightColor");
  if(Definition.HasDiffuseTexture)
  {
    AddUniform(RenderGroup, UniformType::U32,  ProgramHandle, "DiffuseTexture");
  }else{
    AddUniform(RenderGroup, UniformType::V4,  ProgramHandle, "MaterialDiffuse");
  }
  AddUniform(RenderGroup, UniformType::V4,  ProgramHandle, "MaterialAmbient");
  AddUniform(RenderGroup, UniformType::V4,  ProgramHandle, "MaterialSpecular");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "Shininess");

  char* VertexHeaders = PushArray(GlobalTransientArena, 1, char);
  *VertexHeaders = '\n';

  char** VertexShaderCode = PushArray(GlobalTransientArena, 3, char*);
  VertexShaderCode[0] = Defines;
  VertexShaderCode[1] = VertexHeaders;
  VertexShaderCode[2] = *LoadFileFromDisk("..\\render\\shaders\\phong\\phong_vertex.glsl");
  
  char* FragmentHeaders = PushArray(GlobalTransientArena, 1, char);
  *FragmentHeaders = '\n';

  char** FragmentShaderCode = PushArray(GlobalTransientArena, 3, char*);
  FragmentShaderCode[0] = Defines;
  FragmentShaderCode[1] = FragmentHeaders;
  FragmentShaderCode[2] = *LoadFileFromDisk("..\\render\\shaders\\phong\\phong_fragment.glsl");

  CompileShader(RenderGroup, ProgramHandle,
     3,  VertexShaderCode,
     3,  FragmentShaderCode);
  return ProgramHandle;



}

definition GetProgramDefinition(asset::phong_material* Material)
{
  definition Result = {};
  Assert(Material->Ka); // Ambient
  Assert(Material->HasDiffuseTexture); // All current materials has a texture to fit the old shader system.
  //Assert(Material->Kd); // Diffse
  Assert(Material->Ks); // Specular
  Assert(Material->Ns); // Shininess

  // Note: This was a workaround from before. Objects which had no textures instead used a single white pixel.
  //       We no longer need that workaround here, but its necessary still for the other render-path.
  //       A white 1x1 texture means no texture in this system
  asset::image* Image = (asset::image*) asset::Find(asset::type::IMAGE, Material->DiffuseTexture.Image);
  if(Image->Width == 1 && Image->Height == 1)
  {
    Result.HasDiffuseTexture = false;
    Result.Transparent = Material->Kd->W < 1;
  }else{
    Result.HasDiffuseTexture = true;
    Result.Transparent = false;
  }
  
  // Unused Assert(!Material->Tf); // Transmission, // Just to see when it happens
  // Unused Assert(!Material->Ke); // Emissive
  // Unused Assert(!Material->d); // Dissolve
  // Unused Assert(!Material->Ni); // // Index of refraction
  // Unused r32 BumpMapBM = Material->BumpMapBM; // ??
  // Unused Assert(!Material->HasBumpMap);
  // Unused Assert(!Material->HasSpecularTexture);
  return Result;
}

void SetMaterialUniforms(render_group* RenderGroup, render_object* Object, asset::phong_material* Material)
{
  Object->TextureCount = 0;
  if(!Material)
  {
    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MaterialDiffuse"),   V4(0.7, 0.7, 0.7, 1));
    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MaterialAmbient"),   V4(0.7, 0.7, 0.7, 1));
    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MaterialSpecular"),  V4(0.7, 0.7, 0.7, 1));
    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Shininess"),         (r32)124.0);
  }else{
    if(Material->HasDiffuseTexture)
    {
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "DiffuseTexture"), (u32) Object->TextureCount);
      u32 DiffuseTextureHandle = GetOrCreateTexture(&Material->DiffuseTexture);
      Object->TextureHandles[Object->TextureCount] = DiffuseTextureHandle;
      Object->TextureCount++;
    }else{
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MaterialDiffuse"), *Material->Kd);
    }

    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MaterialAmbient"),   *Material->Ka);
    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MaterialSpecular"),  *Material->Ks);
    PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Shininess"),         *Material->Ns);
  }
} 

} // namespace phong;
} // namespace render