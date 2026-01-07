#pragma once

#define CMN_MALLOC_FUNCTION  void* Allocate(size_t sz)
#define CMN_REALLOC_FUNCTION void* Reallocate(void* p, size_t sz)
#define CMN_FREE_FUNCTION    void  Free(void* p)
#define CMN_MALLOC_FUNCTION_NAMED(name)  void* name(size_t sz)
#define CMN_REALLOC_FUNCTION_NAMED(name) void* name(void* p, size_t sz)
#define CMN_FREE_FUNCTION_NAMED(name)    void  name(void* p)
namespace cmn { 
  typedef CMN_MALLOC_FUNCTION_NAMED(_cmn_malloc);
  typedef CMN_REALLOC_FUNCTION_NAMED(_cmn_realloc);
  typedef CMN_FREE_FUNCTION_NAMED(_cmn_free);
}