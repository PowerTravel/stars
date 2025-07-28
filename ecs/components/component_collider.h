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

void Init(component* Component, asset::mesh* Mesh)
{
  collider::mesh ColliderMesh = {};
  Component->Mesh.nvi = Mesh->IndexCount;
  Component->Mesh.nv  = Mesh->vCount;
  Component->Mesh.vi  = Mesh->vi;
  Component->Mesh.v   = Mesh->v;
  Component->AABB     = Mesh->AABB;
}


}
}