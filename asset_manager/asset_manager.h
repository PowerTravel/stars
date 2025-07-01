#pragma once
#include "commons/types.h"
#include "commons/string.h"
#include "commons/macros.h"
#include "platform/obj_loader.h"
#include "ecs/components/component_collider.h"


namespace asset {

enum class type {
  NONE,
  OBJ // Points to loaded_obj*.
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
void* Load(type Type, c8* Name, c8* Path, u32* ResultKey = 0);
void* Find(type Type, u32 Key);
void* Find(type Type, c8* Name);
void Free(type Type, u32 Key);
void Free(type Type, c8* Name);

obj_loaded_file* LoadObj(c8* Name, c8* Path, u32* ResultKey = 0)
{
  obj_loaded_file* Result = (obj_loaded_file*) Load(type::OBJ, Name, Path, ResultKey);
  return Result;
}

}