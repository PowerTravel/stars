#include "ecs/entity_components.h"
#include "ecs/entity_components_backend.h"
#include "ecs/components/component_position.h"
#include "ecs/components/component_collider.h"
#include "ecs/components/component_render.h"

namespace ecs {

entity_manager* CreateEntityManager() {
  u32 CameraChunkCount = 4;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  entity_manager_definition Definitions[] = 
  {
    {flag::POSITION, flag::NONE,     EntityChunkCount,  sizeof(position::component)},
    {flag::COLLIDER, flag::POSITION, EntityChunkCount,  sizeof(collider::component)},
    {flag::RENDER,   flag::POSITION, EntityChunkCount,  sizeof(render::component)}
 //   {COMPONENT_FLAG_DYNAMICS,         COMPONENT_FLAG_COLLIDER,                           EntityChunkCount,     sizeof(component_dynamics)},
 //   {COMPONENT_FLAG_RENDER,           COMPONENT_FLAG_POSITION,                           EntityChunkCount,     sizeof(component_render)}
  };

  entity_manager* Result = CreateEntityManager(EntityChunkCount, EntityChunkCount, ArrayCount(Definitions), Definitions);
  return Result;
}

}