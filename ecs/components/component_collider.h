#pragma once

#include "commons/types.h"

namespace ecs{ 
namespace collider {

struct mesh
{
  u32 nv;   // Nr Vertices
  u32 nvi;  // 3 times nr Vertice Indeces (CCW Triangles)

  v3* v;    // Vertices
  u32* vi;  // Vertex Indeces
};

struct component
{
  aabb3f AABB;
  mesh* Mesh;
};


}
}