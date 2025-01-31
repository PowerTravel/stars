#include "ecs/systems/system_position.h"
#include "platform/jwin_platform.h"

namespace ecs::system{

using namespace component;

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
void UpdateAbsolutePosition(memory_arena* Arena, position* Position)
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
  position* PositionComponent = GetPositionComponentFromNode(PositionNode);
  UpdateAbsolutePosition(Arena, PositionComponent);
}

void UpdatePositions(entity_manager* EntityManager)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EntityManager, flag::POSITION);

  while(Next(&EntityIterator))
  {
    position* Position = GetPositionComponent(&EntityIterator);
    if(Position->Dirty)
    {
      UpdateAbsolutePosition(GlobalTransientArena, Position);  
    }
  }
}

}