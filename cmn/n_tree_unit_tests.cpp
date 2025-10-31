#pragma once

#include <cstdio>

#include "vector.h"
#include "n_tree.h"
#include "debug_asserts.h"

#ifndef ArrayCount
#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]))
#endif // ArrayCount


bool gShouldPrint = false;
int gReallocCount = 0;
int gMallocCount = 0;
int gFreeCount = 0;

CMN_MALLOC_FUNCTION(customMalloc){
  gMallocCount++;
  return malloc(sz);
}
CMN_REALLOC_FUNCTION(customRealloc){
  gReallocCount++;
  return realloc(p, sz);
}
CMN_FREE_FUNCTION(customFree){
  gFreeCount++;
  free(p);
}

void ResetAllocationCounters()
{
  gReallocCount = 0;
  gMallocCount = 0;
  gFreeCount = 0;
}

void SetDefaultGlobalAllocators(){
  cmn::SetDefaultCustomAllocators(malloc, realloc, free);
}

void SetCustomGlobalAllocators(){
  cmn::SetDefaultCustomAllocators(customMalloc, customRealloc, customFree);
}

void ResetTestEnvironment()
{
  ResetAllocationCounters();
  SetDefaultGlobalAllocators();
}

/*
  CMN_N_TREE_TRAVERSE_CALLBAK(Test1VerifyCallback)
  {
    verify_int_vec* VerifyVec = (verify_int_vec*) UserData;
    Assert(VerifyVec.Index == cmn::NodeCount(NTree));
    int Index = VerifyVec.Index++;
    int* Val = (int*) cmn::Get(Node);
    Assert(VerifyVec.Vec[Index] == Val);
  }
*/

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
- Level Order  1,2,3,4,5,6,7,8,9
Depth First
- Pre Order    1,2,3,5,8,6,4,7,9 (Done)
- In Order     2,8,5,3,6,1,9,7,4 
- Post Order   2,8,5,6,3,9,7,4,1 (Done)
Also want a traversal which is conidtional Given were on a node, have some function which ranks the priority of the children.
*/


cmn::n_tree<int> createTree()
{
  cmn::n_tree<int> tree = cmn::n_tree<int>();
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

void Test1()
{
  cmn::n_tree<int> tree = createTree();
  
  int PreOrderBuf[] = {1,2,3,5,8,6,4,7,9};
  user_data PreOrderData = {};
  PreOrderData.vec = cmn::vector<int>(ArrayCount(PreOrderBuf), PreOrderBuf);
  cmn::PreOrderTraversal(tree, treeTraversealAssert,(void*) &PreOrderData);

  int PostOrderBuf[] = {2,8,5,6,3,9,7,4,1};
  user_data PostOrderData = {};
  PostOrderData.vec = cmn::vector<int>(ArrayCount(PostOrderBuf), PostOrderBuf);
  cmn::PostOrderTraversal(tree, treeTraversealAssert,(void*) &PostOrderData);

  int LevelOrderBuf[] = {1,2,3,4,5,6,7,8,9};
  user_data LevelOrderData = {};
  LevelOrderData.vec = cmn::vector<int>(ArrayCount(LevelOrderBuf), LevelOrderBuf);
  cmn::LevelOrderTraversal(tree, treeTraversealAssert,(void*) &LevelOrderData);


}


int main ()
{  
  DBG_RunTest(Test1);
}
