#pragma once

#define CMN_MALLOC_FUNCTION(name)  void* name(size_t sz)
#define CMN_REALLOC_FUNCTION(name) void* name(void* p, size_t sz)
#define CMN_FREE_FUNCTION(name)    void  name(void* p)

// Note: Move this to jwin so that it can safely be referenced form jwin_platform

namespace cmn {
  typedef CMN_MALLOC_FUNCTION(_cmn_malloc);
  typedef CMN_REALLOC_FUNCTION(_cmn_realloc);
  typedef CMN_FREE_FUNCTION(_cmn_free);
  extern _cmn_malloc* _g_cmn_malloc;
  extern _cmn_realloc* _g_cmn_realloc;
  extern _cmn_free* _g_cmn_free;

  // Transient allocators are those which are assumed to be wiped regularly
  // This means we can set more efficient custom allocators such as memory arenas
  // and even have empty versions of free.
  // Realloc is annoying, since one has to copy, be aware that if you use 
  // vectors with custom transient allocators, always preallocate them.
  extern _cmn_malloc*  _g_cmn_transient_malloc;
  extern _cmn_realloc* _g_cmn_transient_realloc;
  extern _cmn_free*    _g_cmn_transient_free;

  void SetDefaultCustomAllocators
  (
    _cmn_malloc* aMalloc,           _cmn_realloc* aRealloc,          _cmn_free* aFree,
    _cmn_malloc* aTransientMalloc,  _cmn_realloc* aTransientRealloc, _cmn_free* aTransientFree
  ) {
    _g_cmn_malloc = aMalloc;
    _g_cmn_realloc = aRealloc;
    _g_cmn_free = aFree;
    _g_cmn_transient_malloc = aTransientMalloc;
    _g_cmn_transient_realloc = aTransientRealloc;
    _g_cmn_transient_free = aTransientFree;
  }
} // cmn

// If CMN_ALLOC_FUNCTIONS is not defined, we assume no global allocators have been set and use the C++ library to assign them.
#ifndef CMN_ALLOC_FUNCTIONS
#include <cstdlib>

CMN_MALLOC_FUNCTION(CmnAllocate){
  return malloc(sz);  
}
CMN_REALLOC_FUNCTION(CmnReallocate){
  return realloc(p,sz);
}
CMN_FREE_FUNCTION(CmnFree){
  free(p);
}
CMN_MALLOC_FUNCTION(CmnTransientAllocate){
  return malloc(sz);  
}
CMN_REALLOC_FUNCTION(CmnTransientReallocate){
  return realloc(p,sz);
}
CMN_FREE_FUNCTION(CmnTransientFree){
  free(p);
}
CMN_MALLOC_FUNCTION(CmnFrameTransientAllocate){
  return malloc(sz);  
}
CMN_REALLOC_FUNCTION(CmnFrameTransientReallocate){
  return realloc(p,sz);
}
CMN_FREE_FUNCTION(CmnFrameTransientFree){
  free(p);
}

namespace cmn {
  _cmn_malloc*  _g_cmn_malloc  = malloc;
  _cmn_realloc* _g_cmn_realloc = realloc;
  _cmn_free*    _g_cmn_free    = free;
  _cmn_malloc*  _g_cmn_transient_malloc  = malloc;  
  _cmn_realloc* _g_cmn_transient_realloc = realloc; 
  _cmn_free*    _g_cmn_transient_free    = free;
} // cmn

#endif // CMN_ALLOC_FUNCTIONS
