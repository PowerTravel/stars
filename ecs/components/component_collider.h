#pragma once

#include "commons/types.h"

namespace ecs{ 
namespace collider {

struct mesh
{
  int nvi;  // 3 times nr Vertice Indeces (CCW Triangles)
  int* vi;  // Vertex Indeces

  int nv;   // Nr Vertices
  v3* v;    // Vertices
};

struct component
{
  aabb3f AABB;
  mesh Mesh;
};

void Init(component* Component, asset::gltf_tmp::mesh* Mesh)
{
  collider::mesh ColliderMesh = {};
  Component->Mesh.nvi = Mesh->IndexCount;
  Component->Mesh.nv  = Mesh->VertexCount;
  Component->Mesh.vi  = Mesh->Indeces;
  Component->Mesh.v   = Mesh->Vertex;
  Component->AABB     = Mesh->AABB;
}


}
}