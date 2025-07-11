#include "dynamic_aabb_tree.h"
#include "ecs/components/component_collider.h"

inline internal bool
IsLeaf( aabb_tree_node* Node )
{
  b32 Result = IsValid(&Node->EntityID);
  return Result;
}

inline internal midx
Size(aabb_tree_node** Base,
     aabb_tree_node** Head)
{
  Assert(Head >= Base);
  return (Head - Base);
}

inline internal aabb_tree_node**
Push(aabb_tree_node** Head,
     aabb_tree_node*  Node)
{
  ++Head;
  *Head = Node;
  return Head;
}

inline internal aabb_tree_node**
Pop(aabb_tree_node** Head)
{
  --Head;
  return Head;
}

inline internal aabb_tree_node**
Push2(aabb_tree_node** Head,
      aabb_tree_node*  Node)
{
  Head+=2;
  *Head = Node;
  return Head;
}

inline internal aabb_tree_node**
Pop2(aabb_tree_node** Head)
{
  Head-=2;
  return Head;
}

inline internal aabb_tree_node*
Get(aabb_tree_node** Head)
{
  return *Head;
}

u32 GetAABBList(aabb_tree* Tree, aabb3f** Result)
{
  aabb_tree_node** const Base = PushArray(GlobalTransientArena, Tree->NodeCount, aabb_tree_node*);
  aabb_tree_node** Head = Base;

  *Result = PushArray(GlobalTransientArena, Tree->NodeCount, aabb3f);
  aabb3f* AABBList = *Result;

  u32 Count = 0;

  // Init the stacks
  aabb_tree_node* CurrentNode = Tree->Root;
  while( CurrentNode || (Size(Base,Head) > 0) )
  {
    while(CurrentNode)
    {
      Head = Push(Head, CurrentNode);
      CurrentNode = CurrentNode->Left;
    }
    CurrentNode = Get(Head);
    Head  = Pop(Head);

    aabb_tree_node* ParentNode = Get(Head);

    *AABBList++ = CurrentNode->AABB;
    Assert(Count < Tree->NodeCount);
    Count++;

    CurrentNode = CurrentNode->Right;
  }

  return Tree->NodeCount;
}

broad_phase_result_stack* GetCollisionPairs( aabb_tree* Tree, u32* ResultStackSize)
{
  *ResultStackSize = 0;
  if(Tree->NodeCount < 2)
  {
    return 0;
  }
  aabb_tree_node** const Base = (aabb_tree_node**) PushArray(GlobalTransientArena, 2*Tree->NodeCount, aabb_tree_node*);
  aabb_tree_node** const LeftBase  = Base;
  aabb_tree_node** const RightBase = Base+1;
  broad_phase_result_stack* ResultHead = {};
  aabb_tree_node** LeftHead  = LeftBase;
  aabb_tree_node** RightHead = RightBase;

  // Init the stacks
  LeftHead  = Push2(LeftHead,  Tree->Root->Left);
  RightHead = Push2(RightHead, Tree->Root->Right);
  while( Size(LeftBase,LeftHead) > 0 )
  {
    Assert(LeftHead - Base < 2*Tree->NodeCount);
    Assert(RightHead - Base < 2*Tree->NodeCount);
    aabb_tree_node* Left  = Get(LeftHead);
    aabb_tree_node* Right = Get(RightHead);

    if( IsLeaf(Left) )
    {
      if( IsLeaf(Right) )
      {
        // Both are leaves
        if(AABBIntersects(&Left->AABB, &Right->AABB))
        {
          broad_phase_result_stack* NewResult = (broad_phase_result_stack*) PushStruct(GlobalTransientArena ,broad_phase_result_stack);
          NewResult->EntityIDA = Left->EntityID;
          NewResult->EntityIDB = Right->EntityID;
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

internal aabb_tree_node**
GetLeastVolumeincreaseBranch(aabb_tree_node* Parent,  aabb3f* LeafAABB)
{
  r32 PreLeftVolume   = GetSize(&Parent->Left->AABB);
  r32 PreRightVolume  = GetSize(&Parent->Right->AABB);
  aabb3f LeftMerge    = MergeAABB(Parent->Left->AABB,  *LeafAABB);
  aabb3f RightMerge   = MergeAABB(Parent->Right->AABB, *LeafAABB);
  r32 PostLeftVolume  = GetSize(&LeftMerge);
  r32 PostRightVolume = GetSize(&RightMerge);
  r32 LeftDiff = PostLeftVolume - PreLeftVolume;
  r32 RightDiff = PostRightVolume - PreRightVolume;

  aabb_tree_node** Result = 0;
  if(LeftDiff <= RightDiff)
  {
    Result = &Parent->Left;
  }else{
    Result = &Parent->Right;
  }

  return Result;
}

void AABBTreeInsert(memory_arena* Arena, aabb_tree* Tree, ecs::entity_id EntityID , aabb3f& AABBWorldSpace )
{
  aabb_tree_node* Leaf = (aabb_tree_node*) PushStruct(Arena, aabb_tree_node);
  Leaf->EntityID = EntityID;
  Leaf->AABB   = AABBWorldSpace;

  if(Tree->NodeCount == 0)
  {
    Tree->Root = Leaf;
    Tree->NodeCount = 1;
    return;
  }

  aabb_tree_node** CurrentNodePtr = &Tree->Root;
  aabb_tree_node*  CurrentNode    = *CurrentNodePtr;
  while(!IsLeaf(CurrentNode))
  {
    CurrentNode->AABB = MergeAABB(CurrentNode->AABB, Leaf->AABB);
    CurrentNodePtr    = GetLeastVolumeincreaseBranch(CurrentNode, &Leaf->AABB);
    CurrentNode       = *CurrentNodePtr;
  }

  // We are at leaf
  aabb_tree_node* Node = (aabb_tree_node*) PushStruct(Arena, aabb_tree_node);
  Node->Left  = Leaf;
  Node->Right = CurrentNode;
  Node->AABB  = MergeAABB(Node->Left->AABB, Node->Right->AABB);
  *CurrentNodePtr = Node;
  Tree->NodeCount+=2;
}

void PointPick( memory_arena* Arena, aabb_tree* Tree, v3* point, v3 direction, vector_list<ecs::entity_id> & Result)
{
  vector_list<aabb_tree_node*> queue = vector_list<aabb_tree_node*>(Arena, Tree->NodeCount);

  if (Tree->Root)
  {
    queue.PushBack(Tree->Root);
  }

  while (!queue.IsEmpty())
  {
    aabb_tree_node* Node = queue.PopFront();

    if (IsLeaf(Node))
    {
      if (AABBIntersects(point, &Node->AABB))
      {
        Result.PushBack(Node->EntityID);
      }
    }
    else
    {
      queue.PushBack(Node->Left);
      queue.PushBack(Node->Right);
    }
  }
}

raycast_result ColliderRaycast( v3 const & RayOrigin, v3 const & RayDirection, ecs::position::component* Position, ecs::collider::component* Collider)
{
  Assert(Equals(Norm(RayDirection),1, 10E-3));
  ecs::collider::mesh Mesh = Collider->Mesh;
  raycast_result Result{};
  Result.Distance = R32Max;
  m4 const ModelMatrix = ecs::position::GetModelMatrix(Position);
  v3 RayDirectionModelSpace = Normalize( V3(AffineInverse(ModelMatrix) * V4(RayDirection,0)));
  v3 RayOriginModelSpace = V3(AffineInverse(ModelMatrix) * V4(RayOrigin,1));

  u32 VerticeIndexCount = 0;
  v3 ResultHitNormal{};
  b32 ResultHit = false;
  while (VerticeIndexCount < Mesh.nvi)
  {
    u32 Index0 = Mesh.vi[VerticeIndexCount++];
    u32 Index1 = Mesh.vi[VerticeIndexCount++];
    u32 Index2 = Mesh.vi[VerticeIndexCount++];
    v3 p0 = Mesh.v[Index0];
    v3 p1 = Mesh.v[Index1];
    v3 p2 = Mesh.v[Index2];
    v3 PlaneNormal = GetPlaneNormal(p0, p1, p2);
    r32 t = RaycastPlane(RayOriginModelSpace, RayDirectionModelSpace, p0, PlaneNormal);
    if (t < 0)
    {
      continue;
    }
    v3 ProjectedPointModelSpace = RayOriginModelSpace + t*RayDirectionModelSpace;
    if (IsVertexInsideTriangle( ProjectedPointModelSpace, PlaneNormal, p0, p1, p2))
    {
      v3 ProjectedPointWorldSpace = V3(ModelMatrix * V4(ProjectedPointModelSpace,1));
      r32 WorldDistance = Norm(ProjectedPointWorldSpace - RayOrigin);
      if (WorldDistance < Result.Distance)
      {
        Result.IntersectionObjectSpace = ProjectedPointModelSpace;
        Result.Intersection = ProjectedPointWorldSpace;
        Result.Distance = WorldDistance;
        Result.HitNormal = V3(Transpose(RigidInverse(ModelMatrix)) * V4(PlaneNormal,0));
        Result.Hit = true;
      }
    }
  }
  return Result;
}

raycast_result RayCast( memory_arena* Arena, aabb_tree* Tree, v3 const & RayOrigin, v3 const & RayDirection )
{
  Assert(Equals(Norm(RayDirection), 1, 10E-3));
  raycast_result Result{};

  vector_list<aabb_tree_node*> queue = vector_list<aabb_tree_node*>(Arena, Tree->NodeCount);
  if (Tree->Root)
  {
    queue.PushBack(Tree->Root);
  }

  while (!queue.IsEmpty())
  {
    aabb_tree_node* Node = queue.PopFront();
    aabb3f AABB = Node->AABB;
    r32 MaxDistance{};
    r32 MinDistance{};
    if (AABBRay(RayOrigin, RayDirection, AABB, &MinDistance, &MaxDistance))
    {
      if(Result.Hit && Result.Distance < MinDistance)
      {
        continue;
      }

      if (IsLeaf(Node))
      {

        ecs::position::component* Position = (ecs::position::component*) GetComponent(GlobalEntityManager, &Node->EntityID, ecs::flag::POSITION);
        ecs::collider::component* Collider = (ecs::collider::component*) GetComponent(GlobalEntityManager, &Node->EntityID, ecs::flag::COLLIDER);

        Assert(Collider);
        raycast_result ColliderResult = ColliderRaycast(RayOrigin, RayDirection, Position, Collider);
        if(ColliderResult.Hit)
        {
          if (Result.Hit)
          {
            if (ColliderResult.Distance < Result.Distance)
            {
              Result = ColliderResult;
              Result.EntityID = Node->EntityID;
            }
          }
          else
          {
            // First Hit
            Result = ColliderResult;
            Result.EntityID = Node->EntityID;
          }
        }
      }
      else
      {
        queue.PushBack(Node->Left);
        queue.PushBack(Node->Right);
      }
    }
  }

  Result.RayOrigin = RayOrigin;
  Result.RayDirection = RayDirection;
  return Result;
}

aabb_tree BuildBroadPhaseTree()
{
  aabb_tree Result = {};
  ecs::filtered_entity_iterator EntityIterator = GetComponentsOfType(GlobalEntityManager, ecs::flag::COLLIDER);
  while(ecs::Next(&EntityIterator))
  {
    ecs::entity_id EntityID = ecs::GetEntityID(&EntityIterator);
    ecs::position::component* Position = (ecs::position::component*) ecs::GetComponent(GlobalEntityManager, &EntityIterator, ecs::flag::POSITION);
    ecs::collider::component* Collider = (ecs::collider::component*) ecs::GetComponent(GlobalEntityManager, &EntityIterator, ecs::flag::COLLIDER);
    m4 ModelMatrix = ecs::position::GetModelMatrix(Position);
    aabb3f AABBWorldSpace = TransformAABB(Collider->AABB, ModelMatrix );
    // TODO: Don't do a insert every timestep. Update an existing tree
    AABBTreeInsert(GlobalTransientArena, &Result, EntityID, AABBWorldSpace );
  }

  return Result;
}
