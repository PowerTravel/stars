#pragma once
#include "asset_manager/asset_types.h"

namespace ecs { 
namespace material {
struct component
{
  asset::phong_material* PhongMaterial;
  asset::pbr_material*   PbrMaterial;
};
} // namespace material
} // namespace ecs
