#include "ecs/systems/system_position.h"
#include "platform/jwin_platform.h"

extern ecs::position::system* GlobalPositionSystem;
extern ecs::entity_manager* GlobalEntityManager;

namespace ecs { 
namespace position {


system CreatePositionSystem() {
  system Result = {};
  Result.Positions = cmn::n_tree<component*>::Create();
  Result.Positions.NewNode();
  return Result;
}

void InitiatePosition(v3 Position, r32 Angle, v3 Axis, v3 Scale, ecs::entity_id* EntityID, ecs::entity_id* ParentEntityID)
{
  if(!HasComponents(GlobalEntityManager, EntityID,  ecs::flag::POSITION))
  {
    ecs::NewComponents(GlobalEntityManager, EntityID, ecs::flag::POSITION);
  }
  ecs::position::component* PositionComponent = GetPositionComponent(EntityID);
  if(ParentEntityID)
  {
    Assert(HasComponents(GlobalEntityManager, ParentEntityID,  ecs::flag::POSITION));
    ecs::position::component* ParentPosition = GetPositionComponent(ParentEntityID);
    Assert(ParentPosition->Node);
    PositionComponent->Node = GlobalPositionSystem->Positions.NewNode(ParentPosition->Node, PositionComponent);
  }else{
    PositionComponent->Node = GlobalPositionSystem->Positions.NewNode(GlobalPositionSystem->Positions.m_root, PositionComponent);
  }
  ecs::position::Set(PositionComponent, Position, Angle, Axis, Scale);
}

struct position_update_helper {
  world_coordinate AbsolutePosition;
  quat AbsoluteRotation;
  m4 T;
};

void UpdatePositions()
{
  position_tree* PositionTree = &GlobalPositionSystem->Positions;
  
  cmn::vector<position_update_helper> TransformVec = cmn::vector<position_update_helper>::CreateTransient(PositionTree->MaxDepth());
  cmn::n_tree_pre_order_it<component*> It = PositionTree->PreOrderIterator();
  while(position_node* Node = It.Next())
  {
    int Index = It.Depth() - 1;
    position_update_helper& CurrentPosition = TransformVec[Index];
    if(Index == 0){
      // The Root node has no data
      Assert(!Node->Data);
      CurrentPosition.AbsolutePosition = V3(0,0,0);
      CurrentPosition.AbsoluteRotation = Quaternion();
      CurrentPosition.T = M4Identity();
    }else{
      component* Component = *Node->Data;
      position_update_helper& ParentPosition = TransformVec[Index-1];


      m4 Trans = GetTranslationMatrix(V4(Component->RelativePosition,1));
      m4 Rot = QuaternionAsMatrix(Component->RelativeRotation);
      CurrentPosition.T = ParentPosition.T * Trans* Rot;

      Component->AbsolutePosition = V3(Column(CurrentPosition.T, 3));
      Component->AbsoluteRotation = QuaternionFromMatrix(CurrentPosition.T);
      //CurrentPosition.AbsolutePosition = Component->AbsolutePosition;
      //CurrentPosition.AbsoluteRotation = Component->AbsoluteRotation;
    }
  }
}

} // namespace position
} // namespace ecs