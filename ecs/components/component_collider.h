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

  // We need to decide how we want to handle using render_mesh as a physics mesh
  // This is just to alert us if we run into trying to make a physics-mesh of a render-mesh with several primitives
  Assert(Mesh->PrimitiveCount == 1);
  asset::gltf_tmp::mesh::primitive* Primitive = Mesh->Primitives;
  Component->Mesh.nvi = Primitive->IndexCount;
  Component->Mesh.nv  = Primitive->VertexCount;
  Component->Mesh.vi  = Primitive->Indeces;
  Component->Mesh.v   = Primitive->Vertex;
  Component->AABB     = Primitive->AABB;
}


}
}