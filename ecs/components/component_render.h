#pragma once

#include "asset_manager\asset_types.h"

namespace ecs{ 
namespace render {

struct component {
  u32 MeshHandle; // NOT AN ASSET KEY!!! Its a handle to the platform renderer
  asset::key PhongMaterialHandle;
  asset::mesh_id MeshID;

  asset::key RenderTreeHandle;
};


} // namespace render
} // namespace ecs