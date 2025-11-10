#pragma once 

#include "allocators.h"
#include <map>

namespace cmn{

template <typename K, typename V>
struct hash_map {

  struct pair {
    K Key;
    V Val;
  };

  cmn::vector<pair> m_vec;

  static hash_map Create(size_t Size = 128){
    hash_map Result = {};
    Result.m_vec = cmn::vector<pair>::Create(Size);
    return Result;
  };

  void Delete(){
    m_vec.Delete();
  };

  V* Find(K Key) {
    V* Result = 0;
    for (int i = 0; i < m_vec.Size(); ++i)
    {
      pair& Pair = m_vec[i];
      if(Pair.Key == Key)
      {
        Result = &Pair.Val;
        break;
      }
    }
    return Result;
  }

  V At(K Key) const {
    V* ResultPtr = Find(Key);
    if(ResultPtr)
    {
      return *ResultPtr;
    }else{
      ResultPtr = &m_vec.PushBack();
    }
    return ResultPtr;
  }

  V& At(K Key) {
    V* ResultPtr = Find(Key);
    if(!ResultPtr)
    {
      pair Pair = {};
      Pair.Key = Key;
      m_vec.PushBack(Pair);
      ResultPtr = &m_vec.Back().Val;
    }

    return *ResultPtr;
  }

  


};

} // namespace cmn