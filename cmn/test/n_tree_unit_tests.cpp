#pragma once

#include <cstdio>

#include "vector.h"
#include "n_tree.h"
#include "debug_asserts.h"


#ifndef ArrayCount
#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]))
#endif // ArrayCount


bool gShouldPrint = false;
#include <string>
#include <iostream>

template<class T> using vector_dbg = cmn::vector<T, CustomAllocators>;
template<class T> using tree_dbg   = cmn::n_tree<T, CustomAllocators, CustomAllocators>;
template<class T> using tree_std   = cmn::n_tree<T, StdAllocators, CustomAllocators>;

struct user_data {
  int i;
  vector_dbg<int> vec;
};

NodeVisitFunction2(treeTraversealAssert){
  DBG_Assert(*Node->Data, UserData->vec[UserData->i],"Values");
  UserData->i++;
}

/*
      1
    / | \
   2  3  4
     / \  \
    5   6  7
   /        \
  8          9
Breath first:
- Level Order  1,2,3,4,5,6,7,8,9 (Done)
Depth First
- Pre Order    1,2,3,5,8,6,4,7,9 (Done)
- In Order     2,8,5,3,6,1,9,7,4 (Do we need? Better for binary trees.)
- Post Order   2,8,5,6,3,9,7,4,1 (Done)
Also want a traversal which is conidtional Given were on a node, have some function which ranks the priority of the children.
*/


tree_dbg<int>::node* CreateAndAssertNode(tree_dbg<int>* Tree, tree_dbg<int>::node* Parent, int Value, size_t NodeCount, size_t NodeDepth, size_t MaxDepth, size_t ParentChildCount)
{
  tree_dbg<int>::node* Result = Tree->NewNode(Parent, Value);
  DBG_Assert(Tree->MaxDepth(), MaxDepth, "Max Depth");
  DBG_Assert(Tree->NodeCount(), NodeCount, "Node count");
  DBG_Assert(Result->Depth, NodeDepth, "Max Depth");
  DBG_Assert(*Result->Data, Value, "Node Value");
  if(Parent)
  {
    DBG_Assert(Parent->ChildCount, ParentChildCount, "Node Value");
  }
  return Result; 
}

tree_dbg<int> createTree()
{
  tree_dbg<int> Tree = tree_dbg<int>::Create();
  DBG_Assert(Tree.NodeCount(), 0, "Node Count");  
  tree_dbg<int>::node* Root = CreateAndAssertNode(&Tree, NULL, 1, 1, 0, 1, 0);
  tree_dbg<int>::node* n2   = CreateAndAssertNode(&Tree, Root, 2, 2, 1, 2, 1);
  tree_dbg<int>::node* n3   = CreateAndAssertNode(&Tree, Root, 3, 3, 1, 2, 2);
  tree_dbg<int>::node* n4   = CreateAndAssertNode(&Tree, Root, 4, 4, 1, 2, 3);
  tree_dbg<int>::node* n5   = CreateAndAssertNode(&Tree, n3,   5, 5, 2, 3, 1);
  tree_dbg<int>::node* n6   = CreateAndAssertNode(&Tree, n3,   6, 6, 2, 3, 2);
  tree_dbg<int>::node* n7   = CreateAndAssertNode(&Tree, n4,   7, 7, 2, 3, 1);
  tree_dbg<int>::node* n8   = CreateAndAssertNode(&Tree, n5,   8, 8, 3, 4, 1);
  tree_dbg<int>::node* n9   = CreateAndAssertNode(&Tree, n7,   9, 9, 3, 4, 1);
  DBG_Assert(Tree.NodeCount(), 9, "Node Count");
  DBG_Assert(Tree.MaxDepth(), 4, "Max Depth");
  DBG_Assert(Tree.CountNodes(), 9, "Node Count Calc");
  Tree.CalculateMaxDepth();
  //DBG_Assert(Tree.CalculateMaxDepth(), 4, "Max Depth calc");
  DBG_Assert(Tree.MaxDepth(), 4, "Max Depth");
  DBG_Assert(Tree.NodeCount(), 9, "Node Count");
  return Tree;
}

void Test_Traversal()
{
  {
    tree_dbg<int> tree = createTree();
    int PreOrderBuf[] = {1,2,3,5,8,6,4,7,9};
    user_data PreOrderData = {};
    PreOrderData.vec = vector_dbg<int>::Create(ArrayCount(PreOrderBuf), PreOrderBuf);
    tree.PreOrderTraversal<user_data>(treeTraversealAssert, &PreOrderData);
    tree.Delete();
    PreOrderData.vec.Delete();
  }
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");

  {
    tree_dbg<int> tree = createTree();
    int PostOrderBuf[] = {2,8,5,6,3,9,7,4,1};
    user_data PostOrderData = {};
    PostOrderData.vec = vector_dbg<int>::Create(ArrayCount(PostOrderBuf), PostOrderBuf);
    tree.PostOrderTraversal<>(treeTraversealAssert, &PostOrderData);
    tree.Delete();
    PostOrderData.vec.Delete();
  }
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");

  {
    tree_dbg<int> tree = createTree();
    int LevelOrderBuf[] = {1,2,3,4,5,6,7,8,9};
    user_data LevelOrderData = {};
    LevelOrderData.vec = vector_dbg<int>::Create(ArrayCount(LevelOrderBuf), LevelOrderBuf);
    tree.LevelOrderTraversal<user_data>(treeTraversealAssert, &LevelOrderData);
    tree.Delete();
    LevelOrderData.vec.Delete();
  }  
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");
}


void Test_LevelOrderVecList()
{
  tree_dbg<int> tree = createTree();
  cmn::node_list<int,CustomAllocators, CustomAllocators, CustomAllocators> TreeList = tree.GetLevelOrderList<CustomAllocators>();
  cmn::node_vec<int, CustomAllocators, CustomAllocators, CustomAllocators> TreeVec  = tree.GetLevelOrderVector<CustomAllocators>();

  int LevelOrderBuf[] = {1,2,3,4,5,6,7,8,9};
  user_data LevelOrderData = {};
  LevelOrderData.vec = vector_dbg<int>::Create(ArrayCount(LevelOrderBuf), LevelOrderBuf);

  DBG_Assert(TreeVec.Size(), tree.NodeCount(), "Vector Size");
  DBG_Assert(TreeList.Size(), tree.NodeCount(), "Vector Size");
  for (int i = 0; i < TreeList.Size(); ++i)
  {
    DBG_Assert(*TreeList.GetCopy(i)->Data, LevelOrderData.vec[i], "List Value");
    DBG_Assert(*TreeVec[i]->Data, LevelOrderData.vec[i], "Vector Value");
  }

  TreeList.Delete();
  tree.Delete();
  TreeVec.Delete();
  LevelOrderData.vec.Delete();

  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");

}

void Test_Copy()
{
  tree_dbg<int> tree = createTree();
  tree_std<int> treeCopy = tree.Copy<StdAllocators>();
  DBG_AssertFalse(tree.m_root, treeCopy.m_root, "m_root Pointer" );
  treeCopy.Delete();
  tree.Delete();
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");
}

void Test_PreOrderIterator()
{
  tree_dbg<int> Tree = createTree();
  auto It = Tree.PreOrderIterator<CustomAllocators>();

  int GroundTruthDepth[] = {2,4,3,4};
  int GroundTruth[4][4] = { 
    {1,2},
    {1,3,5,8},
    {1,3,6},
    {1,4,7,9}
  };

  int LeafCount = 0;
  while(cmn::n_tree_node<int, CustomAllocators, CustomAllocators>* Node = It.Next())
  {
    if(It.AtLeaf())
    {
      DBG_Assert(It.Depth(), GroundTruthDepth[LeafCount], "Depth");
      for (int i = 0; i < It.NodeLadder.Size(); ++i)
      {
        auto Step = It.NodeLadder[i];
        DBG_Assert(*(Step.Node->Data),  GroundTruth[LeafCount][i], "Value");
        DBG_Assert(Step.Node->Depth, i, "NodeDepth");
      }
      LeafCount++;
    }
    
  }
  It.Delete(); // Not necessary in real code since It only uses transient memory. 
  Tree.Delete();
  DBG_Assert(gMallocCount, gFreeCount, "Malloc Equal to Free");
}

void Test_Sum()
{
  tree_dbg<int> Tree = createTree();
  auto It = Tree.PreOrderIterator<CustomAllocators>();

  int GroundTruthSum[]   = {3,17,10,21};
  int LeafCount = 0;
  vector_dbg<int> IntSumVec = vector_dbg<int>::Create(Tree.MaxDepth());
  while(cmn::n_tree_node<int, CustomAllocators, CustomAllocators>* Node = It.Next())
  {
    int Depth = It.Depth()-1;
    IntSumVec[Depth] = *It.GetNode()->Data + (Depth == 0 ? 0 : IntSumVec[Depth-1]);
    if(It.AtLeaf())
    {
      DBG_Assert(IntSumVec[Depth], GroundTruthSum[LeafCount], "Sum of node values");
      LeafCount++;
    }
  }
  IntSumVec.Delete(); // Not necessary in real code since It only uses transient memory. 
  It.Delete();        // Not necessary in real code since It only uses transient memory. 
  Tree.Delete();
  DBG_Assert(gMallocCount, gFreeCount, "Malloc Equal to Free");
}

int main (int argc, char* argv[])
{
  gShouldPrint = argc > 1;
  DBG_RunTest(Test_Traversal);
  DBG_RunTest(Test_LevelOrderVecList);
  DBG_RunTest(Test_Copy);
  DBG_RunTest(Test_PreOrderIterator);
  DBG_RunTest(Test_Sum);
  printf("Success\n");
  return 0;
}
