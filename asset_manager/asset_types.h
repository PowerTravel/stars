#pragma once

#include "commons/types.h"

namespace asset {

  enum class type {
    NONE,
    MESH,
    TEXTURE,
    MATERIAL,
    RENDER_GROUP
  };


  const c8* TypeToString(type Type)
  {
    switch(Type)
    {
      case type::MESH: return "MESH";
      case type::TEXTURE: return "TEXTURE";
      case type::MATERIAL: return "MATERIAL";
      case type::RENDER_GROUP: return "RENDER_GROUP";
      default: {
        INVALID_CODE_PATH
      }
    }
    return "\0";
  }

  struct string {
    u32 Length; // Length of string + 1;
    c8* String;
  };


// Map from obj to gl-mesh, but keep each v/vn/vt separated into their own vectors.
  struct mesh {
    u32 IndexCount;
    u32* vi;
    u32* vni;
    u32* vti;

    u32 vCount;
    v3* v;     // Vertices
    u32 vnCount;
    v3* vn;    // Vertice Normals
    u32 vtCount;
    v2* vt;    // Texture Vertices

    aabb3f AABB;
  };

  enum class texture_type {
    DIFFUSE_COLOR,
    SPECULAR_COLOR,
    BUMP_MAP
  };

  struct texture {
    texture_type Type;
    u32 BPP; // Bits Per pixel 8,16,24,32
    u32 Width;
    u32 Height;
    bptr Pixels;
  };

  struct material {
    v4*  Ka;
    v4*  Kd;
    v4*  Tf;
    v4*  Ks;
    v4*  Ke;
    r32* d;  // Specifies the dissolve for the current material
    r32* Ni; // Index of refraction
    r32* Ns; // Specifies the specular exponent for the current material.

    r32 BumpMapBM;
    u32 BumpMapHandle;
    u32 MapKdHandle;
    u32 MapKsHandle;
  };

  // Combines a mesh (a shape), with a material (How its rendered)
  // Does not have a header
  struct render_group_element {
    // Obj_groups that share a smoothing group have joined edges that should be smoothe
    // -1 means _no smothing group used_
    s32 SmoothingGroup;

    mesh* Mesh;
    material* Material;
  };

  // A collection of shapes and materials that makes up an object.
  // A render object can reference this render_group.
  struct render_group {
    u32 ElementCount;
    render_group_element* Elements;
  };

}