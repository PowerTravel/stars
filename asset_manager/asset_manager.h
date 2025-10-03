#pragma once
#include "asset_types.h"
#include "commons/types.h"
#include "commons/string.h"
#include "commons/macros.h"
#include "ecs/components/component_collider.h"
#include "containers/linked_memory.h"

struct gl_vertex_buffer;
struct obj_loaded_file;

#define ASSET_MAX_NAME_LENGTH 256
#define ASSET_MAX_PATH_LENGTH 256
#define ASSET_MAX_KEY_LENGTH 2048

namespace asset {

struct header {
  type Type;
  u32 Key;
  string Name;
  string FilePath;
  string KeyString;
  midx DataSize;
  void* Data;
};

struct manager {
  memory_arena Arena;
  linked_memory Memory;

  // Key is Asset Handle (Hash of asset name)
  // Value is pointer to headers
  rb_tree Headers;
};

manager* CreateAssetManager() {
  manager* Result = BootstrapPushStruct(manager, Arena);
  Result->Memory = NewLinkedMemory(&Result->Arena, Megabytes(1));
  Result->Headers = NewRBTree(&Result->Arena, 256, 256);

  return Result;
}

header* CreateHeader(type Type, const c8* UniqueName, const c8* Name, const c8* Path, midx DataSize);

header* ToHeader(mesh* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(image* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(pbr_material* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(phong_material* Asset) {
  return (header*) RetreatByType(Asset, header);
}

u32 ToKey(type Type, const c8* UniqueName);
void* Find(type Type, u32 Key);
void* Find(type Type, const c8* Name);
void Free(type Type, u32 Key);
void Free(type Type, const c8* Name);

u32 LoadObj(const c8* Path, const c8* UniqueName = 0);
u32 LoadTga(const c8* Path, const c8* UniqueName = 0);

mesh* LoadMesh(const c8* UniqueName, const mesh* Mesh, u32* ResultKey = 0);
phong_material* LoadMaterial(const c8* UniqueName, const phong_material* Material, u32* ResultKey = 0);

///  New loaders for gltf
image* LoadImage(const c8* UniqueName, const c8* Name, const c8* Path, const image* Image, u32* ResultKey = 0);
pbr_material* LoadPbrMaterial(const c8* UniqueName, const pbr_material* Image, u32* ResultKey = 0);
}