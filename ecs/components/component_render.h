#pragma once

#include "asset_manager\asset_types.h"

namespace ecs{ 
namespace render {

struct component {
  asset::phong_material* PhongMaterial;
  asset::pbr_material*   PbrMaterial;
  asset::geometry*       Geometry;
};


} // namespace render
} // namespace ecs