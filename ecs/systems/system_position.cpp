#include "ecs/systems/system_position.h"
#include "platform/jwin_platform.h"

extern ecs::entity_manager* GlobalEntityManager;

namespace ecs { 
namespace position {

void UpdatePositions()
{
  ecs::entity_tree* EntityTree = &GlobalEntityManager->EntityTree;
  cmn::vector<m4> TransformVec = cmn::vector<m4>::CreateTransient(EntityTree->MaxDepth());
  cmn::n_tree_pre_order_it<entity*> It = EntityTree->PreOrderIterator();
  while(entity_node* Node = It.Next())
  {
    int Index = It.Depth() - 1;
    m4& CurrentTransform = TransformVec[Index];
    if(Index == 0){
      // The Root node has no data
      Assert(!Node->Data);
      CurrentTransform = M4Identity();
    }else{
      entity* Entity = *Node->Data;
      component* Component = GetPositionComponent(Entity);
      m4& ParentTransform = TransformVec[Index-1];
      if(Component) {
        m4 Trans = GetTranslationMatrix(V4(Component->RelativePosition,1));
        m4 Rot = QuaternionAsMatrix(Component->RelativeRotation);
        CurrentTransform = ParentTransform * Trans* Rot;

        Component->AbsolutePosition = V3(Column(CurrentTransform, 3));
        Component->AbsoluteRotation = QuaternionFromMatrix(CurrentTransform);
      }else{
        TransformVec[Index] = TransformVec[Index-1];
      }
    }
  }
}

} // namespace position
} // namespace ecs