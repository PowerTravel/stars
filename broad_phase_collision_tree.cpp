#include "broad_phase_collision_tree.h"


namespace broadphase {

inline internal b32
IsLeaf( node* Node )
{
  return !(Node->Left || Node->Right);
}

inline internal midx
Size(node** Base,
     node** Head)
{
  Assert(Head >= Base);
  return (Head - Base);
}

inline internal node**
Push2(node** Head,
      node*  Node)
{
  Head+=2;
  *Head = Node;
  return Head;
}

inline internal node**
Pop2(node** Head)
{
  Head-=2;
  return Head;
}

inline internal node*
Get(node** Head)
{
  return *Head;
}


internal node**
GetLeastVolumeincreaseBranch(node* Parent,  aabb3f* LeafAABB)
{
  r32 PreLeftVolume   = GetVolume(&Parent->Left->AABB);
  r32 PreRightVolume  = GetVolume(&Parent->Right->AABB);
  aabb3f LeftMerge    = MergeAABB(Parent->Left->AABB,  *LeafAABB);
  aabb3f RightMerge   = MergeAABB(Parent->Right->AABB, *LeafAABB);
  r32 PostLeftVolume  = GetVolume(&LeftMerge);
  r32 PostRightVolume = GetVolume(&RightMerge);
  r32 LeftDiff = PostLeftVolume - PreLeftVolume;
  r32 RightDiff = PostRightVolume - PreRightVolume;

  node** Result = 0;
  if(LeftDiff <= RightDiff)
  {
    Result = &Parent->Left;
  }else{
    Result = &Parent->Right;
  }

  return Result;
}

void Insert(tree* Tree, aabb3f AABB, void* (*Allocate)(u32 Size), void* CustomData)
{
  node* Leaf = (node*) Allocate(sizeof(node));
  *Leaf = {};
  Leaf->AABB = AABB;
  Leaf->CustomData = CustomData;

  if(Tree->Size == 0)
  {
    Tree->Root = Leaf;
    Tree->Size = 1;
    return;
  }

  node** CurrentNodePtr = &Tree->Root;
  node*  CurrentNode    = *CurrentNodePtr;
  while(!IsLeaf(CurrentNode))
  {
    CurrentNode->AABB = MergeAABB(CurrentNode->AABB, Leaf->AABB);
    CurrentNodePtr    = GetLeastVolumeincreaseBranch(CurrentNode, &Leaf->AABB);
    CurrentNode       = *CurrentNodePtr;
  }

  // We are at leaf
  node* Node = (node*) Allocate(sizeof(node));
  *Node = {};
  Node->Left  = Leaf;
  Node->Right = CurrentNode;
  Node->AABB  = MergeAABB(Node->Left->AABB, Node->Right->AABB);
  *CurrentNodePtr = Node;
  Tree->Size+=2;
}



collision_result* GetCollisionPairs(tree* Tree, u32* ResultStackSize, void* (*Allocate)(u32 Size))
{
  *ResultStackSize = 0;
  if(Tree->Size < 2)
  {
    return 0;
  }
  node** const Base = (node**) Allocate(2 * Tree->Size *sizeof(node*));
  node** const LeftBase  = Base;
  node** const RightBase = Base+1;
  collision_result* ResultHead = 0;
  node** LeftHead  = LeftBase;
  node** RightHead = RightBase;

  // Init the stacks
  LeftHead  = Push2(LeftHead,  Tree->Root->Left);
  RightHead = Push2(RightHead, Tree->Root->Right);
  while( Size(LeftBase,LeftHead) > 0 )
  {
    Assert(LeftHead - Base < 2*Tree->Size);
    Assert(RightHead - Base < 2*Tree->Size);
    node* Left  = Get(LeftHead);
    node* Right = Get(RightHead);

    if( IsLeaf(Left) )
    {
      if( IsLeaf(Right) )
      {
        // Both are leaves
        if(AABBIntersects(&Left->AABB, &Right->AABB))
        {
          collision_result* NewResult = (collision_result*) Allocate(sizeof(collision_result));
          NewResult->Node1 = Left;
          NewResult->Node2 = Right;
          NewResult->Previous = ResultHead;
          ResultHead = NewResult;
          ++(*ResultStackSize);
        }
        // Pop the leafs
        LeftHead  = Pop2(LeftHead);
        RightHead = Pop2(RightHead);
      }else{
        // Left is leaf, Right is Node
        if(!Right->Entered)
        {
          Right->Entered = true;
          LeftHead =  Push2(LeftHead,  Right->Left);
          RightHead = Push2(RightHead, Right->Right);
        }else{
          LeftHead =  Pop2(LeftHead);
          RightHead = Pop2(RightHead);

          if(AABBIntersects(&Left->AABB, &Right->AABB))
          {
            LeftHead  = Push2(LeftHead,  Left);
            RightHead = Push2(RightHead, Right->Left);
            LeftHead  = Push2(LeftHead,  Left);
            RightHead = Push2(RightHead, Right->Right);
          }
        }
      }
    }else{
      if(IsLeaf(Right))
      {
        // Left is Node, Right is leaf
        if(!Left->Entered)
        {
          Left->Entered = true;
          LeftHead  = Push2(LeftHead,  Left->Left);
          RightHead = Push2(RightHead, Left->Right);
        }else{
          LeftHead =  Pop2(LeftHead);
          RightHead = Pop2(RightHead);
          if(AABBIntersects(&Left->AABB, &Right->AABB))
          {
            LeftHead  = Push2(LeftHead,  Left->Left);
            RightHead = Push2(RightHead, Right);
            LeftHead  = Push2(LeftHead,  Left->Right);
            RightHead = Push2(RightHead, Right);
          }
        }
      }else{
        // Left is Node, Right is Node
        if( !Left->Entered )
        {
          Left->Entered = true;
          LeftHead  = Push2(LeftHead,  Right->Left);
          RightHead = Push2(RightHead, Right->Right);
        }else if( !Right->Entered ){
          Right->Entered = true;
          LeftHead  = Push2(LeftHead,  Left->Left);
          RightHead = Push2(RightHead, Left->Right);
        }else{
          // Pop2 the top head and add all the cross terms
          // This ensures that we wont end up in a crossterm-infinity-loop
          LeftHead  = Pop2(LeftHead);
          RightHead = Pop2(RightHead);
          if(AABBIntersects(&Left->AABB, &Right->AABB))
          {
            LeftHead  = Push2(LeftHead,  Left->Left);
            RightHead = Push2(RightHead, Right->Left);
            LeftHead  = Push2(LeftHead,  Left->Left);
            RightHead = Push2(RightHead, Right->Right);

            LeftHead  = Push2(LeftHead,  Left->Right);
            RightHead = Push2(RightHead, Right->Left);
            LeftHead  = Push2(LeftHead,  Left->Right);
            RightHead = Push2(RightHead, Right->Right);
          }
        }
      }
    }
  }
  return ResultHead;
}


}



