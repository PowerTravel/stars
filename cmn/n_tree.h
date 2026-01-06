#pragma once

#include "allocators.h"
#include "macros.h"
#include "utils.h"
#include "vector.h"
#include "list.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

#define _TreeAllocatorParams(Prefix) _AllocatorParams(Prefix), _AllocatorParams(Prefix##Temp)
#define _TreeAllocatorArgs(Prefix)   _AllocatorArgs(Prefix), _AllocatorArgs(Prefix##Temp)

// Interaface
namespace cmn {

  template <typename T, _TreeAllocatorParams(My)>
  struct n_tree {
    struct node {
      T* Data;
      node* Parent;
      node* NextSibling;
      node* PreviousSibling;
      node* FirstChild;
      size_t ChildCount;
      size_t Depth;

      node() : Data(0), Parent(0), NextSibling(0), PreviousSibling(0), FirstChild(0), ChildCount(0), Depth(0){
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

      void AddChild(node* NewChild)
      {
        if(this->FirstChild)
        {
          TreeNodeInsertBefore( this->FirstChild, NewChild );
        }else{
          this->FirstChild = NewChild;
        }
        NewChild->Parent = this;
        NewChild->Depth = NewChild->Parent->Depth + 1;
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

      size_t CalculateDepth(){
        size_t Result = 0;
        node* Node = this;
        while(Node->Parent)
        {
          Node = Node->Parent;
          Result++;
        }
        Depth = Result;
        return Result;
      }

    };

    node* m_root;
    size_t m_nodeCount;
    size_t m_maxDepth;

    n_tree() = default;

    static n_tree Create()
    {
      n_tree Result{};
      return Result;
    }

    void Delete();
  
    n_tree::node* AllocateNode() {
      n_tree::node* Result = new(MyAllocate(sizeof(n_tree::node))) n_tree::node();
      m_nodeCount++;
      return Result;
    }

    void* SetData(n_tree::node* Node, const T* Data)
    {
      if(!Node->Data)
      {
        Node->Data = (T*) MyAllocate(sizeof(T));
      }
      utils::Copy(sizeof(T), Data, Node->Data);
      return Node->Data;
    }

    node* NewNode(node* Parent = 0, const T* Data = 0) {
      n_tree::node* Result = AllocateNode();
      if(Parent) {  
        Parent->AddChild(Result);
        if(Result->Depth >= m_maxDepth)
        {
          m_maxDepth = Result->Depth+1;
        }
      }else{
        m_root = Result;
        Assert(m_maxDepth==0);
        m_maxDepth = 1;
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

    size_t NodeCount(){return m_nodeCount;};
    size_t CountNodes();
    size_t MaxDepth(){return m_maxDepth;}
    size_t CalculateMaxDepth();

    // Create a copy of Tree
    template<_AllocatorParams(List)>
    cmn::list<node*, _AllocatorArgs(List)>   GetLevelOrderList  ();

    template<_AllocatorParams(Vec)>
    cmn::vector<node*, _AllocatorArgs(Vec)> GetLevelOrderVector();
    
    template<_TreeAllocatorParams(Other)>
    n_tree<T, _TreeAllocatorArgs(Other)> Copy();

    template <_AllocatorParams(It)>
    struct pre_order_iterator {

      struct node_step {
        cmn::n_tree<T, _TreeAllocatorArgs(My)>::node* Node;
        int SiblingIndex;
        int SiblingCount;
      };

      cmn::vector<node_step, _AllocatorArgs(It)> NodeLadder;
      size_t MaxSize;
      n_tree* Tree;

      cmn::n_tree<T,_TreeAllocatorArgs(My)>::node* GetNode(){
        if(NodeLadder.Size()){
          return NodeLadder.Back().Node;
        }
        return 0;
      }

      void Delete(){
        NodeLadder.Delete();
        *this = {};
      }

      static bool UpdateWithSibling(node_step* Step)
      {
        if(Step && (Step->SiblingIndex+1) < Step->SiblingCount)
        {
          Step->Node = Step->Node->NextSibling;
          Step->SiblingIndex++;
          return true;
        }
        return false;
      }

      static bool HasChild(node_step* Step){
        return Step->Node->FirstChild;
      }

      static inline node_step CreateStep(cmn::n_tree<T,_TreeAllocatorArgs(My)>::node* Node, size_t SiblingCount ){
        node_step Result = {};
        Result.Node = Node;
        Result.SiblingIndex = 0;
        Result.SiblingCount = SiblingCount;
        return Result;
      }

      cmn::n_tree<T, _TreeAllocatorArgs(My)>::node* Next(bool SkipSubtree = false) {

        if(NodeLadder.Reserved() == 0)
        {
          // First step, add root.
          size_t MaxDepth = MaxSize ? MaxSize : Tree->MaxDepth();
          NodeLadder = cmn::vector<node_step, _AllocatorArgs(It)>::Create(MaxDepth);
          node_step Step = CreateStep(Tree->m_root, 0);
          NodeLadder.PushBack(Step);
        } else {
          node_step* PreviousStep = NodeLadder.BackPtr();
          if(!SkipSubtree && HasChild(PreviousStep))
          {
            // If Node in Step has a child we add it.
            node_step NextStep = CreateStep(PreviousStep->Node->FirstChild, PreviousStep->Node->ChildCount);
            NodeLadder.PushBack(NextStep);
          }else{
            // If node does not have a child we are at a leaf,
            // See if leaf has a sibling
            if(!UpdateWithSibling(PreviousStep)){
              // If updating with sibling failed (there are no siblings, or we were at last sibling)
              while(!NodeLadder.Empty())
              {
                node_step PoppedNode = NodeLadder.PopBack();
                // Step back up the node-ladder untill we find a node with a sibling.
                // Once UpdateWithSibling succeeds (we are at a new node), break.
                if(UpdateWithSibling(NodeLadder.BackPtr())) break;
              }
            }
          }
        }

        cmn::n_tree<T, _TreeAllocatorArgs(My)>::node* Result = !NodeLadder.Empty() ? NodeLadder.Back().Node : 0;
        return Result;
      }

      bool AtLeaf() {return NodeLadder.Empty() || NodeLadder.Back().Node->FirstChild == 0;}
      int Depth() { return NodeLadder.Size(); }
    };
    
    template <_AllocatorParams(It)>
    pre_order_iterator<_AllocatorArgs(It)> PreOrderIterator(int MaxSize = 0) {
      pre_order_iterator<_AllocatorArgs(It)> Result = {};
      Result.MaxSize = MaxSize;
      Result.Tree = this;
      return Result;
    };

  };

#define _NodeVisitFunction(name) void name(typename cmn::n_tree<T, _TreeAllocatorArgs(Tree)>* Tree, typename cmn::n_tree<T, _TreeAllocatorArgs(Tree)>::node* Node, void* UserData)
template<typename T, _TreeAllocatorParams(Tree)> using n_tree_node_callback = _NodeVisitFunction((*));
#define NodeVisitFunction(name) template<typename T, _TreeAllocatorParams(Tree)> _NodeVisitFunction(name)

template<typename T, _TreeAllocatorParams(Tree)> using n_tree_node = typename cmn::n_tree<T,_TreeAllocatorArgs(Tree)>::node;
template<typename T, _TreeAllocatorParams(Tree), _AllocatorParams(List)> using node_list = cmn::list<n_tree_node<T, _TreeAllocatorArgs(Tree)>*, _AllocatorArgs(List)>;
template<typename T, _TreeAllocatorParams(Tree), _AllocatorParams(Vec)> using node_vec = cmn::vector< n_tree_node<T, _TreeAllocatorArgs(Tree)>*, _AllocatorArgs(Vec)>;
template<typename T, _TreeAllocatorParams(Tree)> using n_tree_pre_order_it = typename cmn::n_tree<T, _TreeAllocatorArgs(Tree)>::pre_order_iterator;
template<typename T, _TreeAllocatorParams(Tree)> using n_tree_pre_order_step = typename cmn::n_tree<T, _TreeAllocatorArgs(Tree)>::pre_order_iterator::node_step;

template<typename T, _TreeAllocatorParams(Tree)>
void PreOrderTraversal(cmn::n_tree<T, _TreeAllocatorArgs(Tree)>& Tree, n_tree_node_callback<T, _TreeAllocatorArgs(Tree)> Callback, void* UserData) {
  if(!Tree.m_root) return;
  auto NodeQueue = node_list<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(TreeTemp) >::Create();

  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    n_tree_node<T, _TreeAllocatorArgs(Tree)>* Node = NodeQueue.PopBack();
    Callback(&Tree, Node, UserData);
    if(Node->FirstChild)
    {
      n_tree_node<T, _TreeAllocatorArgs(Tree)>* Start = Node->FirstChild->PreviousSibling;
      n_tree_node<T, _TreeAllocatorArgs(Tree)>* Child = Start;
      do
      {
        NodeQueue.PushBack(Child);
        Child = Child->PreviousSibling;
      }while(Child != Start);
    }
  }
  NodeQueue.Delete();
}

template<typename T, _TreeAllocatorParams(Tree)>
void LevelOrderTraversal(cmn::n_tree<T,_TreeAllocatorArgs(Tree)>& Tree, n_tree_node_callback<T, _TreeAllocatorArgs(Tree)> Callback, void* UserData) {
  if(!Tree.m_root) return;

  auto NodeQueue = node_list<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(TreeTemp)>::Create();
  NodeQueue.PushBack(Tree.m_root);

  while(!NodeQueue.Empty())
  {
    n_tree_node<T,_TreeAllocatorArgs(Tree)>* Node = NodeQueue.PopFront();
    Callback(&Tree, Node, UserData);

    if(Node->FirstChild)
    {
      n_tree_node<T, _TreeAllocatorArgs(Tree)>* Start = Node->FirstChild;
      n_tree_node<T, _TreeAllocatorArgs(Tree)>* Child = Start;
      do{
        NodeQueue.PushBack(Child);
        Child = Child->NextSibling;
      }while(Start != Child);
    }
  }
  NodeQueue.Delete();
}

namespace internal {
  template<typename T, _TreeAllocatorParams(Tree)>
  struct post_order_traverse {
    n_tree_node<T, _TreeAllocatorArgs(Tree)>* Node;
    bool Opened;
  };

  template<typename T, _TreeAllocatorParams(Tree), _AllocatorParams(List)>
  static void Push(cmn::list< post_order_traverse<T, _TreeAllocatorArgs(Tree)>, _AllocatorArgs(List)>& Queue, n_tree_node<T, _TreeAllocatorArgs(Tree)>* Node)
  {
    if(!Node) return;
    post_order_traverse<T,_TreeAllocatorArgs(Tree)> TraversePair = {};
    TraversePair.Node = Node;
    TraversePair.Opened = false;
    Queue.PushBack(TraversePair);
  }
} // internal

template<typename T, _TreeAllocatorParams(Tree)>
void PostOrderTraversal(cmn::n_tree<T, _TreeAllocatorArgs(Tree)>& Tree, n_tree_node_callback<T, _TreeAllocatorArgs(Tree)> Callback, void* UserData)
{
  if(!Tree.m_root) return;
  auto NodeQueue = cmn::list<internal::post_order_traverse<T, _TreeAllocatorArgs(Tree)>, _AllocatorArgs(TreeTemp)>::Create();
  internal::Push(NodeQueue, Tree.m_root);
  while(!NodeQueue.Empty())
  {
    internal::post_order_traverse<T, _TreeAllocatorArgs(Tree)>& TraversePair = NodeQueue.GetRef(NodeQueue.Last());
    n_tree_node<T, _TreeAllocatorArgs(Tree)>* Node = TraversePair.Node;

    if(!TraversePair.Opened && Node->FirstChild)
    {
      n_tree_node<T, _TreeAllocatorArgs(Tree)>* Start = Node->FirstChild->PreviousSibling;
      n_tree_node<T, _TreeAllocatorArgs(Tree)>* Child = Start;
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

NodeVisitFunction(CountNodeCallback){
  size_t* NodeCount = (size_t*) UserData;
  *NodeCount = *NodeCount + 1;
}

template <typename T, _TreeAllocatorParams(Tree)>
size_t n_tree<T, _TreeAllocatorArgs(Tree)>::CountNodes(){
  size_t Result = 0;
  cmn::LevelOrderTraversal<T, _TreeAllocatorArgs(Tree)>(*this, CountNodeCallback,(void*) &Result);
  m_nodeCount = Result;
  return Result;
};

NodeVisitFunction(CalcDepthCallback){
  if(!Node->FirstChild) {
    // Leaf;
    int TmpDepth = Node->Depth;
    Assert(Node->CalculateDepth() == TmpDepth);

    size_t* MaxDepth = (size_t*) UserData;
    if(*MaxDepth < Node->Depth)
    {
      *MaxDepth = Node->Depth;
    }
  } 
}

template <typename T, _TreeAllocatorParams(Tree)>
size_t n_tree<T, _TreeAllocatorArgs(Tree)>::CalculateMaxDepth(){
  size_t Result = 0;
  cmn::LevelOrderTraversal<T, _TreeAllocatorArgs(Tree)>(*this, CalcDepthCallback,(void*) &Result);
  m_nodeCount = Result;
  return Result;
};

NodeVisitFunction(DeleteLeafNode){
  //Tree, Node, UserData;
  Assert(!Node->FirstChild);
  Node->Remove();
  Tree->m_nodeCount--;
  TreeFree(Node->Data);
  TreeFree(Node);
}

template <typename T, _TreeAllocatorParams(My)>
void n_tree<T, _TreeAllocatorArgs(My)>::Delete()
{
  if(m_root){
    PostOrderTraversal<T, _TreeAllocatorArgs(My)>(*this, DeleteLeafNode, 0);
  }
}

struct level_order_node_list_helper {
  basic_list BasicList;
  basic_list::element* Last;
};

NodeVisitFunction(LevelOrderNodeList){
  //Tree, Node, UserData;
  level_order_node_list_helper* Helper = (level_order_node_list_helper*) UserData;
  Helper->BasicList.InsertAt(Helper->Last, (void*) &Node);
  Helper->Last = Helper->Last->Next;
}
 
template <typename T, _TreeAllocatorParams(Tree)>
template <_AllocatorParams(List)>
node_list<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(List)> n_tree<T, _TreeAllocatorArgs(Tree)>::GetLevelOrderList() {
  auto NodeList = node_list<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(List)>::Create(NodeCount());
  level_order_node_list_helper Helper = {};
  Helper.BasicList = NodeList.ToBasic();
  Helper.Last = NodeList.First();
  LevelOrderTraversal<T, _TreeAllocatorArgs(Tree)>(*this, LevelOrderNodeList, (void*) &Helper);
  return NodeList;
}

NodeVisitFunction(LevelOrderNodeVec){
  //Tree, Node, UserData;
  auto* NodeList = (node_vec<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(TreeTemp)>*) UserData;
  NodeList->PushBack(Node);
}

template <typename T, _TreeAllocatorParams(Tree)>
template <_AllocatorParams(Vec)>
node_vec<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(Vec)> n_tree<T,_TreeAllocatorArgs(Tree)>::GetLevelOrderVector() {
  size_t NodeCount = this->NodeCount();
  auto NodeVec = node_vec<T, _TreeAllocatorArgs(Tree), _AllocatorArgs(Vec)>::Create(NodeCount);
  LevelOrderTraversal<T, _TreeAllocatorArgs(Tree)>(*this, LevelOrderNodeVec, (void*) &NodeVec);

  return NodeVec;
}

NodeVisitFunction(PreOrderValueVec){
  //Tree, Node, UserData;
  cmn::vector<T*>* NodeVec = (cmn::vector<T>*) UserData;
  NodeVec->PushBack(Node->Data);
}

template <typename T, _TreeAllocatorParams(My)>
struct copy_n_tree_help_struct {
   
  struct pair {
    n_tree_node<T, _TreeAllocatorArgs(My)>* SrcNode;
    n_tree_node<T, _TreeAllocatorArgs(My)>* DstNode;
  };

  cmn::vector<pair, _AllocatorArgs(My)> NodePair;
  n_tree<T,_TreeAllocatorArgs(My)>* DstTree;

  copy_n_tree_help_struct(n_tree<T, _TreeAllocatorArgs(My)>* aDstTree, size_t aSize)
  {
    NodePair = cmn::vector<pair, _AllocatorArgs(My)>::Create(aSize);
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

  n_tree_node<T, _TreeAllocatorArgs(My)>* GetDstParent(n_tree_node<T, _TreeAllocatorArgs(My)>* SrcNode) {

    size_t ReservedSize = NodePair.Reserved();
    n_tree_node<T, _TreeAllocatorArgs(My)>* SrcParent = SrcNode->Parent;
    uint32_t DirectHash = GetHash((void*)SrcParent);
    uint32_t Hash = DirectHash % ReservedSize;
    size_t LoopIndex = 0;
    while(NodePair[Hash].SrcNode != SrcParent){
      Hash = (Hash+1) % ReservedSize;
      LoopIndex++;  
      Assert(LoopIndex < ReservedSize);
    }
    
    pair* Pair = &NodePair[Hash];
    Assert(Pair->SrcNode == SrcParent);
    return Pair->DstNode;
  }

  void PushNewPair(n_tree_node<T, _TreeAllocatorArgs(My)>* SrcNode, n_tree_node<T, _TreeAllocatorArgs(My)>* DstNode)
  {
    size_t ReservedSize = NodePair.Reserved();
    uint32_t DirectHash = GetHash((void*)SrcNode);
    uint32_t Hash = DirectHash % ReservedSize;

    pair* Pair = &NodePair[Hash];
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
NodeVisitFunction(CopyTreeFun) {
  auto* NodeMap = (copy_n_tree_help_struct<T, _TreeAllocatorArgs(Tree)>*) UserData;
  n_tree_node<T, _TreeAllocatorArgs(Tree)>* NewParent = NodeMap->GetDstParent(Node);
  n_tree_node<T, _TreeAllocatorArgs(Tree)>* NewNode = NodeMap->DstTree->NewNode(NewParent, *Node->Data);
  NodeMap->PushNewPair(Node,NewNode);
}

template <typename T, _TreeAllocatorParams(My)>
template <_TreeAllocatorParams(Other)>
n_tree<T, _TreeAllocatorArgs(Other)> n_tree<T, _TreeAllocatorArgs(My)>::Copy() {

  n_tree<T, _TreeAllocatorArgs(Other)> Result = n_tree<T, _TreeAllocatorArgs(Other)>::Create();
  
  size_t nodeCount = NodeCount();
  size_t VecSize = utils::GetNextPowerOfTwo(nodeCount);
  auto HelpStruct = copy_n_tree_help_struct<T, _TreeAllocatorArgs(Other)>(&Result, VecSize);

  LevelOrderTraversal<T, _TreeAllocatorArgs(Other)>(*this, CopyTreeFun, &HelpStruct);

  return Result;
}

} // cmn
