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
// Note: Never move list basic_list::elements between lists which has different allocators

template <typename T, typename Allocators>
struct list {

  struct element {
    T* Data;
    element* Next;
    element* Previous;
  };

  size_t m_count;
  element* m_sentinel;

  inline bool IsEnd(element* Position){return !m_sentinel || Position == m_sentinel;};
  inline bool Empty(){return !m_sentinel || m_sentinel == m_sentinel->Next;}
  inline bool Initiated(){return m_sentinel;}
  inline size_t Size() const { return m_count; };
  inline void* Get(element* Element){ return Element->Data; };

  void InsertAt(element* Position, const void* Data) {
    Assert(Position->Next && Position->Previous && Position->Data);
    utils::Copy(sizeof(T), Data, Position->Data);
  }

  inline T* GetPtr(element* Element) const {
    T* Result = Element->Data;
    return Result;
  }
  inline T& GetRef(element* Element) const {
    T* Result = Element->Data;
    return *Result;
  };
  inline T GetCopy(element* Element) const{
    T* Result = Element->Data;
    return *Result;
  };

  element* NewElement(const T* Data = 0) {
    element* Result = (element*) Allocators::Allocate(sizeof(element));
    Result->Data = (T*) Allocators::Allocate(sizeof(T));
    if(Data){
      utils::Copy(sizeof(T), (void*) Data, (void*) Result->Data);
    }
    m_count++;
    return Result;
  }

  list() = default;

  static inline list Create(){
    list Result = {};
    return Result;
  }

  element* GetSentinel(){
    if(!m_sentinel){
      m_sentinel =  (element*) Allocators::Allocate(sizeof(element));
      *m_sentinel = {};
      ListInitiate(m_sentinel);
    }
    return m_sentinel;
  }

  element* First() {return GetSentinel()->Next;}
  element* Last()  {return GetSentinel()->Previous;}

  void InsertBefore(element* Position, const T& Data) {
    element* e = NewElement(&Data);
    ListInsertBefore(Position, e);
  }

  void InsertAfter(element* Position, const T& Data) {
    element* e = NewElement(&Data);
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
    Allocators::Free(ElementToRemove->Data);
    Allocators::Free(ElementToRemove);
  }

  void Delete() {

    if(m_sentinel)
    {
      element* e = m_sentinel->Next;
      while(e != m_sentinel)
      {
        element* eNext = e->Next;
        Allocators::Free(e->Data);
        Allocators::Free(e);
        e = eNext;
      }
      
      Allocators::Free(m_sentinel);
      m_sentinel = 0;
      m_count = 0;
    }
  }

  T PopBack() {
    T Result = {};
    if(!Empty()){
      Result = GetCopy(Last());
      Delete(Last());
    }
    return Result;
  }

  T PopFront() {
    T Result = {};
    if(!Empty()){
      Result = GetCopy(First());
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
    return GetCopy(E);
  }
  T& GetRef(size_t Index){
    element* E = At(Index);
    return GetRef(E);
  }
  T* GetPtr(size_t Index){
    element* E = At(Index);
    return GetPtr(E);
  }
};

}

