#pragma once

#include "asset_manager\asset_types.h"

namespace ecs{ 
namespace render {

struct component {
  asset::phong_material_id PhongMaterialHandle;
  asset::mesh_id MeshID;
  asset::render_tree_id RenderTreeHandle;
};


} // namespace render
} // namespace ecs