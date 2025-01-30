#include "entity_components.h"
#include "entity_components_backend.h"
#include "component_position.h"
//#include "component_collider.h"
//#include "component_dynamics.h"
//#include "component_render.h"

entity_manager* CreateEntityManager()
{
  u32 CameraChunkCount = 4;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  entity_manager_definition Definitions[] = 
  {
    {COMPONENT_FLAG_POSITION,         COMPONENT_FLAG_NONE,                               EntityChunkCount,     sizeof(component_position)}
 //   {COMPONENT_FLAG_COLLIDER,         COMPONENT_FLAG_POSITION,                           EntityChunkCount,     sizeof(component_collider)},
 //   {COMPONENT_FLAG_DYNAMICS,         COMPONENT_FLAG_COLLIDER,                           EntityChunkCount,     sizeof(component_dynamics)},
 //   {COMPONENT_FLAG_RENDER,           COMPONENT_FLAG_POSITION,                           EntityChunkCount,     sizeof(component_render)}
  };

  entity_manager* Result = CreateEntityManager(EntityChunkCount, EntityChunkCount, ArrayCount(Definitions), Definitions);
  return Result;
}
