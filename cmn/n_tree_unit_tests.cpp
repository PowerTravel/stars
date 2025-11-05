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

NodeVisitFunction(treeTraversealAssert){
  user_data* data = (user_data*) UserData;
  #if 1
  DBG_Assert(*Node->Data, data->vec[data->i],"Values");
  #else
  printf("Vec[%d] = %d, Node %d\n", data->i, data->vec[data->i], (int) *Node->Data);
  #endif
  data->i++;
}

struct user_data {
  int i;
  cmn::vector<int> vec;
};

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


cmn::n_tree<int> createTree()
{
  cmn::n_tree<int> tree = cmn::n_tree<int>::Create();
  DBG_Assert(tree.NodeCount(), 0, "Node Count");
  cmn::n_tree<int>::node* Root = tree.NewNode(NULL, 1);
  cmn::n_tree<int>::node* n2   = tree.NewNode(Root, 2);
  cmn::n_tree<int>::node* n3   = tree.NewNode(Root, 3);
  cmn::n_tree<int>::node* n4   = tree.NewNode(Root, 4);
  cmn::n_tree<int>::node* n5   = tree.NewNode(n3, 5);
  cmn::n_tree<int>::node* n6   = tree.NewNode(n3, 6);
  cmn::n_tree<int>::node* n7   = tree.NewNode(n4, 7);
  cmn::n_tree<int>::node* n8   = tree.NewNode(n5, 8);
  cmn::n_tree<int>::node* n9   = tree.NewNode(n7, 9);
  DBG_Assert(tree.NodeCount(), 9, "Node count");
  return tree;
}

void Test_Traversal()
{
  dbg::SetCustomGlobalAllocators();
  {
    cmn::n_tree<int> tree = createTree();
    int PreOrderBuf[] = {1,2,3,5,8,6,4,7,9};
    user_data PreOrderData = {};
    PreOrderData.vec = cmn::vector<int>::Create(ArrayCount(PreOrderBuf), PreOrderBuf);
    cmn::PreOrderTraversal(tree, treeTraversealAssert,(void*) &PreOrderData);
    tree.Delete();
    PreOrderData.vec.Delete();
  }
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");

  {
    cmn::n_tree<int> tree = createTree();
    int PostOrderBuf[] = {2,8,5,6,3,9,7,4,1};
    user_data PostOrderData = {};
    PostOrderData.vec = cmn::vector<int>::Create(ArrayCount(PostOrderBuf), PostOrderBuf);
    cmn::PostOrderTraversal(tree, treeTraversealAssert,(void*) &PostOrderData);
    tree.Delete();
    PostOrderData.vec.Delete();
  }
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");

  {
    cmn::n_tree<int> tree = createTree();
    int LevelOrderBuf[] = {1,2,3,4,5,6,7,8,9};
    user_data LevelOrderData = {};
    LevelOrderData.vec = cmn::vector<int>::Create(ArrayCount(LevelOrderBuf), LevelOrderBuf);
    cmn::LevelOrderTraversal(tree, treeTraversealAssert,(void*) &LevelOrderData);
    tree.Delete();
    LevelOrderData.vec.Delete();
  }  
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");
}


void Test_LevelOrderVecList()
{
  dbg::SetCustomGlobalAllocators();
  cmn::n_tree<int> tree = createTree();
  cmn::node_list<int> TreeList = tree.GetLevelOrderList();
  cmn::node_vec<int> TreeVec = tree.GetLevelOrderVector();

  int LevelOrderBuf[] = {1,2,3,4,5,6,7,8,9};
  user_data LevelOrderData = {};
  LevelOrderData.vec = cmn::vector<int>::Create(ArrayCount(LevelOrderBuf), LevelOrderBuf);

  DBG_Assert(TreeList.Size(), tree.NodeCount(), "Vector Size");
  for (int i = 0; i < TreeList.Size(); ++i)
  {
    DBG_Assert(*TreeList.GetCopy(i)->Data, LevelOrderData.vec[i], "Vector Size");
    DBG_Assert(*TreeVec[i]->Data, LevelOrderData.vec[i], "Vector Size");
  }

  TreeList.Delete();
  tree.Delete();
  TreeVec.Delete();
  LevelOrderData.vec.Delete();

  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");
}

void Test_Copy()
{
  dbg::SetCustomGlobalAllocators();
  cmn::n_tree<int> tree = createTree();
  cmn::n_tree<int> treeCopy = tree.Copy();
  DBG_AssertFalse(tree.m_root, treeCopy.m_root, "m_root Pointer" );
  treeCopy.Delete();
  tree.Delete();
  DBG_Assert(gMallocCount, gFreeCount, "Alloc equal to free");
}

int main (int argc, char* argv[])
{
  gShouldPrint = argc > 1;
  DBG_RunTest(Test_Traversal);
  DBG_RunTest(Test_LevelOrderVecList);
  DBG_RunTest(Test_Copy);
  printf("Success\n");
  return 0;
}
