#pragma once
#include "commons/types.h"
#include "commons/string.h"
#include "commons/macros.h"
#include "ecs/components/component_collider.h"
#include "containers/linked_memory.h"

struct gl_vertex_buffer;
struct obj_loaded_file;

namespace asset {

enum class type {
  NONE,
  OBJ, // Points to loaded_obj*.
  TGA, // obj_bitmap*
  GL_VERTEX_BUFFER
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

u32 ToKey(type Type, c8* Name);
void* Find(type Type, u32 Key);
void* Find(type Type, c8* Name);
void Free(type Type, u32 Key);
void Free(type Type, c8* Name);

gl_vertex_buffer* LoadGLVertexBuffer(c8* Name, const gl_vertex_buffer Data, u32* ResultKey = 0);
obj_loaded_file* LoadObj(c8* Name, c8* Path, u32* ResultKey = 0);
obj_bitmap* LoadTga(c8* Name, c8* Path, u32* ResultKey = 0);

}