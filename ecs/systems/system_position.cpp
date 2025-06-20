#include "ecs/systems/system_position.h"
#include "platform/jwin_platform.h"

namespace ecs::position{

using namespace component;

void Update(component* Position)
{
  Position->AbsolutePosition = Position->RelativePosition; 
  //Position->RelativeRotation = QuaternionMultiplication(Position->RelativeRotation, RotateQuaternion(-0.01, V3(0,1,0)));
  Position->AbsoluteRotation = Position->RelativeRotation;
  Position->Dirty = false;
}

void UpdatePositions(entity_manager* EntityManager)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EntityManager, flag::POSITION);

  while(Next(&EntityIterator))
  {
    component* Position = GetPositionComponent(&EntityIterator);
    if(Position->Dirty)
    {
      Update(Position);
    }
  }
}

}