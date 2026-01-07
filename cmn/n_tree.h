#pragma once

#include "allocators.h"
#include "macros.h"
#include "utils.h"
#include "vector.h"
#include "list.h"

#ifndef Assert
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif


namespace cmn {



  template <typename T, typename TreeAllocators, typename TempAllocators>
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
    
    #define _NodeVisitFunction2(name) void name(typename cmn::n_tree<T, TreeAllocators, TempAllocators>* Tree, typename cmn::n_tree<T, TreeAllocators, TempAllocators>::node* Node, D* UserData)
    template<typename T, typename D, typename TreeAllocators, typename TempAllocators> using n_tree_node_callback_2 = _NodeVisitFunction2((*));     // Function Pointer
    #define NodeVisitFunction2(name) template<typename T, typename D, typename TreeAllocators, typename TempAllocators> _NodeVisitFunction2(name) // Implementation macro


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
      n_tree::node* Result = new(TreeAllocators::Allocate(sizeof(n_tree::node))) n_tree::node();
      m_nodeCount++;
      return Result;
    }

    void* SetData(n_tree::node* Node, const T* Data)
    {
      if(!Node->Data)
      {
        Node->Data = (T*) TreeAllocators::Allocate(sizeof(T));
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
    template<typename ListAllocators>
    cmn::list<node*, ListAllocators>   GetLevelOrderList  ();

    template<typename VectorAllocators>
    cmn::vector<node*, VectorAllocators> GetLevelOrderVector();
    
    template<typename OtherAllocators>
    n_tree<T, OtherAllocators, TempAllocators> Copy();

    template<typename D>
    void PreOrderTraversal(n_tree_node_callback_2<T, D, TreeAllocators, TempAllocators> Callback, D* UserData);

    template<typename D>
    void LevelOrderTraversal(n_tree_node_callback_2<T, D, TreeAllocators, TempAllocators> Callback, D* UserData);

    template<typename D>
    void PostOrderTraversal(n_tree_node_callback_2<T, D, TreeAllocators, TempAllocators> Callback, D* UserData);

    template <typename ItAllocators>
    struct pre_order_iterator {

      struct node_step {
        cmn::n_tree<T, TreeAllocators, TempAllocators>::node* Node;
        int SiblingIndex;
        int SiblingCount;
      };

      cmn::vector<node_step, ItAllocators> NodeLadder;
      size_t MaxSize;
      n_tree* Tree;

      cmn::n_tree<T, TreeAllocators, TempAllocators>::node* GetNode(){
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

      static inline node_step CreateStep(cmn::n_tree<T, TreeAllocators, TempAllocators>::node* Node, size_t SiblingCount ){
        node_step Result = {};
        Result.Node = Node;
        Result.SiblingIndex = 0;
        Result.SiblingCount = SiblingCount;
        return Result;
      }

      cmn::n_tree<T, TreeAllocators, TempAllocators>::node* Next(bool SkipSubtree = false) {

        if(NodeLadder.Reserved() == 0)
        {
          // First step, add root.
          size_t MaxDepth = MaxSize ? MaxSize : Tree->MaxDepth();
          NodeLadder = cmn::vector<node_step, ItAllocators>::Create(MaxDepth);
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

        cmn::n_tree<T, TreeAllocators, TempAllocators>::node* Result = !NodeLadder.Empty() ? NodeLadder.Back().Node : 0;
        return Result;
      }

      bool AtLeaf() {return NodeLadder.Empty() || NodeLadder.Back().Node->FirstChild == 0;}
      int Depth() { return NodeLadder.Size(); }
    };
    
    template <typename ItAllocators>
    pre_order_iterator<ItAllocators> PreOrderIterator(int MaxSize = 0) {
      pre_order_iterator<ItAllocators> Result = {};
      Result.MaxSize = MaxSize;
      Result.Tree = this;
      return Result;
    };

  };

#define _NodeVisitFunction(name) void name(typename cmn::n_tree<T, TreeAllocators, TempAllocators>* Tree, typename cmn::n_tree<T, TreeAllocators, TempAllocators>::node* Node, D* UserData)
template<typename T, typename D, typename TreeAllocators, typename TempAllocators> using n_tree_node_callback = _NodeVisitFunction((*));     // Function Pointer
#define NodeVisitFunction(name) template<typename T, typename D, typename TreeAllocators, typename TempAllocators> _NodeVisitFunction(name) // Implementation macro

template<typename T, typename TreeAllocators, typename TempAllocators> using n_tree_node = typename cmn::n_tree<T, TreeAllocators, TempAllocators>::node;
template<typename T, typename TreeAllocators, typename TempAllocators, typename ListAllocators> using node_list = cmn::list<n_tree_node<T, TreeAllocators, TempAllocators>*, ListAllocators>;
template<typename T, typename TreeAllocators, typename TempAllocators, typename VecAllocators> using node_vec = cmn::vector< n_tree_node<T, TreeAllocators, TempAllocators>*, VecAllocators>;
template<typename T, typename TreeAllocators, typename TempAllocators> using n_tree_pre_order_it = typename cmn::n_tree<T, TreeAllocators, TempAllocators>::pre_order_iterator;
template<typename T, typename TreeAllocators, typename TempAllocators> using n_tree_pre_order_step = typename cmn::n_tree<T, TreeAllocators, TempAllocators>::pre_order_iterator::node_step;

template<typename T, typename TreeAllocators, typename TempAllocators>
template<typename D>
void n_tree<T, TreeAllocators, TempAllocators>::PreOrderTraversal(n_tree_node_callback_2<T, D, TreeAllocators, TempAllocators> Callback, D* UserData)
{
  if(!m_root) return;
  auto NodeQueue = node_list<T, TreeAllocators, TempAllocators, TempAllocators >::Create();

  NodeQueue.PushBack(m_root);

  while(!NodeQueue.Empty())
  {
    n_tree_node<T, TreeAllocators, TempAllocators>* Node = NodeQueue.PopBack();
    Callback(this, Node, UserData);
    if(Node->FirstChild)
    {
      n_tree_node<T, TreeAllocators, TempAllocators>* Start = Node->FirstChild->PreviousSibling;
      n_tree_node<T, TreeAllocators, TempAllocators>* Child = Start;
      do
      {
        NodeQueue.PushBack(Child);
        Child = Child->PreviousSibling;
      }while(Child != Start);
    }
  }
  NodeQueue.Delete();
}

template<typename T, typename TreeAllocators, typename TempAllocators>
template<typename D>
void n_tree<T, TreeAllocators, TempAllocators>::LevelOrderTraversal(n_tree_node_callback_2<T, D, TreeAllocators, TempAllocators> Callback, D* UserData)
{
  if(!m_root) return;

  auto NodeQueue = node_list<T, TreeAllocators, TempAllocators, TempAllocators>::Create();
  NodeQueue.PushBack(m_root);

  while(!NodeQueue.Empty())
  {
    n_tree_node<T,TreeAllocators, TempAllocators>* Node = NodeQueue.PopFront();
    Callback(this, Node, UserData);

    if(Node->FirstChild)
    {
      n_tree_node<T, TreeAllocators, TempAllocators>* Start = Node->FirstChild;
      n_tree_node<T, TreeAllocators, TempAllocators>* Child = Start;
      do{
        NodeQueue.PushBack(Child);
        Child = Child->NextSibling;
      }while(Start != Child);
    }
  }
  NodeQueue.Delete();
}

namespace internal {
  template<typename T, typename TreeAllocators, typename TempAllocators>
  struct post_order_traverse {
    n_tree_node<T, TreeAllocators, TempAllocators>* Node;
    bool Opened;
  };

  template<typename T, typename TreeAllocators, typename TempAllocators, typename ListAllocators>
  static void Push(cmn::list< post_order_traverse<T, TreeAllocators, TempAllocators>, ListAllocators>& Queue, n_tree_node<T, TreeAllocators, TempAllocators>* Node)
  {
    if(!Node) return;
    post_order_traverse<T,TreeAllocators, TempAllocators> TraversePair = {};
    TraversePair.Node = Node;
    TraversePair.Opened = false;
    Queue.PushBack(TraversePair);
  }
} // internal

template<typename T, typename TreeAllocators, typename TempAllocators>
template<typename D>
void n_tree<T, TreeAllocators, TempAllocators>::PostOrderTraversal(n_tree_node_callback_2<T, D, TreeAllocators, TempAllocators> Callback, D* UserData)
{
  if(!m_root) return;
  auto NodeQueue = cmn::list<internal::post_order_traverse<T, TreeAllocators, TempAllocators>, TempAllocators>::Create();
  internal::Push(NodeQueue, m_root);
  while(!NodeQueue.Empty())
  {
    internal::post_order_traverse<T, TreeAllocators, TempAllocators>& TraversePair = NodeQueue.GetRef(NodeQueue.Last());
    n_tree_node<T, TreeAllocators, TempAllocators>* Node = TraversePair.Node;

    if(!TraversePair.Opened && Node->FirstChild)
    {
      n_tree_node<T, TreeAllocators, TempAllocators>* Start = Node->FirstChild->PreviousSibling;
      n_tree_node<T, TreeAllocators, TempAllocators>* Child = Start;
      do
      {
        internal::Push(NodeQueue, Child);
        Child = Child->PreviousSibling;
      }while(Child != Start);
      TraversePair.Opened = true;
    }else{
      NodeQueue.PopBack();
      Callback(this, Node, UserData);
    }
  }
  NodeQueue.Delete();
}

NodeVisitFunction(CountNodeCallback){
  size_t* NodeCount = UserData;
  *NodeCount = (*NodeCount) + 1;
}

template <typename T, typename TreeAllocators, typename TempAllocators>
size_t n_tree<T, TreeAllocators, TempAllocators>::CountNodes(){
  size_t Result = 0;
  LevelOrderTraversal<size_t>(CountNodeCallback, &Result);
  m_nodeCount = Result;
  return Result;
};

NodeVisitFunction(CalcDepthCallback){
  if(!Node->FirstChild) {
    // Leaf;
    size_t TmpDepth = Node->Depth;
    Assert(Node->CalculateDepth() == TmpDepth);

    size_t* MaxDepth = UserData;
    if(*MaxDepth < Node->Depth)
    {
      *MaxDepth = Node->Depth;
    }
  } 
}

template <typename T, typename TreeAllocators, typename TempAllocators>
size_t n_tree<T, TreeAllocators, TempAllocators>::CalculateMaxDepth(){
  size_t Result = 0;
  LevelOrderTraversal<size_t>(CalcDepthCallback, &Result);
  m_maxDepth = Result+1; // Max depth is large enough to crete a vector containing all nodes in a chain. Root is at depth 0, but a tree with only a root has depth one.
  return Result;
};

NodeVisitFunction(DeleteLeafNode){
  //Tree, Node, UserData;
  Assert(!Node->FirstChild);
  Node->Remove();
  Tree->m_nodeCount--;
  TreeAllocators::Free(Node->Data);
  TreeAllocators::Free(Node);
}

template <typename T, typename TreeAllocators, typename TempAllocators>
void n_tree<T, TreeAllocators, TempAllocators>::Delete()
{
  if(m_root){
    PostOrderTraversal<void>(DeleteLeafNode, 0);
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
 
template <typename T, typename TreeAllocators, typename TempAllocators>
template <typename ListAllocators>
node_list<T, TreeAllocators, TempAllocators, ListAllocators> n_tree<T, TreeAllocators, TempAllocators>::GetLevelOrderList() {
  auto NodeList = node_list<T, TreeAllocators, TempAllocators, ListAllocators>::Create(NodeCount());
  level_order_node_list_helper Helper = {};
  Helper.BasicList = NodeList.ToBasic();
  Helper.Last = NodeList.First();
  LevelOrderTraversal<>(LevelOrderNodeList, &Helper);
  return NodeList;
}

NodeVisitFunction(LevelOrderNodeVec){
  //Tree, Node, UserData;
  node_vec<T, TreeAllocators, TempAllocators, TempAllocators>* NodeList= UserData;
  NodeList->PushBack(Node);
}

template <typename T, typename TreeAllocators, typename TempAllocators>
template <typename VecAllocators>
node_vec<T, TreeAllocators, TempAllocators, VecAllocators> n_tree<T,TreeAllocators, TempAllocators>::GetLevelOrderVector() {
  size_t NodeCount = this->NodeCount();
  auto NodeVec = node_vec<T, TreeAllocators, TempAllocators, VecAllocators>::Create(NodeCount);
  LevelOrderTraversal<>(LevelOrderNodeVec, &NodeVec);

  return NodeVec;
}

NodeVisitFunction(PreOrderValueVec){
  //Tree, Node, UserData;
  cmn::vector<T*>* NodeVec = (cmn::vector<T>*) UserData;
  NodeVec->PushBack(Node->Data);
}

template <typename T, typename SrcAllocators, typename DstAllocators, typename TempAllocators>
struct copy_n_tree_help_struct {
   
  struct pair {
    n_tree_node<T, SrcAllocators, TempAllocators>* SrcNode;
    n_tree_node<T, DstAllocators, TempAllocators>* DstNode;
  };

  cmn::vector<pair, TempAllocators> NodePair;
  n_tree<T, DstAllocators, TempAllocators>* DstTree;

  copy_n_tree_help_struct(n_tree<T, DstAllocators, TempAllocators>* aDstTree, size_t aSize)
  {
    NodePair = cmn::vector<pair, TempAllocators>::Create(aSize);
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

  n_tree_node<T, DstAllocators, TempAllocators>* GetDstParent(n_tree_node<T, SrcAllocators, TempAllocators>* SrcNode) {

    size_t ReservedSize = NodePair.Reserved();
    n_tree_node<T, SrcAllocators, TempAllocators>* SrcParent = SrcNode->Parent;
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

  void PushNewPair(n_tree_node<T, SrcAllocators, TempAllocators>* SrcNode, n_tree_node<T, DstAllocators, TempAllocators>* DstNode)
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

NodeVisitFunction(CopyTreeFun) {
  auto* NewParent = UserData->GetDstParent(Node);
  auto* NewNode = UserData->DstTree->NewNode(NewParent, *Node->Data);
  UserData->PushNewPair(Node,NewNode);
}

template <typename T, typename TreeAllocators, typename TempAllocators>
template <typename OtherAllocators>
n_tree<T, OtherAllocators, TempAllocators> n_tree<T, TreeAllocators, TempAllocators>::Copy() {

  n_tree<T, OtherAllocators, TempAllocators> Result = n_tree<T, OtherAllocators, TempAllocators>::Create();
  
  size_t nodeCount = NodeCount();
  size_t VecSize = utils::GetNextPowerOfTwo(nodeCount);
  auto HelpStruct = copy_n_tree_help_struct<T, TreeAllocators, OtherAllocators, TempAllocators>(&Result, VecSize);

  LevelOrderTraversal<>(CopyTreeFun, &HelpStruct);

  return Result;
}

} // cmn
