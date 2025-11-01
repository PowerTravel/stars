#pragma once

#include "allocators.h"
#include "macros.h"
#include "utils.h"

namespace cmn{

// Note: Never move list elements between lists which has different allocators

template  <typename T>
struct list {
  struct element {
    T* Data;
    element* Next;
    element* Previous;
    T* GetPtr(){
      return Data;
    }
    T& GetRef(){
      return *Data;
    };
    T GetCopy(){
      return *Data;
    };
  };

  _cmn_malloc*  m_malloc;
  _cmn_free*    m_free;

  size_t m_count;
  element* m_sentinel;

  element* NewElement(const T& Data) {
    element* Result = (element*)m_malloc(sizeof(element));
    Result->Data = (T*)m_malloc(sizeof(T));
    utils::Copy(sizeof(T), (void*) &Data, (void*) Result->Data);
    m_count++;
    return Result;
  }

  size_t Size() const { return m_count; };

  list(_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_free* Free = _g_cmn_free) : m_malloc(Malloc), m_free(Free), m_count(0), m_sentinel(0)
  {

  };

  list(const list& List) : list(List.m_malloc, List.m_free) {
    element* Src = List.m_sentinel->Next;
    while(Src != List.m_sentinel)
    {
      Assert(Src->Data);
      PushBack(*Src->Data);
      Src = Src->Next;
    }
    Assert(m_count == List.m_count);
  };

  ~list()  {
    if(m_sentinel)
    {
      element* e = m_sentinel->Next;
      while(e != m_sentinel)
      {
        element* eNext = e->Next;
        m_free(e->Data);
        m_free(e);
        e = eNext;
      }
      m_free(m_sentinel);
      m_sentinel = 0;
      m_count = 0;
    }
  }

  element* GetSentinel(){
    if(!m_sentinel){
      m_sentinel =  (element*) m_malloc(sizeof(element));
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

  void Delete(element* ElementToRemove){
    if(m_count == 0 || IsEnd(ElementToRemove)){
      return;
    }

    ListRemove(ElementToRemove);
    m_free(ElementToRemove->Data);
    m_free(ElementToRemove);
    m_count--;
  }

  T PopBack() {
    T Result = {};
    if(!Empty()){
      Result = *Last()->Data;
      Delete(Last());
    }
    return Result;
  }

  T PopFront() {
    T Result = {};
    if(!Empty()){
      Result = *First()->Data;
      Delete(First());
    }
    return Result;
  }

  // m_count = 5
  // Midpoint = 5/2 = 2
  // index = 0

  // 0 -> 0
  // 1 -> 0, 1
  // 1 -> 0, 1

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

  bool IsEnd(element* Position){return !m_sentinel || Position == m_sentinel;};
  bool Empty(){return !m_sentinel || m_sentinel == m_sentinel->Next;}

};

template struct list<int>;

}

