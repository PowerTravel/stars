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
    
    void Delete();
  
    n_tree::node* AllocateNode() {
      n_tree::node* Result = new(m_malloc(sizeof(n_tree::node))) n_tree::node();
      return Result;
    }

    void* SetData(n_tree::node* Node, const T* Data)
    {
      if(!Node->Data)
      {
        Node->Data = (T*) m_malloc(sizeof(T));
      }
      utils::Copy(sizeof(T), Data, Node->Data);
      return Node->Data;
    }

    node* NewNode(node* Parent = 0, const T* Data = 0) {
      n_tree::node* Result = AllocateNode();
      if(Parent) {  
        Parent->AddChild(Result);
      }else{
        m_root = Result;
      }

      if(Data)
      {
        SetData(Result, Data);
      }
      return Result;
    }

    node* NewNode(node* Parent, const T& Data) {
      return NewNode(Parent, &Data);
    }

    size_t NodeCount();

    // Create a copy of Tree
    cmn::list<node*>   GetLevelOrderList  (_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_free* Free = _g_cmn_free);
    cmn::vector<node*> GetLevelOrderVector(_cmn_malloc* Malloc = _g_cmn_malloc, _cmn_realloc* Realloc = _g_cmn_realloc, _cmn_free* Free = _g_cmn_free);
    n_tree<T> Copy(_cmn_malloc* Malloc = 0, _cmn_free* Free = 0); // If 0 the allocators of 'this' are used
  };

#define _NodeVisitFunction(name) void name(typename cmn::n_tree<T>* Tree, typename cmn::n_tree<T>::node* Node, void* UserData)
template<class T> using n_tree_node_callback = _NodeVisitFunction((*));
#define NodeVisitFunction(name) template<typename T> _NodeVisitFunction(name)

template<class T> using n_tree_node = typename cmn::n_tree<T>::node;
template<class T> using node_list = cmn::list< n_tree_node<T>* >;
template<class T> using node_vec = cmn::vector< n_tree_node<T>* >;
template<typename T>

void PreOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData){
  if(!Tree.m_root) return;
  node_list<T> NodeQueue = node_list<T>::Create(_g_cmn_transient_malloc, _g_cmn_transient_free);

  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    n_tree_node<T>* Node = NodeQueue.PopBack();
    Callback(&Tree, Node, UserData);
    if(Node->FirstChild)
    {
      n_tree_node<T>* Start = Node->FirstChild->PreviousSibling;
      n_tree_node<T>* Child = Start;
      do
      {
        NodeQueue.PushBack(Child);
        Child = Child->PreviousSibling;
      }while(Child != Start);
    }
  }
  NodeQueue.Delete();
}

template<typename T>
void LevelOrderTraversal(cmn::n_tree<T>& Tree, n_tree_node_callback<T> Callback, void* UserData) {
  if(!Tree.m_root) return;

  node_list<T> NodeQueue = node_list<T>::Create(_g_cmn_transient_malloc, _g_cmn_transient_free);
  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    n_tree_node<T>* Node = NodeQueue.PopFront();
    Callback(&Tree, Node, UserData);

    if(Node->FirstChild)
    {
      n_tree_node<T>* Start = Node->FirstChild;
      n_tree_node<T>* Child = Start;
      do{
        NodeQueue.PushBack(Child);
        Child = Child->NextSibling;
      }while(Start != Child);
    }
  }
  NodeQueue.Delete();
}

namespace internal {
  template<typename T>
  struct post_order_traverse {
    n_tree_node<T>* Node;
    bool Opened;
  };
  template<typename T>
  static void Push(cmn::list< post_order_traverse<T> >& Queue, n_tree_node<T>* Node)
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
  cmn::list< internal::post_order_traverse<T> > NodeQueue = cmn::list<internal::post_order_traverse<T>>::Create();
  internal::Push(NodeQueue, Tree.m_root);
  while(!NodeQueue.Empty())
  {
    internal::post_order_traverse<T>& TraversePair = NodeQueue.Last()->GetRef();
    n_tree_node<T>* Node = TraversePair.Node;
    

    if(!TraversePair.Opened && Node->FirstChild)
    {
      n_tree_node<T>* Start = Node->FirstChild->PreviousSibling;
      n_tree_node<T>* Child = Start;
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
  NodeQueue.Delete();
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

template <typename T>
void n_tree<T>::Delete()
{
  if(m_root){
    PostOrderTraversal(*this, DeleteLeafNode, 0);
  }
}

NodeVisitFunction(LevelOrderNodeList){
  //Tree, Node, UserData;
  node_list<T>* NodeList = (node_list<T>*) UserData;
  NodeList->PushBack(Node);
}

template <typename T>
node_list<T> n_tree<T>::GetLevelOrderList(_cmn_malloc* Malloc, _cmn_free* Free) {
  node_list<T> NodeList = node_list<T>::Create(Malloc, Free);
  LevelOrderTraversal(*this, LevelOrderNodeList, (void*) &NodeList);
  return NodeList;
}

NodeVisitFunction(LevelOrderNodeVec){
  //Tree, Node, UserData;
  node_vec<T>* NodeList = (node_vec<T>*) UserData;
  NodeList->PushBack(Node);
}

template <typename T>
node_vec<T> n_tree<T>::GetLevelOrderVector(_cmn_malloc* Malloc, _cmn_realloc* Realloc, _cmn_free* Free) {
  size_t NodeCount = this->NodeCount();
  node_vec<T> NodeVec = node_vec<T>::Create(NodeCount, Malloc, Realloc, Free);
  LevelOrderTraversal(*this, LevelOrderNodeVec, (void*) &NodeVec);

  return NodeVec;
}

template <typename T>
struct copy_n_tree_help_struct {
  
  template <typename T> 
  struct pair {
    n_tree_node<T>* SrcNode;
    n_tree_node<T>* DstNode;
  };

  cmn::vector<pair<T>> NodePair;
  n_tree<T>* DstTree;

  copy_n_tree_help_struct(n_tree<T>* aDstTree, size_t aSize)
  {
    NodePair = cmn::vector<pair<T>>::CreateTransient(aSize);
    DstTree = aDstTree;
  }
  ~copy_n_tree_help_struct(){
    NodePair.Delete();
    DstTree = 0;
  }

  inline uint32_t GetHash(void* Node)
  {
    char* C = (char*) Node;
    int len = sizeof(Node);
    return utils::SuperFastHash(C, len);
  }

  n_tree_node<T>* GetDstParent(n_tree_node<T>* SrcNode) {

    size_t ReservedSize = NodePair.Reserved();
    n_tree_node<T>* SrcParent = SrcNode->Parent;
    uint32_t DirectHash = GetHash((void*)SrcParent);
    uint32_t Hash = DirectHash % ReservedSize;
    size_t LoopIndex = 0;
    while(NodePair[Hash].SrcNode != SrcParent){
      Hash = (Hash+1) % ReservedSize;
      LoopIndex++;  
      Assert(LoopIndex < ReservedSize);
    }
    
    pair<T>* Pair = &NodePair[Hash];
    Assert(Pair->SrcNode == SrcParent);
    return Pair->DstNode;
  }

  void PushNewPair(n_tree_node<T>* SrcNode, n_tree_node<T>* DstNode)
  {
    size_t ReservedSize = NodePair.Reserved();
    uint32_t DirectHash = GetHash((void*)SrcNode);
    uint32_t Hash = DirectHash % ReservedSize;

    pair<T>* Pair = &NodePair[Hash];
    size_t LoopIndex = 0;
    while(Pair->SrcNode)
    {
      Hash = (Hash+1)%ReservedSize;
      Pair = &NodePair[Hash];
      LoopIndex++;
      Assert(LoopIndex < ReservedSize);
    }
    Assert(!Pair->SrcNode && !Pair->DstNode);

    Pair->SrcNode = SrcNode;
    Pair->DstNode = DstNode;
  }
};


//Tree, Node, UserData;
NodeVisitFunction(CopyTreeFun){
  copy_n_tree_help_struct<T>* NodeMap = (copy_n_tree_help_struct<T>*) UserData;
  n_tree_node<T>* NewParent = NodeMap->GetDstParent(Node);
  n_tree_node<T>* NewNode = NodeMap->DstTree->NewNode(NewParent, *Node->Data);
  NodeMap->PushNewPair(Node,NewNode);
}

template <typename T>
n_tree<T> n_tree<T>::Copy(_cmn_malloc* aMalloc, _cmn_free* aFree) {
  // Both or None of the allocators need to be defined in this function
  Assert((aMalloc==0 && aFree==0) || (aMalloc!=0 && aFree!=0));

  _cmn_malloc* Malloc = aMalloc ? aMalloc : this->m_malloc;
  _cmn_free* Free = aFree ? aFree : this->m_free;
  n_tree<T> Result = n_tree<T>::Create(Malloc, Free);
  
  size_t nodeCount = NodeCount();
  size_t VecSize = utils::GetNextPowerOfTwo(nodeCount);
  copy_n_tree_help_struct<T> HelpStruct = copy_n_tree_help_struct<T>(&Result, VecSize);

  LevelOrderTraversal(*this, CopyTreeFun, &HelpStruct); 

  return Result;
}

template struct n_tree<int>;
} // cmn
