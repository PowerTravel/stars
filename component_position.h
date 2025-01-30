#pragma once

#include "platform/coordinate_systems.h"
#include "stars.h"

struct component_position;
struct position_node
{
  // These values are modified directly
  world_coordinate RelativePosition;
  r32 RelativeRotation;
  
  // These values are calculated from the tree structure once per frame
  // after Relative position / Rotations have been updated.
  world_coordinate AbsolutePosition;
  r32 AbsoluteRotation;

  component_position* PositionComponent;
  position_node* FirstChild;
  position_node* NextSibling;
  position_node* Parent;
};

// component position can be linked to other positions to have a hierarchy of transformations
// The position of something is always relative to the parent. If parent is null it's relative to world origin.
struct component_position
{
  u32 NodeCount;
  b32 Dirty;
  position_node* FirstChild;
};

// Creates a new position node, initializes and if parent exists, insert it into the tree
void InitiatePositionComponent(component_position* PositionComponent, world_coordinate Position, r32 Rotation);
void InsertPositionNode(component_position* PositionComponent, position_node* Parent, position_node* Child);
component_position* GetPositionComponentFromNode(position_node const * Node);
position_node* CreatePositionNode(world_coordinate Position, r32 Rotation);
world_coordinate GetPositionRelativeTo(component_position const * PositionComponent, world_coordinate Position);
world_coordinate GetPositionRelativeTo(position_node const * Node, world_coordinate Position);
world_coordinate GetAbsolutePosition(position_node const * PositionComponent);
world_coordinate GetAbsolutePosition(component_position const * PositionComponent);
r32 GetAbsoluteRotation(position_node const * PositionComponent);
r32 GetAbsoluteRotation(component_position const * PositionComponent);
void SetRelativePosition(position_node* Node, world_coordinate Position, r32 Rotation);
void UpdateAbsolutePosition(memory_arena* Arena, component_position* Position);
void UpdateAbsolutePosition(memory_arena* Arena, position_node* PositionNode);
void ClearPositionComponent(component_position* PositionComponent);
void PositionSystemUpdate(entity_manager* EntityManager);