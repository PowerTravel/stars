#pragma once

#include "platform/jwin_platform.h"

namespace ecs {

struct entity_manager;

namespace component {
  struct position;
  struct geometry;
  struct material;
  struct collider;
  struct light;
  struct camera;
  struct controller;
  struct render;
}

namespace flag{
  enum component_type
  {
    NONE       = 0,
    POSITION   = 1<<0,
    GEOMETRY   = 1<<1,
    MATERIAL   = 1<<2,
    COLLIDER   = 1<<3,
    LIGHT      = 1<<4,
    CAMERA     = 1<<5,
    CONTROLLER = 1<<6,
    RENDER     = 1<<7, // Remove in favor of Mesh and Material
    END        = 1<<8
  };
}

  entity_manager* CreateEntityManager();
  const c8* ComponentTypeToString(flag::component_type Type)
  {
    switch(Type){
      case flag::NONE:       return "NONE";
      case flag::POSITION:   return "POSITION";
      case flag::GEOMETRY:   return "GEOMETRY";
      case flag::MATERIAL:   return "MATERIAL";
      case flag::COLLIDER:   return "COLLIDER";
      case flag::LIGHT:      return "LIGHT";
      case flag::CAMERA:     return "CAMERA";
      case flag::RENDER:     return "RENDER";
      case flag::CONTROLLER: return "CONTROLLER";
      case flag::END:        return "END";
    }
    return "NONE";
  }
}


#define GetPositionComponent(EntityID)   ((ecs::position::component*)   ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::POSITION))
#define GetMeshComponent(EntityID)       ((ecs::geometry_::component*)  ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::GEOMETRY))
#define GetMaterialComponent(EntityID)   ((ecs::material::component*)   ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::MATERIAL))
#define GetColliderComponent(EntityID)   ((ecs::collider::component*)   ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::COLLIDER))
#define GetLightComponent(EntityID)      ((ecs::light::component*)      ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::LIGHT))
#define GetCameraComponent(EntityID)     ((ecs::camera_::component*)    ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::CAMERA))
#define GetControllerComponent(EntityID) ((ecs::controller::component*) ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::CONTROLLER))
#define GetRenderComponent(EntityID)     ((ecs::render::component*)     ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::RENDER))
