#pragma once

#include "commons/types.h"

namespace ecs{ 
namespace collider {

struct mesh
{
  u32 nvi;  // 3 times nr Vertice Indeces (CCW Triangles)
  u32 nv;   // Nr Vertices

  u32* vi;  // Vertex Indeces
  v3* v;    // Vertices
};

struct component
{
  aabb3f AABB;
  mesh Mesh;
};

component NewComponent(asset::mesh* Mesh)
{
  component Result = {};
  collider::mesh ColliderMesh = {};
  Result.Mesh.nvi = Mesh->IndexCount;
  Result.Mesh.nv  = Mesh->vCount;
  Result.Mesh.vi  = Mesh->vi;
  Result.Mesh.v   = Mesh->v;
  Result.AABB     = Mesh->AABB;
  return Result;
}


}
}