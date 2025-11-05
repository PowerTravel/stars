#pragma once

#include "macros.h"
#include <cstdint> // uint32_t etc

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

    
  // From: https://www.azillionmonkeys.com/qed/hash.html
  #undef get16bits
  #if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
    || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
  #define get16bits(d) (*((const uint16_t *) (d)))
  #endif

  #if !defined (get16bits)
  #define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                         +(uint32_t)(((const uint8_t *)(d))[0]) )
  #endif

  uint32_t SuperFastHash (const char * data, int len) {
    uint32_t hash = len;
    uint32_t tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
  }


} // util
} // cmn