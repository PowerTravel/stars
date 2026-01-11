#pragma once
#include <cstdint> // uint32_t etc

#include "allocators.h"
#include "utils.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

namespace cmn {

// Note: Bucket array functions like normal vector, but it separates its data into a list of arrays.
//       If more memory is needed it pushes another fixed size array to the list.
//       It loses memory coherence, but does not have to use the expensive reallocate function.

template <typename T, typename Allocator>
struct bucket_array {

  struct bucket_index{
    size_t BucketIndex;
    size_t IndexInBucket;
  };

  struct bucket {
    T* Data;
    bucket* Next;
  };

  inline bucket_index GetBucketIndex(size_t i){
    bucket_index Result = {};
    Result.IndexInBucket = i%m_bucketSize;
    Result.BucketIndex = (i - Result.IndexInBucket)/m_bucketSize;
    return Result;
  }
  
  bucket* GetBucket(const bucket_index& BucketIndex) {
    size_t Index = BucketIndex.BucketIndex;
    bucket* Result = m_firstBucket;
    while(Index--){
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

  void Clear(bool ZeroMemory = false) {
    m_count = 0;
    m_lastBucket = m_firstBucket;
    if(ZeroMemory){
      bucket* Bucket = m_firstBucket;
      while(Bucket){
        utils::Zero(sizeof(T)*m_bucketSize, (uint8_t*) Bucket->Data);
        Bucket = Bucket->Next;
      }
    }
  };

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
    bucket_index BucketIndex = GetBucketIndex(i);
    bucket* Bucket = GetBucket(BucketIndex);
    return &Bucket->Data[BucketIndex.IndexInBucket];
  }

  T  operator[](size_t i) const {return *At(i);}
  T& operator[](size_t i)       {return *At(i);}
  T  Front()              const {Assert(m_count>0); return *At(0);}
  T& Front()                    {Assert(m_count>0); return *At(0);}
  T* FrontPtr()                 {return m_count ? At(0) : 0;}
  T  Back()               const {Assert(m_count>0); return *At(m_count-1);}
  T& Back()                     {Assert(m_count>0); return *At(m_count-1);}
  T* BackPtr()                  {return m_count ? At(m_count-1) : 0;}

  T* PushBack(const T& Value){
    if(m_count >= Reserved())
    {
      PushNewBucket();
    }
    bucket_index BucketIndex = GetBucketIndex(m_count++);
    bucket* Bucket = m_lastBucket;
    if(BucketIndex.BucketIndex < m_bucketCount){
      Bucket = GetBucket(BucketIndex);
    }
    T* DataPtr = &Bucket->Data[BucketIndex.IndexInBucket];
    *DataPtr   = Value;
    return DataPtr;
  }
  T* PushBack(){
    if(m_count >= Reserved())
    {
      PushNewBucket();
    }
    bucket_index BucketIndex = GetBucketIndex(m_count++);
    bucket* Bucket = m_lastBucket;
    if(BucketIndex.BucketIndex < m_bucketCount){
      Bucket = GetBucket(BucketIndex);
    }
    T* DataPtr = &Bucket->Data[BucketIndex.IndexInBucket];
    return DataPtr;
  }

  T PopBack(){
    Assert(m_count>0);
    m_count--;
    return *At(m_count);
  };
};

} // cmn
