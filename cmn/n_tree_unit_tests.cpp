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
- Level Order  1,2,3,4,5,6,7,8,9 (Done)
Depth First
- Pre Order    1,2,3,5,8,6,4,7,9 (Done)
- In Order     2,8,5,3,6,1,9,7,4 (Do we need? Better for binary trees.)
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

void Test_Traversal()
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

  void AssertNavigator(const cmn::n_tree<int>::navigator& Navigator, const cmn::n_tree<int>& Tree, const cmn::n_tree<int>::node* Node,
    size_t ChildCount, uint32_t SiblingCount, uint32_t SiblingIndex, uint32_t Depth, int Value)
  {
    DBG_Assert(Navigator.GetCopy(), Value, "ValueCopy");
    DBG_Assert(Navigator.GetRef(), Value, "ValueRef");
    DBG_Assert(*Navigator.GetPtr(), Value, "ValuePtr");
    DBG_Assert((void*) Navigator.m_node, (void*) Node, "Node");
    DBG_Assert(Navigator.ChildCount(), ChildCount, "ChildCount");
    DBG_Assert(Navigator.SiblingCount(), SiblingCount, "SiblingCount");
    DBG_Assert(Navigator.SiblingIndex(), SiblingIndex, "SiblingIndex");
    DBG_Assert(Navigator.Depth(), Depth, "Depth");
    DBG_Assert(Navigator.IsLeaf(), ChildCount == 0, "IsLeaf");
  }

  void Test_navigator(){
    cmn::n_tree<int> Tree = createTree();
    cmn::n_tree<int>::navigator Nav = Tree.NewNavigator();
    
    AssertNavigator(Nav, Tree, Tree.m_root, 3, 0, 0, 0, 1);
    DBG_Assert(Nav.MoveToParent(), false, "ValuePtr");
    AssertNavigator(Nav, Tree, Tree.m_root, 3, 0, 0, 0, 1); // Nav remains unchanged after "failed" Move-command
    
    DBG_Assert(Nav.MoveToChild(), true, "MoveToChild"); // Move to first child


    // Move back and forth in different ways in depth 1
    cmn::node_list_element<int>* Node2 = Tree.m_root->Children.First();
    AssertNavigator(Nav, Tree, Node2->GetCopy(), 0, 3, 0, 1, 2);
    DBG_Assert(Nav.NextSibling(1), true, "NextSibling()");
    AssertNavigator(Nav, Tree, Node2->Next->GetCopy(), 2, 3, 1, 1, 3);
    DBG_Assert(Nav.NextSibling(), true, "NextSibling");
    AssertNavigator(Nav, Tree, Node2->Next->Next->GetCopy(), 1, 3, 2, 1, 4);
    DBG_Assert(Nav.NextSibling(), false, "NextSibling");
    AssertNavigator(Nav, Tree, Node2->Next->Next->GetCopy(), 1, 3, 2, 1, 4);

    DBG_Assert(Nav.NextSibling(-3), false, "NextSibling");
    AssertNavigator(Nav, Tree, Node2->Next->Next->GetCopy(), 1, 3, 2, 1, 4);
    DBG_Assert(Nav.NextSibling(-2), true, "NextSibling");
    AssertNavigator(Nav, Tree, Node2->GetCopy(), 0, 3, 0, 1, 2);
    DBG_Assert(Nav.MoveToSibling(4), false, "MoveToSibling");
    AssertNavigator(Nav, Tree, Node2->GetCopy(), 0, 3, 0, 1, 2);
    DBG_Assert(Nav.MoveToSibling(2), true, "MoveToSibling");
    AssertNavigator(Nav, Tree, Node2->Next->Next->GetCopy(), 1, 3, 2, 1, 4);
    DBG_Assert(Nav.PreviousSibling(), true, "PreviousSibling");
    AssertNavigator(Nav, Tree, Node2->Next->GetCopy(), 2, 3, 1, 1, 3);

    // Move Up and down a bit
    cmn::node_list_element<int>* Node5 = Node2->Next->GetCopy()->Children.First();
    DBG_Assert(Nav.MoveToChild(1), true, "MoveToChild");
    AssertNavigator(Nav, Tree, Node5->Next->GetCopy(), 0, 2, 1, 2, 6);
    DBG_Assert(Nav.PreviousSibling(), true, "MoveToChild");
    AssertNavigator(Nav, Tree, Node5->GetCopy(), 1, 2, 0, 2, 5);
    cmn::node_list_element<int>* Node8 = Node5->GetCopy()->Children.First();
    DBG_Assert(Nav.MoveToChild(), true, "MoveToChild");
    AssertNavigator(Nav, Tree, Node8->GetCopy(), 0, 1, 0, 3, 8);


    int DepthCount = 3;
    while(Nav.MoveToParent())
    {
      DepthCount--;
    }
    DBG_Assert(DepthCount, 0, "Depth climbing up");
    AssertNavigator(Nav, Tree, Tree.m_root, 3, 0, 0, 0, 1);

  }

int main ()
{  
  DBG_RunTest(Test_Traversal);
  DBG_RunTest(Test_navigator);
}
