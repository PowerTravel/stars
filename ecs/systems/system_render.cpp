#include "ecs/systems/system_render.h"

namespace ecs::system {

void Render(entity_manager* EntityManager)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EntityManager, flag::RENDER);

  while(Next(&EntityIterator))
  {
    render* Render = GetRenderComponent(&EntityIterator);
    // Do Rendering
    int a = 10;
  }
}

}