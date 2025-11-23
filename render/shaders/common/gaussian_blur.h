#pragma once

#include "commons/types.h"

namespace gaussian_blur {
 
// Note: BinomialDepth must be even.
// CutOff must be less than half BinomialDepth
u32 Kernel(u32 BinomialDepth, u32 CutOff, r32* OutOffset, r32* OutWeight)
{
  r32 CoefficientsA[1028] = {};
  r32 CoefficientsB[1028] = {};
  r32 Offset[1028] = {};
  r32* Current = CoefficientsA;
  r32* Previous = CoefficientsB;
  for (int i = 0; i <= BinomialDepth; ++i)
  {
    if(i > 0)
    {
      for (int j = 0; j <= i; ++j)
      {
        if(j == 0)
        {
          Current[0] = Previous[0];
        }else if (j == i)
        {
          Current[j] = Previous[i-1];
        }else{
          Current[j] = Previous[j] + Previous[j-1];
        }
      }  
    }else{
      Current[0] = 1;
    }
    r32* Tmp = Previous;
    Previous = Current;
    Current = Tmp;
  }
  
  for (int i = CutOff; i <= BinomialDepth-CutOff; ++i)
  {
    Current[i-CutOff] = Previous[i];
  }

  u32 ReducedSize = BinomialDepth-2*CutOff + 1;
  r32 Sum = 0;
  for (int i = 0; i < ReducedSize; ++i)
  {
    Sum += Current[i];
  }

  for (int i = 0; i < ReducedSize; ++i)
  {
    Current[i] /= Sum;
  }

  u32 ReducedHalfSize = ReducedSize / 2 + 1;

  r32* Tmp = Previous;
  Previous = Current;
  Current = Tmp;
  for (int i = 0; i < ReducedHalfSize; ++i)
  {
    Offset[i] = i;
    Current[ReducedHalfSize - 1 - i] = Previous[i];
  }

  u32 Size = ReducedHalfSize/2 + 1;
  Tmp = Previous;
  Previous = Current;
  Current = Tmp;
  OutWeight[0] = Previous[0];
  for (int i = 1; i < Size; ++i)
  {
    u32 idx = 2*i-1;
    OutWeight[i] = Previous[idx] + Previous[idx+1];
    OutOffset[i] = (Previous[idx] * Offset[idx] + Previous[idx+1] * Offset[idx+1]) / OutWeight[i];
  }

  return Size;
}

} // namespace gaussian_blur