#pragma once

namespace ecs{ 
namespace render {

struct component {
  u32 MeshHandle;

  v4 Ambient;
  v4 Diffuse;
  v4 Specular;
  r32 Shininess;

  u32 DiffuseTextureHandle;
};


} // namespace render
} // namespace ecs