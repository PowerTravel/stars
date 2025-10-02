#pragma once

#include "commons/types.h"
#include "commons/jstring.h"

namespace asset {

  enum class type {
    NONE,
    IMAGE,
    MESH,
    MATERIAL,
    RENDER_GROUP,
    PBR_MATERIAL
  };


  const c8* TypeToString(type Type)
  {
    switch(Type)
    {
      case type::IMAGE: return "IMAGE";
      case type::MESH: return "MESH";
      case type::MATERIAL: return "MATERIAL";
      case type::RENDER_GROUP: return "RENDER_GROUP";
      case type::PBR_MATERIAL: return "PBR_MATERIAL";
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



  // Basic Asset Type
  struct image
  {
    int Height;
    int Width;
    int Channels;
    uint8_t* Pixels;
  };


  namespace gltf_tmp {

    typedef int mesh_id;
    typedef int pbr_material_id;
  

    // A mesh primitive mesh
    struct mesh // mesh_id (Can be OBJ as well)
    {
      enum class topology {
        POINTS,
        LINES,
        LINE_LOOP,
        LINE_STRIP,
        TRIANGLES,
        TRIANGLE_STRIP,
        TRIANGLE_FAN
      };

      int IndexCount;
      int* Indeces;

      int VertexCount;
      v3* Vertex;     // Vertices
      int VertexNormalCount;
      v3* VertexNormal;    // Vertice Normals

      int TextureVertexSetCount;
      int* TextureVertexCounts;
      v2** TextureVertices;    // Texture Vertices

      topology Topology;

      aabb3f AABB;
    };


    // Things needed for pbr-rendering
    struct pbr_material //  pbr_material_id
    {
      // Maps a Image and info on how to display it 
      struct texture {
        enum class filter {
          NEAREST,
          LINEAR,
          NEAREST_MIPMAP_NEAREST,
          LINEAR_MIPMAP_NEAREST,
          NEAREST_MIPMAP_LINEAR,
          LINEAR_MIPMAP_LINEAR
        };

        enum class wrap {
          CLAMP_TO_EDGE,
          MIRRORED_REPEAT,
          REPEAT,
        };

        image* Image; // required
        int TexCoord;   // required - References the mesh the material is attached to
        filter MagFilter;
        filter MinFilter;
        wrap WrapS;
        wrap WrapT;
      };

      struct metallic_roughness
      {
        v4 BaseColorFactor;
        texture* BaseColorTexture;
        float MetallicFactor;
        float RoughnessFactor;
        texture* MetallicRoughnessTexture;
      };

      struct occlusion_texture {
        texture Texture;
        float Strength;
      };

      struct normal_texture {
        texture Texture;
        float Scale;
      };

      metallic_roughness* MetallicRoughness;
      normal_texture* NormalTexture;
      occlusion_texture* OcclusionTexture;
      texture* EmissiveTexture;
      v3 EmissiveFactor;
      cmn::string AlphaMode;
      float AlphaCutoff;
      bool DoubleSided;
    };

    struct render_asset { // render_asset_id
      
      struct mesh_info {
        mesh* Mesh;
        pbr_material* Material;
      };

      struct node {

        size_t ChildCount;
        node* Parent;
        node* NextSibling;
        node* PreviousSibling;
        node* FirstChild;

        size_t MeshCount;
        mesh_info* MeshInfos;

        bool HasTransform;
        m4 Transform;
      };

      size_t NodeCount;
      node* Nodes;
      node* Root;
    };
  } // namespace gltf_tmp

}