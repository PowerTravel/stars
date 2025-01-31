#include "ecs/systems/system_render.h"

namespace ecs::render {

void Draw(entity_manager* EntityManager)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EntityManager, flag::RENDER);

  while(Next(&EntityIterator))
  {
    render::component* Component = GetRenderComponent(&EntityIterator);
    // Do Rendering
    int a = 10;
  }
}

}