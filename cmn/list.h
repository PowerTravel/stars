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
// Note: Never move list list_elements between lists which has different allocators

struct list_element {
  void* Data;
  list_element* Next;
  list_element* Previous;
};


template <typename T, _cmn_malloc* Allocate, _cmn_free* Free>
struct list {

  inline T* GetPtr(list_element* Element) const {
    T* Result = (T*) Element->Data;
    return Result;
  }
  inline T& GetRef(list_element* Element) const {
    T* Result = (T*) Element->Data;
    return *Result;
  };
  inline T GetCopy(list_element* Element) const{
    T* Result = ( (T*) Element->Data);
    return *Result;
  };

  size_t m_count;
  list_element* m_sentinel;

  list_element* NewElement(const T* Data = 0) {
    list_element* Result = (list_element*) Allocate(sizeof(list_element));
    Result->Data = (T*) Allocate(sizeof(T));
    if(Data){
      utils::Copy(sizeof(T), (void*) Data, (void*) Result->Data);
    }
    m_count++;
    return Result;
  }

  size_t Size() const { return m_count; };

  list() = default;

  static inline list Create(size_t InitialCount = 0){
    list Result = {};
    if(InitialCount)
    {
      Result.m_sentinel = Result.GetSentinel();
      for (int i = 0; i < InitialCount; ++i)
      {
        list_element* NewElement = Result.NewElement();
        ListInsertBefore(Result.m_sentinel, NewElement);
      }
    }
    return Result;
  }

  list_element* GetSentinel(){
    if(!m_sentinel){
      m_sentinel =  (list_element*) Allocate(sizeof(list_element));
      *m_sentinel = {};
      ListInitiate(m_sentinel);
    }
    return m_sentinel;
  }

  list_element* First(){
    return GetSentinel()->Next;
  }
  list_element* Last(){
    return GetSentinel()->Previous;
  }

  void InsertBefore(list_element* Position, const T& Data) {
    list_element* e = NewElement(&Data);
    ListInsertBefore(Position, e);
  }

  void InsertAfter(list_element* Position, const T& Data) {
    list_element* e = NewElement(&Data);
    ListInsertAfter(Position, e);
  }

  void InsertAt(list_element* Position, const T& Data) {
    Assert(Position->Next && Position->Previous);
    utils::Copy(sizeof(T), (void*) &Data, (void*) Position->Data);
  }

  void PushBack(const T& Data) {
    InsertBefore(GetSentinel(), Data);
  }

  void PushFront(const T& Data) {
    InsertAfter(GetSentinel(), Data);
  }

  list_element* Detach(list_element* ElementToDetach) {
    if(m_count == 0 || IsEnd(ElementToDetach)){
      return 0;
    }
    ListRemove(ElementToDetach);
    ListInitiate(ElementToDetach);
    m_count--;
    return ElementToDetach;
  }

  void Delete(list_element* ElementToRemove) {
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
      list_element* e = m_sentinel->Next;
      while(e != m_sentinel)
      {
        list_element* eNext = e->Next;
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

  list_element* At(size_t Index)
  {
    if(!m_sentinel) return 0;
    if(Index >= m_count) return 0;

    list_element* Result = 0;
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
    list_element* E = At(Index);
    return GetCopy(E);
  }
  T& GetRef(size_t Index){
    list_element* E = At(Index);
    return GetRef(E);
  }
  T* GetPtr(size_t Index){
    list_element* E = At(Index);
    return GetPtr(E);
  }

  bool IsEnd(list_element* Position){return !m_sentinel || Position == m_sentinel;};
  bool Empty(){return !m_sentinel || m_sentinel == m_sentinel->Next;}
  bool Initiated(){return m_sentinel;}

};

}

