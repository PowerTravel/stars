#pragma once

#include <cstdio>

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif // Assert

extern bool gShouldPrint;

namespace dbg{

  void jAssert(int Actual, int Desired, const char* name = 0, const char* File = 0, int Line = 0)
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

  void Print(const char* str)
  {
    if(gShouldPrint)
    {
      printf(str);
    }
  }
}

#define DBG_Assert(Actual, Desired, Name) dbg::jAssert(Actual, Desired, Name, __FILE__, __LINE__)

#define DBG_RunTest(testfunction) {\
  ResetTestEnvironment();\
  dbg::Print("\n\t");\
  dbg::Print(#testfunction);\
  dbg::Print(":");\
  testfunction();\
  dbg::Print(" success");\
}\
