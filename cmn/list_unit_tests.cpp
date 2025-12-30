#include <cstdio>


#include "list.h"
#include "debug_asserts.h"

bool gShouldPrint = false;


template<typename T> using list_dbg = cmn::list<T, customMalloc, customFree>;
template<typename T> using list_std = cmn::list<T, malloc, free>;

void TestConstructDestructEmpty()
{
  list_dbg<int> List = list_dbg<int>::Create();
  List.Delete();
  DBG_Assert(gMallocCount, 0, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, 0,   "Free Call Count");
}

void TestPushPop_1(){
  int N = 32;
  int AllocFreeCount = N*2+1; // There are two allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.
  
  list_dbg<int> List = list_dbg<int>::Create();

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

  List.Delete();

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");

}


void TestPushPop_2(){

  int N = 32;
  int AllocFreeCount =2*(N*2+1); // There are two lists and allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.
  {
    list_dbg<int> List = list_dbg<int>::Create();

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
    List.Delete();
  }
  {
    list_dbg<int> List = list_dbg<int>::Create();

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
    for (int i = 0; i < N; ++i)
    {
      List.PushFront(i);
    }

    DBG_Assert(List.Empty(), false, "List Empty");
    DBG_Assert(List.Size(), N, "List Size");

    // Test looping Right
    {
      list_dbg<int>::element* Element = List.First();
      int ListValue = N-1;
      while (!List.IsEnd(Element))
      {
        DBG_Assert(Element->GetCopy(), ListValue--, "Loop Right");
        Element = Element->Next;
      }
    }

    // Test looping Left
    {
      int ListValue = 0;
      list_dbg<int>::element* Element = List.Last();
      while (!List.IsEnd(Element))
      {
        DBG_Assert(Element->GetCopy(), ListValue++, "Loop Left");
        Element = Element->Previous;
      }
    }
    
    // Test At function
    {
      for (int i = 0; i < N; ++i)
      {
        DBG_Assert(List.At(i)->GetCopy(), N-i-1, "List At");
      }
    }

    int Index = 1;
    while(!List.Empty())
    {
      int val = List.PopFront();
      int truVal = N-Index++;
      DBG_Assert(val, truVal, "PopBack");
    }

    DBG_Assert(List.Empty(), true, "List Empty");
    DBG_Assert(List.Size(), 0, "List Size");
    List.Delete();
  }

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");

}

void TestLoop(){
  int N = 32;
  int AllocFreeCount = N*2+1; // There are two allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.

  list_dbg<int> List = list_dbg<int>::Create();

  for (int i = 0; i < N; ++i)
  {
    List.PushBack(i);
  }

  int Index = 0;
  list_dbg<int>::element* E = List.First();
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
  List.Delete();

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");
}

void TestConstructDestruct()
{
  dbg::SetCustomGlobalAllocators();
  int N = 32;
  int AllocFreeCount = N*2+1; // There are two allocs per node. One for the node + 1 for the data. The sentinel receives one alloc but no data alloc.

  list_dbg<int> List = list_dbg<int>::Create();

  DBG_Assert(List.Empty(), true, "List Empty");
  DBG_Assert(List.Size(), 0, "List Size");

  for (int i = 0; i < N; ++i)
  {
    List.InsertAfter(List.Last(),i);
  }
  DBG_Assert(List.Empty(), false, "List Empty");
  DBG_Assert(List.Size(), N, "List Size");

  List.Delete();

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
  printf("Success\n");
  return 0;
}