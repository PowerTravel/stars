#pragma once
#include "asset_manager/asset_types.h"
#include "utils.h"
// Defines pbr shader. Given a pbr_material it will create a shader program
namespace render {

namespace pbr {

  struct definition {
    bool AlbedoMap;
    bool MetallicRoughnessMap;
  };

  definition GetProgramDefinition(asset::pbr_material* Material)
  {
    definition Definition = {};
    if(Material)
    {
      if(Material->HasMetallicRoughness)
      {
        if(Material->MetallicRoughness.HasBaseColorTexture)
        {
          Definition.AlbedoMap = true;
        }

        if(Material->MetallicRoughness.HasMetallicRoughnessTexture)
        {
          Definition.MetallicRoughnessMap = true;
        }
      }
    }
    return Definition;
  }

  int GetProgramName(definition Definition, size_t BufferSize, char* Buffer)
  {
    int CharCount = FormatString(Buffer, BufferSize, "BRDF_%s_%s",
      Definition.AlbedoMap ? "AlbedoMap" : "Albedo",
      Definition.MetallicRoughnessMap ? "MetallicRoughnessMap" : "MetallicFactor");
    return CharCount;
  }


  u32 CreateProgram(render_group* RenderGroup, definition Definition)
  {
    char* ProgramName = PushArray(&GlobalRenderer->RenderTransientArena, 1024, char);
    GetProgramName(Definition, 1024*sizeof(char), ProgramName);
    u32 ProgramHandle = NewShaderProgram(RenderGroup, ProgramName);
    Platform.DEBUGPrint("Creating Program %d '%s'\n",ProgramHandle, ProgramName);
    AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ProjectionMat");
    AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "View");
    AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "Model");
    AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "NormalModel");

    // Material properties as per model Constants
    AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "CamPos");
    AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "LightPos");

    char* Defines = PushArray(&GlobalRenderer->RenderTransientArena, 1024, char);
    FormatString(Defines, 1024*sizeof(char), 
      "#version 330 core\n"
      "#define ALBEDO_MAP %d\n"
      "#define METALLIC_ROUGHNESS_MAP %d\n",
      Definition.AlbedoMap,
      Definition.MetallicRoughnessMap);

    if(Definition.AlbedoMap){
      AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "AlbedoMap");
    }else{
      AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "Albedo");
    }

    if(Definition.MetallicRoughnessMap)
    { 
      AddUniform(RenderGroup, UniformType::U32,  ProgramHandle, "MetallicRoughnessMap");
    }else{
      AddUniform(RenderGroup, UniformType::R32,  ProgramHandle, "Metalness");
      AddUniform(RenderGroup, UniformType::R32,  ProgramHandle, "Roughness");
    }

    char* VertexHeaders = PushArray(&GlobalRenderer->RenderTransientArena, 1, char);
    VertexHeaders[0] = '\0';

    char** VertexShaderCode = PushArray(&GlobalRenderer->RenderTransientArena, 3, char*);
    VertexShaderCode[0] = Defines;
    VertexShaderCode[1] = VertexHeaders;
    VertexShaderCode[2] = *LoadFileFromDisk("..\\render\\shaders\\pbr\\pbr_vertex.glsl");
    
    char* FragmentHeaders = PushArray(&GlobalRenderer->RenderTransientArena, 1, char);
    FragmentHeaders[0] = '\0';

    char** FragmentShaderCode = PushArray(&GlobalRenderer->RenderTransientArena, 3, char*);
    FragmentShaderCode[0] = Defines;
    FragmentShaderCode[1] = FragmentHeaders;
    FragmentShaderCode[2] = *LoadFileFromDisk("..\\render\\shaders\\pbr\\pbr_fragment.glsl");

    CompileShader(RenderGroup, ProgramHandle,
       3,  VertexShaderCode,
       3,  FragmentShaderCode);
    return ProgramHandle;
  }


  void SetMaterialUniforms(render_group* RenderGroup, render_object* Object, asset::pbr_material* Material)
  {
    Object->TextureCount = 0;
    if(!Material)
    {
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Albedo"),     V3(0.7, 1, 0.7));
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Metalness"),  (r32) 0.5);
      PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Roughness"),  (r32) 0.5);
    }else{
      Assert(Material);
      if(Material->HasMetallicRoughness)
      {
        asset::pbr_material::metallic_roughness* MetallicRoughness = &Material->MetallicRoughness;

        if(MetallicRoughness->HasBaseColorTexture) {
          u32 AlbedoHandle = GetOrCreateTexture(&MetallicRoughness->BaseColorTexture);
          PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "AlbedoMap"), (u32) Object->TextureCount);
          Object->TextureHandles[Object->TextureCount] = AlbedoHandle;
          Object->TextureCount++;
        }else{
          PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Albedo"), V3(MetallicRoughness->BaseColorFactor));
        }

        if(MetallicRoughness->HasMetallicRoughnessTexture)
        {
          u32 MetallicRoughnessTextureHandle = GetOrCreateTexture(&MetallicRoughness->BaseColorTexture);
          PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "MetallicRoughnessMap"), (u32) Object->TextureCount);
          Object->TextureHandles[Object->TextureCount] = MetallicRoughnessTextureHandle;
          Object->TextureCount++;
        } else {
          PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Metalness"),  MetallicRoughness->MetallicFactor);
          PushUniform(Object, GetUniformHandle(RenderGroup, Object->ProgramHandle, "Roughness"),  MetallicRoughness->RoughnessFactor);
        }
      }
    }
  } 

} // namespace pbr
} // namespace render