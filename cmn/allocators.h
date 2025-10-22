#pragma once

#define CMN_MALLOC_FUNCTION(name)  void* name(size_t sz)
#define CMN_REALLOC_FUNCTION(name) void* name(void* p, size_t sz)
#define CMN_FREE_FUNCTION(name)    void  name(void* p)

namespace cmn {
  typedef CMN_MALLOC_FUNCTION(_cmn_malloc);
  typedef CMN_REALLOC_FUNCTION(_cmn_realloc);
  typedef CMN_FREE_FUNCTION(_cmn_free);
  extern _cmn_malloc* _g_cmn_malloc;
  extern _cmn_realloc* _g_cmn_realloc;
  extern _cmn_free* _g_cmn_free;
}

#ifndef CMN_ALLOC_FUNCTIONS
#define CMN_ALLOC_FUNCTIONS
#include <cstdlib>

namespace cmn {
  _cmn_malloc*  _g_cmn_malloc  = malloc;
  _cmn_realloc* _g_cmn_realloc = realloc;
  _cmn_free*    _g_cmn_free    = free;


void SetDefaultCustomAllocators(_cmn_malloc* aMalloc,  _cmn_realloc* aRealloc, _cmn_free* aFree) {
  _g_cmn_malloc = aMalloc;
  _g_cmn_realloc = aRealloc;
  _g_cmn_free = aFree;
}
}

#endif // cmn
