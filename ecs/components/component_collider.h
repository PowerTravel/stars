#pragma once

#include "commons/types.h"

namespace ecs { 
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

void Init(component* Component, asset::geometry* Geometry)
{
  // We need to decide how we want to handle using render_mesh as a physics mesh
  // This is just to alert us if we run into trying to make a physics-mesh of a render-mesh with several primitives
  Component->Mesh.nvi = Geometry->IndexCount;
  Component->Mesh.nv  = Geometry->VertexCount;
  Component->Mesh.vi  = Geometry->Indeces;
  Component->Mesh.v   = Geometry->Vertex;
  Component->AABB     = Geometry->AABB;
}


}
}