#include <cstdio>

#include "list.h"
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

void TestConstructDestructEmpty()
{
  SetCustomGlobalAllocators();
  {
    cmn::list<int> List = cmn::list<int>();
  }
  DBG_Assert(gMallocCount, 0, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, 0,   "Free Call Count");
}

void TestPushPop_1(){
  SetCustomGlobalAllocators();

  int N = 32;
  int AllocFreeCount = N*2+1; // There are two allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.
  {
    cmn::list<int> List = cmn::list<int>();

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
    for (int i = 0; i < N; ++i)
    {
      List.InsertAfter(List.Last(), i);
    }

    DBG_Assert(List.Empty(), false, "List Empty");
    DBG_Assert(List.Size(), N, "List Size");

    while(!List.IsEnd(List.Last()))
    {
      List.Delete(List.Last());
    }

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
  }

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");

}


void TestPushPop_2(){

  SetCustomGlobalAllocators();

  int N = 32;
  int AllocFreeCount =2*(N*2+1); // There are two lists and allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.
  {
    cmn::list<int> List = cmn::list<int>();

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
    for (int i = 0; i < N; ++i)
    {
      List.PushBack(i);
    }

    DBG_Assert(List.Empty(), false, "List Empty");
    DBG_Assert(List.Size(), N, "List Size");

    int Index = 1;
    while(!List.Empty())
    {
      int val = List.PopBack();
      int truVal = N-Index++;
      DBG_Assert(val, truVal, "PopBack");
    }

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
  }
  {
    cmn::list<int> List = cmn::list<int>();

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
    for (int i = 0; i < N; ++i)
    {
      List.PushFront(i);
    }

    DBG_Assert(List.Empty(), false, "List Empty");
    DBG_Assert(List.Size(), N, "List Size");

    int Index = 1;
    while(!List.Empty())
    {
      int val = List.PopFront();
      int truVal = N-Index++;
      DBG_Assert(val, truVal, "PopBack");
    }

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
  }

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");

}

void TestLoop(){
  ResetTestEnvironment();
  SetCustomGlobalAllocators();

  int N = 32;
  int AllocFreeCount = N*2+1; // There are two allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.
  {
    cmn::list<int> List = cmn::list<int>();

    for (int i = 0; i < N; ++i)
    {
      List.PushBack(i);
    }

    int Index = 0;
    cmn::list<int>::element* E = List.First();
    while( !List.IsEnd(E) )
    {
      int truVal = Index++;
      DBG_Assert(E->GetCopy(), truVal, "Data");
      E = E->Next;
    }
    DBG_Assert(Index, N, "Index");

    Index = 0;
    E = List.Last();
    while( !List.IsEnd(E) )
    {
      int truVal = N-1-Index++;
      DBG_Assert(E->GetCopy(), truVal, "Data");
      E = E->Previous;
    }
    DBG_Assert(Index, N, "Index");
  }

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");

  ResetTestEnvironment();
}

void TestConstructDestruct()
{
  SetCustomGlobalAllocators();
  int N = 32;
  int AllocFreeCount = N*2+1; // There are two allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.
  {
    cmn::list<int> List = cmn::list<int>();

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");

    for (int i = 0; i < N; ++i)
    {
      List.InsertAfter(List.Last(),i);
    }
    DBG_Assert(List.Empty(), false, "List Empty");
    DBG_Assert(List.Size(), N, "List Size");
  }

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");
}

int main(int argc, char* argv[]) {
  gShouldPrint = argc > 1;
  DBG_RunTest(TestConstructDestructEmpty);
  DBG_RunTest(TestPushPop_1);
  DBG_RunTest(TestPushPop_2);
  DBG_RunTest(TestConstructDestruct);
  DBG_RunTest(TestLoop);
  return 0;
}