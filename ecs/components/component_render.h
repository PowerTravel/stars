#pragma once

#include "asset_manager\asset_types.h"

namespace ecs{ 
namespace render {

struct component {
  asset::key MeshHandle;
  asset::key PhongMaterialHandle;

  asset::key RenderTreeHandle;
};


} // namespace render
} // namespace ecs