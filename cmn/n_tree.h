#pragma once

#include "allocators.h"
#include "macros.h"
#include "utils.h"
#include "list.h"
#include "vector.h"

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
      list<node*> Children;
      size_t ChildCount(){
        return Children.Size();
      }
    };

    struct navigator {
      n_tree<T>* m_tree;
      n_tree<T>::node* m_node;
      uint32_t m_depth; // why not
      uint32_t m_siblingIndex;

      uint32_t ChildCount(){return 0;};       // Number of children
      uint32_t SiblingCount(){return 0;};     // Number of Siblings
      uint32_t SiblingIndex(){return m_siblingIndex;};     // Number of Current Sibling
      uint32_t Depth(){return m_depth;};            // Get the depth
      navigator Parent(){return {};};                     // Move to parent
      navigator Child(uint32_t ChildIndex){return {};}     // Move to Child
      navigator Sibling(uint32_t SiblingIndex){return {};}; // Move to Sibling
      T GetCopy(){return {};}; // Get Node Value As Copy
      //T& GetRef(){return {};};  // Get Node Value As Ref
      T* GetPtr(){return 0;};  // Get Node Value As Ptr
      //n_tree<T>::node& GetNodeRef(); // Get The Node 
      n_tree<T>::node* GetNodePtr(){return {};}; // Get The Node 
      bool IsLeaf(){return ChildCount()==0;};  // Is the node a leaf (same as ChildCount =`= 0)
    };

    _cmn_malloc*  m_malloc;
    _cmn_free*    m_free;

    node* m_root;
    list<node*> m_nodeList;

    n_tree(_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_free* Free = _g_cmn_free) : m_malloc(Malloc), m_free(Free), m_root(0), m_nodeList(Malloc,Free)
    {

    }

    n_tree::node* AllocateNode() {
      n_tree::node* Result = (n_tree::node*) m_malloc(sizeof(n_tree::node));
      *Result = {};
      Result->Children = list<node*>(m_malloc, m_free);
      m_nodeList.PushBack(Result);
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
        Result->Parent = Parent;
        Parent->Children.PushBack(Result);
      }else{
        Assert(!m_root);
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

    size_t NodeCount() const {
      return m_nodeList.Size();
    }

    navigator Start() {
      return {};
    }

  };

#define _NodeVisitFunction(name) void name(typename cmn::n_tree<T>* Tree, typename cmn::n_tree<T>::node* Node, void* UserData)
template<class T> using n_tree_node_callback = _NodeVisitFunction((*));
#define NodeVisitFunction(name) template<typename T> _NodeVisitFunction(name)

template<class T> using node_type = typename cmn::n_tree<T>::node;
template<class T> using node_vec = cmn::vector< node_type<T>* >;
template<class T> using node_list = cmn::list< node_type<T>* >;
template<class T> using node_list_element = typename cmn::list<node_type<T>*>::element;
template<typename T>
void PreOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData){

  node_vec<T> NodeQueue = node_vec<T>(Tree.NodeCount());

  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    node_type<T>* Node = NodeQueue.PopBack();

    Callback(&Tree, Node, UserData);

    node_list<T>* ChildList = &Node->Children;
    node_list<T>::element* ChildElement = ChildList->Last();
    while(!ChildList->IsEnd(ChildElement))
    {
      NodeQueue.PushBack(ChildElement->GetCopy());
      ChildElement = ChildElement->Previous;
    }
  }
}

template<typename T>
void LevelOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData) {

  node_list<T> NodeQueue = node_list<T>();
  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    node_type<T>* Node = NodeQueue.PopFront();
    Callback(&Tree, Node, UserData);
    node_list<T>* ChildList = &Node->Children;
    node_list<T>::element* ChildElement = ChildList->First();
    while(!ChildList->IsEnd(ChildElement))
    {
      NodeQueue.PushBack(ChildElement->GetCopy());
      ChildElement = ChildElement->Next;
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
  static void Push(cmn::vector< post_order_traverse<T> >& Queue, node_type<T>* Node)
  {
    post_order_traverse<T> TraversePair = {};
    TraversePair.Node = Node;
    TraversePair.Opened = false;
    Queue.PushBack(TraversePair);
  }
} // internal

template<typename T>
void PostOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData)
{
  cmn::vector< internal::post_order_traverse<T> > NodeQueue = cmn::vector<internal::post_order_traverse<T>>(Tree.NodeCount());
  internal::Push(NodeQueue, Tree.m_root);
  while(!NodeQueue.Empty())
  {
    internal::post_order_traverse<T>& TraversePair = NodeQueue.Back();
    node_type<T>* Node = TraversePair.Node;
    node_list<T>& ChildList = Node->Children;
    if(!TraversePair.Opened && !ChildList.Empty())
    {
      node_list_element<T>* ChildElement = ChildList.Last();
      while(!Node->Children.IsEnd(ChildElement)){
        Push(NodeQueue, ChildElement->GetCopy());
        ChildElement = ChildElement->Previous;
      }
      TraversePair.Opened = true;
    }else{
      NodeQueue.PopBack();
      Callback(&Tree, Node, UserData);
    }
  }
}

template struct n_tree<int>;
} // cmn
