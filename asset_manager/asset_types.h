#pragma once

#include "commons/types.h"
#include "commons/jstring.h"

namespace asset {

  enum class type {
    NONE,
    IMAGE,
    MESH,
    PHONG_MATERIAL,
    PBR_MATERIAL,
    RENDER_TREE
  };


  const c8* TypeToString(type Type)
  {
    switch(Type)
    {
      case type::IMAGE: return "IMAGE";
      case type::MESH: return "MESH";
      case type::PHONG_MATERIAL: return "PHONG_MATERIAL";
      case type::PBR_MATERIAL: return "PBR_MATERIAL";
      case type::RENDER_TREE: return "RENDER_TREE";
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


  // Basic Asset Type
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

  // Basic Asset Type
  struct image
  {
    int Height;
    int Width;
    int Channels;
    uint8_t* Pixels;
  };

  // __Not__ asset type
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

    image* Image; // required // TODO: Change back to using a imgage handle.
    //            Reason: Since the asset-api works such that it takes a loadable asset and copies the struct into the asset_manager memory,
    //                    we don't know if this image is loaded into _our_ memory or not unless we use a handle.
    int TexCoord; // required - References the mesh the material is attached to
    filter MagFilter;
    filter MinFilter;
    wrap WrapS;
    wrap WrapT;
  };

  texture DefaultTexture(u32 ImageHandle);

  struct phong_material { 
    v4*  Ka;
    v4*  Kd;
    v4*  Tf;
    v4*  Ks;
    v4*  Ke;
    r32* d;  // Specifies the dissolve for the current material
    r32* Ni; // Index of refraction
    r32* Ns; // Specifies the specular exponent for the current material.

    r32 BumpMapBM;
    b32 HasBumpMap;
    texture BumpMap;
    b32 HasDiffuseTexture;
    texture DiffuseTexture;
    b32 HasSpecularTexture;
    texture SpecularTexture;
  };


  // Things needed for pbr-rendering
  // Asset Type
  struct pbr_material
  {
    struct metallic_roughness
    {
      v4 BaseColorFactor;
      bool HasBaseColorTexture;
      texture BaseColorTexture;
      float MetallicFactor;
      float RoughnessFactor;
      bool HasMetallicRoughnessTexture;
      texture MetallicRoughnessTexture;
    };

    struct occlusion_texture {
      texture Texture;
      float Strength;
    };

    struct normal_texture {
      texture Texture;
      float Scale;
    };

    bool HasMetallicRoughness;
    metallic_roughness MetallicRoughness;

    bool HasNormalTexture;
    normal_texture NormalTexture;

    bool HasOcclusionTexture;
    occlusion_texture OcclusionTexture;

    bool HasEmissiveTexture;
    texture EmissiveTexture;

    v3 EmissiveFactor;
    cmn::string AlphaMode;
    float AlphaCutoff;
    bool DoubleSided;
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

      // Vertex, VertexNormal and each of the TextureVertices* Must have the same size of VertexCount if they exist
      int VertexCount;
      v3* Vertex;     // Vertices
      v3* VertexNormal;    // Vertice Normals

      int TextureVertexSetCount;
      v2** TextureVertices;    // Texture Vertices

      topology Topology;

      aabb3f AABB;
    };

    struct render_tree { // render_asset_id
      
      struct mesh_info {
        mesh* Mesh;
        //union {
          pbr_material* Material;
          phong_material* PhongMaterial;
        //}
      };

      struct node {

        size_t ChildCount;
        node* Parent;
        node* NextSibling;
        node* PreviousSibling;
        node* FirstChild;

        size_t MeshInfoCount;
        mesh_info* MeshInfos;

        bool HasTransform;
        m4 Transform;
      };

      size_t MeshInfoCount;
      mesh_info* MeshInfos;

      size_t NodeCount;
      node* Nodes;
      node* Root;
    };
  } // namespace gltf_tmp

}