#pragma once

#include "platform/coordinate_systems.h"
#include "math/affine_transformations.h"
#include "cmn/n_tree.h"
#include "ecs/entity_components_backend.h"
#include "print_utils.h"
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
  m4 T;
  m4 gT;
};
m4 GetRelativeModelMatrix(ecs::position::component* Position);
m4 GetAbsoluteModelMatrix(ecs::position::component* Position);
// Creates a new position node, initializes and if parent exists, insert it into the tree
inline void Set(component* Component, world_coordinate Position, quat Rotation, v3 Scale)
{
  Component->RelativePosition = Position;
  Component->RelativeRotation = Rotation;
  Component->Scale = Scale;

  Component->T = GetRelativeModelMatrix(Component);
  Component->gT = M4Identity();
}
//inline void Set(ecs::entity_id& EntityID, world_coordinate Position, quat Rotation, v3 Scale)
//{
//  Set(GetPositionComponent(&EntityID), Position, Rotation, Scale);
//}

inline void Set(component* Component, world_coordinate Position, euler_angle Euler, v3 Scale)
{
  Component->RelativePosition = Position;
  Component->RelativeRotation = Quaternion(Euler);
  Component->Scale = Scale;

  Component->T = GetRelativeModelMatrix(Component);
  Component->gT = M4Identity();
}
//inline void Set(ecs::entity_id& EntityID, world_coordinate Position, euler_angle Euler, v3 Scale)
//{
//  Set(GetPositionComponent(&EntityID), Position, Euler, Scale);
//}

inline void Set(component* Component, world_coordinate Position, r32 Angle, v3 Axis, v3 Scale)
{
  Component->RelativePosition = Position;
  Component->RelativeRotation = RotateQuaternion(Angle, Axis);
  Component->Scale = Scale;
  Component->T = GetRelativeModelMatrix(Component);
  Component->gT = M4Identity();
}

inline void Set(component* Component, m4 Transformation)
{
  Component->T = Transformation;
  Component->gT = M4Identity();

  Component->RelativePosition = GetPositionFromMatrix(Transformation);
  Component->RelativeRotation = QuaternionFromMatrix(Transformation);
  m4 M = GetRotationMatrix(Component->RelativeRotation);
  m4 Minv = AffineInverse(M);
  dpu::Print(M);
  m4 TDecompose = Transformation;
  Index(TDecompose,0,3,0);
  Index(TDecompose,1,3,0);
  Index(TDecompose,2,3,0);
  TDecompose = Minv*TDecompose;
  Component->Scale = V3(Diagonal(TDecompose));
  
}
//inline void Set(ecs::entity_id& EntityID, world_coordinate Position, r32 Angle, v3 Axis, v3 Scale)
//{
//  Set(GetPositionComponent(&EntityID), Position, Euler, Scale);
//}

inline v3 GetAbsolutePosition(ecs::position::component* Position)
{
  return Position->AbsolutePosition;
}
inline v4 GetAbsoluteRotation(ecs::position::component* Position)
{
  return Position->AbsoluteRotation;
}
inline v3 GetScale(ecs::position::component* Position)
{
  return Position->Scale;
}

m4 GetRelativeModelMatrix(ecs::position::component* Position)
{
  v3 P = Position->RelativePosition;
  m4 R = GetRotationMatrix(Position->RelativeRotation);
  v3 S = GetScale(Position);
  m4 Result = M4(
    Index(R,0,0)*S.X, Index(R,0,1)*S.Y, Index(R,0,2)*S.Z, P.X,
    Index(R,1,0)*S.X, Index(R,1,1)*S.Y, Index(R,1,2)*S.Z, P.Y,
    Index(R,2,0)*S.X, Index(R,2,1)*S.Y, Index(R,2,2)*S.Z, P.Z,
                   0,                0,                0,   1);
  return Result;
}

m4 GetAbsoluteModelMatrix(ecs::position::component* Position)
{
  v3 P = GetAbsolutePosition(Position);
  m4 R = GetRotationMatrix(GetAbsoluteRotation(Position));
  v3 S = GetScale(Position);
  m4 Result = M4(
    Index(R,0,0)*S.X, Index(R,0,1)*S.Y, Index(R,0,2)*S.Z, P.X,
    Index(R,1,0)*S.X, Index(R,1,1)*S.Y, Index(R,1,2)*S.Z, P.Y,
    Index(R,2,0)*S.X, Index(R,2,1)*S.Y, Index(R,2,2)*S.Z, P.Z,
                   0,                0,                0,   1);
  return Result;
}
}
}
