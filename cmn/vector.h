#pragma once
#include <cstdint> // uint32_t etc

#include "allocators.h"
#include "utils.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

namespace cmn {

template <typename T, typename Allocator>
struct vector {
  size_t m_reservedCount; // How many elements there is room for
  size_t m_count;         // The number of stored elements using Push and Pop functions.
  T* m_data;              // A byte array of count elementSize * reservedCount bytes.

  // Tested in function TestConstructor
  vector() = default;

  // Reserved initialization (Tested in function TestCreateDelete)
  static inline vector Create(size_t reservedCount = 0) {
    vector Result = {};
    Result.m_reservedCount = reservedCount;
    if(Result.m_reservedCount)
    {
      Result.m_data = (T*) Allocator::Allocate(Result.m_reservedCount * sizeof(T));
      cmn::utils::Zero(Result.m_reservedCount * sizeof(T), (uint8_t*) Result.m_data);
    }
    return Result;
  }

  // Array initialization
  static inline vector Create(size_t Count, const T* Data)
  {
    vector Result = vector::Create(Count);
    utils::Copy(Count*sizeof(T), Data, Result.m_data);
    Result.m_count = Count;
    return Result;
  }

  void Delete() {
    Allocator::Free(m_data);
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

  void Clear(bool ZeroMemory = false) {
    m_count = 0;
    if(ZeroMemory) {
      utils::Zero(sizeof(T)*m_reservedCount, (uint8_t*) m_data);
    }
  };

  void PushBack(const T& Value){
    if(m_count >= m_reservedCount)
    {
      uint64_t oldReservedCount = m_reservedCount;
      m_reservedCount = utils::GetNextPowerOfTwo(oldReservedCount);
      size_t newMemSizeBytes = m_reservedCount*sizeof(T);
      if(m_data)
      {
        m_data = (T*) Allocator::Reallocate((void*)m_data, newMemSizeBytes);
      }else{
        m_data = (T*) Allocator::Allocate(newMemSizeBytes);
      }
    }
    m_data[m_count++] = Value;
  }

  T PopBack(){
    Assert(m_count>0);
    m_count--;
    return m_data[m_count];
  };

  template <typename OtherAllocator>
  vector<T, OtherAllocator> CustomCopy()
  {
    auto Result = vector<T, OtherAllocator>::Create(m_reservedCount);
    if(m_count)
    {
      Result.m_count = m_count;
      size_t MemSize = sizeof(T) * m_reservedCount;
      utils::Copy(MemSize, (void*) m_data, (void*) Result.m_data);
    }
    return Result;
  }

  vector Copy()
  {
    vector<T, Allocator> Result = CustomCopy<Allocator>();
    return Result;
  }
};

} // cmn
