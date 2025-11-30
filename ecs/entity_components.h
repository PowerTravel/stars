#pragma once

#include "platform/jwin_platform.h"

namespace ecs {

struct entity_manager;

namespace component {
  struct position;
  struct collider;
  struct render;
}

namespace flag{
  enum component_type
  {
    NONE     = 0,
    POSITION = 1<<0,
    COLLIDER = 1<<1,
    RENDER   = 1<<2,
    END      = 1<<4
  };
}

  entity_manager* CreateEntityManager();
  const c8* ComponentTypeToString(flag::component_type Type)
  {
    switch(Type){
      case flag::NONE: return "NONE";
      case flag::POSITION: return "POSITION";
      case flag::COLLIDER: return "COLLIDER";
      case flag::RENDER: return "RENDER";
      case flag::END: return "END";
    }
    return "NONE";
  }
}


#define GetPositionComponent(EntityID) ((ecs::position::component*) ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::POSITION))
#define GetColliderComponent(EntityID) ((ecs::collider::component*) ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::COLLIDER))
#define GetRenderComponent(EntityID)   ((ecs::render::component*)   ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::RENDER))
