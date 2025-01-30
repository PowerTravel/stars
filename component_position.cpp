#include "component_position.h"
#include "platform/jwin_platform.h"

void InsertPositionNode(component_position* PositionComponent, position_node* Parent, position_node* Child)
{
  PositionComponent->NodeCount++;
  Assert(Parent->PositionComponent);
  Child->PositionComponent = PositionComponent;
  if(Parent->FirstChild)
  {
    position_node* Tmp = Parent->FirstChild;
    Parent->FirstChild = Child;
    Child->NextSibling = Tmp;
    Child->Parent = Parent;
  }else{
    Parent->FirstChild = Child;
    Child->Parent = Parent;
  }

  PositionComponent->Dirty = true;
}

position_node* CreatePositionNode(world_coordinate Position, r32 Rotation)
{
  position_node* Result = (position_node*) GetNewBlock(GlobalPersistentArena, &GlobalState->World.PositionNodes);
  Result->RelativePosition = Position;
  Result->RelativeRotation = Rotation;
  return Result;
}

void InitiatePositionComponent(component_position* PositionComponent, world_coordinate Position, r32 Rotation)
{
  Assert(!PositionComponent->FirstChild);

  PositionComponent->NodeCount = 1;
  PositionComponent->FirstChild = CreatePositionNode(Position, Rotation);
  PositionComponent->FirstChild->PositionComponent = PositionComponent;
  PositionComponent->Dirty = true;
}

component_position* GetPositionComponentFromNode(position_node const * Node)
{
  Assert(Node->PositionComponent);
  return Node->PositionComponent;
}

world_coordinate GetPositionRelativeTo(position_node const * Node, world_coordinate Position)
{
  world_coordinate Result = Position - Node->AbsolutePosition;
  return Result;
}

world_coordinate GetPositionRelativeTo(component_position const * PositionComponent, world_coordinate Position)
{
  world_coordinate Result = GetPositionRelativeTo(PositionComponent->FirstChild, Position);
  return Result;
}

world_coordinate GetAbsolutePosition(position_node const * PositionNode)
{
  world_coordinate Result = PositionNode->AbsolutePosition;
  return Result;
}

world_coordinate GetAbsolutePosition(component_position const * PositionComponent)
{
  world_coordinate Result = GetAbsolutePosition(PositionComponent->FirstChild);
  return Result;
}

r32 GetAbsoluteRotation(position_node const * PositionNode)
{
  r32 Result = PositionNode->AbsoluteRotation;
  return Result;
}
r32 GetAbsoluteRotation(component_position const * PositionComponent)
{
  r32 Result = GetAbsoluteRotation(PositionComponent->FirstChild);
  return Result;
}

struct node_queue
{
  u32 Count;
  position_node** Nodes;
};

void Push(node_queue* Queue, position_node* Node)
{
  if(Node)
  {
    Queue->Nodes[Queue->Count++] = Node;
  }
}

position_node* Pop(node_queue* Queue)
{
  position_node* Result = 0;
  if(Queue->Count)
  {
    Result = Queue->Nodes[--Queue->Count];
  }

  return Result;
}


internal inline void UpdateAbsolutePositionFromParent(position_node* Node, world_coordinate ParentPosition, r32 ParentRotation)
{
  Node->AbsolutePosition = ParentPosition + Node->RelativePosition;
  Node->AbsoluteRotation = ParentRotation + Node->RelativeRotation;
  if(Node->AbsoluteRotation > Pi32)
  {
    Node->AbsoluteRotation -= Tau32; 
  }else if(Node->AbsoluteRotation < -Pi32)
  {
    Node->AbsoluteRotation += Tau32; 
  }
}

// Note untested with several siblings
void UpdateAbsolutePosition(memory_arena* Arena, component_position* Position)
{
  if(!Position->Dirty)
  {
    return;
  }

  temporary_memory TempMem = BeginTemporaryMemory(Arena);
  node_queue NodeQueue = {};
  NodeQueue.Nodes = PushArray(Arena, Position->NodeCount, position_node*);

  position_node* Root = Position->FirstChild;

  Assert(!Root->NextSibling);
  Assert(!Root->Parent);

  Push(&NodeQueue, Root);

  while(NodeQueue.Count > 0)
  {
    position_node* Node = Pop(&NodeQueue);

    world_coordinate ParentPosition = {};
    r32 ParentRotation = 0.f;

    if(Node->Parent)
    {
      ParentPosition = Node->Parent->AbsolutePosition;
      ParentRotation = Node->Parent->AbsoluteRotation;
    }

    UpdateAbsolutePositionFromParent(Node, ParentPosition, ParentRotation);

    position_node* Sibling = Node->NextSibling;
    while(Sibling)
    {
      Push(&NodeQueue, Sibling);
      Sibling = Sibling->NextSibling;
    }

    Push(&NodeQueue, Node->FirstChild);
  }

  Position->Dirty = false;
  EndTemporaryMemory(TempMem);
}

void UpdateAbsolutePosition(memory_arena* Arena, position_node* PositionNode)
{
  component_position* PositionComponent = GetPositionComponentFromNode(PositionNode);
  UpdateAbsolutePosition(Arena, PositionComponent);
}

void PositionSystemUpdate(entity_manager* EntityManager)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EntityManager, COMPONENT_FLAG_POSITION);

  while(Next(&EntityIterator))
  {
    component_position* Position = GetPositionComponent(&EntityIterator);
    if(Position->Dirty)
    {
      UpdateAbsolutePosition(GlobalTransientArena, Position);  
    }
  }
}

void SetRelativePosition(position_node* Node, world_coordinate Position, r32 Rotation)
{
  Node->RelativePosition = Position;
  Node->RelativeRotation = Rotation;
  if(Node->RelativeRotation > Pi32)
  {
    Node->RelativeRotation -= Tau32; 
  }else if(Node->RelativeRotation < Pi32)
  {
    Node->RelativeRotation += Tau32; 
  }

  Node->PositionComponent->Dirty = true;
}

// Note untested with several siblings
void ClearPositionComponent(component_position* PositionComponent)
{
  temporary_memory TempMem = BeginTemporaryMemory(GlobalTransientArena);

  chunk_list* PositionNodeList = &GlobalState->World.PositionNodes;

  node_queue NodeQueue = {};
  NodeQueue.Nodes = PushArray(GlobalTransientArena, PositionComponent->NodeCount, position_node*);

  position_node* Root = PositionComponent->FirstChild;

  Assert(!Root->NextSibling);
  Assert(!Root->Parent);

  Push(&NodeQueue, Root);

  while(NodeQueue.Count > 0)
  {
    position_node* Node = Pop(&NodeQueue);
    position_node* Sibling = Node->NextSibling;
    while(Sibling)
    {
      Push(&NodeQueue, Sibling);
      Sibling = Sibling->NextSibling;
    }

    Push(&NodeQueue, Node->FirstChild);

    FreeBlock(PositionNodeList, (bptr) Node);
  }
  EndTemporaryMemory(TempMem);
}