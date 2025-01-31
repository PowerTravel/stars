#include "ecs/components/component_position.h"
#include "platform/jwin_platform.h"
namespace ecs::position {

void InsertPositionNode(component* PositionComponent, position_node* Parent, position_node* Child)
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

void InitiatePositionComponent(component* PositionComponent, world_coordinate Position, r32 Rotation)
{
  Assert(!PositionComponent->FirstChild);

  PositionComponent->NodeCount = 1;
  PositionComponent->FirstChild = CreatePositionNode(Position, Rotation);
  PositionComponent->FirstChild->PositionComponent = PositionComponent;
  PositionComponent->Dirty = true;
}

component* GetPositionComponentFromNode(position_node const * Node)
{
  Assert(Node->PositionComponent);
  return Node->PositionComponent;
}

world_coordinate GetPositionRelativeTo(position_node const * Node, world_coordinate Position)
{
  world_coordinate Result = Position - Node->AbsolutePosition;
  return Result;
}

world_coordinate GetPositionRelativeTo(component const * PositionComponent, world_coordinate Position)
{
  world_coordinate Result = GetPositionRelativeTo(PositionComponent->FirstChild, Position);
  return Result;
}

world_coordinate GetAbsolutePosition(position_node const * PositionNode)
{
  world_coordinate Result = PositionNode->AbsolutePosition;
  return Result;
}

world_coordinate GetAbsolutePosition(component const * PositionComponent)
{
  world_coordinate Result = GetAbsolutePosition(PositionComponent->FirstChild);
  return Result;
}

r32 GetAbsoluteRotation(position_node const * PositionNode)
{
  r32 Result = PositionNode->AbsoluteRotation;
  return Result;
}
r32 GetAbsoluteRotation(component const * PositionComponent)
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
void ClearPositionComponent(component* PositionComponent)
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

} 