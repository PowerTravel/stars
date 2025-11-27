#pragma once

#include "cmn/n_tree.h"
#include "ecs/components/component_position.h"
#include "ecs/entity_components_backend.h"


namespace ecs { 
namespace position {

struct system {
  position_tree Positions;
};

ecs::position::system CreatePositionSystem();
void InitiatePosition(v3 Position, r32 Angle, v3 Axis, v3 Scale, ecs::entity_id* EntityID, ecs::entity_id* ParentEntityID = 0);
void UpdatePositions();
} // namespace position
} // namespace ecs
