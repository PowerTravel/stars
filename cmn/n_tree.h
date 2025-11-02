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
      node(_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_free* Free = _g_cmn_free) : Parent(0), Data(0), Children(Malloc,Free){

      }
      size_t ChildCount(){
        return Children.Size();
      }
    };

    struct navigator {
      n_tree<T>* m_tree;
      n_tree<T>::node* m_node;
      int32_t m_depth; // why not
      int32_t m_siblingIndex;

      size_t ChildCount() const {return m_node->ChildCount();};       // Number of children
      bool IsLeaf() const {return m_node->ChildCount()==0;};       // Is the node a leaf (same as ChildCount =`= 0)
      uint32_t SiblingCount() const { // Number of Siblings including m_node
        if(m_depth == 0) {return 0;} // Root
        return m_node->Parent->ChildCount();
      }
      uint32_t SiblingIndex() const {return m_siblingIndex;};     // Number of Current Sibling
      uint32_t Depth() const {return m_depth;};            // Get the depth
      bool MoveToParent() // Move to parent
      {
        if(m_depth == 0) return false;
        m_node = m_node->Parent;
        m_depth--;
        m_siblingIndex = GetSiblingIndex(m_node);
        return true;
      };                     
      bool MoveToChild(uint32_t ChildIndex = 0)  // Move to Child
      {
        if(ChildIndex >= ChildCount()) return false;
        m_node = GetChildNode(m_node, ChildIndex);
        m_depth = m_depth+1;
        m_siblingIndex = ChildIndex;
        return true;
      }
      bool NextSibling(int32_t NextCount){
        if(NextCount==0) return false;
        int32_t NewSiblingIndex = m_siblingIndex + NextCount;
        if(NewSiblingIndex < 0 || NewSiblingIndex >= SiblingCount()) return false;
        list<node*>* Siblings = &m_node->Parent->Children;
        m_node = Siblings->At(NewSiblingIndex)->GetCopy();
        m_siblingIndex = NewSiblingIndex;
        return true;
      }
      bool NextSibling(){
        return NextSibling(1);
      } 
      bool PreviousSibling(){
        return NextSibling(-1);
      }
      bool MoveToSibling(uint32_t SiblingIndex){
        return NextSibling(SiblingIndex - m_siblingIndex);
      };

      T GetCopy() const {return *m_node->Data;}; // Get Node Value As Copy
      T& GetRef() const {return *m_node->Data;};  // Get Node Value As Ref
      T* GetPtr() const {return m_node->Data;};  // Get Node Value As Ptr
      n_tree<T>::node& GetNodeRef(){return *m_node;}; // Get The Node 
      n_tree<T>::node* GetNode(){return m_node;}; // Get The Node 

      // Tree modifications

      // Removes the sub-tree from the current tree and returns it as a new tree
      // The navigator is now in the old tree but on the parent of the dissconnected node.
      n_tree<T> Dissconnect(){
        /*
        n_tree<T>* m_tree;
        n_tree<T>::node* m_node;
        int32_t m_depth; // why not
        int32_t m_siblingIndex;
        */
        if(m_depth == 0) *m_tree;

        // Create a new tree
        n_tree<T> Result = n_tree<T>(m_tree->m_malloc, m_tree->m_free);

        node_list<T>& Siblings = m_node->Parent->Children;
        node_list_element<T>* NodeElementToTransfer = Siblings.At(m_siblingIndex);

        // Remove the node
        NodeElementToTransfer = Siblings.Detach(NodeElementToTransfer);

        // Copy over the node pointer
        Result.m_root = NodeElementToTransfer->GetCopy();
        Siblings.Delete(NodeElementToTransfer);        

        return Result;

      }
      // Attaches the tree in the argument as a child to the Navigator
      // The navigator remains unchanged
      void Attach(n_tree<T>* Tree){};

      // Deletes the node and its subtree then moves to a sibling.
      // If there are no more siblings, moves to parent. 
      // Returns false if the tree is empty (cannot delete anymore)
      bool Delete(){
        return false;
      };

      // Inserts a new node into the tree as a child and return it
      node* InsertChild(const T& Value){ 
        node* Result =m_tree->NewNode(m_node, Value);
        return Result;
      };

      // Inserts a new node into the tree as a Sibling
      node* InsertSibling(const T& Value){
        if(m_depth == 0) return 0;
        node* Result = m_tree->NewNode(m_node->Parent, Value);
        m_siblingIndex = GetSiblingIndex(m_node);
        return Result;
      };
    };

    _cmn_malloc*  m_malloc;
    _cmn_free*    m_free;

    node* m_root;
    list<node*> m_nodeList;

    n_tree(_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_free* Free = _g_cmn_free) : m_malloc(Malloc), m_free(Free), m_root(0), m_nodeList(Malloc,Free)
    {

    }

    n_tree::node* AllocateNode() {
      n_tree::node* Result = new(m_malloc(sizeof(n_tree::node))) n_tree::node(m_malloc, m_free);
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

    navigator NewNavigator(n_tree<T>::node* Node = 0) {
      navigator Result = {};

      if(Node == 0)
      {
        // Start At Root
        Result.m_tree = this;
        Result.m_node = m_root;
        Result.m_depth = 0;
        Result.m_siblingIndex = 0;
      }else{
        // Start At Node
        Result.m_tree = this;
        Result.m_node = Node;
        Result.m_depth = GetDepth(Node);
        Result.m_siblingIndex = GetSiblingIndex(Node);
      }
      return Result;
    }

    static size_t GetDepth(n_tree<T>::node* Node){
      size_t Result = 0;
      while(Node->Parent)
      {
        Node = Node->Parent;
        Result++;
      }
      return Result;
    }
    static size_t GetSiblingIndex(n_tree<T>::node* Node){
      if(!Node->Parent) return 0;
      n_tree<T>::node* Parent = Node->Parent;
      list<node*>& ChildList = Parent->Children;
      list<node*>::element* ChildElement = ChildList.First();
      size_t Result = 0;
      while(!ChildList.IsEnd(ChildElement)){
        if(ChildElement->GetCopy() == Node){
          return Result;
        }
        Result++;;
        ChildElement = ChildElement->Next;
      }
      return Result;
    }
    static n_tree<T>::node* GetChildNode(n_tree<T>::node* Node, uint32_t ChildIndex)
    {
      if(Node->ChildCount() == 0 || Node->ChildCount() < ChildIndex  ) return 0;
      return Node->Children.At(ChildIndex)->GetCopy();
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
