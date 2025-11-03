#pragma once

#include "allocators.h"
#include "macros.h"
#include "utils.h"
#include "vector.h"
#include "list.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

// Note: Never move nodes between trees which has different allocators

// Interaface
namespace cmn {

  template <typename T>
  struct n_tree {


    struct node {
      T* Data;
      node* Parent;
      node* NextSibling;
      node* PreviousSibling;
      node* FirstChild;
      size_t ChildCount;

      node() : Data(0), Parent(0), NextSibling(0), PreviousSibling(0), FirstChild(0){
        TreeNodeInitiate( this );
      }

      node* GetChild(int32_t Index)
      {
        node* Child = Parent->FirstChild;
        if(Index>0) {
          while(Index--) {
            Child = Child->NextSibling;
          }
        } else if(Index <0) {
          while(Index++){
            Child = Child->PreviousSibling;
          }
        }
        return Child;
      }

      node* GetSibling(int32_t Index)
      {
        if(!Parent){return 0;}
        return Parent->GetChild(Index);
      }

      size_t GetSiblingIndex() {
        if(!this->Parent) return 0;
        node* Child = this->Parent->FirstChild;
        size_t Result = 0;
        while(Child != this) {
          Assert(Result < this->Parent->ChildCount);
          Result++;
          Child = Child->NextSibling;
        }

        return Result;
      }

      void AddChild(node* Node)
      {
        if(this->FirstChild)
        {
          TreeNodeInsertBefore( this->FirstChild, Node ); 
        }else{
          this->FirstChild = Node;
        }
        Node->Parent = this;
        ChildCount++;
      }

      // Removes this node from the tree
      void Remove(){
        if(Parent)
        {
          TreeNodeRemove(this);
          Parent->ChildCount--;
        }
      }

      size_t Depth(){
        size_t Result = 0;
        node* Node = this;
        while(Node->Parent)
        {
          Node = Node->Parent;
          Result++;
        }
        return Result;
      }

    };

    _cmn_malloc*  m_malloc;
    _cmn_free*    m_free;

    node* m_root;

    n_tree() = default;
    
    static n_tree Create(_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_free* Free = _g_cmn_free)
    {
      n_tree Result{};
      Result.m_malloc = Malloc;
      Result.m_free = Free;
      return Result;
    }
    
    void Delete()
    {
      if(m_root){
        PostOrderTraversal(*this, DeleteLeafNode, 0);
      }
    }

  
    n_tree::node* AllocateNode() {
      n_tree::node* Result = new(m_malloc(sizeof(n_tree::node))) n_tree::node();
      return Result;
    }

    void* AllocateData(n_tree::node* Node, const T* Data)
    {
      size_t DataSize = sizeof(T);
      Node->Data = (T*) m_malloc(DataSize);
      utils::Copy(sizeof(T), Data, Node->Data);
      return Node->Data;
    }

    node* NewNode(node* Parent, const T* Data) {
      n_tree::node* Result = AllocateNode();
      if(Parent) {  
        Parent->AddChild(Result);
      }else{
        m_root = Result;
      }

      if(Data)
      {
        AllocateData(Result, Data);
      }
      return Result;
    }

    node* NewNode(node* Parent, const T& Data) {
      return NewNode(Parent, &Data);
    }

    size_t NodeCount();
  };

#define _NodeVisitFunction(name) void name(typename cmn::n_tree<T>* Tree, typename cmn::n_tree<T>::node* Node, void* UserData)
template<class T> using n_tree_node_callback = _NodeVisitFunction((*));
#define NodeVisitFunction(name) template<typename T> _NodeVisitFunction(name)

template<class T> using node_type = typename cmn::n_tree<T>::node;
template<class T> using node_list = cmn::list< node_type<T>* >;
template<typename T>

void PreOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData){
  if(!Tree.m_root) return;
  node_list<T> NodeQueue = node_list<T>();

  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    node_type<T>* Node = NodeQueue.PopBack();
    Callback(&Tree, Node, UserData);
    if(Node->FirstChild)
    {
      node_type<T>* Start = Node->FirstChild->PreviousSibling;
      node_type<T>* Child = Start;
      do
      {
        NodeQueue.PushBack(Child);
        Child = Child->PreviousSibling;
      }while(Child != Start);
    }
  }
}

template<typename T>
void LevelOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData) {
  if(!Tree.m_root) return;

  node_list<T> NodeQueue = node_list<T>();
  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    node_type<T>* Node = NodeQueue.PopFront();
    Callback(&Tree, Node, UserData);

    if(Node->FirstChild)
    {
      node_type<T>* Start = Node->FirstChild;
      node_type<T>* Child = Start;
      do{
        NodeQueue.PushBack(Child);
        Child = Child->NextSibling;
      }while(Start != Child);
    }
  }
}

namespace internal {
  template<typename T>
  struct post_order_traverse {
    node_type<T>* Node;
    bool Opened;
  };
  template<typename T>
  static void Push(cmn::list< post_order_traverse<T> >& Queue, node_type<T>* Node)
  {
    if(!Node) return;
    post_order_traverse<T> TraversePair = {};
    TraversePair.Node = Node;
    TraversePair.Opened = false;
    Queue.PushBack(TraversePair);
  }
} // internal

template<typename T>
void PostOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData)
{
  if(!Tree.m_root) return;
  cmn::list< internal::post_order_traverse<T> > NodeQueue = cmn::list<internal::post_order_traverse<T>>();
  internal::Push(NodeQueue, Tree.m_root);
  while(!NodeQueue.Empty())
  {
    internal::post_order_traverse<T>& TraversePair = NodeQueue.Last()->GetRef();
    node_type<T>* Node = TraversePair.Node;
    

    if(!TraversePair.Opened && Node->FirstChild)
    {
      node_type<T>* Start = Node->FirstChild->PreviousSibling;
      node_type<T>* Child = Start;
      do
      {
        internal::Push(NodeQueue, Child);
        Child = Child->PreviousSibling;
      }while(Child != Start);
      TraversePair.Opened = true;
    }else{
      NodeQueue.PopBack();
      Callback(&Tree, Node, UserData);
    }
  }
}

NodeVisitFunction(CountNodes){
  size_t* NodeCount = (size_t*) UserData;
  *NodeCount = *NodeCount + 1;
}

template <typename T>
size_t n_tree<T>::NodeCount(){
  size_t Result = 0;
  cmn::LevelOrderTraversal(*this, CountNodes,(void*) &Result);
  return Result;
};

NodeVisitFunction(DeleteLeafNode){
  //Tree, Node, UserData;
  Assert(!Node->FirstChild);
  Node->Remove();
  Tree->m_free(Node->Data);
  Tree->m_free(Node);
}


template struct n_tree<int>;
} // cmn
