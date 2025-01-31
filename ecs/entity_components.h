#pragma once

#include "platform/jwin_platform.h"

namespace ecs {

struct entity_manager;

namespace component {
  struct position;
  struct render;
}

namespace flag{
  enum component_type
  {
    NONE               = 0,
    POSITION           = 1<<0,
    RENDER             = 1<<1,
    END                = 1<<2
  };
}

  entity_manager* CreateEntityManager();

}


#define GetPositionComponent(EntityID) ((ecs::position::component*) ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::POSITION))
#define GetRenderComponent(EntityID) ((ecs::render::component*) ecs::GetComponent(GlobalState->World.EntityManager, EntityID, ecs::flag::RENDER))