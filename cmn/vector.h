#pragma once
#include <cstdint> // uint32_t etc

#include "allocators.h"
#include "utils.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

namespace cmn {

// Note: Having allocators as function pointers messes up live code editing.
//       When building a new DLL the allocation functions will get new adressess 
//       ones that are stored here are invalid.
//       There are workarounds such as 
//          1: using the funciton pointer pool or
//          2: Collecting the allocator function pointers into heap-allocated objects.
//             The objects gets updated on DLL reload to point to the proper function and
//             those heap allocated objects are what these types use for custom allocators.


template <typename T>
struct vector {
  _cmn_malloc* m_malloc;
  _cmn_realloc* m_realloc;
  _cmn_free* m_free;
  size_t m_reservedCount; // How many elements there is room for
  size_t m_count;         // The number of stored elements using Push and Pop functions.
  T* m_data;              // A byte array of count elementSize * reservedCount bytes.

  vector() = default;

  // Reserved initialization
  static inline vector Create( size_t reservedCount = 0, _cmn_malloc* aMalloc = _g_cmn_malloc, _cmn_realloc* aRealloc = _g_cmn_realloc, _cmn_free* aFree = _g_cmn_free) {
    vector<T> Result = {};
    Result.m_malloc  = aMalloc;
    Result.m_realloc = aRealloc;
    Result.m_free    = aFree;
    Result.m_reservedCount = reservedCount;
    if(Result.m_reservedCount)
    {
      Result.m_data = (T*) Result.m_malloc(Result.m_reservedCount * sizeof(T));

      cmn::utils::Zero(Result.m_reservedCount * sizeof(T), (uint8_t*) Result.m_data);
    }
    return Result;
  }

  static inline vector CreateTransient(size_t reservedCount = 0)
  {
    return Create(reservedCount, _g_cmn_transient_malloc, _g_cmn_transient_realloc, _g_cmn_transient_free);
  }

  // Array initialization
  static inline vector Create(size_t Count, const T* Data, _cmn_malloc* aMalloc = _g_cmn_malloc, _cmn_realloc* aRealloc = _g_cmn_realloc, _cmn_free* aFree = _g_cmn_free)
  {
    vector Result = vector::Create(Count);
    Result.m_count = Count;
    utils::Copy(Count*sizeof(T), Data, Result.m_data);
    return Result;
  }

  static inline vector CreateTransient(size_t Count, const T* Data)
  {
    return Create(Count, Data, _g_cmn_transient_malloc, _g_cmn_transient_realloc, _g_cmn_transient_free);
  }
  
  void Delete()
  {
    if(m_data)
    {
      m_free(m_data);
    }
    m_data = 0;
    m_reservedCount = 0;
    m_count = 0;
  }

  bool   Empty()             {return m_count==0;};
  size_t Size()              {return m_count;};
  size_t Reserved()          {return m_reservedCount;};
  T  operator[](int i) const {Assert(i<m_reservedCount); return m_data[i];};
  T& operator[](int i)       {Assert(i<m_reservedCount); return m_data[i];};
  T  Back()            const {Assert(m_count>0);         return m_data[m_count-1];};
  T& Back()                  {Assert(m_count>0);         return m_data[m_count-1];};
  T* BackPtr()               {return m_count ? &m_data[m_count-1] : 0;};
  T& Front()                 {Assert(m_count>0); return m_data[0];};
  T  Front()           const {Assert(m_count>0); return m_data[0];};
  T* FrontPtr()              {return m_count ? &m_data[0] : 0;};


  void PushBack(const T& Value){
    if(m_count >= m_reservedCount)
    {
      uint64_t oldReservedCount = m_reservedCount;
      m_reservedCount = utils::GetNextPowerOfTwo(oldReservedCount);
      size_t newMemSizeBytes = m_reservedCount*sizeof(T);
      if(m_data)
      {
        m_data = (T*) m_realloc((void*)m_data, newMemSizeBytes);
      }else{
        m_data = (T*) m_malloc(newMemSizeBytes);
      }
    }
    m_data[m_count++] = Value;
  }

  T PopBack(){
    Assert(m_count>0);
    m_count--;
    return m_data[m_count];
  };

  vector Copy(_cmn_malloc* aMalloc,_cmn_realloc* aRealloc,_cmn_free* aFree)
  {
    vector Result = vector::Create(m_reservedCount, aMalloc, aRealloc, aFree);
    if(m_count)
    {
      Result.m_count = m_count;
      size_t MemSize = sizeof(T) * m_reservedCount;
      Result.m_data = (T*) Result.m_malloc(MemSize);
      utils::Copy(MemSize, (void*) m_data, (void*) Result.m_data);
    }
    return Result;
  }
  
  vector Copy()
  {
    return Copy(m_malloc, m_realloc, m_free);
  }
};



template struct vector<int>;

} // cmn