#pragma once

#define CMN_MALLOC_FUNCTION  void* Allocate(size_t sz)
#define CMN_REALLOC_FUNCTION void* Reallocate(void* p, size_t sz)
#define CMN_FREE_FUNCTION    void  Free(void* p)
