#pragma once
#if 1
#include "commons/types.h"
#include "commons/intrinsics.h"
#include "cmn/bucket_array.h"
#include "platform/jwin_platform.h"
#include "platform/jwin_debug.h"
#include "memory.h"
#include "string.h"

#include "cmn/vector.h"
#include "platform/platform_containers.h"

#pragma intrinsic(__rdtsc)

struct debug_state;
extern debug_state* GlobalDebugState;

struct DebugStateAllocators;
template<typename T> using list_dbg = cmn::list<T, DebugStateAllocators>;
template<typename T> using tree_dbg = cmn::n_tree<T, DebugStateAllocators, DebugStateAllocators>;
template<typename T> using b_array_dbg = cmn::bucket_array<T, DebugStateAllocators>;

file_local debug_state* DEBUGGetState();
#if 0
enum class function_sorting
{
  None,
  Ascending,
  Descending
};
#endif
struct debug_record_entry
{
  u32 LineNumber;
  char BlockName[256];
  u32 CycleCount;
  r32 HitCount;
  r32 HCCount;
};

struct debug_statistics
{
  r32 HitCount;
  r32 Min;
  r32 Max;
  r32 Tot;
  debug_record_entry* Record;
};

// The information for the frame
#define MAX_BLOCKS_PER_FRAME 16384
#define MAX_THREAD_COUNT 16
#define MAX_DEBUG_FRAME_COUNT 60
#define MAX_DEBUG_FUNCTION_COUNT 256

//#define MAX_DEBUG_EVENT_ARRAY_COUNT 120  // How many frames we are tracking
//#define MAX_DEBUG_TRANSLATION_UNITS (2)  // How many translation units we have
//#define MAX_DEBUG_EVENT_COUNT (8*65536)
//#define MAX_DEBUG_RECORD_COUNT (65536)

struct debug_block
{
  debug_record_entry* Record;
  u32 ThreadIndex;
  u64 BeginClock;
  u64 EndClock;
  debug_event OpeningEvent;
  debug_block* Parent;
  debug_block* FirstChild;
  debug_block* Next;

  //debug_event ClosingEvent;
};

struct debug_thread
{
  u32 ID;
  u32 LaneIndex;
  list_dbg<debug_block>::element* FirstBlock;
  list_dbg<debug_block>::element* OpenBlock;
  list_dbg<debug_block>::element* ClosedBlock;
  list_dbg<debug_block>::element* SelectedBlock;
};

struct debug_frame
{
  u64 BeginClock;
  u64 EndClock;
  r32 WallSecondsElapsed;

  u32 FrameBarLaneCount;

  midx MaxBlockCount;
  b_array_dbg<debug_block>   Blocks;
  b_array_dbg<debug_thread>  Threads;
  list_dbg<debug_statistics> Statistics;
};


struct debug_state
{
  b32 Initialized;

  //b32 Paused;

  memory_arena Arena;
  temporary_memory StatisticsTemp;

  // This is a rolling buffer that holds all data for all frames
  debug_frame* SelectedFrame;
  
  b32 ThreadSelected;
  u32 SelectedThreadIndex;
  
  list_dbg<debug_frame>::element* CurrentFrame;
  list_dbg<debug_frame> Frames;

  // Keeps a global record of all seen functions and their execution time and hit cout.
  list_dbg<debug_record_entry> FunctionList;

  // b32 Compiling;
  // debug_executing_process Compiler;
};

struct DebugStateAllocators
{
  CMN_MALLOC_FUNCTION {
    return PushSize(&GlobalDebugState->Arena, sz);
  };
  CMN_REALLOC_FUNCTION {
    Assert(0); // Don't wanna use this, should never be used
    return p;
  };
  CMN_FREE_FUNCTION {

  };
};


#endif