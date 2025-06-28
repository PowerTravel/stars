#pragma once

#include "platform/coordinate_systems.h"
#include "math/affine_transformations.h"
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
  v3 Scale;
  b32 Dirty;
};

// Creates a new position node, initializes and if parent exists, insert it into the tree
void Set(component* Component, world_coordinate Position, quat Rotation, v3 Scale)
{
  Component->Dirty = true;
  Component->RelativePosition = Position;
  Component->RelativeRotation = Rotation;
  Component->Scale = Scale;
}

void Set(component* Component, world_coordinate Position, euler_angle Euler, v3 Scale)
{
  Component->Dirty = true;
  Component->RelativePosition = Position;
  Component->RelativeRotation = Quaternion(Euler);
  Component->Scale = Scale;
}

// Creates a new position node, initializes and if parent exists, insert it into the tree
void Set(component* Component, world_coordinate Position, r32 Angle, v3 Axis, v3 Scale)
{
  Component->Dirty = true;
  Component->RelativePosition = Position;
  Component->RelativeRotation = RotateQuaternion(Angle, Axis);
  Component->Scale = Scale;
}

v3 GetAbsolutePosition(ecs::position::component* Position)
{
  return Position->AbsolutePosition;
}
v4 GetAbsoluteRotation(ecs::position::component* Position)
{
  return Position->AbsoluteRotation;
}
v3 GetScale(ecs::position::component* Position)
{
  return Position->Scale;
}

m4 GetModelMatrix(ecs::position::component* Position)
{
  m4 Scale = GetScaleMatrix(V4(GetScale(Position),1));
  m4 Rotation = GetRotationMatrix(GetAbsoluteRotation(Position));
  m4 Translation = GetTranslationMatrix(V4(GetAbsolutePosition(Position),1));
  m4 ModelMat = Translation*Rotation*Scale;
  return ModelMat;
}
}
}