#include "ecs/entity_components.h"
#include "ecs/entity_components_backend.h"
#include "ecs/components/component_position.h"
#include "ecs/components/component_geometry.h"
#include "ecs/components/component_material.h"
#include "ecs/components/component_collider.h"
#include "ecs/components/component_light.h"
#include "ecs/components/component_camera.h"
#include "ecs/components/component_controller.h"
#include "ecs/components/component_render.h"

namespace ecs {

entity_manager* CreateEntityManager() {
  u32 CameraChunkCount = 4;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  entity_manager_definition Definitions[] = 
  {
    {flag::POSITION,   flag::NONE,     EntityChunkCount,  sizeof(position::component)},
    {flag::GEOMETRY,   flag::NONE,     EntityChunkCount,  sizeof(geometry_::component)},
    {flag::MATERIAL,   flag::NONE,     EntityChunkCount,  sizeof(material::component)},
    {flag::COLLIDER,   flag::NONE,     EntityChunkCount,  sizeof(collider::component)},
    {flag::LIGHT,      flag::NONE,     EntityChunkCount,  sizeof(light::component)},
    {flag::CAMERA,     flag::NONE,     EntityChunkCount,  sizeof(camera_::component)},
    {flag::CONTROLLER, flag::NONE,     EntityChunkCount,  sizeof(controller::component)},
    {flag::RENDER,     flag::POSITION, EntityChunkCount,  sizeof(render::component)},
 //   {COMPONENT_FLAG_DYNAMICS,         COMPONENT_FLAG_COLLIDER,                           EntityChunkCount,     sizeof(component_dynamics)},
 //   {COMPONENT_FLAG_RENDER,           COMPONENT_FLAG_POSITION,                           EntityChunkCount,     sizeof(component_render)}
  };

  entity_manager* Result = CreateEntityManager(EntityChunkCount, EntityChunkCount, ArrayCount(Definitions), Definitions);
  return Result;
}

}