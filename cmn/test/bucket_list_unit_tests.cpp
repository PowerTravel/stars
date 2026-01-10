#include <cstdio>

#include "bucket_list.h"
#include "debug_asserts.h"

bool gShouldPrint = false;

template<typename T> using bucket_list_dbg = cmn::bucket_list<T, CustomAllocators>;

void TestConstructDeleteEmpty()
{
  size_t MallocCountAfterCreate = 4;
  bucket_list_dbg<int> BucketList = bucket_list_dbg<int>::Create(32);
  DBG_Assert(BucketList.Valid(), true, "Bucket List is Valid");
  DBG_Assert(BucketList.Size(), 0, "BucketList has size 0");
  DBG_Assert(BucketList.IsEmpty(), true, "BucketList is Empty");

  BucketList.Delete();
  DBG_Assert(gMallocCount, MallocCountAfterCreate, "Malloc Call Count");
  DBG_Assert(gFreeCount,   MallocCountAfterCreate, "Free Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(BucketList.Valid(), false, "Bucket List is invalid");
}

void TestPushWithASingleBucket()
{
  const size_t BucketSize = 32;
  const size_t MallocCountAfterCreate = 4;

  bucket_list_dbg<int> BucketList = bucket_list_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize; ++i)
  {
    BucketList.PushBack(i);
  }
  DBG_Assert(gMallocCount, MallocCountAfterCreate, "Malloc Call Count");

  BucketList.Delete();
  DBG_Assert(gFreeCount, MallocCountAfterCreate, "Free Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
}

void TestPushPopDeleteWithASingleBucket()
{
  const size_t BucketSize = 32;
  const size_t MallocCountAfterCreate = 4;

  bucket_list_dbg<int> BucketList = bucket_list_dbg<int>::Create(BucketSize);
  for (int i = 0; i < BucketSize; ++i)
  {
    BucketList.PushBack(i);
  }
  DBG_Assert(gMallocCount, MallocCountAfterCreate, "Malloc Call Count");

  int PopCount = 0;
  while(!BucketList.IsEmpty())
  {
    PopCount++;
    BucketList.PopBack();
  }
  DBG_Assert(gMallocCount, MallocCountAfterCreate, "Malloc Call Count");
  

  BucketList.Delete();
  DBG_Assert(gFreeCount, MallocCountAfterCreate, "Free Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
}

void TestPushDeleteWithMultipleBuckets()
{
  const size_t BucketCount = 3;
  const size_t BucketSize = 32;
  const size_t TotalCount = BucketSize * BucketCount;
  const size_t MallocCountAfterCreate = 4;

  bucket_list_dbg<int> BucketList = bucket_list_dbg<int>::Create(BucketSize);
  for (int i = 0; i < TotalCount; ++i)
  {
    BucketList.PushBack(i);
  }
  DBG_Assert(gMallocCount, MallocCountAfterCreate+BucketCount, "Malloc Call Count");

  BucketList.Delete();
  DBG_Assert(gFreeCount,   MallocCountAfterCreate+BucketCount, "Free Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
}

void TestPushPopDeleteWithMultipleBuckets()
{
  const size_t BucketCount = 3;
  const size_t BucketSize = 32;
  const size_t TotalCount = BucketSize * BucketCount;
  const size_t MallocCountAfterCreate = 4;
  const size_t TotalMallocCount = MallocCountAfterCreate  + BucketCount;

  bucket_list_dbg<int> BucketList = bucket_list_dbg<int>::Create(BucketSize);
  for (int i = 0; i < TotalCount; ++i)
  {
    BucketList.PushBack(i);
  }
  DBG_Assert(gMallocCount, TotalMallocCount, "Malloc Call Count");

  int PopCount = 0;
  while(!BucketList.IsEmpty())
  {
    PopCount++;
    BucketList.PopBack();
  }
  DBG_Assert(PopCount, TotalMallocCount, "Malloc Call Count");
  DBG_Assert(gFreeCount, MallocCountAfterCreate, "Free Call Count");

  BucketList.Delete();

  DBG_Assert(gMallocCount, TotalMallocCount, "Malloc Call Count");
  DBG_Assert(gFreeCount,   TotalMallocCount, "Free Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
}


// Clear Removes data but keeps memory

int main(int argc, char* argv[]) {
  gShouldPrint = argc > 1;
  DBG_RunTest(TestConstructDeleteEmpty);
  DBG_RunTest(TestPushWithASingleBucket);
  DBG_RunTest(TestPushPopDeleteWithASingleBucket);
  DBG_RunTest(TestPushDeleteWithMultipleBuckets);
  DBG_RunTest(TestPushPopDeleteWithMultipleBuckets);
  printf("Success\n");
  return 0;
}