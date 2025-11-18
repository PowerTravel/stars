#pragma once

#include "commons/types.h"
#include "commons/jstring.h"
#include "cmn/n_tree.h"


namespace asset {

  typedef size_t key;

  typedef key image_id;
  typedef key mesh_id;
  typedef key pbr_material_id;
  typedef key phong_material_id;
  typedef key camera_id;
  typedef key render_tree_id;
  typedef key package_id;

  enum class type {
    NONE,
    IMAGE,
    MESH,
    PHONG_MATERIAL,
    PBR_MATERIAL,
    RENDER_TREE,
    CAMERA,
    PACKAGE,
  };

  const c8* TypeToString(type Type)
  {
    switch(Type)
    {
      case type::IMAGE:          return "IMAGE";
      case type::MESH:           return "MESH";
      case type::PHONG_MATERIAL: return "PHONG_MATERIAL";
      case type::PBR_MATERIAL:   return "PBR_MATERIAL";
      case type::RENDER_TREE:  return "RENDER_TREE";
      case type::CAMERA:         return "CAMERA";
      case type::PACKAGE:        return "PACKAGE";  
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
      REPEAT
    };

    image_id Image;
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

    v4*  Ka; // Ambient
    v4*  Kd; // Diffuse
    v4*  Tf; // Transmission
    v4*  Ks; // Spekular
    v4*  Ke; // Emissive
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


  // A mesh primitive mesh
  struct mesh {
    struct primitive // mesh_id (Can be OBJ as well)
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

      pbr_material_id PbrMaterial;
      phong_material_id PhongMaterial;
    };

    size_t PrimitiveCount;
    primitive* Primitives;

  };

  struct camera {
    enum class type {
      PERSPECTIVE,
      ORTHOGRAPHIC
    };
    struct orthographic {
      float XMag;
      float YMag;
      float ZFar;
      float ZNear;
    };

    struct perspective {
      // When undefined, the aspect ratio of the rendering viewport MUST be used.
      float AspectRatio;
      float YFov;
      // If zfar is undefined, client implementations SHOULD use infinite projection matrix
      float ZFar;
      float ZNear;
    };

    type Type;
    union {
      orthographic Orthographic;
      perspective Perspective;
    };
  };

  struct render_tree_data {
    mesh_id Mesh;
    camera_id Camera;
    bool HasTransform;
    m4 Transform;
  };

  typedef cmn::n_tree<render_tree_data> render_tree;

  // A package is a collection of assets grouped by being loaded by the same base file such as OBJ or GLTF.
  // Files like PNGs or similar do not get a package file.
  struct package {
    size_t PhongMaterialCount;
    phong_material_id* PhongMaterials;

    size_t PBRMaterialCount;
    pbr_material_id* PBRMaterials;

    size_t CameraCount;
    camera_id* Cameras;

    size_t ImageCount;
    image_id* Images;

    size_t MeshCount;
    mesh_id* Meshes;

    size_t RenderTreeCount;
    render_tree_id* RenderTrees;

    size_t DefaultRenderTree;
  };
}