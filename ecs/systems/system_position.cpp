#include "ecs/systems/system_position.h"
#include "platform/jwin_platform.h"

extern ecs::entity_manager* GlobalEntityManager;

namespace ecs { 
namespace position {

void UpdatePositions()
{
  ecs::entity_tree* EntityTree = &GlobalEntityManager->EntityTree;
  cmn::vector<m4> TransformVec1 = cmn::vector<m4>::CreateTransient(EntityTree->MaxDepth());
  cmn::vector<m4> TransformVec2 = cmn::vector<m4>::CreateTransient(EntityTree->MaxDepth());
  cmn::n_tree_pre_order_it<entity*> It = EntityTree->PreOrderIterator();
  while(entity_node* Node = It.Next())
  {
    int Index = It.Depth() - 1;
    m4& CurrentTransform1 = TransformVec1[Index];
    m4& CurrentTransform2 = TransformVec2[Index];
    if(Index == 0){
      // The Root node has no data
      Assert(!Node->Data);
      CurrentTransform1 = M4Identity();
      CurrentTransform2 = M4Identity();
    }else{
      entity* Entity = *Node->Data;
      component* Position = GetPositionComponent(Entity);
      m4& ParentTransform1 = TransformVec1[Index-1];
      m4& ParentTransform2 = TransformVec2[Index-1];
      if(Position) {
        m4 Trans = GetTranslationMatrix(V4(Position->RelativePosition,1));
        m4 Rot = QuaternionAsMatrix(Position->RelativeRotation);
        CurrentTransform1 = ParentTransform1 * Trans * Rot;
        Position->AbsolutePosition = V3(Column(CurrentTransform1, 3));
        Position->AbsoluteRotation = QuaternionFromMatrix(CurrentTransform1);

        CurrentTransform2 = ParentTransform2 * Position->T;
        Position->gT = CurrentTransform2;
      }else{
        CurrentTransform1 = ParentTransform1;
        CurrentTransform2 = ParentTransform2;
      }
    }
  }
}

} // namespace position
} // namespace ecs