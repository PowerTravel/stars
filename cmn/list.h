#pragma once

#include "allocators.h"
#include "macros.h"
#include "utils.h"

namespace cmn{
// Note: Having allocators as function pointers messes up live code editing.
//       When building a new DLL the allocation functions will get new adressess 
//       ones that are stored here are invalid.
//       There are workarounds such as 
//          1: using the funciton pointer pool or
//          2: Collecting the allocator function pointers into heap-allocated objects.
//             The objects gets updated on DLL reload to point to the proper function and
//             those heap allocated objects are what these types use for custom allocators.
// Note: Never move list elements between lists which has different allocators

template  <typename T>
struct list {
  struct element {
    T* Data;
    element* Next;
    element* Previous;
    T* GetPtr() const {
      return Data;
    }
    T& GetRef() const {
      return *Data;
    };
    T GetCopy() const{
      return *Data;
    };
  };

  _cmn_malloc*  m_malloc;
  _cmn_free*    m_free;

  bool m_transient;

  void* Malloc(size_t sz)
  {
    if(m_malloc)
    {
      return m_malloc(sz);
    }
    
    return m_transient ? _g_cmn_transient_malloc(sz) : _g_cmn_malloc(sz);
  }

  void Free(void * p)
  {
    if(m_free){
      return m_free(p);
    }
    return m_transient ? _g_cmn_transient_free(p) : _g_cmn_free(p);
  }


  size_t m_count;
  element* m_sentinel;

  element* NewElement(const T& Data) {
    element* Result = (element*)Malloc(sizeof(element));
    Result->Data = (T*)Malloc(sizeof(T));
    utils::Copy(sizeof(T), (void*) &Data, (void*) Result->Data);
    m_count++;
    return Result;
  }

  size_t Size() const { return m_count; };

  list() = default;

  static inline list Create(_cmn_malloc* aMalloc = 0, _cmn_free* aFree = 0){
    list Result = {};
    Result.m_malloc = aMalloc;
    Result.m_free = aFree;
    Result.m_transient = false;
    return Result;
  }

  static inline list CreateTransient(){
    list Result = list::Create(0,0);
    Result.m_transient = false;
    return Result;
  }

  element* GetSentinel(){
    if(!m_sentinel){
      m_sentinel =  (element*) Malloc(sizeof(element));
      *m_sentinel = {};
      ListInitiate(m_sentinel);
    }
    return m_sentinel;
  }

  element* First(){
    return GetSentinel()->Next;
  }
  element* Last(){
    return GetSentinel()->Previous;
  }

  void InsertBefore(element* Position, const T& Data) {
    element* e = NewElement(Data);
    ListInsertBefore(Position, e);
  }

  void InsertAfter(element* Position, const T& Data) {
    element* e = NewElement(Data);
    ListInsertAfter(Position, e);
  }

  void PushBack(const T& Data) {
    InsertBefore(GetSentinel(), Data);
  }

  void PushFront(const T& Data) {
    InsertAfter(GetSentinel(), Data);
  }

  element* Detach(element* ElementToDetach) {
    if(m_count == 0 || IsEnd(ElementToDetach)){
      return 0;
    }
    ListRemove(ElementToDetach);
    ListInitiate(ElementToDetach);
    m_count--;
    return ElementToDetach;
  }

  void Delete(element* ElementToRemove) {
    if(m_count == 0 || IsEnd(ElementToRemove)){
      return;
    }
    ElementToRemove = Detach(ElementToRemove);
    Free(ElementToRemove->Data);
    Free(ElementToRemove);
  }

  void Delete() {

    if(m_sentinel)
    {
      element* e = m_sentinel->Next;
      while(e != m_sentinel)
      {
        element* eNext = e->Next;
        Free(e->Data);
        Free(e);
        e = eNext;
      }
      
      Free(m_sentinel);
      m_sentinel = 0;
      m_count = 0;
    }
  }

  T PopBack() {
    T Result = {};
    if(!Empty()){
      Result = Last()->GetCopy();
      Delete(Last());
    }
    return Result;
  }

  T PopFront() {
    T Result = {};
    if(!Empty()){
      Result = First()->GetCopy();
      Delete(First());
    }
    return Result;
  }

  element* At(size_t Index)
  {
    if(!m_sentinel) return 0;
    if(Index >= m_count) return 0;

    element* Result = 0;
    size_t Midpoint = m_count / 2;
    if(Index <= Midpoint)
    {
      Result = First();
      while(Index-- && Result != m_sentinel){
        Result = Result->Next;
      }
    }else{
      Index = m_count - Index - 1;
      Result = Last();
      while(Index-- &&Result != m_sentinel){
       Result = Result->Previous;
      }
    }
    if(Result == m_sentinel){
     Result = 0;
    }
    return Result;
  }


  T GetCopy(size_t Index){
    element* E = At(Index);
    return E->GetCopy();
  }
  T& GetRef(size_t Index){
    element* E = At(Index);
    return E->GetRef();
  }
  T* GetPtr(size_t Index){
    element* E = At(Index);
    return E->GetPtr();
  }

  bool IsEnd(element* Position){return !m_sentinel || Position == m_sentinel;};
  bool Empty(){return !m_sentinel || m_sentinel == m_sentinel->Next;}
  bool Initiated(){return m_sentinel;}

};

template struct list<int>;

}

