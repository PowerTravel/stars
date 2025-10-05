#pragma once

#include "math/vector_math.h"
#include "math/aabb.h"

// http://www.paulbourke.net/dataformats/obj/
// http://www.paulbourke.net/dataformats/mtl/

struct obj_bitmap
{
  u32 NameLength;
  char* Name;
  u32 PathLength;
  char* Path;

  u32   BPP;
  u32   Width;
  u32   Height;
  void* Pixels;
};

enum illum_mode {
  ILLUM_MODE_COLOR_ON_AMBIENT_OFF,
  ILLUM_MODE_COLOR_ON_AMBIENT_ON,
  ILLUM_MODE_HIGHLIGHT_ON,
  ILLUM_MODE_REFLECTION_RAYTRAYCING_ON,
  ILLUM_MODE_TRANSPACRENCEY_GLASS_ON_REFLECTION_RAY_TRACE_ON,
  ILLUM_MODE_REFLECTION_FRESNEL_RAY_TRACE_ON,
  ILLUM_MODE_TRANSPACRENCEY_REFRACTION_ON_REFLECTION_OFF_RAY_TRACE_ON,
  ILLUM_MODE_TRANSPACRENCEY_REFRACTION_ON_REFLECTION_ON_FRESNEL_ON_RAY_TRACE_ON,
  ILLUM_MODE_REFLECTION_ON_RAY_TRACE_OFF,
  ILLUM_MODE_TRANSPACRENCEY_GLASS_ON_REFLECTION_RAY_TRACE_OFF,
  ILLUM_MODE_CASTS_SHADOWS_ONTO_INVISIBLE_SURFACES
};

struct mtl_material
{
  u32 NameLength;
  char* Name;

  // Start Indice Index where this material is applied
  u32 IndiceOffset;

  v4* Kd;
  v4* Ka;
  v4* Tf;
  v4* Ks;
  v4* Ke;
  r32* d;
  r32* Ni;
  r32* Ns;

  // Unused atm, but stored for future use.
  // Tells us how the rendering should be done.
  u32* IlluminationMode;

  r32 BumpMapBM;
  obj_bitmap* BumpMap;
  obj_bitmap* MapKd;
  obj_bitmap* MapKs;
  obj_bitmap* MapNs;
};

struct obj_mtl_data
{
  u32 PathLength;
  c8* Path;

  u32 MaterialCount;
  mtl_material* Materials;
};

struct obj_mesh_indeces
{
  u32 Count;  // 3 times Nr Triangles
  u32* vi;    // Vertex Indeces
  u32* ti;    // Texture Indeces
  u32* ni;    // Normal Indeces
  aabb3f AABB;
  char Name[128];  // Where is this set?
};

struct obj_group
{
  u32 GroupNameLength;
  char* GroupName;

  u32* SmoothingGroup; // Obj_groups that share a smoothing group have joined edges that should be smoothed

  obj_mesh_indeces* Indeces;

  aabb3f aabb;

  mtl_material* Material;
};

struct obj_mesh_data
{
  u32 nv;    // Nr Verices
  u32 nvn;   // Nr Vertice Normals
  u32 nvt;   // Nr Trxture Vertices

  v3* v;     // Vertices
  v3* vn;    // Vertice Normals
  v2* vt;    // Texture Vertices
};

struct obj_loaded_file
{
  u32 ObjectNameLength;
  char* ObjectName;

  u32 ObjectCount;
  obj_group* ObjectGroups;

  obj_mesh_data* MeshData;

  obj_mtl_data* MaterialData;
};

obj_loaded_file* ReadOBJFile(void* (*AssetAllocator)(u32 ByteSize), memory_arena* TempArena, const char* FileName);
obj_bitmap* LoadTGA(void* (*AssetAllocator)(u32 ByteSize), const char* FileName);
void FreeObj(void (*FreeMemory)(void* Data), obj_loaded_file* Obj);

