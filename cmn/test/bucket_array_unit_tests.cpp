#include <cstdio>

#include "bucket_array.h"
#include "allocators.h"
#include "debug_asserts.h"

bool gShouldPrint = false;

template<class T> using bucket_array_dbg = cmn::bucket_array<T, CustomAllocators>;
template<class T> using bucket_array_std = cmn::bucket_array<T, StdAllocators>;

void TestCreateDelete()
{
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(10);
  v.Delete();
  DBG_Assert(gMallocCount, 2, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount, 2, "Free Call Count");
}

void Test_PushDelete_SingleBucket()
{
  size_t BucketSize = 10;
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize; ++i)
  {
    v.PushBack(i);
    DBG_Assert(v.Back(),  i, "Value from Back()");
    DBG_Assert(v.Front(), 0, "Value from Front()");
    DBG_Assert(*v.BackPtr(),  i, "Value from BackPtr()");
    DBG_Assert(*v.FrontPtr(), 0, "Value from FrontPtr()");
  }

  DBG_Assert(gMallocCount,  2, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  for (int i = 0; i < BucketSize; ++i)
  {
    DBG_Assert(*v.At(i),  i, "Value from At()");
    DBG_Assert(v[i],      i,     "Value from []");
  }

  v.Delete();

  DBG_Assert(gMallocCount,  2, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    2, "Free Call Count");
}

void Test_PushDelete_MultipleBuckets()
{
  size_t BucketSize  = 10;
  size_t BucketCount = 10;
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    v.PushBack(BucketSize-i);
    DBG_Assert(v.Back(),  BucketSize-i, "Value from Back()");
    DBG_Assert(v.Front(), BucketSize, "Value from Front()");  
  }

  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  v.Delete();

  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    2*BucketCount, "Free Call Count");
}

void Test_PushDeleteWithPtr_MultipleBuckets()
{
  size_t BucketSize  = 10;
  size_t BucketCount = 10;
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    int* Data = v.PushBack();
    *Data = BucketSize-i;
    DBG_Assert(v.Back(),  BucketSize-i, "Value from Back()");
    DBG_Assert(v.Front(), BucketSize, "Value from Front()");  
  }
  
  DBG_Assert(v.Size(), BucketSize*BucketCount, "Size");
  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  v.Delete();
  DBG_Assert(v.Size(), 0, "Size");
  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    2*BucketCount, "Free Call Count");
}

void Test_SquareBracketAssignment_SingleBucket()
{
  size_t BucketSize = 10;
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize; ++i)
  {
    v[i] = i;
  }

  DBG_Assert(gMallocCount,  2, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  for (int i = 0; i < BucketSize; ++i)
  {
    DBG_Assert(*v.At(i),  i, "Value from At()");
    DBG_Assert(v[i],      i, "Value from []");
  }

  v.Delete();

  DBG_Assert(gMallocCount,  2, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    2, "Free Call Count");
}

void Test_SquareBracketAssignment_MultipleBuckets()
{
  size_t BucketSize  = 10;
  size_t BucketCount = 10;
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    v.PushBack(BucketSize-i);
  }

  DBG_Assert(v.Reserved(),  BucketSize*BucketCount, "Malloc Call Count");

  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  // Test bracket assignemnt with several buckets
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    v[i] = i;
  }

  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    DBG_Assert(*v.At(i), i, "Value from At()");
    DBG_Assert(v[i],     i, "Value from []");
  }

  v.Delete();

  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    2*BucketCount, "Free Call Count");
}


void Test_Clear()
{
  size_t BucketSize  = 10;
  size_t BucketCount = 10;
  bucket_array_dbg<int> v = bucket_array_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    v.PushBack(BucketSize-i);
  }

  DBG_Assert(v.Reserved(),  BucketSize*BucketCount, "Reserved Size");
  DBG_Assert(v.Size(),      BucketSize*BucketCount, "Size");
  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  // Test Clear without zeroing
  v.Clear();
  DBG_Assert(v.Reserved(),  BucketSize*BucketCount, "Reserved Size");
  DBG_Assert(v.Size(),      0, "Size");

  // Fill again, make sure no new mallocs are needed.
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    v.PushBack(i);
    DBG_Assert(*v.At(i), i, "Value from At()");
    DBG_Assert(v[i],     i, "Value from []");
  }
  DBG_Assert(v.Reserved(),  BucketSize*BucketCount, "Reserved Size");
  DBG_Assert(v.Size(),      BucketSize*BucketCount, "Size");
  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    0, "Free Call Count");

  // Clear but tell it to zero the memory
  v.Clear(true);
  for (int i = 0; i < BucketSize*BucketCount; ++i)
  {
    DBG_Assert(*v.At(i), 0, "Value from At()");
    DBG_Assert(v[i],     0, "Value from []");
  }

  v.Delete();

  DBG_Assert(gMallocCount,  2*BucketCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount,    2*BucketCount, "Free Call Count");
}


int main(int argc, char* argv[]){
  gShouldPrint = argc > 1;
  DBG_RunTest(TestCreateDelete);
  DBG_RunTest(Test_PushDelete_SingleBucket);
  DBG_RunTest(Test_PushDelete_MultipleBuckets);
  DBG_RunTest(Test_PushDeleteWithPtr_MultipleBuckets);
  DBG_RunTest(Test_SquareBracketAssignment_SingleBucket);
  DBG_RunTest(Test_SquareBracketAssignment_MultipleBuckets);
  DBG_RunTest(Test_Clear);
  printf("Succeess\n");
  return 0;
}
