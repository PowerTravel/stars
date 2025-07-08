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

u32 ToKey(type Type, c8* Name);
void* Find(type Type, u32 Key);
void* Find(type Type, c8* Name);
void Free(type Type, u32 Key);
void Free(type Type, c8* Name);

gl_vertex_buffer* LoadGLVertexBuffer(c8* Name, const gl_vertex_buffer Data, u32* ResultKey = 0);
u32 LoadObj(c8* Path, c8* UniqueName);
texture* LoadTga(c8* Name, c8* Path, u32* ResultKey = 0);
mesh* LoadMesh(c8* Name, const mesh* Mesh, u32* ResultKey = 0);
}