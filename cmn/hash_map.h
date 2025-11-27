#pragma once 

#include "allocators.h"
#include "containers/rb_tree.h"
#include "containers/chunk_list.h"

namespace cmn {

template <typename V>
struct hash_map {

  chunk_list m_val;
  rb_tree m_keys;

  static hash_map Create(size_t Size = 128){
    hash_map Result = {};
    Result.m_val    = NewChunkList(GlobalPersistentArena, sizeof(V), Size);
    Result.m_keys   = NewRBTree(GlobalPersistentArena, Size, Size);
    return Result;
  }

  V* FindVal(size_t Key) {
    void* Val = Find(&m_keys, Key,0,0);
    V* Result = (V*) Val;
    return Result;
  }

  V* AtVal(size_t Key) {
    V* ResultPtr = FindVal(Key);
    if(!ResultPtr)
    {
      ResultPtr = (V*) GetNewBlock(GlobalPersistentArena, &m_val);
      Insert(&m_keys, Key, (void*) ResultPtr);
    }
    return ResultPtr;
  }

};

} // namespace cmn