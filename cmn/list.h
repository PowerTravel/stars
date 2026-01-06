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


struct basic_list {
  struct element {
    void* Data;
    basic_list::element* Next;
    basic_list::element* Previous;
  };

  size_t m_dataSize;
  size_t m_count;
  element* m_sentinel;

  inline bool IsEnd(basic_list::element* Position){return !m_sentinel || Position == m_sentinel;};
  inline bool Empty(){return !m_sentinel || m_sentinel == m_sentinel->Next;}
  inline bool Initiated(){return m_sentinel;}
  inline size_t Size() const { return m_count; };
  inline void* Get(basic_list::element* Element){ return Element->Data; };
  virtual basic_list::element* First(){return m_sentinel ? m_sentinel->Next : 0;}
  virtual basic_list::element* Last() {return m_sentinel ? m_sentinel->Previous : 0;}

  void InsertAt(basic_list::element* Position, const void* Data) {
    Assert(Position->Next && Position->Previous && Position->Data);
    utils::Copy(m_dataSize, Data, Position->Data);
  }

};

template <typename T, _AllocatorParams(My)>
struct list : public basic_list {

  inline T* GetPtr(basic_list::element* Element) const {
    T* Result = (T*) Element->Data;
    return Result;
  }
  inline T& GetRef(basic_list::element* Element) const {
    T* Result = (T*) Element->Data;
    return *Result;
  };
  inline T GetCopy(basic_list::element* Element) const{
    T* Result = ( (T*) Element->Data);
    return *Result;
  };

  basic_list::element* NewElement(const T* Data = 0) {
    basic_list::element* Result = (basic_list::element*) MyAllocate(sizeof(basic_list::element));
    Result->Data = MyAllocate(sizeof(T));
    if(Data){
      utils::Copy(sizeof(T), (void*) Data, (void*) Result->Data);
    }
    m_count++;
    return Result;
  }

  list() = default;

  basic_list ToBasic()
  {
    return *this;
  }

  static inline list Create(size_t InitialCount = 0){
    list Result = {};
    Result.m_dataSize = sizeof(T);
    if(InitialCount)
    {
      Result.m_sentinel = Result.GetSentinel();
      for (int i = 0; i < InitialCount; ++i)
      {
        basic_list::element* NewElement = Result.NewElement();
        ListInsertBefore(Result.m_sentinel, NewElement);
      }
    }
    return Result;
  }

  basic_list::element* GetSentinel(){
    if(!m_sentinel){
      m_sentinel =  (basic_list::element*) MyAllocate(sizeof(basic_list::element));
      *m_sentinel = {};
      ListInitiate(m_sentinel);
    }
    return m_sentinel;
  }

  basic_list::element* First() override {return GetSentinel()->Next;}
  basic_list::element* Last()  override {return GetSentinel()->Previous;}

  void InsertBefore(basic_list::element* Position, const T& Data) {
    basic_list::element* e = NewElement(&Data);
    ListInsertBefore(Position, e);
  }

  void InsertAfter(basic_list::element* Position, const T& Data) {
    basic_list::element* e = NewElement(&Data);
    ListInsertAfter(Position, e);
  }

  void PushBack(const T& Data) {
    InsertBefore(GetSentinel(), Data);
  }

  void PushFront(const T& Data) {
    InsertAfter(GetSentinel(), Data);
  }

  basic_list::element* Detach(basic_list::element* ElementToDetach) {
    if(m_count == 0 || IsEnd(ElementToDetach)){
      return 0;
    }
    ListRemove(ElementToDetach);
    ListInitiate(ElementToDetach);
    m_count--;
    return ElementToDetach;
  }

  void Delete(basic_list::element* ElementToRemove) {
    if(m_count == 0 || IsEnd(ElementToRemove)){
      return;
    }
    ElementToRemove = Detach(ElementToRemove);
    MyFree(ElementToRemove->Data);
    MyFree(ElementToRemove);
  }

  void Delete() {

    if(m_sentinel)
    {
      basic_list::element* e = m_sentinel->Next;
      while(e != m_sentinel)
      {
        basic_list::element* eNext = e->Next;
        MyFree(e->Data);
        MyFree(e);
        e = eNext;
      }
      
      MyFree(m_sentinel);
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

  basic_list::element* At(size_t Index)
  {
    if(!m_sentinel) return 0;
    if(Index >= m_count) return 0;

    basic_list::element* Result = 0;
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
    basic_list::element* E = At(Index);
    return GetCopy(E);
  }
  T& GetRef(size_t Index){
    basic_list::element* E = At(Index);
    return GetRef(E);
  }
  T* GetPtr(size_t Index){
    basic_list::element* E = At(Index);
    return GetPtr(E);
  }



};

}

