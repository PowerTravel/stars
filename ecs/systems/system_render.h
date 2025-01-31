#pragma once

#include "platform/jwin_platform.h"
#include "containers/chunk_list.h"

namespace ecs {

namespace render {

  namespace data {
  enum class programs{
    PHONG,
    TRANSPARENT_PHONG,
    TRANSPARENT_COMPOSITION
  };

  }// namespace data

  struct system {
    memory_arena Arena;
    chunk_list SolidObjects;
    chunk_list TransparentObjects;
  };
  
  system* InitiateRenderSystem()
  {
    system* Result = BootstrapPushStruct(system, Arena);
    Result->SolidObjects = NewChunkList(&Result->Arena, sizeof(render::component), 64);
    Result->TransparentObjects = NewChunkList(&Result->Arena, sizeof(render::component), 64);
    return Result;
  }

  void Draw(system* RenderSystem, entity_manager* EntityManager);
}
}
