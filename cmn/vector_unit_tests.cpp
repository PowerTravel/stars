#include <cstdio>

#include "vector.h"
#include "debug_asserts.h"

bool gShouldPrint = false;
int gReallocCount = 0;
int gMallocCount = 0;
int gFreeCount = 0;

CMN_MALLOC_FUNCTION(customMalloc){
  gMallocCount++;
  return malloc(sz);
}
CMN_REALLOC_FUNCTION(customRealloc){
  gReallocCount++;
  return realloc(p, sz);
}
CMN_FREE_FUNCTION(customFree){
  gFreeCount++;
  free(p);
}

void ResetAllocationCounters()
{
  gReallocCount = 0;
  gMallocCount = 0;
  gFreeCount = 0;
}

void SetDefaultGlobalAllocators(){
  cmn::SetDefaultCustomAllocators(malloc, realloc, free);
}

void SetCustomGlobalAllocators(){
  cmn::SetDefaultCustomAllocators(customMalloc, customRealloc, customFree);
}

void ResetTestEnvironment()
{
  ResetAllocationCounters();
  SetDefaultGlobalAllocators();
}

void BasicPushPop(cmn::vector<int>& v) {

  const size_t SizeOfVecPowTwo_1 = 64;
  const size_t SizeOfVecPowTwo_2 = 128;

  for (int i = 0; i < SizeOfVecPowTwo_1; ++i)
  {
    v.PushBack(i);
    DBG_Assert(v.Back(), i, "Value from Back()");  
    DBG_Assert(v.Front(), 0, "Value from Back()");  
  }

  // Assert the size is SizeOfVecPowTwo_1 and the reservedSize is also SizeOfVecPowTwo_1.
  // Reserved and actual size is the same because SizeOfVecPowTwo_1 is a power of 2
  
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

void Test1_BasicPushPop()
{
  cmn::vector<int> v = cmn::vector<int>();
  BasicPushPop(v);
}


void Test2_BasicPushPop_GlobalCustomAllocators()
{
  SetCustomGlobalAllocators();
  cmn::vector<int> v1 = cmn::vector<int>();
  BasicPushPop(v1);
  DBG_Assert(gMallocCount, 1, "Malloc Call Count");
  DBG_Assert(gReallocCount, 7, "Realloc Call Count");
  DBG_Assert(gFreeCount, 1, "Free Call Count");

  // Reset default allocators
  SetDefaultGlobalAllocators();
  cmn::vector<int> v2 = cmn::vector<int>();
  BasicPushPop(v2);
  DBG_Assert(gMallocCount, 1, "Malloc Call Count");
  DBG_Assert(gReallocCount, 7, "Realloc Call Count");
  DBG_Assert(gFreeCount, 1, "Free Call Count"); 
}

void Test3_BasicPushPop_LocalCustomAllocators(){
  cmn::vector<int> v1 = cmn::vector<int>();
  cmn::vector<int> v2 = cmn::vector<int>(0, customMalloc, customRealloc, customFree);
  BasicPushPop(v1); // Using global allocators 
  BasicPushPop(v2); // Using differen local allocators 

  DBG_Assert(gMallocCount, 1, "Malloc Call Count");
  DBG_Assert(gReallocCount, 7, "Realloc Call Count");
  DBG_Assert(gFreeCount, 1, "Free Call Count");
}

void TestConstructorDestructor_1()
{
  SetCustomGlobalAllocators();
  {
    // No allocations should happen with an empty vector
    cmn::vector<float> v = cmn::vector<float>();
  }
  DBG_Assert(gMallocCount, 0, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount, 0, "Free Call Count"); 
}


void TestConstructorDestructor_2()
{
  SetCustomGlobalAllocators();
  {
    // No allocations should happen with an empty vector
    cmn::vector<float> v = cmn::vector<float>(10);
  }
  DBG_Assert(gMallocCount, 1, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0, "Realloc Call Count");
  DBG_Assert(gFreeCount, 1, "Free Call Count");
}


void TestRangeConstructor()
{
  int buf[] = {1,2,3,4,5,6,7,7,8,4,665};
  cmn::vector<int> v = cmn::vector<int>(ArrayCount(buf),buf);
  for (int i = 0; i < ArrayCount(buf); ++i)
  {
    DBG_Assert(v[i], buf[i], "v[i]");
  }
}

void TestCopyConstructor()
{
  int buf[] = {1,2,3,4,5,6,7,7,8,4,665};
  cmn::vector<int> v1 = cmn::vector<int>(ArrayCount(buf),buf);

  for (int i = 0; i < ArrayCount(buf); ++i)
  {
    DBG_Assert(v1[i], buf[i], "v1[i]");
  }

  // Copy over v1 to v2 and delete v1
  cmn::vector<int> v2 = cmn::vector<int>(v1);
  v1.Delete();
  DBG_Assert(v1.Reserved(), 0, "v1 Reserved");
  DBG_Assert(v1.Size(), 0, "v1 Size");
  DBG_Assert(v1.Empty(), true, "v1 Empty");

  for (int i = 0; i < ArrayCount(buf); ++i){
    DBG_Assert(v2[i], buf[i], "v2[i]");
  }
  DBG_Assert(v2.Reserved(), ArrayCount(buf), "v2 Reserved");
  DBG_Assert(v2.Size(), ArrayCount(buf), "v2 Size");
  DBG_Assert(v2.Empty(), false, "v2 Empty");
}

void TestCopyOperator() {  
  int buf[] = {1,2,3,4,5,6,7,7,8,4,665};
  cmn::vector<int> v1 = cmn::vector<int>(ArrayCount(buf),buf);

  for (int i = 0; i < ArrayCount(buf); ++i)
  {
    DBG_Assert(v1[i], buf[i], "v1[i]");
  }

  // Copy over v1 to v2 and delete v1
  cmn::vector<int> v2 = v1;
  v1.Delete();
  DBG_Assert(v1.Reserved(), 0, "v1 Reserved");
  DBG_Assert(v1.Size(), 0, "v1 Size");
  DBG_Assert(v1.Empty(), true, "v1 Empty");

  for (int i = 0; i < ArrayCount(buf); ++i){
    DBG_Assert(v2[i], buf[i], "v2[i]");
  }
  DBG_Assert(v2.Reserved(), ArrayCount(buf), "v2 Reserved");
  DBG_Assert(v2.Size(), ArrayCount(buf), "v2 Size");
  DBG_Assert(v2.Empty(), false, "v2 Empty");
}

int main(int argc, char* argv[]){
  gShouldPrint = argc > 1;
  DBG_RunTest(TestConstructorDestructor_1);
  DBG_RunTest(TestConstructorDestructor_2);
  DBG_RunTest(TestRangeConstructor);
  DBG_RunTest(TestCopyConstructor);
  DBG_RunTest(TestCopyOperator);

  DBG_RunTest(Test1_BasicPushPop);
  DBG_RunTest(Test2_BasicPushPop_GlobalCustomAllocators);
  DBG_RunTest(Test3_BasicPushPop_LocalCustomAllocators);
  return 0;
}
