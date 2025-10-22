#pragma once

#include "macros.h"

namespace cmn{
namespace utils {
  inline void* Copy(size_t aSize, const void* SourceInit, void* DestInit)
  {
    uint8_t *Source = (uint8_t *)SourceInit;
    uint8_t *Dest = (uint8_t *)DestInit;
    uint32_t i = 0;
    while (aSize--) { *Dest++ = *Source++;}

    return(DestInit);
  }

  inline void Zero(size_t MemSizeBytes, uint8_t* Mem){
    for (int i = 0; i < MemSizeBytes; ++i)
    {
      *Mem++ = 0;
    }
  }

  struct bit_scan_result
  {
    bool found;
    uint32_t index;
  };

  inline bit_scan_result
  FindMostSignificantSetBit32( uint32_t value )
  {
    bit_scan_result result = {};

  #ifdef COMPILER_MSVC
    result.found = _BitScanReverse( (unsigned long*) &result.index, value);
  #else
    for(uint32_t test = 31; test >= 0; test--)
    {
      uint32_t mask = (1 << test);
      if( (value & mask ) != 0)
      {
        result.index = test;
        result.found = true;
        break;
      }
    }
  #endif
    return result;
  }

  inline bit_scan_result
  FindMostSignificantSetBit64( uint64_t val )
  {
    bit_scan_result result = {};
   
    uint32_t high = (val & 0xFFFFFFFF00000000)>>32;
    uint32_t low = val & 0x00000000FFFFFFFF;
    if(high)
    {
      result = FindMostSignificantSetBit32(high);
      result.index += 32;
    }else{
      result = FindMostSignificantSetBit32(low);
    }
    return result;
  }

  #include <float.h>

  inline uint64_t
  GetNextPowerOfTwo( uint64_t v )
  {
    uint64_t result = 1;
    bit_scan_result bitScan = FindMostSignificantSetBit64( v );
    if(bitScan.found)
    {
      Assert(bitScan.index < 63);
      result = 1<<(bitScan.index+1);
    }
    return result;
  }

} // util
} // cmn