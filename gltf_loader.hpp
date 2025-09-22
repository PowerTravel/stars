#pragma once
#include "externals/json.hpp"
#include "commons/jstring.h"

namespace gltf {

#define GLTF_MEMORY_ALLOCATOR(name) void* name(size_t ByteSize)
typedef GLTF_MEMORY_ALLOCATOR( gltf_memory_allocator );


#define GLTF_READ_ENTIRE_FILE(name) void* name( const c8* Filename, size_t* FileSize )
typedef GLTF_READ_ENTIRE_FILE( gltf_read_entire_file );

#define GLTF_FREE_FILE_MEMORY(name) void name( void* Memory )
typedef GLTF_FREE_FILE_MEMORY( gltf_free_file_memory );


  namespace internal{
    GLTF_MEMORY_ALLOCATOR(TransientAllocator){
      return PushSize(GlobalTransientArena, ByteSize);
    };
    GLTF_MEMORY_ALLOCATOR(PersistentAllocator){
      return PushSize(GlobalTransientArena, ByteSize);
    };

    #define GltfNewBlock(MemoryAllocator, Size) (uint8_t*) MemoryAllocator(Size)
    #define GltfNewStruct(MemoryAllocator, Type) (Type*) MemoryAllocator(sizeof(Type))
    #define GltfNewArray(MemoryAllocator, Count, Type) (Type*) MemoryAllocator(sizeof(Type)*(Count))

  }

  struct trs {
    v3 t;     // Translation
    quat r;   // Rotation
    v3 s;     // Scale
  };

  struct raw_attribute {

    enum class type {
      ERROR,
      POSITION,    /* VEC3      - float - Unitless XYZ vertex positions */
      NORMAL,      /* VEC3      - float - Normalized XYZ vertex normals 29 */
      TANGENT,     /* VEC4      - float - XYZW vertex tangents where the XYZ portion is normalized, and the W component is a sign value (- 1 or +1) indicating handedness of the tangent basis */
      TEXCOORD_0,  /* VEC2      - float - unsigned byte normalized, unsigned short normalized, - ST texture coordinates */
      TEXCOORD_1,
      TEXCOORD_2,
      TEXCOORD_3,
      COLOR_0,      /* VEC3 VEC4 - float unsigned byte normalized, unsigned short normalized - RGB or RGBA vertex color linear multiplier */
      COLOR_1,
      COLOR_2,
      COLOR_3,
      JOINTS_0,     /* VEC4      - unsigned byte, unsigned short, - See Skinned Mesh Attributes */
      JOINTS_1,
      JOINTS_2,
      JOINTS_3,
      WEIGHTS_0,    /* VEC4      - float unsigned byte normalized, unsigned short normalized - See Skinned Mesh Attributes */
      WEIGHTS_1,
      WEIGHTS_2,
      WEIGHTS_3, 
    };

    type Type;
    int Index;
  };

  struct raw_primitive {

    enum class mode {
      POINTS, // 0 
      LINES, // 1 
      LINE_LOOP, // 2 
      LINE_STRIP, // 3 
      TRIANGLES, // 4 
      TRIANGLE_STRIP, // 5 
      TRIANGLE_FAN, // 6 
    };

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
    mode Mode;
    
    // Not implemented:
    
    // Key: targets
    // Required: No
    // Note: An array of morph targets
    // size_t TargetCount;
    // target* Targets;
    
    // key: extensions (not required)
    // key: extras     (not required)
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
  };

  struct raw_node {
    // key:
    // Reqiuired:
    // Note:

    // key: camera
    // Required: No
    // Note: The index of the camera referenced by this node.
    // int Camera;

    // key: children
    // Reqiuired: No
    // Note: The indices of this node’s children.
    // int ChildCount;
    // int* Children;

    // key: skin
    // Reqiuired: No
    // Note: The index of the skin referenced by this node.
    // int Skin;

    // key: matrix
    // Reqiuired: No default [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
    // Note: 16 A floating-point 4x4 transformation matrix stored in column-major order.
    m4 Matrix;

    // key: mesh
    // Reqiuired: No
    // Note: The index of the mesh in this node
    int Mesh;

    // key: rotation
    // Reqiuired: No, default [0,0,0,1]
    // Note: A floating-point 4x4 transformation matrix stored in column-major order.
    quat Rotation;

    // key: scale
    // Reqiuired: No, default [1,1,1]
    // Note: The node’s non-uniform scale, given as the scaling factors along the x, y, and z axes.
    v3 Scale;

    // key: translation
    // Reqiuired: No, default [0,0,0]
    // Note: The node’s translation along the x, y, and z axes.
    v3 Translation;

    // key: weights
    // required: No [1-*]
    // Note: The weights of the instantiated morph target. The number of array elements MUST match the number of morph targets of the referenced mesh.
    //       When defined, mesh MUST also be defined.
    int WeightCount;
    int* Weights;

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

  struct raw_texture_info{
    // key: index
    // Required: Yes
    // Note: The index of the texture
    int Index;

    // key: texCoord
    // Required: No, default 0
    // Note: The index of the texture
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
    // Note: The index of the texture
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
    // Note: The index of the texture
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
    int MetallicFactor;

    // key: roughnessFactor
    // Required: No, default 1
    // Note: The factor for the roughness of the material.
    int RoughnessFactor;

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

  struct node {

    cmn::string Name;

    union {
      m4 Matrix;
      trs Trs;
    };

    void* Camera; // Not used atm

    //mesh* Mesh;

    node* Parent;
    node* FirstChild;
    node* NextSibling;
    node* PreviousSibling;
  };

  size_t JsonToIntArray(const nlohmann::json& j, int** Array, gltf_memory_allocator* Alloc)
  {
    size_t Count = j.size();
    int* IntArr = (int*) Alloc(Count * sizeof(int));
    int i = 0;
    for (const nlohmann::json& JsonNode : j)
    {
      IntArr[i++] = JsonNode.get<int>();
    }
    *Array = IntArr;
    
    return Count;
  }

  #define JsonToArrayTemplate(Name, Type) \
  size_t Name(const nlohmann::json& j, Type** Array, gltf_memory_allocator* Alloc) { \
    size_t Count = j.size(); \
    Type* Arr = GltfNewArray(Alloc, Count, Type); \
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
  size_t JsonToByteArray(const nlohmann::json& j, char** Array, gltf_memory_allocator* Alloc){
    size_t Count = j.size();
    char* Arr = GltfNewArray(Alloc, Count, char);
    int i = 0;
    for (const nlohmann::json& JsonNode : j)
    {
      Arr[i++] = JsonNode.get<char>();
    }
    *Array = Arr;
    
    return Count;
  }

  size_t JsonToUnsignedByteArray(const nlohmann::json& j, unsigned char** Array, gltf_memory_allocator* Alloc) {
    size_t Count = j.size();
    char* Arr = GltfNewArray(Alloc, Count, char);
    int i = 0;
    for (const nlohmann::json& JsonNode : j)
    {
      Arr[i++] = JsonNode.get<char>();
    }
    *Array = Arr;
    
    return Count;
  }

  size_t JsonToShortArray(const nlohmann::json& j, void** Array, gltf_memory_allocator* Alloc){

  }
  size_t JsonToUnsignedShortArray(const nlohmann::json& j, void** Array, gltf_memory_allocator* Alloc){

  }
  size_t JsonToUnsignedIntArray(const nlohmann::json& j, void** Array, gltf_memory_allocator* Alloc){

  }
  size_t JsonToFloatArray(const nlohmann::json& j, void** Array, gltf_memory_allocator* Alloc){

  }
*/

  size_t JsonToArray(const nlohmann::json& j, void** Array, raw_accessor::component_type ComponentType, gltf_memory_allocator* Alloc) {

    switch(ComponentType)
    {
      case raw_accessor::component_type::BYTE: return JsonToByteArray(j, (char**) Array, Alloc); break;
      case raw_accessor::component_type::UNSIGNED_BYTE: return JsonToUnsignedByteArray(j, (unsigned char**) Array, Alloc); break;
      case raw_accessor::component_type::SHORT: return JsonToShortArray(j, (short**) Array, Alloc); break;
      case raw_accessor::component_type::UNSIGNED_SHORT: return JsonToUnsignedShortArray(j, (unsigned short**) Array, Alloc); break;
      case raw_accessor::component_type::UNSIGNED_INT: return JsonToUnsignedIntArray(j, (unsigned int**) Array, Alloc); break;
      case raw_accessor::component_type::FLOAT: return JsonToFloatArray(j, (float**) Array, Alloc); break;
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

  cmn::string JsonToString(const nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    cmn::string Str = cmn::String(j.get<std::string>().c_str(), Alloc);
    return Str;
  }

  raw_node JsonToRawNode(nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    raw_node Result = {};

    if(j.contains("camera"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains camera. No parser yet written. Ignoring.\n");
    }
    if(j.contains("children"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains children. No parser yet written. Ignoring.\n");
    }
    if(j.contains("skin"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains Skin. No parser yet written. Ignoring.\n");
    }

    if(j.contains("matrix"))
    {
      Result.Matrix = JsonToM4(j.at("matrix"));
    }else{
      Result.Matrix = M4Identity();
    }

    if(j.contains("mesh"))
    {
      Result.Mesh = j.at("mesh").get<int>();
    }else{
      Result.Mesh = -1;
    }

    if(j.contains("rotation"))
    {
      Result.Rotation = JsonToQuaternion(j.at("rotation"));
    }else{
      Result.Rotation =  Quaternion();
    }

    if(j.contains("scale"))
    {
      Result.Scale = JsonToV3(j.at("scale"));
    }else{
      Result.Scale = V3(1,1,1);
    }

    if(j.contains("translation"))
    {
      Result.Translation = JsonToV3(j.at("translation"));
    }else{
      Result.Translation = V3(0,0,0);
    }

    if(j.contains("weights"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains weights. No parser yet written. Ignoring.\n");
      Assert(j.contains("mesh")); // Mesh is required if weights is set.
    }

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"), Alloc);
      Platform.DEBUGPrint("%s\n", Result.Name.data);
    }

    if(j.contains("extensions"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains extensions. No parser yet written. Ignoring.\n");
    }
    if(j.contains("extras"))
    {
      Platform.DEBUGPrint("WARN: raw_node contains extras. No parser yet written. Ignoring.\n");
    }

    return Result;
  }

  raw_attribute::type ToAttributeType( const char* Type ){
    if(jstr::Equals("POSITION", Type))
    {
      return raw_attribute::type::POSITION;
    }
    else if(jstr::Equals("NORMAL", Type))
    {
      return raw_attribute::type::NORMAL;
    }
    else if(jstr::Equals("TANGENT", Type))
    {
      return raw_attribute::type::TANGENT;
    }
    else if(jstr::BeginsWith("TEXCOORD_", Type))
    {
      const char* Num = Type + 9;
      if(jstr::Equals("0", Num))
      {
        return raw_attribute::type::TEXCOORD_0;   
      }
      else if(jstr::Equals("1", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with TEXCOORD_1 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::TEXCOORD_1;   
      }
      else if(jstr::Equals("2", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with TEXCOORD_2 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::TEXCOORD_2;   
      }
      else if(jstr::Equals("3", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with TEXCOORD_3 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::TEXCOORD_3;   
      }
    }
    else if(jstr::Equals("COLOR_", Type))
    {
      const char* Num = Type + 6;
      if(jstr::Equals("0", Num))
      {
        return raw_attribute::type::COLOR_0;
      }
      else if (jstr::Equals("1", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with COLOR_1 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::COLOR_1;
      }
      else if (jstr::Equals("2", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with COLOR_2 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::COLOR_2;
      }
      else if (jstr::Equals("3", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with COLOR_3 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::COLOR_3;
      }
    }
    else if(jstr::Equals("JOINTS_", Type))
    {
      const char* Num = Type + 7;
      if(jstr::Equals("0", Num))
      {
        return raw_attribute::type::JOINTS_0;
      }
      else if (jstr::Equals("1", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with JOINTS_1 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::JOINTS_1;
      }
      else if (jstr::Equals("2", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with JOINTS_2 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::JOINTS_2;
      }
      else if (jstr::Equals("3", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with JOINTS_3 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::JOINTS_3;
      }
    }
    else if(jstr::Equals("WEIGHTS_", Type))
    {
      const char* Num = Type + 8;
      if(jstr::Equals("0", Num))
      {
        return raw_attribute::type::WEIGHTS_0;
      }
      else if (jstr::Equals("1", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with WEIGHTS_1 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::WEIGHTS_1;
      }
      else if (jstr::Equals("2", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with WEIGHTS_2 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::WEIGHTS_2;
      }
      else if (jstr::Equals("3", Num))
      {
        Platform.DEBUGPrint("Note: Loading gltf file with WEIGHTS_3 (No idea when several sets are used: Investigate)\n");
        return raw_attribute::type::WEIGHTS_3;
      }
    }

    Platform.DEBUGPrint("Note: Loading gltf file with type: '%s' which is not supported or expected.\n", Type);
    return raw_attribute::type::ERROR;
  }

  raw_attribute RawAttribute(const char* Key, int Value)
  {
    raw_attribute Result = {};
    Result.Type = ToAttributeType(Key);
    Result.Index = Value;
    return Result;
 }

  raw_primitive JsonToPrimitive(const nlohmann::json& j,  gltf_memory_allocator* Alloc)
  {
    raw_primitive Result = {};
    Assert(j.contains("attributes"));
    const nlohmann::json& JsonAttributes = j.at("attributes");
    Result.AttributeCount = JsonAttributes.size();
    Result.Attributes = (raw_attribute*) Alloc(sizeof(raw_attribute) * Result.AttributeCount);
    int i = 0;
    for(auto& Attribute : JsonAttributes.items())
    {
      Result.Attributes[i++] = RawAttribute(Attribute.key().c_str(), Attribute.value());
    }

    if(j.contains("indices"))
    {
      Result.Indices = GltfNewStruct(Alloc, int);
      *Result.Indices = j.at("indices").get<int>();
    }

    if(j.contains("material"))
    {
      Result.Material = GltfNewStruct(Alloc, int);
      *Result.Material = j.at("material").get<int>();
    }


    Result.Mode = raw_primitive::mode::TRIANGLES;
    if(j.contains("mode"))
    {
      Result.Mode = (raw_primitive::mode) j.at("mode").get<int>();
      if(Result.Mode != raw_primitive::mode::TRIANGLES)
      {
        Platform.DEBUGPrint("Note: Primitive mode is %d which is different from %d (TRIANGLES). Unless handles will break rendering.\n",
          (int) Result.Mode, raw_primitive::mode::TRIANGLES);
      }
    }

    
    if(j.contains("targets"))
    {
      Platform.DEBUGPrint("Warn: Encountered unsupported value 'mesh.primitive.targets'\n");
    }

    return Result;
  }

  raw_texture_info* JsonToRawTextureInfo(const nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    raw_texture_info* Result = GltfNewStruct(Alloc, raw_texture_info);
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

  raw_pbr_metallic_roughness* JsonToRawPbrMetallicRoughness(const nlohmann::json& j, gltf_memory_allocator* Alloc)
  {

    raw_pbr_metallic_roughness* Result = GltfNewStruct(Alloc, raw_pbr_metallic_roughness);

    if(j.contains("baseColorFactor"))
    {
      Result->BaseColorFactor = JsonToV4(j.at("baseColorFactor"));
    }else{
      Result->BaseColorFactor = V4(1,1,1,1);
    }

    if(j.contains("baseColorTexture"))
    {
      Result->BaseColorTexture = JsonToRawTextureInfo(j.at("baseColorTexture"), Alloc);
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
      Result->MetallicRoughnessTexture = JsonToRawTextureInfo(j.at("metallicRoughnessTexture"), Alloc);
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    return Result;
  }

  raw_material_normal_texture_info* JsonToRawNormalTexture(const nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    raw_material_normal_texture_info* Result = GltfNewStruct(Alloc, raw_material_normal_texture_info);
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

  raw_material_occlusion_texture_info* JsonToRawOcclusionTextureInfo(const nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    raw_material_occlusion_texture_info* Result = GltfNewStruct(Alloc, raw_material_occlusion_texture_info);

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

  raw_mesh JsonToRawMesh(const nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    raw_mesh Result = {};
    Assert(j.contains("primitives"));
    const nlohmann::json& JsonPrimitives = j.at("primitives");
    Result.PrimitiveCount = JsonPrimitives.size();
    Result.Primitives = (raw_primitive*) Alloc(sizeof(raw_primitive) * Result.PrimitiveCount);
    int i = 0;
    for (const nlohmann::json& JsonPrimitive : JsonPrimitives) {
      Result.Primitives[i++] = JsonToPrimitive(JsonPrimitive, Alloc);
    }

    if(j.contains("weights"))
    {
      Platform.DEBUGPrint("WARN: Weights found in mesh when loading gltf. Ignore for now but you should investigate");
    }
     
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"), Alloc);
      Platform.DEBUGPrint("Mehs Name: %s\n", Result.Name.data);
    }

    return Result;
  }

  raw_material JsonToRawMaterial(const nlohmann::json& j,  gltf_memory_allocator* Alloc)
  {
    Platform.DEBUGPrint("JsonToRawMaterial\n");
    for(const auto& el : j.items())
    {
      Platform.DEBUGPrint("%s\n", el.key().c_str());
    }
    raw_material Result = {};
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"), Alloc);
      Platform.DEBUGPrint("Scene Name: %s\n", Result.Name.data);
    }

    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    if(j.contains("pbrMetallicRoughness")){
      Platform.DEBUGPrint("ALLOCATING pbrMetallicRoughness\n");
      Result.PbrMetallicRoughness = JsonToRawPbrMetallicRoughness(j.at("pbrMetallicRoughness"), Alloc);
    }else{
      Platform.DEBUGPrint("WTF\n");
    }

    if(j.contains("normalTexture")){
      Result.NormalTexture = JsonToRawNormalTexture(j.at("normalTexture"), Alloc);
    }

    if(j.contains("occlusionTexture")){
      Result.OcclusionTexture = JsonToRawOcclusionTextureInfo(j.at("occlusionTexture"), Alloc);
    }

    if(j.contains("emissiveTexture")){
      Result.EmissiveTexture = JsonToRawTextureInfo(j.at("emissiveTexture"), Alloc);
    }

    if(j.contains("emissiveFactor")){
      Result.EmissiveFactor = JsonToV3(j.at("emissiveFactor"));
    }else{
      Result.EmissiveFactor = V3(0,0,0);
    }

    if(j.contains("alphaMode")){
      Result.AlphaMode = JsonToString(j.at("alphaMode"), Alloc);
    }else{
      Result.AlphaMode = cmn::String("OPAQUE", Alloc);
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

  raw_accessor::sparse::indices JsonToSparseIndices(const nlohmann::json& j, gltf_memory_allocator Alloc)
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

  raw_accessor::sparse::values JsonToSparseValues(const nlohmann::json& j, gltf_memory_allocator Alloc)
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

  raw_accessor::sparse* JsonToSparse(const nlohmann::json& j, gltf_memory_allocator Alloc)
  {
    raw_accessor::sparse* Result = GltfNewStruct(Alloc, raw_accessor::sparse);

    Result->Count = j.at("count").get<int>();
    Result->Indices = JsonToSparseIndices(j.at("indices"), Alloc);
    Result->Values = JsonToSparseValues(j.at("indices"), Alloc);

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

  raw_accessor JsonToRawAccessor(const nlohmann::json& j, gltf_memory_allocator Alloc)
  {
    raw_accessor Result = {};
    
    if(j.contains("bufferView")){
      Result.BufferView = GltfNewStruct(Alloc, int);
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
      Result.MaxCount = JsonToArray(j.at("max"), &Result.Max, Result.ComponentType, Alloc);
      Assert( Result.MaxCount == 1|| Result.MaxCount == 2 || Result.MaxCount == 3 || Result.MaxCount == 4 || Result.MaxCount == 9 || Result.MaxCount == 16 );
    }

    if(j.contains("min")){
      Result.MinCount = JsonToArray(j.at("min"), &Result.Min, Result.ComponentType, Alloc);
      Assert( Result.MinCount == 1|| Result.MinCount == 2 || Result.MinCount == 3 || Result.MinCount == 4 || Result.MinCount == 9 || Result.MinCount == 16 );
    }

    if(j.contains("sparse")){
      Result.Sparse = JsonToSparse(j.at("sparse"), Alloc);
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


  raw_buffer_view JsonToRawBufferView(const nlohmann::json& j, gltf_memory_allocator Alloc)
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
      Result.ByteStride = GltfNewStruct(Alloc, int);
      *Result.ByteStride = j.at("byteStride").get<int>();
    }

    Result.Target = raw_buffer_view::target::NONE;
    if(j.contains("target")){
      Result.Target = ToTarget(j.at("target").get<int>());
    }

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"), Alloc);
    }

    return Result;
  }

  raw_buffer JsonToRawBuffer(const nlohmann::json& j, gltf_memory_allocator Alloc){
    // key: uri
    // Required: No
    // Note: The URI (or IRI) of the buffer. Relative paths are relative to the current glTF asset.
    //       Instead of referencing an external file, this field MAY contain a data:-URI.
    raw_buffer Result = {};

    if(j.contains("uri"))
    {
      Result.Uri = JsonToString(j.at("uri"), Alloc);
    }

    Result.ByteLength = j.at("byteLength").get<size_t>();

    // key: name
    // Required: No
    // Note: The user-defined name of this object
    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"), Alloc);
    }

    return Result;
  }


  struct raw_scene {
    cmn::string Name;
    int NodeCount;
    int* Nodes;

    // Extensions, Extras omitted
  };


  raw_scene JsonToRawScene(nlohmann::json& j, gltf_memory_allocator* Alloc)
  {
    Assert(!j.contains("extensions"));  // We can ignore extensions but I want to see if / when they appear
    Assert(!j.contains("extras"));      // We can ignore extras but I want to see if / when they appear

    raw_scene Result = {};
    
    Platform.DEBUGPrint("%s\n", j.dump().c_str());

    if(j.contains("name"))
    {
      Result.Name = JsonToString(j.at("name"), Alloc);
      Platform.DEBUGPrint("Scene Name: %s\n", Result.Name.data);
    }
    if(j.contains("nodes"))
    {
      const nlohmann::json JsonNodes = j.at("nodes");
      Result.NodeCount = JsonToIntArray(JsonNodes, &Result.Nodes, Alloc);
    }

    return Result;
  }



  struct scene {
    cmn::string Name;
    node* SceneRoot;
  };

  

  struct mesh{

    struct primitive {
      int IndexCount;
      int* Indeces;

      int vCount;
      v3* v;     // Vertices
      int vnCount;
      v3* vn;    // Vertice Normals
      int vtCount;
      v2* vt;    // Texture Vertices
    };

    int PrimitiveCount;
    primitive* Primitives;
  };

  struct document {
    size_t MeshCount;
//    mesh* Meshes;

    size_t MaterialCount;
  //  material* Materials;

    size_t NodeCount;
    node* Nodes;

    size_t SceneCount;
    s32 ActiveScene; // -1 means no scene is active;
    scene* Scenes;
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

/*
  struct buffer_data_result {
    size_t Count;
    size_t ElementSizeBytes;
    void* Data;
  };

  buffer_data_result ExtractData(raw_accessor* RawAccessor, raw_buffer_view* RawBufferView, raw_buffer* RawBuffers, gltf_memory_allocator* Alloc)
  {
    buffer_data_result Result = {};
    
  }
*/
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


  buffer_extract_result Extract(raw_accessor* RawAccessor, raw_buffer_view* RawBufferView, raw_buffer* RawBuffer, gltf_memory_allocator* Alloc)
  { 
    uint8_t* Src = RawBuffer->LoadedData + RawBufferView->ByteOffset;
    
    size_t ElementCount = RawAccessor->Count;
    size_t ComponentCount = AccessorComponentCount(RawAccessor->Type);

    size_t SrcComponentSize = AccessorComponentSize(RawAccessor->ComponentType);
    size_t SrcStride = RawBufferView->ByteStride ? *RawBufferView->ByteStride : ComponentCount * SrcComponentSize;

    buffer_extract_result Result = Extract(ElementCount, ComponentCount,
      SrcComponentSize, SrcStride, Src,
      sizeof(float), GltfNewBlock(Alloc, ElementCount * ComponentCount * SrcComponentSize) );

    return Result;
  }

  buffer_extract_result Extract(size_t AccessorIndex, raw_accessor* RawAccessors, raw_buffer_view* RawBufferViews, raw_buffer* RawBuffers, gltf_memory_allocator* Alloc)
  {
    raw_accessor*    RawAccessor   = &RawAccessors[AccessorIndex];
  
    // The following fields are optional:
    // But I have no idea in what cases they may appear
    // These asserts are here to catch those cases if we come across them.
    Assert(RawAccessor->BufferView);
    // Sparse storage is not yet supported. If we come across it, implement it then.
    // This assert is here to catch thos cases if we come across them.
    Assert(!RawAccessor->Sparse);

    raw_buffer_view* RawBufferView = &RawBufferViews[*RawAccessor->BufferView];
    raw_buffer*      RawBuffer     = RawBuffers + RawBufferViews->Buffer;
    buffer_extract_result Result   = Extract(RawAccessor, RawBufferView, RawBuffer, Alloc);
    return Result;
  }

  mesh::primitive ToPrimitive(raw_primitive* RawPrimitive, raw_accessor* RawAccessors, raw_buffer_view* RawBufferViews, raw_buffer* RawBuffers, gltf_memory_allocator* Alloc)
  {
    mesh::primitive Result = {};
    // Note: When indices property is not defined, the number of vertex indices to render is defined by count of
    //       attribute accessors (with the implied values from range [0..count)); when indices property is
    //       defined, the number of vertex indices to render is defined by count of accessor referred to by
    //       indices. In either case, the number of vertex indices MUST be valid for the topology type used:

    u32* Indeces = 0;
    if(RawPrimitive->Indices)
    { 
      buffer_extract_result ExtractRestult = Extract(*RawPrimitive->Indices, RawAccessors, RawBufferViews, RawBuffers, Alloc);
      Result.IndexCount = ExtractRestult.Count;
      Result.Indeces = (int*) ExtractRestult.DataBytes;
    }

    for (int i = 0; i < RawPrimitive->AttributeCount; ++i)
    {
      raw_attribute* RawAttribute = &RawPrimitive->Attributes[i];
      buffer_extract_result ExtractRestult = Extract(RawAttribute->Index, RawAccessors, RawBufferViews, RawBuffers, Alloc);
      switch(RawAttribute->Type)
      {
        case raw_attribute::type::ERROR: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type ERROR\n");
        }break;
        case raw_attribute::type::POSITION: {
          // Min and max values _must_ exist for position attributes.
          //Assert(RawAccessor->Min);
          //Assert(RawAccessor->Max);
          //Assert(RawAccessor->Type == raw_accessor::type::VEC3);

          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 3);
          Result.vCount = ExtractRestult.Count;
          Result.v = (v3*) ExtractRestult.DataBytes;

        }break;
        case raw_attribute::type::NORMAL: {

          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 3);
          Result.vnCount = ExtractRestult.Count;
          Result.vn = (v3*) ExtractRestult.DataBytes;

        }break;
        case raw_attribute::type::TANGENT: {

        }break;
        case raw_attribute::type::TEXCOORD_0: {
          
          Assert(ExtractRestult.ComponentSize == 4);
          Assert(ExtractRestult.ComponentCount == 2);
          Result.vtCount = ExtractRestult.Count;
          Result.vt = (v2*) ExtractRestult.DataBytes;
        }break;
        case raw_attribute::type::TEXCOORD_1: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type TEXCOORD_1\n");
        }break;
        case raw_attribute::type::TEXCOORD_2: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type TEXCOORD_2\n");
        }break;
        case raw_attribute::type::TEXCOORD_3: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type TEXCOORD_3\n");
        }break;
        case raw_attribute::type::COLOR_0: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type COLOR_0\n");
        }break;
        case raw_attribute::type::COLOR_1: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type COLOR_1\n");
        }break;
        case raw_attribute::type::COLOR_2: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type COLOR_2\n");
        }break;
        case raw_attribute::type::COLOR_3: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type COLOR_3\n");
        }break;
        case raw_attribute::type::JOINTS_0: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type JOINTS_0\n");
        }break;
        case raw_attribute::type::JOINTS_1: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type JOINTS_1\n");
        }break;
        case raw_attribute::type::JOINTS_2: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type JOINTS_2\n");
        }break;
        case raw_attribute::type::JOINTS_3: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type JOINTS_3\n");
        }break;
        case raw_attribute::type::WEIGHTS_0: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type WEIGHTS_0\n");
        }break;
        case raw_attribute::type::WEIGHTS_1: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type WEIGHTS_1\n");
        }break;
        case raw_attribute::type::WEIGHTS_2: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type WEIGHTS_2\n");
        }break;
        case raw_attribute::type::WEIGHTS_3: {
          Platform.DEBUGPrint("Warn: GltfLoader found unhandled attribute type WEIGHTS_3\n");
        }break;
      }
    }
    return Result;
  }

  mesh ToMesh(raw_mesh* RawMesh, raw_accessor* RawAccessors, raw_buffer_view* RawBufferViews, raw_buffer* RawBuffers, gltf_memory_allocator* Alloc)
  {
    mesh Result = {};

    Assert(RawMesh->PrimitiveCount > 0); // Required
    Result.PrimitiveCount = RawMesh->PrimitiveCount;
    Result.Primitives = GltfNewArray(Alloc, Result.PrimitiveCount, mesh::primitive);
    for (int i = 0; i < RawMesh->PrimitiveCount; ++i)
    {
      raw_primitive* RawPrimitive = RawMesh->Primitives + i;
      Result.Primitives[i] = ToPrimitive(RawPrimitive, RawAccessors, RawBufferViews, RawBuffers, Alloc);
    }

    return Result;
  }

  scene* Load(const char* FolderPath, const char* FileName, gltf_read_entire_file ReadFile, gltf_free_file_memory FreeFile, gltf_memory_allocator* PersistentAllocator, gltf_memory_allocator* TmpAllocator)
  {
    char Buff[256] = {};
    cmn::string GltfFIlePath = cmn::String(ArrayCount(Buff), Buff);
    cmn::PushBack(GltfFIlePath, FolderPath);
    cmn::PushBack(GltfFIlePath, "\\");
    cmn::PushBack(GltfFIlePath, FileName);
    
    size_t DataSize = 0;
    void* GltfData = ReadFile(GltfFIlePath.data, &DataSize);

    nlohmann::json GltfJson = nlohmann::json::parse( (const char*) GltfData);

    nlohmann::json JsonVersion = GltfJson.at("asset").at("version");
    cmn::string version = JsonToString(JsonVersion, internal::TransientAllocator);
    Assert(cmn::Equals(version, "2.0"));

    int SceneIndex = -1;
    if(GltfJson.contains("scene"))
    {
      SceneIndex = GltfJson.at("scene").get<int>();
    }

    int RawSceneCount = 0;
    raw_scene* RawScenes = 0;
    if(GltfJson.contains("scenes"))
    {
      nlohmann::json JsonScenes = GltfJson.at("scenes");
      RawSceneCount = JsonScenes.size();
      RawScenes = GltfNewArray(TmpAllocator,  RawSceneCount, raw_scene);
      int i = 0;
      for(nlohmann::json& JsonScene : JsonScenes)
      {
        RawScenes[i++] = JsonToRawScene(JsonScene, TmpAllocator);
      }
    }


    int RawNodeCount = 0;
    raw_node* RawNodes = 0;
    if(GltfJson.contains("nodes"))
    {
      nlohmann::json JsonNodes = GltfJson.at("nodes");
      RawNodeCount = JsonNodes.size();
      RawNodes = GltfNewArray(TmpAllocator, RawNodeCount, raw_node);
      int i = 0;
      for(nlohmann::json& JsonNode : JsonNodes)
      {
        RawNodes[i++] = JsonToRawNode(JsonNode, TmpAllocator);
      }
    }


    int RawMeshCount = 0;
    raw_mesh* RawMeshes = 0;
    if(GltfJson.contains("meshes"))
    {
      nlohmann::json JsonMeshes = GltfJson.at("meshes");
      RawMeshCount = JsonMeshes.size();
      RawMeshes = GltfNewArray(TmpAllocator, RawMeshCount, raw_mesh);
      int i = 0;
      for(nlohmann::json& JsonMesh : JsonMeshes)
      {
        RawMeshes[i++] = JsonToRawMesh(JsonMesh, TmpAllocator);
      }
    }

    int RawMaterialCount = 0;
    raw_material* RawMaterial = 0;
    if(GltfJson.contains("materials"))
    {
      nlohmann::json JsonMaterials = GltfJson.at("materials");
      RawMaterialCount = JsonMaterials.size();
      RawMaterial = GltfNewArray(TmpAllocator, RawMaterialCount, raw_material);
      int i = 0;
      for(nlohmann::json& JsonMaterial : JsonMaterials)
      {
        RawMaterial[i++] = JsonToRawMaterial(JsonMaterial, TmpAllocator);
      }
    }

    int RawAccessorsCount = 0;
    raw_accessor* RawAccessors = 0;
    if(GltfJson.contains("accessors"))
    {
      nlohmann::json JsonAccessors = GltfJson.at("accessors");
      RawAccessorsCount = JsonAccessors.size();
      RawAccessors = GltfNewArray(TmpAllocator, RawAccessorsCount, raw_accessor);
      int i = 0;
      for(nlohmann::json& JsonAccessors : JsonAccessors)
      {
        RawAccessors[i++] = JsonToRawAccessor(JsonAccessors, TmpAllocator);
      }
    }

    int BufferViewCount = 0;
    raw_buffer_view* RawBufferViews = 0;
    if(GltfJson.contains("bufferViews"))
    {
      nlohmann::json JsonBufferViews = GltfJson.at("bufferViews");
      BufferViewCount = JsonBufferViews.size();
      RawBufferViews = GltfNewArray(TmpAllocator, BufferViewCount, raw_buffer_view);
      int i = 0;
      for(const nlohmann::json& JsonBufferView : JsonBufferViews)
      {
        RawBufferViews[i++] = JsonToRawBufferView(JsonBufferView, TmpAllocator);
      }
    }

    int BufferCount = 0;
    raw_buffer* RawBuffers = 0;
    if(GltfJson.contains("buffers"))
    {
      nlohmann::json JsonBuffers = GltfJson.at("buffers");
      BufferCount = JsonBuffers.size();
      RawBuffers = GltfNewArray(TmpAllocator, BufferCount, raw_buffer);
      int i = 0;
      for(nlohmann::json& JsonBuffer : JsonBuffers)
      {
        raw_buffer* RawBuffer = RawBuffers + i++;
        *RawBuffer = JsonToRawBuffer(JsonBuffer, TmpAllocator);
        
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

    FreeFile(GltfData);

    for (int i = 0; i < RawMeshCount; ++i)
    {
      mesh Mesh = ToMesh(&RawMeshes[i], RawAccessors, RawBufferViews, RawBuffers, TmpAllocator);
    }


    for (int i = 0; i < BufferCount; ++i)
    {
      if(RawBuffers[i].LoadedData)
      {
        FreeFile(RawBuffers[i].LoadedData);
      }
    }
    
    return 0;
  }

}
