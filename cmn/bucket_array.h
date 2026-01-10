#pragma once
#include <cstdint> // uint32_t etc

#include "allocators.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

namespace cmn {

// Note: Bucket array functions like normal vector, but it separates its data into a list of arrays.
//       If more memory is needed it pushes another fixed size array to the list.
//       It loses memory coherence, but does not have to use the expensive reallocate function.

template <typename T, typename Allocator>
struct bucket_array {

  struct bucket {
    T* Data;
    bucket* Next;
  };

  inline size_t GetIndexInBucket(size_t i){
    size_t Result = i%m_bucketSize;
    return Result;
  }

  inline size_t GetBucketIndex(size_t i){
    size_t IndexInBucket = GetIndexInBucket(i);
    size_t Result = (i - IndexInBucket)/m_bucketSize;
    return Result;
  }
  
  bucket* GetBucket(size_t BucketIndex){
    bucket* Result = m_firstBucket;
    while(BucketIndex--){
      Result = Result->Next;
    }
    return Result;
  }
  void PushNewBucket(){
    bucket* NewBucket = (bucket*) Allocator::Allocate(sizeof(bucket));
    NewBucket->Data =   (T*)      Allocator::Allocate(sizeof(T)*m_bucketSize);
    NewBucket->Next = 0;
    if(m_lastBucket)
    {
      m_lastBucket->Next = NewBucket;
      m_lastBucket = NewBucket;
    }else{
      m_firstBucket = NewBucket;
      m_lastBucket  = NewBucket;
    }
    m_bucketCount++;
  }

  size_t m_bucketSize;
  size_t m_bucketCount;
  size_t m_count;
  bucket* m_firstBucket;
  bucket* m_lastBucket;

  // Tested in function TestConstructor
  bucket_array() = default;

  // Reserved initialization (Tested in function TestCreateDelete)
  static inline bucket_array Create(size_t BucketSize) {
    bucket_array Result = {};
    Result.m_bucketSize = BucketSize;
    Result.PushNewBucket();
    return Result;
  }

  void Delete() {

    bucket* Bucket = m_firstBucket;
    while(Bucket){
      bucket* Tmp = Bucket;
      Bucket = Bucket->Next;
      Allocator::Free(Tmp->Data);
      Allocator::Free(Tmp);
    }
    m_bucketCount = 0;
    m_count = 0;
    m_firstBucket = 0;
    m_lastBucket = 0;
  }

  bool   Empty()             {return m_count==0;};
  size_t Size()              {return m_count;};
  size_t Reserved()          {return m_bucketSize*m_bucketCount;};
  size_t BucketSize()        {return m_bucketSize;};
  T* At(size_t i){
    Assert(i<Reserved());
    size_t BucketIndex = GetBucketIndex(i);
    bucket* Bucket = GetBucket(BucketIndex);
    size_t IndexInBucket = GetIndexInBucket(i);
    return &Bucket->Data[IndexInBucket];
  }

  T  operator[](size_t i) const {return *At(i);}
  T& operator[](int i)          {return *At(i);}
  T  Front()              const {Assert(m_count>0); return *At(0);}
  T& Front()                    {Assert(m_count>0); return *At(0);}
  T* FrontPtr()                 {return m_count ? At(0) : 0;}
  T  Back()               const {Assert(m_count>0); return *At(m_count-1);}
  T& Back()                     {Assert(m_count>0); return *At(m_count-1);}
  T* BackPtr()                  {return m_count ? At(m_count-1) : 0;}

  void PushBack(const T& Value){
    if(m_count >= Reserved())
    {
      PushNewBucket();
    }
    size_t Index = GetIndexInBucket(m_count++);
    T* DataPtr = &m_lastBucket->Data[Index];
    *DataPtr   = Value;
  }

  T PopBack(){
    Assert(m_count>0);
    m_count--;
    return *At(m_count);
  };
};

} // cmn
