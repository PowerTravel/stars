#pragma once
#include <cstdint> // uint32_t etc

#include "allocators.h"
#include "utils.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

namespace cmn {



template <typename T>
struct vector {
  _cmn_malloc* m_malloc;
  _cmn_realloc* m_realloc;
  _cmn_free* m_free;
  size_t m_reservedCount; // How many elements there is room for
  size_t m_count;         // The number of stored elements using Push and Pop functions.
  T* m_data;              // A byte array of count elementSize * reservedCount bytes.

  vector(size_t reservedCount = 0, _cmn_malloc* aMalloc   = _g_cmn_malloc, _cmn_realloc* aRealloc = _g_cmn_realloc, _cmn_free* aFree = _g_cmn_free);
  vector(size_t valueCount, const T* data, _cmn_malloc* aMalloc   = _g_cmn_malloc, _cmn_realloc* aRealloc = _g_cmn_realloc,_cmn_free* aFree = _g_cmn_free);
  vector(vector& vec) : vector( vec.m_reservedCount, vec.m_data, vec.m_malloc, vec.m_realloc, vec.m_free) {};

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

  ~vector(){
    Delete();
  };

  vector& operator=(const vector& other);

  bool Empty(){return m_count==0;};
  size_t Size(){return m_count;};
  size_t Reserved(){return m_reservedCount;};
  T operator[](int i) const {
    return m_data[i];
  };
  T& operator[](int i){return m_data[i];};
  T& Back() {Assert(m_count>0); return m_data[m_count-1];};
  T Back() const {Assert(m_count>0);return m_data[m_count-1];};
  T& Front(){Assert(m_count>0); return m_data[0];};
  T Front() const {Assert(m_count>0); return m_data[0];};
  void PushBack(const T& Value);
  T PopBack(){
    Assert(m_count>0);
    m_count--;
    return m_data[m_count];
  };
};

template <typename T>
vector<T>& vector<T>::operator=(const vector<T>& other)
{
  if(this != &other)
  {
    if(m_data)
    {
      // Delete the old data whatever it was
      m_free(m_data);  
    }

    m_malloc = other.m_malloc;
    m_realloc = other.m_realloc;
    m_free = other.m_free;
    m_reservedCount = other.m_reservedCount;
    m_count = other.m_count;

    size_t MemSize = sizeof(T) * m_reservedCount;
    m_data = (T*) m_malloc(MemSize);
    utils::Copy(MemSize, (void*) other.m_data, (void*) m_data);
  }
  return *this;
}

template <typename T>
vector<T>::vector(size_t reservedCount, 
  _cmn_malloc* Malloc,
  _cmn_realloc* Realloc,
  _cmn_free* Free) : m_reservedCount(reservedCount), m_data(0), m_count(0), m_malloc(Malloc), m_realloc(Realloc), m_free(Free)
{
  if(m_reservedCount)
  {
    size_t memSizeBytes = m_reservedCount * sizeof(T);
    m_data = (T*) m_malloc(memSizeBytes);
    utils::Zero(memSizeBytes, (uint8_t*) m_data);
  }
}

template <typename T>
vector<T>::vector(size_t valueCount, const T* data, _cmn_malloc* Malloc, _cmn_realloc* Realloc, _cmn_free* Free) :
vector(valueCount, Malloc, Realloc, Free)
{
  for (int i = 0; i < valueCount; ++i)
  {
    PushBack(data[i]);
  }
}

template <typename T>
void vector<T>::PushBack(const T& Value){
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

template struct vector<int>;

} // cmn