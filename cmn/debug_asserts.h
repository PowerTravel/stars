#pragma once

#include <cstdio>
#include <chrono>
#include <ctime>
#include <string>

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif // Assert

extern bool gShouldPrint;

int gReallocCount = 0;
int gMallocCount = 0;
int gFreeCount = 0;

CMN_MALLOC_FUNCTION(customAllocate){
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

#define DebugAllocators customAllocate, customRealloc, customFree

namespace dbg{

  void ResetAllocationCounters()
  {
    gReallocCount = 0;
    gMallocCount = 0;
    gFreeCount = 0;
  }

  void SetDefaultGlobalAllocators(){
    cmn::SetDefaultCustomAllocators(malloc, realloc, free, malloc, realloc, free);
  }

  void SetCustomGlobalAllocators(){
    cmn::SetDefaultCustomAllocators(customAllocate, customRealloc, customFree, customAllocate, customRealloc, customFree);
  }

  void ResetTestEnvironment()
  {
    ResetAllocationCounters();
    SetDefaultGlobalAllocators();
  }


  void jAssert(int Actual, int Desired, const char* name = 0, const char* File = 0, const int Line = 0)
  {
    if(Actual != Desired)
    {
      printf("\nAssert Failed:\n");
      if(Line && File){
        printf("%s:%d : ", File,Line);
      }
      printf("'%s' was: %d, but should be: %d\n", name == 0 ? "value" : name, Actual, Desired);
      printf("\nExiting\n");
      exit(1);
    }
  }

  void jAssert(void* Actual, void* Desired, const char* name = 0, const char* File = 0, const int Line = 0)
  {
    if(Actual != Desired)
    {
      printf("\nAssert Failed:\n");
      if(Line && File){
        printf("%s:%d : ", File,Line);
      }
      printf("'%s' was: %p, but should be: %p\n", name == 0 ? "value" : name, Actual, Desired);
      printf("\nExiting\n");
      exit(1);
    }
  }

  void jAssertFalse(int Actual, int NotDesired, const char* name = 0, const char* File = 0, const int Line = 0)
  {
    if(Actual == NotDesired)
    {
      printf("\nAssert Failed:\n");
      if(Line && File){
        printf("%s:%d : ", File,Line);
      }
      printf("'%s' was: %d, but should be: %d\n", name == 0 ? "value" : name, Actual, NotDesired);
      printf("\nExiting\n");
      exit(1);
    }
  }

  void jAssertFalse(void* Actual, void* NotDesired, const char* name = 0, const char* File = 0, const int Line = 0)
  {
    if(Actual == NotDesired)
    {
      printf("\nAssert Failed:\n");
      if(Line && File){
        printf("%s:%d : ", File,Line);
      }
      printf("'%s' was: %p, but should be: %p\n", name == 0 ? "value" : name, Actual, NotDesired);
      printf("\nExiting\n");
      exit(1);
    }
  }

  void Print(const char* str)
  {
    if(gShouldPrint)
    {
      printf(str);
    }
  }
}

#define DBG_Assert(Actual, Desired, Name) dbg::jAssert(Actual, Desired, Name, __FILE__, __LINE__)
#define DBG_AssertFalse(Actual, NotDesired, Name) dbg::jAssertFalse(Actual, NotDesired, Name, __FILE__, __LINE__)


std::chrono::time_point<std::chrono:: steady_clock> StartTimer()
{
  return std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::now());
}

double Elapsed(std::chrono::time_point<std::chrono::steady_clock>& Clock) {
  return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1,1000>>>(std::chrono::steady_clock::now() - Clock).count();
}

#define DBG_RunTest(testfunction) {\
  std::chrono::time_point<std::chrono::steady_clock> Timer = StartTimer();\
  dbg::ResetTestEnvironment();\
  dbg::Print("\t");\
  dbg::Print(#testfunction);\
  dbg::Print(":");\
  testfunction();\
  dbg::Print(" success ");\
  std::string Time = std::to_string(Elapsed(Timer));\
  dbg::Print(Time.c_str());\
  dbg::Print("\n");\
}
