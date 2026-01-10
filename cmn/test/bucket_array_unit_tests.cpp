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

template<typename Allocs>
void BasicPushPop(cmn::bucket_array<int, Allocs>& v) {

  const size_t SizeOfVecPowTwo_1 = 64;
  const size_t SizeOfVecPowTwo_2 = 128;

  for (int i = 0; i < SizeOfVecPowTwo_1; ++i)
  {
    v.PushBack(i);
    DBG_Assert(v.Back(), i, "Value from Back()");  
    DBG_Assert(v.Front(), 0, "Value from Back()");  
  }

  DBG_Assert(v.Size(), SizeOfVecPowTwo_1, "Vector Size");
  DBG_Assert(v.Reserved(), SizeOfVecPowTwo_1, "Vector Reserved");

  // Add another element and verity that the size grows by one, but reserved size grows by a power of two
  v.PushBack(-5);
  DBG_Assert(v.Size(), SizeOfVecPowTwo_1+1, "Vector Size");
  v.PopBack();
  DBG_Assert(v.Reserved(), SizeOfVecPowTwo_2, "Vector Reserved");
  DBG_Assert(v.Size(), SizeOfVecPowTwo_1, "Vector Size");

  // Test getters
  for (int i = 0; i < v.Size(); ++i)
  {
    int val = v[i];
    DBG_Assert(val, i, "Value");
  }

  // Test using At to store values. (Note: At is valid to call on index within bounds of [0, Reserved])
  // But it does not affect Size();
  size_t N = v.Size();
  for (int i = 0; i < N; ++i)
  {
    DBG_Assert(v.Size(), N, "Vector Size");
    v[i] = v.Size()-i;
  }

  int trueVal = 1;
  while(v.Size())
  {
    int val = v.Back();
    v.PopBack();
    DBG_Assert(val, trueVal++, "True Value");
  }
  // Verify that we are back to 0 but with a larger resrved size
  DBG_Assert(v.Size(), 0, "Vector Size");
  DBG_Assert(v.Reserved(), SizeOfVecPowTwo_2, "Vector Reserved");

  v.Delete();
  DBG_Assert(v.Size(), 0, "Vector Size");
  DBG_Assert(v.Reserved(), 0, "Vector Reserved");
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


int main(int argc, char* argv[]){
  gShouldPrint = argc > 1;
  DBG_RunTest(TestCreateDelete);
  DBG_RunTest(Test_PushDelete_SingleBucket);
  DBG_RunTest(Test_PushDelete_MultipleBuckets);
  DBG_RunTest(Test_SquareBracketAssignment_SingleBucket);
  DBG_RunTest(Test_SquareBracketAssignment_MultipleBuckets);
  printf("Succeess\n");
  return 0;
}
