#pragma once

#include "platform/coordinate_systems.h"

// Wanna make a difference to how position_node vs position works.
// Today position is the root node of a position_tree.
// I want a position_node with no parents to be the root node that gets updated
// and position just be pointers to nodes. That way we can let entities be related to eachother in the entity-manager
// rather than requiring all entities to just have their position relative the world coordinate.
namespace ecs{ 
namespace position {

struct component;
struct position_node
{
  // These values are modified directly
  world_coordinate RelativePosition;
  r32 RelativeRotation;

  quat RelativeRotation_2; // Not implemented yet
  quat AbsoluteRotation_2; // Not implemented yet
  
  // These values are calculated from the tree structure once per frame
  // after Relative position / Rotations have been updated.
  world_coordinate AbsolutePosition;
  r32 AbsoluteRotation;

  component* PositionComponent;
  position_node* FirstChild;
  position_node* NextSibling;
  position_node* Parent;
};

// component position can be linked to other positions to have a hierarchy of transformations
// The position of something is always relative to the parent. If parent is null it's relative to world origin.
struct component
{
  u32 NodeCount;
  b32 Dirty;
  position_node* FirstChild;
};

// Creates a new position node, initializes and if parent exists, insert it into the tree
void InitiatePositionComponent(component* PositionComponent, world_coordinate Position, r32 Rotation);
void InsertPositionNode(component* PositionComponent, position_node* Parent, position_node* Child);
component* GetPositionComponentFromNode(position_node const * Node);
position_node* CreatePositionNode(world_coordinate Position, r32 Rotation);
world_coordinate GetPositionRelativeTo(component const * PositionComponent, world_coordinate Position);
world_coordinate GetPositionRelativeTo(position_node const * Node, world_coordinate Position);
world_coordinate GetAbsolutePosition(position_node const * PositionComponent);
world_coordinate GetAbsolutePosition(component const * PositionComponent);
r32 GetAbsoluteRotation(position_node const * PositionComponent);
r32 GetAbsoluteRotation(component const * PositionComponent);
void SetRelativePosition(position_node* Node, world_coordinate Position, r32 Rotation);
void ClearPositionComponent(component* PositionComponent);

}
}