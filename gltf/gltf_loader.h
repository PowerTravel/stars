#pragma once

#include "externals/json.hpp"
#include "commons/jstring.h"
#include "commons/memory.h"
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#include "externals/stb_image.h"

namespace gltf {

#define GLTF_READ_ENTIRE_FILE(name) void* name( const c8* Filename, size_t* FileSize )
typedef GLTF_READ_ENTIRE_FILE( gltf_read_entire_file );

#define GLTF_FREE_FILE_MEMORY(name) void name( void* Memory )
typedef GLTF_FREE_FILE_MEMORY( gltf_free_file_memory );

  enum class primitive_mode {
    POINTS, // 0 
    LINES, // 1 
    LINE_LOOP, // 2 
    LINE_STRIP, // 3 
    TRIANGLES, // 4 
    TRIANGLE_STRIP, // 5 
    TRIANGLE_FAN, // 6 
  };

  struct raw_attribute {

    struct attribute_type{
    
      enum class type {
        ERROR,
        POSITION,   /* VEC3      - float - Unitless XYZ vertex positions */
        NORMAL,     /* VEC3      - float - Normalized XYZ vertex normals 29 */
        TANGENT,    /* VEC4      - float - XYZW vertex tangents where the XYZ portion is normalized, and the W component is a sign value (- 1 or +1) indicating handedness of the tangent basis */
        TEXCOORD ,  /* VEC2      - float - unsigned byte normalized, unsigned short normalized, - ST texture coordinates */
        COLOR,      /* VEC3 VEC4 - float unsigned byte normalized, unsigned short normalized - RGB or RGBA vertex color linear multiplier */
        JOINTS,     /* VEC4      - unsigned byte, unsigned short, - See Skinned Mesh Attributes */
        WEIGHTS,    /* VEC4      - float unsigned byte normalized, unsigned short normalized - See Skinned Mesh Attributes */
      };
      int Index;
      type Type;
    };

    attribute_type Type;
    int Index;
  };

  struct raw_primitive {

    // Key: attributes
    // Required: Yes
    // Note: A plain JSON object, where each key corresponds to a mesh attribute semantic and each value is the index of the accessor containing attribute’s data.
    size_t AttributeCount;
    raw_attribute* Attributes;

    // Key: indices
    // Required: No
    // Note: The index of the accessor that contains the vertex indices. If No indices is 
    int* Indices;

    // Key: material
    // Required: No
    // Note: The index of the material to apply to this primitive when rendering.
    int* Material;

    // Key: mode
    // Required: No, default 4 (TRIANGLES)
    // Note: The topology type of primitives to render.
    primitive_mode Mode;
    
    // Not implemented:
    
    // Key: targets
    // Required: No
    // Note: An array of morph targets
    // size_t TargetCount;
    // target* Targets;
    
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct extracted_primitive {
    int IndexCount;
    int* Indeces;

    int vCount;
    v3* v;     // Vertices
    int vnCount;
    v3* vn;    // Vertice Normals

    int vtSetCount;
    int* vtCount;
    v2** vt;    // Texture Vertices

    v3 vMin;
    v3 vMax;

    primitive_mode Mode;
  
    int* MaterialIndex;
  };

  struct raw_mesh {

    // key: primitives
    // Required: Yes
    // Note: n array of primitives, each defining geometry to be rendered.
    size_t PrimitiveCount;
    raw_primitive* Primitives;

    // key: weights
    // Required: No
    // Note: Array of weights to be applied to the morph targets. The number of array elements MUST match the number of morph targets.
    size_t WeightCount;
    int* Weights;

    // key: name
    // Required: No
    // Note: The user-defined name of this object
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)

    size_t ExtractedPrimitiveCount;
    extracted_primitive* ExtractedPrimitives;
  };

  struct raw_node {

    enum class transformation_type {
      NONE,
      MATRIX,
      TRS
    };


    // key: children
    // Reqiuired:
    // Note: The indices of this node’s children. If key exist they must have at least one child.
    int ChildCount;
    int* Children;

    transformation_type TransformationType;

    // key: matrix
    // Reqiuired: No default [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
    // Note: 16 A floating-point 4x4 transformation matrix stored in column-major order.
    //       A node may have _either_ a Matrix or any combination of Rotation, Translation, Scale
    m4 Matrix;

    // key: mesh
    // Reqiuired: No
    // Note: The index of the mesh in this node
    int* Mesh;

    // key: rotation
    // Reqiuired: No, default [0,0,0,1]
    // Note: A floating-point 4x4 transformation matrix stored in column-major order.
    //       A node may have _either_ a Matrix or any combination of Rotation, Translation, Scale
    quat Rotation;

    // key: scale
    // Reqiuired: No, default [1,1,1]
    // Note: The node’s non-uniform scale, given as the scaling factors along the x, y, and z axes.
    //       A node may have _either_ a Matrix or any combination of Rotation, Translation, Scale
    v3 Scale;

    // key: translation
    // Reqiuired: No, default [0,0,0]
    // Note: The node’s translation along the x, y, and z axes.
    //       A node may have _either_ a Matrix or any combination of Rotation, Translation, Scale
    v3 Translation;

    // key: weights
    // required: No [1-*]
    // Note: The weights of the instantiated morph target. The number of array elements MUST match the number of morph targets of the referenced mesh.
    //       When defined, mesh MUST also be defined.
    int WeightCount;
    int* Weights;

    // key: skin
    // Reqiuired: No
    // Note: The index of the skin referenced by this node.
    //       When the node contains skin, all mesh.primitives MUST contain JOINTS_0 and WEIGHTS_0 attributes.
    int* Skin;

    // key: camera
    // Required: No
    // Note: The index of the camera referenced by this node.
    int* Camera;

    // key: name
    // Required: No [1]
    // Note: The user-defined name of this object
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  // A view into a buffer generally representing a subset of the buffer
  struct raw_buffer_view {

    enum class target{
      NONE,
      ARRAY_BUFFER,        // 34962
      ELEMENT_ARRAY_BUFFER // 34963
    };

    // key: buffer
    // Required: Yes
    // Note: The index of the buffer.
    int Buffer;

    // key: byteOffset
    // Required: No, default 0
    // Note: The offset into the buffer in bytes.
    size_t ByteOffset;

    // key: byteLength
    // Required: Yes
    // Note: The length of the bufferView in bytes.
    size_t ByteLength;

    // key: byteStride
    // Required: No
    // Note: The stride, in bytes.
    int* ByteStride;

    // key: target
    // Required: No
    // Note: The hint representing the intended GPU buffer type to use with this buffer view.
    //       Allowed Values: [34962 ARRAY_BUFFER, 34963 ELEMENT_ARRAY_BUFFER].
    target Target;

    // key: name
    // Required: No
    // Note: The user-defined name of this object
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_buffer {
    // A buffer points to binary geometry, animation, or skins.

    // key: uri
    // Required: No
    // Note: The URI (or IRI) of the buffer. Relative paths are relative to the current glTF asset.
    //       Instead of referencing an external file, this field MAY contain a data:-URI.
    cmn::string Uri;

    // key: byteLength
    // Required: Yes
    // Note: The length of the buffer in bytes.
    size_t ByteLength;

    // key: name
    // Required: No
    // Note: The user-defined name of this object
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)


    size_t LoadedSize;
    uint8_t* LoadedData;

  };

  struct raw_accessor{

    enum class component_type
    {
      BYTE = 5120,
      UNSIGNED_BYTE = 5121,
      SHORT = 5122,
      UNSIGNED_SHORT = 5123,
      UNSIGNED_INT = 5125,
      FLOAT = 5126,
    };

    enum class type {
      SCALAR,
      VEC2,
      VEC3,
      VEC4,
      MAT2,
      MAT3,
      MAT4,
      INVALID
    };

    struct sparse {

      struct indices {

        // key: bufferView
        // Required: Yes
        // Note: The index of the buffer view with sparse indices. The referenced buffer view MUST NOT have its target or byteStride properties defined.
        //       The buffer view and the optional byteOffset MUST be aligned to the componentType byte length.
        int BufferView;

        // key: byteOffset
        // Required: No, default 0
        // Note: The offset relative to, the start of the buffer, view in bytes.
        size_t ByteOffset;

        // key: componentType
        // Required: Yes
        // Note: The indices data type.
        //       Allowed Values: [5121 UNSIGNED_BYTE, 5123 UNSIGNED_SHORT, 5125 UNSIGNED_INT]
        component_type ComponentType;

        // Not implemented:
        // key: extensions (not required)
        // key: extras     (not required)
      };

      struct values {
        // key: bufferView
        // Required: Yes
        // Note: The index of the bufferView with sparse values. The referenced buffer view MUST NOT have its target or byteStride properties defined.
        int BufferView;

        // key: byteOffset
        // Required: No, default 0
        // Note: The offset relative to, the start of the buffer, view in bytes.
        size_t ByteOffset;

        // Not implemented:
        // key: extensions (not required)
        // key: extras     (not required)
      };

      // key: count
      // Required: Yes
      // Note: Number of deviating accessor values stored in the sparse array.
      size_t Count;

      // key: indices
      // Required: Yes
      // Note: An object pointing to a buffer view containing the indices of deviating accessor values.
      //       The number of indices is equal to count. Indices MUST strictly increase.
      indices Indices;

      // key: values
      // Required: Yes
      // Note: An object pointing to a buffer view containing the deviating accessor values.
      values Values;
      
      // Not implemented:
      // key: extensions (not required)
      // key: extras     (not required)
    };

    // key: bufferView
    // Required: No
    // Note: The index of the bufferView.
    int* BufferView;

    // key: byteOffset
    // Required: No, default: 0
    // Note: The offset relative to the start of the buffer view in bytes.
    size_t ByteOffset;

    // key: componentType
    // Required: Yes
    // Note: The datatype of the accessor’s components.
    //       Allowed values [5120 BYTE, 5121 UNSIGNED_BYTE, 5122 SHORT, 5123 UNSIGNED_SHORT, 5125 UNSIGNED_INT, 5126 FLOAT]
    component_type ComponentType;

    // key: normalized
    // Required: No, default: false
    // Note: Specifies whether integer data values are normalized before usage.
    bool Normalized;

    // key: count
    // Required: Yes
    // Note: The number of elements referenced by this accessor.
    size_t Count;

    // key: type
    // Required: Yes
    // Note: Specifies if the accessor’s elements are scalars, vectors, or matrices.
    type Type;

    // key: max
    // Required: No
    // Note: Maximum value of each component in this accessor.
    //       Valid values for MaxCount is: 1, 2, 3, 4, 9, or 16 which is determined by component_type. Ex Mat3 is 9 and scalar is 1.
    //       The type of Max is the same as ComponentType
    int MaxCount;
    void* Max;

    // key: min
    // Required: 
    // Note: Minimum value of each component in this accessor.
    //       Valid values for MaxCount is: 1, 2, 3, 4, 9, or 16 which is determined by component_type. Ex Mat3 is 9 and scalar is 1.
    //       The type of Max is the same as ComponentType
    int MinCount;
    void* Min;

    // key: sparse
    // Required: No
    // Note: Sparse storage of elements that deviate from their initialization value.
    sparse* Sparse;

    // key: name
    // Required:  No
    // Note: The user-defined name of this object.
    cmn::string Name;


    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_texture {

    // key: sampler
    // Required: No
    // Note: The index of the sampler used by this texture. When undefined, a sampler with repeat wrapping and auto filtering SHOULD be used.
    int* Sampler;

    // key: source
    // Required: No
    // Note: The index of the image used by this texture. When undefined, an extension or other mechanism SHOULD
    //       supply an alternate texture source, otherwise behavior is undefined.
    int* Source;

    // key: name
    // Required: No
    // Note: The user-defined name of this object.
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_image{
    enum class mime_type {
      NONE,
      IMAGE_JPEG, // "image/jpeg"
      IMAGE_PNG   // "image/png"
    };

    static const char MIME_TYPE_JPEG[];
    static const char MIME_TYPE_PNG[];

    // key: uri
    // Required: No
    // Note: The URI (or IRI) of the image.
    cmn::string Uri;

    // key: mimeType
    // Required: No
    // Note: The image’s media type. This field MUST be defined when bufferView is defined.
    //       Allowed values: "image/jpeg", "image/png";
    mime_type MimeType;

    // key: bufferView
    // Required: No
    // Note: The index of the bufferView that contains the image. This field MUST NOT be defined when uri is defined.
    int* BufferView;

    // key: name
    // Required: No
    // Note: The user-defined name of this object.
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)

    int Channels;
    int Width;
    int Height;
    uint8_t* Pixels;
  };

  const char raw_image::MIME_TYPE_JPEG[] = "image/jpeg";
  const char raw_image::MIME_TYPE_PNG[]  = "image/png";

  // Texture sampler properties for filtering and wrapping modes
  struct raw_sampler {
    enum class filter {
      NONE = 0,
      NEAREST = 9728,
      LINEAR = 9729,
      NEAREST_MIPMAP_NEAREST = 9984,
      LINEAR_MIPMAP_NEAREST = 9985,
      NEAREST_MIPMAP_LINEAR = 9986,
      LINEAR_MIPMAP_LINEAR = 9987
    };

    enum class wrap {
      CLAMP_TO_EDGE = 33071,
      MIRRORED_REPEAT = 33648,
      REPEAT = 10497 // (Default)
    };
    // key: magFilter
    // Required: No
    // Note: Magnification filter.
    filter MagFilter;

    // key: minFilter
    // Required: No
    // Note: Minification filter.
    filter MinFilter;
    
    // key: wrapS 
    // Required: No, default: 10497 (REPEAT)
    // Note: S (U) wrapping mode.
    wrap WrapS;
    
    // key: wrapT 
    // Required: No, default: 10497 (REPEAT)
    // Note: T (V) wrapping mode.
    wrap WrapT;

    // key: name
    // Required: No
    // Note: The user-defined name of this object.
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_texture_info{
    // key: index
    // Required: Yes
    // Note: The index of the texture
    int Index;

    // key: texCoord
    // Required: No, default 0
    // Note: The set index of texture’s TEXCOORD attribute used for texture coordinate mapping.
    int TexCoord;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_material_occlusion_texture_info {
    // key: index
    // Required: Yes
    // Note: The index of the texture
    int Index;

    // key: texCoord
    // Required: No, default 0
    // Note: The set index of texture’s TEXCOORD attribute used for texture coordinate mapping.
    int TexCoord;

    // key: strength
    // Required: No, default 1
    // Note: A scalar multiplier controlling the amount of occlusion applied.
    float Strength;
    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_material_normal_texture_info {
    // key: index
    // Required: Yes
    // Note: The index of the texture
    int Index;

    // key: texCoord
    // Required: No, default 0
    // Note: The set index of texture’s TEXCOORD attribute used for texture coordinate mapping.
    int TexCoord;

    // key: scale
    // Required: No, default 1
    // Note: The scalar parameter applied to each normal vector of the normal texture.
    float Scale;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_pbr_metallic_roughness
  {
    // key: baseColorFactor
    // Required: No, default [1,1,1,1] 
    // Note: The factors for the base color of the material.
    v4 BaseColorFactor;

    // key: baseColorTexture
    // Required: No
    // Note: The base color texture.
    raw_texture_info* BaseColorTexture;

    // key: metallicFactor
    // Required: No, default 1
    // Note: The factor for the metalness of the material.
    float MetallicFactor;

    // key: roughnessFactor
    // Required: No, default 1
    // Note: The factor for the roughness of the material.
    float RoughnessFactor;

    // key: metallicRoughnessTexture
    // Required: No
    // Note: The metallic-roughness texture.
    raw_texture_info* MetallicRoughnessTexture;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)
  };

  struct raw_material {
    
    // key: name
    // Required: No
    // Note: The user-defined name of this object
    cmn::string Name;

    // Not implemented:
    // key: extensions (not required)
    // key: extras     (not required)

    // key: pbrMetallicRoughness
    // Note: A set of parameter values that are used to define the metallic- roughness material model from Physically Based Rendering (PBR) methodology.
    //       When undefined, all the default values of pbrMetallicRoughness MUST apply.
    raw_pbr_metallic_roughness* PbrMetallicRoughness;
    
    // key: normalTexture
    // Required: No
    // Note: The tangent space normal texture
    raw_material_normal_texture_info* NormalTexture;

    // key: occlusionTexture
    // Required: No
    // Note: The occlusion texture.
    raw_material_occlusion_texture_info* OcclusionTexture;

    // key: emissiveTexture
    // Required: No
    // Note: The emissive texture.
    raw_texture_info* EmissiveTexture;

    // key: emissiveFactor
    // Required: No, default [0,0,0]
    // Note: The factors for the emissive color of the material.
    v3 EmissiveFactor;

    // key: alphaMode
    // Required: No, default OPAQUE
    // Note: The alpha rendering mode of the material.
    cmn::string AlphaMode;

    // key: alphaCutoff
    // Required: No, default 0.5
    // Note: The alpha cutoff value of the material.
    float AlphaCutoff;

    // key: doubleSided
    // Required: No, default false
    // Note: Specifies whether the material is double sided
    bool DoubleSided;
  };

  struct raw_scene {
    cmn::string Name;
    int NodeCount;
    int* Nodes;

    // Extensions, Extras omitted
  };



  struct raw_gltf_data {
    int DefaultSceneIndex;
    size_t RawSceneCount;
    raw_scene* RawScenes;
    size_t RawNodeCount;
    raw_node* RawNodes;
    size_t RawMeshCount;
    raw_mesh* RawMeshes;
    size_t RawMaterialCount;
    raw_material* RawMaterials;
    size_t RawAccessorsCount;
    raw_accessor* RawAccessors;
    size_t BufferViewCount;
    raw_buffer_view* RawBufferViews;
    size_t BufferCount;
    raw_buffer* RawBuffers;
    size_t RawSamplerCount;
    raw_sampler* RawSamplers;
    size_t RawImageCount;
    raw_image* RawImages;
    size_t RawTextureCount;
    raw_texture* RawTextures;
  };

  size_t JsonToIntArray(const nlohmann::json& j, int** Array)
  {
    size_t Count = j.size();
    int* IntArr = JwinAllocArray(Count, int);
    int i = 0;
    for (const nlohmann::json& JsonNode : j)
    {
      IntArr[i++] = JsonNode.get<int>();
    }
    *Array = IntArr;
    
    return Count;
  }

  #define JsonToArrayTemplate(Name, Type) \
  size_t Name(const nlohmann::json& j, Type** Array) { \
    size_t Count = j.size(); \
    Type* Arr = JwinAllocArray(Count, Type); \
    int i = 0; \
    for (const nlohmann::json& JsonNode : j) \
    { \
      Arr[i++] = JsonNode.get<Type>(); \
    } \
    *Array = Arr; \
    return Count; \
  } 

  JsonToArrayTemplate(JsonToByteArray, char);
  JsonToArrayTemplate(JsonToUnsignedByteArray, unsigned char);
  JsonToArrayTemplate(JsonToShortArray, short);
  JsonToArrayTemplate(JsonToUnsignedShortArray, unsigned short);
  JsonToArrayTemplate(JsonToUnsignedIntArray, unsigned int);
  JsonToArrayTemplate(JsonToFloatArray, float)
/*
  size_t JsonToByteArray(const nlohmann::json& j, char** Array){
    size_t Count = j.size();
    char* Arr = JwinAllocArray(Count, char);
    int i = 0;
    for (const nlohmann::json& JsonNode : j)
    {
      Arr[i++] = JsonNode.get<char>();
    }
    *Array = Arr;
    
    return Count;
  }

  size_t JsonToUnsignedByteArray(const nlohmann::json& j, unsigned char** Array) {
    size_t Count = j.size();
    char* Arr = JwinAllocArray(Count, char);
    int i = 0;
    for (const nlohmann::json& JsonNode : j)
    {
      Arr[i++] = JsonNode.get<char>();
    }
    *Array = Arr;
    
    return Count;
  }

  size_t JsonToShortArray(const nlohmann::json& j, void** Array){

  }
  size_t JsonToUnsignedShortArray(const nlohmann::json& j, void** Array){

  }
  size_t JsonToUnsignedIntArray(const nlohmann::json& j, void** Array){

  }
  size_t JsonToFloatArray(const nlohmann::json& j, void** Array){

  }
*/

  size_t JsonToArray(const nlohmann::json& j, void** Array, raw_accessor::component_type ComponentType)
  {
    switch(ComponentType)
    {
      case raw_accessor::component_type::BYTE: return JsonToByteArray(j, (char**) Array); break;
      case raw_accessor::component_type::UNSIGNED_BYTE: return JsonToUnsignedByteArray(j, (unsigned char**) Array); break;
      case raw_accessor::component_type::SHORT: return JsonToShortArray(j, (short**) Array); break;
      case raw_accessor::component_type::UNSIGNED_SHORT: return JsonToUnsignedShortArray(j, (unsigned short**) Array); break;
      case raw_accessor::component_type::UNSIGNED_INT: return JsonToUnsignedIntArray(j, (unsigned int**) Array); break;
      case raw_accessor::component_type::FLOAT: return JsonToFloatArray(j, (float**) Array); break;
    }
    return 0;
  }


  m4 JsonToM4(const nlohmann::json& j)
  {
    m4 Result = {};
    int i = 0;
    Assert(j.size() == 16);
    for(const nlohmann::json& num : j)
    {
      Result.E[i++] = num.get<float>();
    }
    return Transpose(Result);
  }

  quat JsonToQuaternion(const nlohmann::json& j)
  {
    int i = 0;
    Assert(j.size() == 4);
    v4 Vals = {};
    for(const nlohmann::json& num : j)
    {
      Vals.E[i++] = num.get<float>();
    }

    quat Result = Vals;
    return Result;
  }

  v4 JsonToV4(const nlohmann::json& j)
  {
    int i = 0;
    Assert(j.size() == 4);
    v4 Result = {};
    for(const nlohmann::json& num : j)
    {
      Result.E[i++] = num.get<float>();
    }
    return Result;
  }

  v3 JsonToV3(const nlohmann::json& j)
  {
    int i = 0;
    Assert(j.size() == 3);
    v3 Result = {};
    for(const nlohmann::json& num : j)
    {
      Result.E[i++] = num.get<float>();
    }
    return Result;
  }

  cmn::string JsonToString(const nlohmann::json& j)
  {
    cmn::string Str = cmn::String(j.get<std::string>().c_str());
    return Str;
  }

  raw_node JsonToRawNode(nlohmann::json& j)
  {
    raw_node Result = {};

    if(j.contains("children"))
    {
      const nlohmann::json JsonChildren = j.at("children");
      Result.ChildCount = JsonToIntArray(JsonChildren, &Result.Children);
    }

    if(j.contains("camera"))
    {
      Result.Camera = JwinAllocStruct(int);
      *Result.Camera = j.at("camera").get<int>();
      Platform.DEBUGPrint("WARN: raw_node contains camera. No parser yet written. Ignoring.\n");
      Assert(0);
    }

    if(j.contains("skin"))
    {
      Result.Skin = JwinAllocStruct(int);
      *Result.Skin = j.at("skin").get<int>();
      Platform.DEBUGPrint("WARN: raw_node contains Skin. No parser yet written. Ignoring.\n");
      Assert(0);
    }

    if(j.contains("matrix"))
    {
      Assert(!j.contains("rotation"));
      Assert(!j.contains("scale"));
      Assert(!j.contains("translation"));
      Result.Matrix = JsonToM4(j.at("matrix"));
      Result.TransformationType = raw_node::transformation_type::MATRIX;
    }else{
      Result.Matrix = M4Identity();
    }

    if(j.contains("mesh"))
    {
      Result.Mesh = JwinAllocStruct(int);
      *Result.Mesh = j.at("mesh").get<int>();
    }

    if(j.contains("rotation"))
    {
      Assert(!j.contains("matrix"));
      Result.TransformationType = raw_node::transformation_type::TRS;
      Result.Rotation = JsonToQuaternion(j.at("rotation"));
    }else{
      Result.Rotation =  Quaternion();
    }

    if(j.contains("scale"))
    {
      Assert(!j.contains("matrix"));
      Result.TransformationType = raw_node::transformation_type::TRS;
      Result.Scale = JsonToV3(j.at("scale"));
    }else{
      Result.Scale = V3(1,1,1);
    }

    if(j.contains("translation"))
    {
      Assert(!j.contains("matrix"));
      Result.TransformationType = raw_node::transformation_type::TRS;
      Result.Translation = JsonToV3(j.at("translation"));
    }else{
      Result.Translation = V3(0,0,0);
    }

    if(j.contains("weights"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains weights. No parser yet written. Ignoring.\n");
      Assert(j.contains("mesh")); // Mesh is required if weights is set.
      Assert(0);
    }

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
      Platform.DEBUGPrint("%s\n", Result.Name.data);
    }

    if(j.contains("extensions"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains extensions. No parser yet written. Ignoring.\n");
      Assert(0);
    }
    if(j.contains("extras"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains extras. No parser yet written. Ignoring.\n");
      Assert(0);
    }

    return Result;
  }

  raw_attribute::attribute_type ToAttributeType( const char* Type ){
    raw_attribute::attribute_type Result = {};
    if(jstr::Equals("POSITION", Type))
    {
      Result.Index = 0;
      Result.Type = raw_attribute::attribute_type::type::POSITION;
    }
    else if(jstr::Equals("NORMAL", Type))
    {
      Result.Index = 0;
      Result.Type = raw_attribute::attribute_type::type::NORMAL;
    }
    else if(jstr::Equals("TANGENT", Type))
    {
      Result.Index = 0;
      Result.Type = raw_attribute::attribute_type::type::TANGENT;
    }
    else if(jstr::BeginsWith("TEXCOORD_", Type))
    {
      const char* Num = Type + 9;
      Result.Index = cmn::Stoi(Num);
      Result.Type = Result.Type = raw_attribute::attribute_type::type::TEXCOORD;
    }
    else if(jstr::Equals("COLOR_", Type))
    {
      const char* Num = Type + 6;
      Result.Index = cmn::Stoi(Num);
      Result.Type = Result.Type = raw_attribute::attribute_type::type::COLOR;
    }
    else if(jstr::Equals("JOINTS_", Type))
    {
      const char* Num = Type + 7;
      Result.Index = cmn::Stoi(Num);
      Result.Type = Result.Type = raw_attribute::attribute_type::type::JOINTS;
    }
    else if(jstr::Equals("WEIGHTS_", Type))
    {
      const char* Num = Type + 8;
      Result.Index = cmn::Stoi(Num);
      Result.Type = Result.Type = raw_attribute::attribute_type::type::WEIGHTS;
    }else{
      Platform.DEBUGPrint("Note: Loading gltf file with type: '%s' which is not supported or expected.\n", Type);
      Result.Index = 0;
      Result.Type = Result.Type = raw_attribute::attribute_type::type::ERROR;
    }
    
    return Result;
  }

  raw_attribute RawAttribute(const char* Key, int Value)
  {
    raw_attribute Result = {};
    Result.Type = ToAttributeType(Key);
    Result.Index = Value;
    return Result;
 }

  raw_primitive JsonToPrimitive(const nlohmann::json& j)
  {
    raw_primitive Result = {};
    Assert(j.contains("attributes"));
    const nlohmann::json& JsonAttributes = j.at("attributes");
    Result.AttributeCount = JsonAttributes.size();
    Result.Attributes =  JwinAllocArray(Result.AttributeCount, raw_attribute);
    int i = 0;
    for(auto& Attribute : JsonAttributes.items())
    {
      Result.Attributes[i++] = RawAttribute(Attribute.key().c_str(), Attribute.value());
    }

    if(j.contains("indices"))
    {
      Result.Indices = JwinAllocStruct( int);
      *Result.Indices = j.at("indices").get<int>();
    }

    if(j.contains("material"))
    {
      Result.Material = JwinAllocStruct( int);
      *Result.Material = j.at("material").get<int>();
    }


    Result.Mode = primitive_mode::TRIANGLES;
    if(j.contains("mode"))
    {
      Result.Mode = (primitive_mode) j.at("mode").get<int>();
      if(Result.Mode != primitive_mode::TRIANGLES)
      {
        Platform.DEBUGPrint("Note: Primitive mode is %d which is different from %d (TRIANGLES). Unless handles will break rendering.\n",
          (int) Result.Mode, primitive_mode::TRIANGLES);
      }
    }

    
    if(j.contains("targets"))
    {
      Platform.DEBUGPrint("Warn: Encountered unsupported value 'mesh.primitive.targets'\n");
      Assert(0);
    }

    return Result;
  }

  raw_texture_info* JsonToRawTextureInfo(const nlohmann::json& j)
  {
    raw_texture_info* Result = JwinAllocStruct( raw_texture_info);
    Result->Index = j.at("index").get<int>();
    if(j.contains("texCoord"))
    {
      Result->TexCoord = j.at("texCoord").get<int>();
    }else{
      Result->TexCoord = 0;
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;
  }

  raw_pbr_metallic_roughness* JsonToRawPbrMetallicRoughness(const nlohmann::json& j)
  {

    raw_pbr_metallic_roughness* Result = JwinAllocStruct( raw_pbr_metallic_roughness);

    if(j.contains("baseColorFactor"))
    {
      Result->BaseColorFactor = JsonToV4(j.at("baseColorFactor"));
    }else{
      Result->BaseColorFactor = V4(1,1,1,1);
    }

    if(j.contains("baseColorTexture"))
    {
      Result->BaseColorTexture = JsonToRawTextureInfo(j.at("baseColorTexture"));
    }

    if(j.contains("metallicFactor"))
    {
      Result->MetallicFactor = j.at("metallicFactor").get<float>();
    }else{
      Result->MetallicFactor = 1;
    }

    if(j.contains("roughnessFactor"))
    {
      Result->RoughnessFactor = j.at("roughnessFactor").get<float>();
    }else{
      Result->RoughnessFactor = 1;
    }

    if(j.contains("metallicRoughnessTexture"))
    {
      Result->MetallicRoughnessTexture = JsonToRawTextureInfo(j.at("metallicRoughnessTexture"));
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;
  }

  raw_material_normal_texture_info* JsonToRawNormalTexture(const nlohmann::json& j)
  {
    raw_material_normal_texture_info* Result = JwinAllocStruct( raw_material_normal_texture_info);
    Result->Index = j.at("index").get<int>();
    if(j.contains("texCoord"))
    {
      Result->TexCoord = j.at("texCoord").get<int>();
    }else{
      Result->TexCoord = 0;
    }

    if(j.contains("scale"))
    {
      Result->Scale = j.at("scale").get<float>();
    }else{
      Result->Scale = 1;
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;
  }

  raw_material_occlusion_texture_info* JsonToRawOcclusionTextureInfo(const nlohmann::json& j)
  {
    raw_material_occlusion_texture_info* Result = JwinAllocStruct( raw_material_occlusion_texture_info);

    Result->Index = j.at("index").get<int>();
    if(j.contains("texCoord"))
    {
      Result->TexCoord = j.at("texCoord").get<int>();
    }else{
      Result->TexCoord = 0;
    }

    if(j.contains("strength"))
    {
      Result->Strength = j.at("strength").get<float>();
    }else{
      Result->Strength = 1;
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;
  }

  raw_mesh JsonToRawMesh(const nlohmann::json& j)
  {
    raw_mesh Result = {};
    Assert(j.contains("primitives"));
    const nlohmann::json& JsonPrimitives = j.at("primitives");
    Result.PrimitiveCount = JsonPrimitives.size();
    Result.Primitives = JwinAllocArray(Result.PrimitiveCount,raw_primitive);
    int i = 0;
    for (const nlohmann::json& JsonPrimitive : JsonPrimitives) {
      Result.Primitives[i++] = JsonToPrimitive(JsonPrimitive);
    }

    if(j.contains("weights"))
    {
      Platform.DEBUGPrint("WARN: Weights found in mesh when loading gltf. Ignore for now but you should investigate");
    }
     
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
      Platform.DEBUGPrint("Mehs Name: %s\n", Result.Name.data);
    }

    return Result;
  }

  raw_material JsonToRawMaterial(const nlohmann::json& j)
  {
    Platform.DEBUGPrint("JsonToRawMaterial\n");
    for(const auto& el : j.items())
    {
      Platform.DEBUGPrint("%s\n", el.key().c_str());
    }
    raw_material Result = {};
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
      Platform.DEBUGPrint("Scene Name: %s\n", Result.Name.data);
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    if(j.contains("pbrMetallicRoughness")){
      Platform.DEBUGPrint("ALLOCATING pbrMetallicRoughness\n");
      Result.PbrMetallicRoughness = JsonToRawPbrMetallicRoughness(j.at("pbrMetallicRoughness"));
    }else{
      Platform.DEBUGPrint("WTF\n");
    }

    if(j.contains("normalTexture")){
      Result.NormalTexture = JsonToRawNormalTexture(j.at("normalTexture"));
    }

    if(j.contains("occlusionTexture")){
      Result.OcclusionTexture = JsonToRawOcclusionTextureInfo(j.at("occlusionTexture"));
    }

    if(j.contains("emissiveTexture")){
      Result.EmissiveTexture = JsonToRawTextureInfo(j.at("emissiveTexture"));
    }

    if(j.contains("emissiveFactor")){
      Result.EmissiveFactor = JsonToV3(j.at("emissiveFactor"));
    }else{
      Result.EmissiveFactor = V3(0,0,0);
    }

    if(j.contains("alphaMode")){
      Result.AlphaMode = JsonToString(j.at("alphaMode"));
    }else{
      Result.AlphaMode = cmn::String("OPAQUE");
    }

    if(j.contains("alphaCutoff")){
      Result.AlphaCutoff = j.at("alphaCutoff").get<float>();
    }else{
      Result.AlphaCutoff = 0.5f;
    }

    if(j.contains("doubleSided")){
      Result.DoubleSided = j.at("doubleSided").get<bool>();
    }else{
      Result.DoubleSided = false;
    }

    return Result;
  }

  raw_accessor::sparse::indices JsonToSparseIndices(const nlohmann::json& j)
  {
    raw_accessor::sparse::indices Result = {};
    Result.BufferView = j.at("bufferView").get<int>();
    if(j.contains("byteOffset"))
    {
      Result.ByteOffset = j.at("byteOffset").get<int>();
    }else{
      Result.ByteOffset = 0;
    }

    Result.ComponentType = (raw_accessor::component_type) j.at("componentType").get<int>();
    Assert( Result.ComponentType == raw_accessor::component_type::UNSIGNED_BYTE ||
            Result.ComponentType == raw_accessor::component_type::UNSIGNED_SHORT || 
            Result.ComponentType == raw_accessor::component_type::UNSIGNED_INT);

    
    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear
    return Result;
  }

  raw_accessor::sparse::values JsonToSparseValues(const nlohmann::json& j)
  {
    raw_accessor::sparse::values Result = {};
    Result.BufferView = j.at("bufferView").get<int>();
    if(j.contains("byteOffset"))
    {
      Result.ByteOffset = j.at("byteOffset").get<int>();
    }else{
      Result.ByteOffset = 0;
    }    
    
    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear
    return Result;
  }

  raw_accessor::sparse* JsonToSparse(const nlohmann::json& j)
  {
    raw_accessor::sparse* Result = JwinAllocStruct( raw_accessor::sparse);

    Result->Count = j.at("count").get<int>();
    Result->Indices = JsonToSparseIndices(j.at("indices"));
    Result->Values = JsonToSparseValues(j.at("indices"));

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear
    return Result;
  }
  //raw_accessor::type JsonToAccessorType(const nlohmann::json& j)
  raw_accessor::type JsonToAccessorType(const char* Type)
  {
    //const char* Type = j.get<std::string>().c_str();
    raw_accessor::type Result = raw_accessor::type::INVALID;
    Platform.DEBUGPrint("TYPE: %s\n", Type);
    if(cmn::Equals(Type,"SCALAR")){
      Result = raw_accessor::type::SCALAR;
    }
    else if(cmn::Equals(Type,"VEC2")){
      Result = raw_accessor::type::VEC2;
    }
    else if(cmn::Equals(Type,"VEC3")){
      Result = raw_accessor::type::VEC3;
    }
    else if(cmn::Equals(Type,"VEC4")){
      Result = raw_accessor::type::VEC4;
    }
    else if(cmn::Equals(Type,"MAT2")){
      Result = raw_accessor::type::MAT2;
    }
    else if(cmn::Equals(Type,"MAT3")){
      Result = raw_accessor::type::MAT3;
    }
    else if(cmn::Equals(Type,"MAT4")){
      Result = raw_accessor::type::MAT4;
    }
    return Result;
  }

  raw_accessor JsonToRawAccessor(const nlohmann::json& j)
  {
    raw_accessor Result = {};
    
    if(j.contains("bufferView")){
      Result.BufferView = JwinAllocStruct( int);
      *Result.BufferView = j.at("bufferView").get<int>();
    }

    if(j.contains("byteOffset")){
      Result.ByteOffset = j.at("byteOffset").get<size_t>();
    }else{
      Result.ByteOffset = 0;
    }

    Result.ComponentType = (raw_accessor::component_type) j.at("componentType").get<int>();
    Assert(Result.ComponentType == raw_accessor::component_type::BYTE ||
           Result.ComponentType == raw_accessor::component_type::UNSIGNED_BYTE ||
           Result.ComponentType == raw_accessor::component_type::SHORT ||
           Result.ComponentType == raw_accessor::component_type::UNSIGNED_SHORT || 
           Result.ComponentType == raw_accessor::component_type::UNSIGNED_INT ||
           Result.ComponentType == raw_accessor::component_type::FLOAT);

    if(j.contains("normalized")){
      Result.Normalized = j.at("normalized").get<bool>();
    }else{
      Result.Normalized = false;
    }

    Result.Count = j.at("count").get<size_t>();
    Result.Type = JsonToAccessorType(j.at("type").get<std::string>().c_str());

    if(j.contains("max")){
      Result.MaxCount = JsonToArray(j.at("max"), &Result.Max, Result.ComponentType);
      Assert( Result.MaxCount == 1|| Result.MaxCount == 2 || Result.MaxCount == 3 || Result.MaxCount == 4 || Result.MaxCount == 9 || Result.MaxCount == 16 );
    }

    if(j.contains("min")){
      Result.MinCount = JsonToArray(j.at("min"), &Result.Min, Result.ComponentType);
      Assert( Result.MinCount == 1|| Result.MinCount == 2 || Result.MinCount == 3 || Result.MinCount == 4 || Result.MinCount == 9 || Result.MinCount == 16 );
    }

    if(j.contains("sparse")){
      Result.Sparse = JsonToSparse(j.at("sparse"));
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;
  }

  raw_buffer_view::target ToTarget(int Target)
  {
    switch(Target)
    {
      case 34962:{
        return raw_buffer_view::target::ARRAY_BUFFER;
      }break;
      case 34963:{
        return raw_buffer_view::target::ELEMENT_ARRAY_BUFFER;
      }break;
    }

    Platform.DEBUGPrint("ToTarget: Unexpected enum '%d', Must be either 34962 or 34963\n", Target);
    INVALID_CODE_PATH

    return raw_buffer_view::target::NONE;
  }


  raw_buffer_view JsonToRawBufferView(const nlohmann::json& j)
  {

    raw_buffer_view Result = {};
    Result.Buffer = j.at("buffer").get<int>();

    if(j.contains("byteOffset")){
      Result.ByteOffset = j.at("byteOffset").get<size_t>();
    }else{
      Result.ByteOffset = 0;
    }

    Result.ByteLength = j.at("byteLength").get<size_t>();

    if(j.contains("byteStride")){
      Result.ByteStride = JwinAllocStruct( int);
      *Result.ByteStride = j.at("byteStride").get<int>();
    }

    Result.Target = raw_buffer_view::target::NONE;
    if(j.contains("target")){
      Result.Target = ToTarget(j.at("target").get<int>());
    }

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
    }

    return Result;
  }

  raw_buffer JsonToRawBuffer(const nlohmann::json& j){
    // key: uri
    // Required: No
    // Note: The URI (or IRI) of the buffer. Relative paths are relative to the current glTF asset.
    //       Instead of referencing an external file, this field MAY contain a data:-URI.
    raw_buffer Result = {};

    if(j.contains("uri"))
    {
      Result.Uri = JsonToString(j.at("uri"));
    }

    Result.ByteLength = j.at("byteLength").get<size_t>();

    // key: name
    // Required: No
    // Note: The user-defined name of this object
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
    }

    return Result;
  }


  raw_scene JsonToRawScene(const nlohmann::json& j)
  {
    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    raw_scene Result = {};
    
    Platform.DEBUGPrint("%s\n", j.dump().c_str());

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
      Platform.DEBUGPrint("Scene Name: %s\n", Result.Name.data);
    }
    if(j.contains("nodes"))
    {
      const nlohmann::json JsonNodes = j.at("nodes");
      Result.NodeCount = JsonToIntArray(JsonNodes, &Result.Nodes);
    }

    return Result;
  }

  raw_sampler JsonToRawSampler(const nlohmann::json& j)
  {

    raw_sampler Result = {};

    Result.MagFilter = raw_sampler::filter::NONE;
    if(j.contains("magFilter")){
      Result.MagFilter = (raw_sampler::filter) j.at("magFilter").get<int>();;
      Assert(Result.MagFilter == raw_sampler::filter::NEAREST || 
             Result.MagFilter == raw_sampler::filter::LINEAR);
    }

    Result.MinFilter = raw_sampler::filter::NONE;
    if(j.contains("minFilter")){
      Result.MinFilter = (raw_sampler::filter) j.at("minFilter").get<int>();
      Assert(Result.MinFilter == raw_sampler::filter::NEAREST ||
             Result.MinFilter == raw_sampler::filter::LINEAR ||
             Result.MinFilter == raw_sampler::filter::NEAREST_MIPMAP_NEAREST ||
             Result.MinFilter == raw_sampler::filter::LINEAR_MIPMAP_NEAREST ||
             Result.MinFilter == raw_sampler::filter::NEAREST_MIPMAP_LINEAR ||
             Result.MinFilter == raw_sampler::filter::LINEAR_MIPMAP_LINEAR);
    }

    Result.WrapS = raw_sampler::wrap::REPEAT;
    if(j.contains("wrapS")){
      Result.WrapS = (raw_sampler::wrap) j.at("wrapS").get<int>();
      Assert(Result.WrapS == raw_sampler::wrap::CLAMP_TO_EDGE ||
             Result.WrapS == raw_sampler::wrap::MIRRORED_REPEAT ||
             Result.WrapS == raw_sampler::wrap::REPEAT);
    }
    
    Result.WrapT = raw_sampler::wrap::REPEAT;
    if(j.contains("wrapT")){
      Result.WrapT = (raw_sampler::wrap) j.at("wrapT").get<int>();;
      Assert(Result.WrapT == raw_sampler::wrap::CLAMP_TO_EDGE ||
             Result.WrapT == raw_sampler::wrap::MIRRORED_REPEAT ||
             Result.WrapT == raw_sampler::wrap::REPEAT);
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
    }
    return Result;  
  }

  raw_image JsonToRawImage(const nlohmann::json& j)
  {
    raw_image Result = {};


    if(j.contains("uri"))
    {
      Result.Uri = JsonToString(j.at("uri"));
      Assert(!j.contains("bufferView")); // Cannot be defined if uri is defined
    }
    
    Result.MimeType = raw_image::mime_type::NONE;
    if(j.contains("mimeType"))
    {
      cmn::string MimeTypeStr = JsonToString(j.at("mimeType"));
      if(cmn::Equals(MimeTypeStr, raw_image::MIME_TYPE_PNG))
      {
        Result.MimeType = raw_image::mime_type::IMAGE_PNG;
      }else if(cmn::Equals(MimeTypeStr, raw_image::MIME_TYPE_JPEG)){
        Result.MimeType = raw_image::mime_type::IMAGE_JPEG;
      }
    }

    if(j.contains("bufferView"))
    {
      Result.BufferView = JwinAllocStruct( int);
      *Result.BufferView = j.at("bufferView").get<int>();
      Assert(j.contains("mimeType")); // Note MimeType must be defined if bufferView is defined.
      Assert(!j.contains("uri"));     // Note uri must _NOT_ be defined if bufferView is defined.
      Assert(0); // Don't know how to handle images baked into a bufferview. Break here and handle it when you chance upon it.
    }

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;  
  }


  raw_texture JsonToRawTexture(const nlohmann::json& j)
  {
    raw_texture Result = {};

    // Note: When undefined, a sampler with repeat wrapping and auto filtering SHOULD be used.
    if(j.contains("sampler")){
      Result.Sampler = JwinAllocStruct( int);
      *Result.Sampler = j.at("sampler").get<int>();
    }

    // key: source
    // Required: No
    // Note: The index of the image used by this texture. When undefined, an extension or other mechanism SHOULD
    //       supply an alternate texture source, otherwise behavior is undefined.
    if(j.contains("sampler")){
      Result.Source = JwinAllocStruct( int);
      *Result.Source = j.at("sampler").get<int>();
    }
    
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"));
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear
    return Result;  
  }


  struct sampler {
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

    filter MagFilter;
    filter MinFilter;
    wrap WrapS;
    wrap WrapT;
    cmn::string Name;
  };

  struct image {

    enum {
      Channel_Grey = 1,
      Channel_GreyAlpha = 2,
      Channel_RGB = 3,
      Channel_RGBA = 4
    };

    int Height;
    int Width;
    int Channels;
    uint8_t* Pixels;
    cmn::string Name;
    cmn::string Uri;
  };

  struct texture {
    sampler* Sampler;
    image* Image;
    cmn::string Name;
  };

  struct texture_info {
    texture* Texture;
    int TexCoord;
  };

  struct material 
  {
    struct pbr_metallic_roughness
    {
      v4 BaseColorFactor;
      texture_info* BaseColorTexture;
      float MetallicFactor;
      float RoughnessFactor;
      texture_info* MetallicRoughnessTexture;
    };

    struct occlusion_texture_info {
      texture* Texture;
      int TexCoord;
      float Strength;
    };

    struct normal_texture_info {
      texture* Texture;
      int TexCoord;
      float Scale;
    };

    pbr_metallic_roughness* PbrMetallicRoughness;
    normal_texture_info* NormalTexture;
    occlusion_texture_info* OcclusionTexture;
    texture_info* EmissiveTexture;
    v3 EmissiveFactor;
    cmn::string AlphaMode;
    float AlphaCutoff;
    bool DoubleSided;
    cmn::string Name;
  };


  struct mesh {

    struct primitive {
      int IndexCount;
      int* Indeces;

      int vCount;
      v3* v;     // Vertices
      int vnCount;
      v3* vn;    // Vertice Normals

      int vtSetCount;
      int* vtCount;
      v2** vt;    // Texture Vertices

      v3 vMin;
      v3 vMax;

      primitive_mode Mode;
    
      material Material;
    };

    int PrimitiveCount;
    primitive* Primitives;

    cmn::string Name;
  };

  struct trs {
    v3 t;     // Translation
    quat r;   // Rotation
    v3 s;     // Scale
  };

  struct transformation {
    enum class type {
      NONE,
      MATRIX,
      TRS
    };

    type Type;
    union {
      trs TRS;
      m4 Matrix;
    };
  };

  struct node {

    cmn::string Name;

    transformation Transofmation;

    mesh* Mesh;

    int ChildCount;
    node* Parent;
    node* FirstChild;
    node* NextSibling;
    node* PreviousSibling;
  };

  struct scene {
    cmn::string Name;

    int RootCount;
    node** RootNodes;
  };

  struct document {
    int ActiveScene;

    int SceneCount;
    scene* Scenes;

    int ImageCount;
    image* Images;

    int SamplerCount;
    sampler* Samplers;

    int TextureCount;
    texture* Textures;

    int MeshCount;
    mesh* Meshes;

    int MaterialCount;
    material* Materials;

  };

  size_t AccessorComponentSize(raw_accessor::component_type ComponentType)
  {
    switch(ComponentType)
    {
      case raw_accessor::component_type::BYTE: 
      case raw_accessor::component_type::UNSIGNED_BYTE:
        return 1;
      case raw_accessor::component_type::SHORT: 
      case raw_accessor::component_type::UNSIGNED_SHORT: 
        return 2;
      case raw_accessor::component_type::UNSIGNED_INT:
      case raw_accessor::component_type::FLOAT: 
        return 4;
    }

    Platform.DEBUGPrint("Error: Unknown raw_accessor::component_type found %d", (int) ComponentType);
    INVALID_CODE_PATH;
    return 0;
  }

  size_t AccessorComponentCount(raw_accessor::type Type)
  {
    switch(Type)
    {
      case raw_accessor::type::SCALAR: return 1;  break;
      case raw_accessor::type::VEC2:   return 2;  break;
      case raw_accessor::type::VEC3:   return 3;  break;
      case raw_accessor::type::VEC4:   return 4;  break;
      case raw_accessor::type::MAT2:   return 4;  break;
      case raw_accessor::type::MAT3:   return 9;  break;
      case raw_accessor::type::MAT4:   return 16; break;
    }

    Platform.DEBUGPrint("Error: Unknown raw_accessor::type found %d", (int) Type);
    INVALID_CODE_PATH;
    return 0;
  }

  size_t AccessorElementSize( raw_accessor* RawAccessor) {
    size_t ComponentSize = AccessorComponentSize(RawAccessor->ComponentType);
    size_t TypeSize = AccessorComponentCount(RawAccessor->Type);
    size_t Result = ComponentSize * TypeSize;
    return Result;
  }

  struct buffer_extract_result
  {
    size_t Count;
    size_t ComponentSize;
    size_t ComponentCount;
    uint8_t* DataBytes;
  };

  buffer_extract_result Extract(size_t ElementCount, size_t ComponentCount, 
                   size_t SrcComponentSize, size_t SrcStride, uint8_t* Src,
                   size_t DstComponentSize, uint8_t* Dst)
  {
    uint8_t* DstData = Dst;
    uint8_t* SrcData = Src;

    size_t DstStride = DstComponentSize*ComponentCount;
    
    const size_t SrcElementSize = ComponentCount * SrcComponentSize;
    const size_t DstElementSize = ComponentCount * DstComponentSize;
    
    for (int i = 0; i < ElementCount; ++i) {

      uint8_t* DstElement = DstData;
      uint8_t* SrcElement = SrcData;

      for (int j = 0; j < ComponentCount; ++j) {
        uint8_t* DstComponent = DstElement;
        uint8_t* SrcComponent = SrcElement;
        for (int k = 0; k < SrcComponentSize; ++k) {
          DstComponent[k] = SrcComponent[k];
        }
        DstElement += DstComponentSize;
        SrcElement += SrcComponentSize;
      }
      DstData += DstStride;
      SrcData += SrcStride;
    }

    buffer_extract_result Result = {};
    Result.Count = ElementCount;
    Result.ComponentSize = DstComponentSize;
    Result.ComponentCount = ComponentCount;
    Result.DataBytes = Dst;
    return Result;
  }


  buffer_extract_result Extract(raw_accessor* RawAccessor, raw_buffer_view* RawBufferViews, raw_buffer* RawBuffers)
  {
    // The following fields are optional:
    // But I have no idea in what cases they may appear
    // These asserts are here to catch those cases if we come across them.
    Assert(RawAccessor->BufferView);
    // Sparse storage is not yet supported. If we come across it, implement it then.
    // This assert is here to catch thos cases if we come across them.
    Assert(!RawAccessor->Sparse);

    raw_buffer_view* RawBufferView = &RawBufferViews[*RawAccessor->BufferView];
    raw_buffer*      RawBuffer     = RawBuffers + RawBufferViews->Buffer;

    uint8_t* Src = RawBuffer->LoadedData + RawBufferView->ByteOffset;
    
    size_t ElementCount = RawAccessor->Count;
    size_t ComponentCount = AccessorComponentCount(RawAccessor->Type);

    size_t SrcComponentSize = AccessorComponentSize(RawAccessor->ComponentType);
    size_t SrcStride = RawBufferView->ByteStride ? *RawBufferView->ByteStride : ComponentCount * SrcComponentSize;

    size_t DstComponentSize = sizeof(float);

    buffer_extract_result Result = Extract(ElementCount, ComponentCount,
      SrcComponentSize, SrcStride, Src,
      DstComponentSize, (uint8_t*) JwinAllocSize(ElementCount * ComponentCount * DstComponentSize) );

    return Result;
  }

  void ExtractVertexBoundingBox(extracted_primitive* Primitive, raw_accessor* RawAccessor)
  {
    size_t SrcComponentSize = AccessorComponentSize(RawAccessor->ComponentType);

    Assert(RawAccessor->MinCount == 3);
    Extract(RawAccessor->MinCount, 1, SrcComponentSize, SrcComponentSize, (uint8_t*) RawAccessor->Min, sizeof(float), (uint8_t*) Primitive->vMin.E);
    
    Assert(RawAccessor->MaxCount == 3);
    Extract(RawAccessor->MaxCount, 1, SrcComponentSize, SrcComponentSize, (uint8_t*) RawAccessor->Max, sizeof(float), (uint8_t*) Primitive->vMax.E);
  }

  void ExtractVertexBoundingBox(mesh::primitive* Primitive, raw_accessor* RawAccessor)
  {
    size_t SrcComponentSize = AccessorComponentSize(RawAccessor->ComponentType);

    Assert(RawAccessor->MinCount == 3);
    Extract(RawAccessor->MinCount, 1, SrcComponentSize, SrcComponentSize, (uint8_t*) RawAccessor->Min, sizeof(float), (uint8_t*) Primitive->vMin.E);
    
    Assert(RawAccessor->MaxCount == 3);
    Extract(RawAccessor->MaxCount, 1, SrcComponentSize, SrcComponentSize, (uint8_t*) RawAccessor->Max, sizeof(float), (uint8_t*) Primitive->vMax.E);
  }

  texture_info FromRaw(raw_texture_info* Raw)
  {
    // For now we can only handle one set of texture coordinates.
    // Change this if we ever see more of them.
    Assert(Raw->TexCoord == 0);
    return {};
  }

  material::pbr_metallic_roughness* ToRawPbrMetallicRoughness(raw_pbr_metallic_roughness* RawPbrMetallicRoughness, texture* Textures){
    
    Assert(!RawPbrMetallicRoughness->MetallicRoughnessTexture);

    material::pbr_metallic_roughness* Result = JwinAllocStruct( material::pbr_metallic_roughness);
    
    if(RawPbrMetallicRoughness->BaseColorTexture)
    {
      Result->BaseColorTexture = JwinAllocStruct( texture_info);
      Result->BaseColorTexture->Texture = &Textures[RawPbrMetallicRoughness->BaseColorTexture->Index];
      Result->BaseColorTexture->TexCoord = RawPbrMetallicRoughness->BaseColorTexture->TexCoord;  
    }

    Result->BaseColorFactor = RawPbrMetallicRoughness->BaseColorFactor;
    Result->MetallicFactor  = RawPbrMetallicRoughness->MetallicFactor;
    Result->RoughnessFactor = RawPbrMetallicRoughness->RoughnessFactor;

    return Result;
  }

  material ToMaterial(raw_material* RawMaterial, texture* Textures)
  {
    material Result = {};

    // Note: Handle more material properties as they become needed.
    Assert(!RawMaterial->NormalTexture);
    Assert(!RawMaterial->OcclusionTexture);
    Assert(!RawMaterial->EmissiveTexture);

    if(!cmn::IsEmpty(RawMaterial->Name)){
      Result.Name = cmn::Copy(RawMaterial->Name);
    }

    if(RawMaterial->PbrMetallicRoughness) {
      Result.PbrMetallicRoughness = ToRawPbrMetallicRoughness(RawMaterial->PbrMetallicRoughness, Textures);
    }

    if(!cmn::IsEmpty(RawMaterial->AlphaMode)){
      Result.AlphaMode = cmn::Copy(RawMaterial->AlphaMode);
    }
    Result.DoubleSided = RawMaterial->DoubleSided;
    Result.AlphaCutoff = RawMaterial->AlphaCutoff;
    Result.EmissiveFactor = RawMaterial->EmissiveFactor;
    return Result;
  }


  extracted_primitive ToPrimitive(raw_primitive* RawPrimitive, raw_gltf_data* RawGltfData)
  {
    extracted_primitive Result = {};
    // Note: When indices property is not defined, the number of vertex indices to render is defined by count of
    //       attribute accessors (with the implied values from range [0..count)); when indices property is
    //       defined, the number of vertex indices to render is defined by count of accessor referred to by
    //       indices. In either case, the number of vertex indices MUST be valid for the topology type used:

    u32* Indeces = 0;
    if(RawPrimitive->Indices)
    {
      raw_accessor* RawAccessor = &RawGltfData->RawAccessors[*RawPrimitive->Indices];
      buffer_extract_result ExtractRestult = Extract(RawAccessor, RawGltfData->RawBufferViews, RawGltfData->RawBuffers);
      Result.IndexCount = ExtractRestult.Count;
      Result.Indeces = (int*) ExtractRestult.DataBytes;
    }

    int TextureCoordinateCount = 0;
    int ColorCount = 0;
    int JointsCount = 0;
    int WeightsCount = 0;
    for (int i = 0; i < RawPrimitive->AttributeCount; ++i)
    {
      raw_attribute* RawAttribute = &RawPrimitive->Attributes[i];
      raw_attribute::attribute_type AttributeType = RawAttribute->Type;
      switch(AttributeType.Type)
      {
        case raw_attribute::attribute_type::type::TEXCOORD: {
          if(TextureCoordinateCount < AttributeType.Index+1) {
            TextureCoordinateCount = AttributeType.Index+1;
          }
        } break;
        case raw_attribute::attribute_type::type::COLOR: {
          if(ColorCount < AttributeType.Index+1) {
            ColorCount = AttributeType.Index+1;
          }
        }break;
        case raw_attribute::attribute_type::type::JOINTS: {
          if(JointsCount < AttributeType.Index+1) {
            JointsCount = AttributeType.Index+1;
          }
        }break;
        case raw_attribute::attribute_type::type::WEIGHTS: {
          if(WeightsCount < AttributeType.Index+1) {
            WeightsCount = AttributeType.Index+1;
          }
        }break;
      }
    }
    Assert(ColorCount==0);
    Assert(JointsCount==0);
    Assert(WeightsCount==0);

    Result.vtSetCount = TextureCoordinateCount;
    Result.vtCount    = JwinAllocArray(Result.vtSetCount, int);
    Result.vt         = JwinAllocArray(Result.vtSetCount, v2*);

    for (int i = 0; i < RawPrimitive->AttributeCount; ++i)
    {
      raw_attribute* RawAttribute = &RawPrimitive->Attributes[i];
      raw_accessor* RawAccessor = &RawGltfData->RawAccessors[RawAttribute->Index];
      raw_attribute::attribute_type AttributeType = RawAttribute->Type;
      buffer_extract_result ExtractRestult = Extract(RawAccessor, RawGltfData->RawBufferViews, RawGltfData->RawBuffers);
      switch(AttributeType.Type)
      {
        case raw_attribute::attribute_type::type::ERROR: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type ERROR\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::POSITION: {
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 3);

          ExtractVertexBoundingBox(&Result, RawAccessor);
          Result.vCount = ExtractRestult.Count;
          Result.v = (v3*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::attribute_type::type::NORMAL: {
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 3);
          Result.vnCount = ExtractRestult.Count;
          Result.vn = (v3*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::attribute_type::type::TANGENT: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type TANGENT\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::TEXCOORD: {
          
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 2);
          Result.vtCount[AttributeType.Index] = ExtractRestult.Count;
          Result.vt[AttributeType.Index] = (v2*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::attribute_type::type::COLOR: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type COLOR\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::JOINTS: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type JOINTS\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::WEIGHTS: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type WEIGHTS\n");
          Assert(0);
        }break;
      }
    }


    Result.MaterialIndex = RawPrimitive->Material;

    return Result;
  }

  void ExtractPrimitives(raw_mesh* RawMesh, raw_gltf_data* RawGltfData)
  {
    RawMesh->ExtractedPrimitiveCount = RawMesh->PrimitiveCount;
    RawMesh->ExtractedPrimitives = JwinAllocArray(RawMesh->ExtractedPrimitiveCount, extracted_primitive);
    for (int i = 0; i < RawMesh->PrimitiveCount; ++i)
    {
      RawMesh->ExtractedPrimitives[i] = ToPrimitive(RawMesh->Primitives, RawGltfData);
    }
  }

  mesh::primitive ToPrimitive(raw_primitive* RawPrimitive, raw_gltf_data* RawGltfData, material* Materials)
  {
    mesh::primitive Result = {};
    // Note: When indices property is not defined, the number of vertex indices to render is defined by count of
    //       attribute accessors (with the implied values from range [0..count)); when indices property is
    //       defined, the number of vertex indices to render is defined by count of accessor referred to by
    //       indices. In either case, the number of vertex indices MUST be valid for the topology type used:

    u32* Indeces = 0;
    if(RawPrimitive->Indices)
    {
      raw_accessor* RawAccessor = &RawGltfData->RawAccessors[*RawPrimitive->Indices];
      buffer_extract_result ExtractRestult = Extract(RawAccessor, RawGltfData->RawBufferViews, RawGltfData->RawBuffers);
      Result.IndexCount = ExtractRestult.Count;
      Result.Indeces = (int*) ExtractRestult.DataBytes;
    }

    int TextureCoordinateCount = 0;
    int ColorCount = 0;
    int JointsCount = 0;
    int WeightsCount = 0;
    for (int i = 0; i < RawPrimitive->AttributeCount; ++i)
    {
      raw_attribute* RawAttribute = &RawPrimitive->Attributes[i];
      raw_attribute::attribute_type AttributeType = RawAttribute->Type;
      switch(AttributeType.Type)
      {
        case raw_attribute::attribute_type::type::TEXCOORD: {
          if(TextureCoordinateCount < AttributeType.Index+1) {
            TextureCoordinateCount = AttributeType.Index+1;
          }
        } break;
        case raw_attribute::attribute_type::type::COLOR: {
          if(ColorCount < AttributeType.Index+1) {
            ColorCount = AttributeType.Index+1;
          }
        }break;
        case raw_attribute::attribute_type::type::JOINTS: {
          if(JointsCount < AttributeType.Index+1) {
            JointsCount = AttributeType.Index+1;
          }
        }break;
        case raw_attribute::attribute_type::type::WEIGHTS: {
          if(WeightsCount < AttributeType.Index+1) {
            WeightsCount = AttributeType.Index+1;
          }
        }break;
      }
    }
    Assert(ColorCount==0);
    Assert(JointsCount==0);
    Assert(WeightsCount==0);

    Result.vtSetCount = TextureCoordinateCount;
    Result.vtCount    = JwinAllocArray(Result.vtSetCount, int);
    Result.vt         = JwinAllocArray(Result.vtSetCount, v2*);

    for (int i = 0; i < RawPrimitive->AttributeCount; ++i)
    {
      raw_attribute* RawAttribute = &RawPrimitive->Attributes[i];
      raw_accessor* RawAccessor = &RawGltfData->RawAccessors[RawAttribute->Index];
      raw_attribute::attribute_type AttributeType = RawAttribute->Type;
      buffer_extract_result ExtractRestult = Extract(RawAccessor, RawGltfData->RawBufferViews, RawGltfData->RawBuffers);
      switch(AttributeType.Type)
      {
        case raw_attribute::attribute_type::type::ERROR: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type ERROR\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::POSITION: {
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 3);

          ExtractVertexBoundingBox(&Result, RawAccessor);
          Result.vCount = ExtractRestult.Count;
          Result.v = (v3*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::attribute_type::type::NORMAL: {
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 3);
          Result.vnCount = ExtractRestult.Count;
          Result.vn = (v3*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::attribute_type::type::TANGENT: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type TANGENT\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::TEXCOORD: {
          
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 2);
          Result.vtCount[AttributeType.Index] = ExtractRestult.Count;
          Result.vt[AttributeType.Index] = (v2*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::attribute_type::type::COLOR: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type COLOR\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::JOINTS: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type JOINTS\n");
          Assert(0);
        }break;
        case raw_attribute::attribute_type::type::WEIGHTS: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type WEIGHTS\n");
          Assert(0);
        }break;
      }
    }

    Result.Material = Materials[*RawPrimitive->Material];

    return Result;
  }

  mesh ToMesh(size_t RawMeshIndex, raw_gltf_data* RawGltfData, material* Materials)
  {
    raw_mesh* RawMesh = &RawGltfData->RawMeshes[RawMeshIndex];
    Assert(RawMesh->PrimitiveCount > 0); // Required
    
    mesh Result = {};
    Result.PrimitiveCount = RawMesh->PrimitiveCount;
    Result.Primitives = JwinAllocArray(Result.PrimitiveCount, mesh::primitive);
    for (int i = 0; i < Result.PrimitiveCount; ++i)
    {
      raw_primitive* RawPrimitive = RawMesh->Primitives + i;
      Result.Primitives[i] = ToPrimitive(RawPrimitive, RawGltfData, Materials);
    }

    
    Result.Name = cmn::Copy(RawMesh->Name);
  

    return Result;
  }

  node ToNode(raw_node* RawNode, mesh* Meshes)
  {
    node Result = {};
    Result.Name = cmn::Copy(RawNode->Name);

    switch(RawNode->TransformationType){
      case raw_node::transformation_type::NONE:{
        Result.Transofmation.Type = transformation::type::NONE;
      }break;
      case raw_node::transformation_type::TRS:{
        Result.Transofmation.Type = transformation::type::TRS;
        Result.Transofmation.TRS.t = RawNode->Translation;
        Result.Transofmation.TRS.r = RawNode->Rotation;
        Result.Transofmation.TRS.s = RawNode->Scale;
      }break;
      case raw_node::transformation_type::MATRIX:{
        Result.Transofmation.Type = transformation::type::MATRIX;
        Result.Transofmation.Matrix = RawNode->Matrix;
      }break;
    }

    if(RawNode->Mesh){
      Result.Mesh = &Meshes[*RawNode->Mesh];
    }

    return Result;
  }

  void ConnectChildren(int ParentIndex, raw_node* RawNodes, node* Nodes)
  {
    raw_node* RawParent = &RawNodes[ParentIndex];
    int ChildCount = RawParent->ChildCount;
    if(ChildCount == 0) return;
    int* ChildIndeces = RawParent->Children;

    node* Parent = &Nodes[ParentIndex];
    Parent->ChildCount = ChildCount;

    if (ChildCount == 1) {
      int FirstChildIndex = ChildIndeces[0];
      Parent->FirstChild = &Nodes[FirstChildIndex];
      Parent->FirstChild->Parent = Parent;
    } else {
      int FirstChildIndex = ChildIndeces[0];
      Parent->FirstChild = &Nodes[FirstChildIndex];
      for (int i = 0; i < ChildCount; ++i)
      {
        int ChildIndex = ChildIndeces[i];
        node* Child = &Nodes[ChildIndex];
        Child->Parent = Parent;
    
        if(i < ChildCount-1)
        {
          int NextSiblingIndex = ChildIndeces[i+1];
          Child->NextSibling = &Nodes[NextSiblingIndex];
          Child->NextSibling->PreviousSibling = Child;  
        }
      }
    }
  }


  struct node_queue {
    size_t Count;
    size_t TotCount;
    int* Queue;
  };

  node_queue NodeQueue(size_t Size)
  {
    node_queue Result = {}; 
    Result.Count = 0;
    Result.TotCount = Size; 
    Result.Queue = JwinAllocArray(Size, int);
    return Result;
  }

  bool IsEmpty(node_queue& Queue)
  {
    bool Result = Queue.Count == 0;
    return Result;
  }

  void Push(node_queue& Queue, int Value)
  {
    Queue.Queue[Queue.Count++] = Value;
  }

  int Pop(node_queue& Queue)
  {
    Assert(Queue.Count > 0);
    if(Queue.Count == 0) return 0;
    int Result = Queue.Queue[--Queue.Count];
    Queue.Queue[Queue.Count+1] = 0;
    return Result;
  }

  // Note: The node hierarchy make up a set of disjoint strict trees which means they are free of cycles and each node must have zero or one parent node.
  //       Nodes with 0 parents are root nodes. The same root node may appear in multiple scenes.
  //       I'm assuming this means each child node only appears once.
  scene* ToScenes(raw_gltf_data* RawGltfData, mesh* Meshes)
  {
    node* Nodes = JwinAllocArray(RawGltfData->RawNodeCount, node);
    raw_node* RawNodes = RawGltfData->RawNodes;

    for (int i = 0; i < RawGltfData->RawNodeCount; ++i)
    {
      Nodes[i] = ToNode(&RawNodes[i], Meshes);
    }

    node_queue Queue = {}; 
    Queue.Count = 0;
    Queue.TotCount = RawGltfData->RawNodeCount; 
    Queue.Queue = JwinAllocArray(Queue.TotCount, int);

    scene* Result = JwinAllocArray(RawGltfData->RawSceneCount, scene);
    for (int i = 0; i < RawGltfData->RawSceneCount; ++i)
    {
      Assert(IsEmpty(Queue));

      scene* Scene = &Result[i];
      raw_scene* RawScene = &RawGltfData->RawScenes[i];

      Scene->Name = cmn::Copy(RawScene->Name);

      Scene->RootCount = RawScene->NodeCount;
      Scene->RootNodes = JwinAllocArray(Scene->RootCount, node*);
      for (int j = 0; j < RawScene->NodeCount; ++j)
      {
        int RootNodeIndex = RawScene->Nodes[j];

        Scene->RootNodes[i] = &Nodes[RootNodeIndex];

        Push(Queue, RootNodeIndex);
        while(!IsEmpty(Queue))
        {
          int ParentNodeIndex = Pop(Queue);
          ConnectChildren(ParentNodeIndex, RawNodes, Nodes);

          raw_node* RawNode = &RawNodes[ParentNodeIndex];
          for (int i = 0; i < RawNode->ChildCount; ++i)
          {
            Push(Queue, RawNode->Children[i]);
          }
        }
      }
    }

    JwinFreeMemory(Queue.Queue);
    return Result;
  }

  void Copy(size_t ByteCount, uint8_t* Src, uint8_t* Dst)
  {
    uint8_t* SrcScan = (uint8_t*) Src;
    uint8_t* DstScan = (uint8_t*) Dst;
    while (ByteCount--) { *DstScan++ = *SrcScan++;}
  }

  image ToImage(raw_image* Raw)
  { 
    image Result = {};
    Result.Uri  = cmn::Copy(Raw->Uri);
    Result.Name = cmn::Copy(Raw->Name);
    Result.Width = Raw->Width;
    Result.Height = Raw->Height;
    Result.Channels = Raw->Channels;
    size_t ImageByteSize = Result.Width * Result.Height * Result.Channels;
    Result.Pixels = (uint8_t*) JwinAllocSize(ImageByteSize);
    Copy(ImageByteSize, (uint8_t*) Raw->Pixels, (uint8_t*) Result.Pixels);
    return Result;  
  }

  sampler::filter FromRaw(raw_sampler::filter Raw)
  {
    sampler::filter Result = sampler::filter::NEAREST;
    switch(Raw)
    {
      case raw_sampler::filter::NONE: Result = sampler::filter::NEAREST; break;
      case raw_sampler::filter::NEAREST: Result = sampler::filter::NEAREST; break;
      case raw_sampler::filter::LINEAR: Result = sampler::filter::LINEAR; break;
      case raw_sampler::filter::NEAREST_MIPMAP_NEAREST: Result = sampler::filter::NEAREST_MIPMAP_NEAREST; break;
      case raw_sampler::filter::LINEAR_MIPMAP_NEAREST: Result = sampler::filter::LINEAR_MIPMAP_NEAREST; break;
      case raw_sampler::filter::NEAREST_MIPMAP_LINEAR: Result = sampler::filter::NEAREST_MIPMAP_LINEAR; break;
      case raw_sampler::filter::LINEAR_MIPMAP_LINEAR: Result = sampler::filter::LINEAR_MIPMAP_LINEAR; break;
    }
    return Result;
  }

  sampler::wrap FromRaw(raw_sampler::wrap Raw)
  {
    sampler::wrap Result = sampler::wrap::REPEAT;
    switch(Raw)
    {
      case raw_sampler::wrap::CLAMP_TO_EDGE: Result = sampler::wrap::CLAMP_TO_EDGE; break;
      case raw_sampler::wrap::MIRRORED_REPEAT: Result = sampler::wrap::MIRRORED_REPEAT; break;
      case raw_sampler::wrap::REPEAT: Result = sampler::wrap::REPEAT; break;
    }
    return Result;
  }

  sampler ToSampler(raw_sampler* Raw )
  {
    sampler Result = {};
    Result.MagFilter = FromRaw(Raw->MagFilter);
    Result.MinFilter = FromRaw(Raw->MinFilter);
    Result.WrapS = FromRaw(Raw->WrapS);
    Result.WrapT = FromRaw(Raw->WrapT);
    Result.Name = cmn::Copy(Raw->Name);
    return Result;  
  }
  texture ToTexture(raw_texture* Raw, sampler* Samplers, image* Images )
  {
    texture Result = {};
    if(Raw->Sampler) {
      Result.Sampler = &Samplers[*Raw->Sampler];
    }
    if(Raw->Source) {
      Result.Image = &Images[*Raw->Source];
    }
    Result.Name = cmn::Copy(Raw->Name);

    return Result;  
  }

  raw_gltf_data Load(const char* FolderPath, const char* FileName, gltf_read_entire_file ReadFile, gltf_free_file_memory FreeFile)
  {

    raw_gltf_data RawGltfData = {};

    char Buff[256] = {};
    cmn::string GltfFIlePath = cmn::String(ArrayCount(Buff), Buff);
    cmn::PushBack(GltfFIlePath, FolderPath);
    cmn::PushBack(GltfFIlePath, "\\");
    cmn::PushBack(GltfFIlePath, FileName);
    
    size_t DataSize = 0;
    void* GltfFile = ReadFile(GltfFIlePath.data, &DataSize);

    nlohmann::json GltfJson = nlohmann::json::parse( (const char*) GltfFile);

    nlohmann::json JsonVersion = GltfJson.at("asset").at("version");
    cmn::string version = JsonToString(JsonVersion);
    Assert(cmn::Equals(version, "2.0"));


    RawGltfData.DefaultSceneIndex = -1;
    if(GltfJson.contains("scene"))
    {
      RawGltfData.DefaultSceneIndex = GltfJson.at("scene").get<int>();
    }

    if(GltfJson.contains("scenes"))
    {
      nlohmann::json JsonList = GltfJson.at("scenes");
      RawGltfData.RawSceneCount = JsonList.size();
      RawGltfData.RawScenes = JwinAllocArray(RawGltfData.RawSceneCount, raw_scene);
      int i = 0;
      for(nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawScenes[i++] = JsonToRawScene(JsonListElement);
      }
    }

    if(GltfJson.contains("nodes"))
    {
      nlohmann::json JsonList = GltfJson.at("nodes");
      RawGltfData.RawNodeCount = JsonList.size();
      RawGltfData.RawNodes = JwinAllocArray(RawGltfData.RawNodeCount, raw_node);
      int i = 0;
      for(nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawNodes[i++] = JsonToRawNode(JsonListElement);
      }
    }

    if(GltfJson.contains("meshes"))
    {
      nlohmann::json JsonList = GltfJson.at("meshes");
      RawGltfData.RawMeshCount = JsonList.size();
      RawGltfData.RawMeshes = JwinAllocArray(RawGltfData.RawMeshCount, raw_mesh);
      int i = 0;
      for(nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawMeshes[i++] = JsonToRawMesh(JsonListElement);
      }
    }

    if(GltfJson.contains("materials"))
    {
      nlohmann::json JsonList = GltfJson.at("materials");
      RawGltfData.RawMaterialCount = JsonList.size();
      RawGltfData.RawMaterials = JwinAllocArray(RawGltfData.RawMaterialCount, raw_material);
      int i = 0;
      for(nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawMaterials[i++] = JsonToRawMaterial(JsonListElement);
      }
    }

    if(GltfJson.contains("accessors"))
    {
      nlohmann::json JsonList = GltfJson.at("accessors");
      RawGltfData.RawAccessorsCount = JsonList.size();
      RawGltfData.RawAccessors = JwinAllocArray(RawGltfData.RawAccessorsCount, raw_accessor);
      int i = 0;
      for(nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawAccessors[i++] = JsonToRawAccessor(JsonListElement);
      }
    }

    if(GltfJson.contains("bufferViews"))
    {
      nlohmann::json JsonList = GltfJson.at("bufferViews");
      RawGltfData.BufferViewCount = JsonList.size();
      RawGltfData.RawBufferViews = JwinAllocArray(RawGltfData.BufferViewCount, raw_buffer_view);
      int i = 0;
      for(const nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawBufferViews[i++] = JsonToRawBufferView(JsonListElement);
      }
    }

    if(GltfJson.contains("buffers"))
    {
      nlohmann::json JsonList = GltfJson.at("buffers");
      RawGltfData.BufferCount = JsonList.size();
      RawGltfData.RawBuffers = JwinAllocArray(RawGltfData.BufferCount, raw_buffer);
      int i = 0;
      for(nlohmann::json& JsonBuffer : JsonList)
      {
        raw_buffer* RawBuffer = &RawGltfData.RawBuffers[i++];

        *RawBuffer = JsonToRawBuffer(JsonBuffer);
        if(!cmn::IsEmpty(RawBuffer->Uri))
        {
          char BinaryPathBuffer[256] = {};
          cmn::string BinPath = cmn::String(ArrayCount(BinaryPathBuffer), BinaryPathBuffer);
          cmn::PushBack(BinPath, FolderPath);
          cmn::PushBack(BinPath, "\\");
          cmn::PushBack(BinPath, RawBuffer->Uri);
          RawBuffer->LoadedData = (uint8_t*) ReadFile(BinPath.data, &RawBuffer->LoadedSize);
          Assert(RawBuffer->LoadedSize == RawBuffer->ByteLength);
        }else{
          // We have a buffer without file name.
          // File name is not required.
          // This is only so we can catch that if it happens and see how to deal with it then.
          INVALID_CODE_PATH
        }
      }
    }

    if(GltfJson.contains("samplers"))
    {
      nlohmann::json JsonList = GltfJson.at("samplers");
      RawGltfData.RawSamplerCount = JsonList.size();
      RawGltfData.RawSamplers = JwinAllocArray(RawGltfData.BufferCount, raw_sampler);
      int i = 0;
      for(const nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawSamplers[i++] = JsonToRawSampler(JsonListElement);
      }
    }
    
    if(GltfJson.contains("images"))
    {
      nlohmann::json JsonList = GltfJson.at("images");
      RawGltfData.RawImageCount = JsonList.size();
      RawGltfData.RawImages = JwinAllocArray(RawGltfData.RawImageCount, raw_image);
      int i = 0;
      for(nlohmann::json& JsonBuffer : JsonList)
      {
        raw_image* RawImage = &RawGltfData.RawImages[i++];

        *RawImage = JsonToRawImage(JsonBuffer);
        if(!cmn::IsEmpty(RawImage->Uri))
        {
          char ImagePathBuff[256] = {};
          cmn::string ImagePath = cmn::String(ArrayCount(ImagePathBuff), ImagePathBuff);
          cmn::PushBack(ImagePath, FolderPath);
          cmn::PushBack(ImagePath, "\\");
          cmn::PushBack(ImagePath, RawImage->Uri);
          
          #if 0
          unsigned char* ImageData = stbi_load(ImagePath.data, &RawImage->Width, &RawImage->Height, &RawImage->Channels, STBI_default);
          #else
          int DesiredChannels = STBI_rgb_alpha; // Regardless of image type, today we only support RGBA images.
          int NativeChannels = 0; // Unused
          unsigned char* ImageData = stbi_load(ImagePath.data, &RawImage->Width, &RawImage->Height, &NativeChannels, DesiredChannels);
          RawImage->Channels = STBI_rgb_alpha;
          #endif
          size_t ImageByteSize = RawImage->Width * RawImage->Height * RawImage->Channels;
          RawImage->Pixels = (uint8_t*) JwinAllocSize(ImageByteSize);
          Copy(ImageByteSize, (uint8_t*) ImageData, (uint8_t*) RawImage->Pixels);
          stbi_image_free(ImageData);
        }else{
          // We have a buffer without file name.
          // File name is not required.
          // This is only so we can catch that if it happens and see how to deal with it then.
          INVALID_CODE_PATH
        }
      }
    }

    if(GltfJson.contains("textures"))
    {
      nlohmann::json JsonList = GltfJson.at("textures");
      RawGltfData.RawTextureCount = JsonList.size();
      RawGltfData.RawTextures = JwinAllocArray(RawGltfData.RawTextureCount, raw_texture);
      int i = 0;
      for(const nlohmann::json& JsonListElement : JsonList)
      {
        RawGltfData.RawTextures[i++] = JsonToRawTexture(JsonListElement);
      }
    }

    for (int i = 0; i < RawGltfData.RawMeshCount; ++i)
    {
      ExtractPrimitives(RawGltfData.RawMeshes, &RawGltfData);
    }

    FreeFile(GltfFile);

    document Result = {};

    Result.ImageCount = RawGltfData.RawImageCount;
    Result.Images = JwinAllocArray(Result.ImageCount, image);
    for (int i = 0; i < Result.ImageCount; ++i)
    {
      Result.Images[i] = ToImage(&RawGltfData.RawImages[i]);
    }

    Result.SamplerCount = RawGltfData.RawSamplerCount;
    Result.Samplers = JwinAllocArray(Result.SamplerCount, sampler);
    for (int i = 0; i < Result.SamplerCount; ++i)
    {
      Result.Samplers[i] = ToSampler(&RawGltfData.RawSamplers[i]);
    }

    Result.TextureCount = RawGltfData.RawTextureCount;
    Result.Textures = JwinAllocArray(Result.TextureCount, texture);
    for (int i = 0; i < Result.TextureCount; ++i)
    {
      Result.Textures[i] = ToTexture(&RawGltfData.RawTextures[i], Result.Samplers, Result.Images);
    }

    Result.MaterialCount = RawGltfData.RawMaterialCount;
    Result.Materials = JwinAllocArray(Result.MaterialCount, material);
    for (int i = 0; i < Result.MaterialCount; ++i)
    {
      Result.Materials[i] = ToMaterial(&RawGltfData.RawMaterials[i], Result.Textures);
    }

    Result.MeshCount = RawGltfData.RawMeshCount;
    Result.Meshes = JwinAllocArray(Result.MeshCount, mesh);
    for (int i = 0; i < Result.MeshCount; ++i)
    {
      Result.Meshes[i] = ToMesh(i, &RawGltfData, Result.Materials);
    }

    Result.SceneCount = RawGltfData.RawSceneCount;
    Result.Scenes = ToScenes(&RawGltfData, Result.Meshes);
    

    for (int i = 0; i < RawGltfData.BufferCount; ++i)
    {
      if(RawGltfData.RawBuffers[i].LoadedData)
      {
        FreeFile(RawGltfData.RawBuffers[i].LoadedData);
      }
    }

    return RawGltfData;
  }


  void Free(raw_gltf_data* RawGltfData)
  {
    
  }

}
