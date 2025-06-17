#pragma once

#include "platform/coordinate_systems.h"

// Wanna make a difference to how position_node vs position works.
// Today position is the root node of a position_tree.
// I want a position_node with no parents to be the root node that gets updated
// and position just be pointers to nodes. That way we can let entities be related to eachother in the entity-manager
// rather than requiring all entities to just have their position relative the world coordinate.
namespace ecs{ 
namespace position {

struct component
{
  world_coordinate RelativePosition;
  world_coordinate AbsolutePosition;
  quat RelativeRotation;
  quat AbsoluteRotation;

  b32 Dirty;
};

// Creates a new position node, initializes and if parent exists, insert it into the tree
void Set(component* Component, world_coordinate Position, quat Rotation)
{
  Component->Dirty = true;
  Component->RelativePosition = Position;
  Component->RelativeRotation = Rotation;
}

void Set(component* Component, world_coordinate Position, euler_angle Euler)
{
  Component->Dirty = true;
  Component->RelativePosition = Position;
  Component->RelativeRotation = Quaternion(Euler.Roll, Euler.Pitch, Euler.Yaw);
}

// Creates a new position node, initializes and if parent exists, insert it into the tree
void Set(component* Component, world_coordinate Position, r32 Angle, v3 Axis)
{
  Component->Dirty = true;
  Component->RelativePosition = Position;
  Component->RelativeRotation = RotateQuaternion(Angle, Axis);
}

}
}