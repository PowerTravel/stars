#include <cstdio>


#include "list.h"
#include "debug_asserts.h"

bool gShouldPrint = false;

void TestConstructDestructEmpty()
{
  dbg::SetCustomGlobalAllocators();
  {
    cmn::list<int> List = cmn::list<int>();
  }
  DBG_Assert(gMallocCount, 0, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, 0,   "Free Call Count");
}

void TestPushPop_1(){
  dbg::SetCustomGlobalAllocators();

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

  dbg::SetCustomGlobalAllocators();

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

    // Test looping Right
    {
      cmn::list<int>::element* Element = List.First();
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
      cmn::list<int>::element* Element = List.Last();
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
  }

  DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
  DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
  DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");

}

void TestLoop(){
  dbg::SetCustomGlobalAllocators();

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
}

void TestConstructDestruct()
{
  dbg::SetCustomGlobalAllocators();
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

void TestMove_1(){
    dbg::SetCustomGlobalAllocators();
    
    int N = 32;
    int AllocFreeCount = N*2+2;
    cmn::list<int>::element* DetachedElement = 0;
    {
      cmn::list<int> ListA = cmn::list<int>();
      for (int i = 0; i < N; ++i)
      {
        ListA.PushBack(i);
      }

      cmn::list<int> ListB = cmn::list<int>();
      for (int i = 0; i < N/2; ++i)
      {
        ListA.MoveInto(ListB, ListA.First());
      }

      {
        cmn::list<int>::element* BElement = ListB.First();
        for (int i = 0; i < N/2; ++i)
        {
          DBG_Assert(BElement->GetCopy(), i, "Size After Detach");
          BElement = BElement->Next;
        }
        DBG_Assert(ListB.IsEnd(BElement), true, "List B Is End");
      }
      {
        cmn::list<int>::element* AElement = ListA.First();
        for (int i = N/2; i < N; ++i)
        {
          DBG_Assert(AElement->GetCopy(), i, "Size After Detach");
          AElement = AElement->Next;
        }
        DBG_Assert(ListA.IsEnd(AElement), true, "List A Is End");
      }

      // Test Get on list
      {
        for (int i = 0; i < N/2; ++i)
        {
          DBG_Assert(ListB.GetCopy(i), i, "Size After Detach");
        }
      }
      {
        for (int i = 0; i < N/2; ++i)
        {
          DBG_Assert(ListA.GetRef(i), i+N/2, "Size After Detach");
        }
      }

      DBG_Assert(ListA.Size(), ListB.Size(), "Size After Detach");
    }

    // Add 1 because we allocated the sentinel of list B
    DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
    DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
    // Detached node is not freed since its no longer connected to the list
    DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");
}

void TestMove_2(){
    dbg::SetCustomGlobalAllocators();
    
    int N = 32;
    int AllocFreeCount = N*2+2;
    cmn::list<int>::element* DetachedElement = 0;
    {
      cmn::list<int> ListA = cmn::list<int>();
      for (int i = 0; i < N; ++i)
      {
        ListA.PushBack(i);
      }

      cmn::list<int> ListB = cmn::list<int>();
      ListA.MoveInto(ListB);

      {
        cmn::list<int>::element* BElement = ListB.First();
        for (int i = 0; i < N; ++i)
        {
          DBG_Assert(BElement->GetCopy(), i, "Size After Detach");
          BElement = BElement->Next;
        }
        DBG_Assert(ListB.IsEnd(BElement), true, "List B Is End");
      }
    }

    // Add 1 because we allocated the sentinel of list B
    DBG_Assert(gMallocCount, AllocFreeCount, "Malloc Call Count");
    DBG_Assert(gReallocCount, 0,  "Realloc Call Count");
    // Detached node is not freed since its no longer connected to the list
    DBG_Assert(gFreeCount, AllocFreeCount,   "Free Call Count");
}

void TestCopyOperatorCopyContructor()
{
  dbg::SetCustomGlobalAllocators();
  int N = 32;
  int AllocFreeCount = N*4+2;

  {
    cmn::list<int> ListA = cmn::list<int>();
    for (int i = 0; i < N; ++i)
    {
      ListA.PushBack(i);
    }

    // Copy List
    cmn::list<int> ListB = ListA;

    for (int i = 0; i < N; ++i)
    {
      // Values are the same
      DBG_Assert(ListA.GetCopy(i), ListB.GetCopy(i), "Value");

      // Adresses are NOT the same
      DBG_AssertFalse((void*) ListA.GetPtr(i), (void*) ListB.GetPtr(i), "Value Address");
    }
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
  DBG_RunTest(TestMove_1);
  DBG_RunTest(TestMove_2);
  DBG_RunTest(TestCopyOperatorCopyContructor);
  return 0;
}