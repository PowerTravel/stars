#pragma once

#include "math/aabb.h"
#include "containers/vector_list.h"
#include "ecs/entity_components_backend.h"

struct memory_arena;

namespace broadphase {

struct node
{
  aabb3f AABB;
  node* Left;
  node* Right;
  b32 Entered;
  void* CustomData;
};

struct tree
{
  u32 Size;
  node* Root;
};

struct raycast_result
{
  v3 RayOrigin;
  v3 RayDirection;
  v3 HitNormal;
  v3 Intersection;
  v3 IntersectionObjectSpace;
  b32 Hit;
  r32 Distance;
  node* Node;
};

//u32 GetAABBList(aabb_tree* Tree, aabb3f** Result);
//raycast_result RayCast( memory_arena* Arena, aabb_tree* Tree, v3 const & RayOrigin, v3 const & Direction);
//void PointPick( memory_arena* Arena, aabb_tree* Tree, v3* point, v3 direction, vector_list<u32> & Result);


// Tree points to a existing broadphase::tree
// Leaf points to empty memory big enought to contain a broadphase::node
// Node points to empty memory big enought to contain a broadphase::node
// AABB is the AABB in world-space
// Custom data is any data one wants to associate with the node. Usually some identifier struct.
void Insert(tree* Tree, aabb3f AABB, void* (*Allocate)(u32 Size), void* CustomData = 0);

struct collision_result
{
  node* Node1;
  node* Node2;
  collision_result* Previous;
};

collision_result* GetCollisionPairs( tree* Tree,  u32* ResultStackSize);

}

