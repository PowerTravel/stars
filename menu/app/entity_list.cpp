#include "entity_list.h"

namespace imgui { 
namespace app {
#if 0

Functions which were used when rendering old entity list. 
Can be useful to keep here for reference

void DrawIcon(b32 RowOpen, v4 TexCoord, rect2f RowRect) {
  rect2f IconRect = Rect2f(RowRect.X, RowRect.Y, RowRect.H, RowRect.H);
  render::DrawIconCanonicalSpace(CenteredRect(IconRect), TexCoord, V4(1,1,1,1));
}

void DrawText(ecs::entity_node* EntityNode, v2 TextPos, rect2f ButtonRect){
  char SuffixBuff[16] = {};
  ecs::entity* Entity = *EntityNode->Data;
  FormatString(SuffixBuff, sizeof(SuffixBuff)-1, " (%d)", EntityNode->ChildCount);
  char ButtonBuff[128] = {};
  FormatString(ButtonBuff, sizeof(ButtonBuff)-1, "%s%s", Entity->Name, EntityNode->ChildCount > 0 ? SuffixBuff : "");
  render::DrawTextCanonicalSpace(TextPos, ButtonRect, GlobalState->ImguiContext.FontSize, (utf8_byte const *) ButtonBuff, V4(1.0,1.0,1.0,1.0));
}


float PosXToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  return Position->RelativePosition.X;
}

void StoreXPos(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  v3 Pos = Position->RelativePosition;
  Pos.X = Val;
  ecs::position::Set(Position, Pos, Position->RelativeRotation, Position->Scale);
}

float PosYToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  return Position->RelativePosition.Y;
}

void StoreYPos(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  v3 Pos = Position->RelativePosition;
  Pos.Y = Val;
  ecs::position::Set(Position, Pos, Position->RelativeRotation, Position->Scale);
}

float PosZToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  return Position->RelativePosition.Z;
}

void StoreZPos(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  v3 Pos = Position->RelativePosition;
  Pos.Z = Val;
  ecs::position::Set(Position, Pos, Position->RelativeRotation, Position->Scale);
}

float RollToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  return EulerAngle.Roll * 180.f/Pi32;
}

void StoreRoll(float Val, void* Data)
{
  r32 Roll = Val * Pi32 / 180.f;
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  EulerAngle.Roll = Roll;
  ecs::position::Set(Position, Position->RelativePosition, EulerAngle, Position->Scale);
}

float YawToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  return EulerAngle.Yaw * 180.f/Pi32;
}

void StoreYaw(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  EulerAngle.Yaw = Val * Pi32 / 180.f;
  ecs::position::Set(Position, Position->RelativePosition, EulerAngle, Position->Scale);
}

float PitchToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  return EulerAngle.Pitch * 180.f/Pi32;
}

void StorePitch(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  EulerAngle.Pitch = Val * Pi32 / 180.f;
  ecs::position::Set(Position, Position->RelativePosition, EulerAngle, Position->Scale);
}

#endif

entity_list* CreateEntityList(memory_arena* Arena){
  entity_list* Result = PushStruct(Arena, entity_list);
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(GlobalImguiContext->FontSize);
  Result->BorderWindow      = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  Result->VerticalScrollbar = CreateVerticalScrollbar();
  Result->EntityTree     = me_tree::Create();
  Result->EntityTree.NewNode(); // EmptyRoot
  return Result;
}

file_local entity_row MenuEntityRow(ecs::entity_id EntityID)
{
  entity_row NewRow = {};
  NewRow.EntityID = EntityID;
  NewRow.ImguiID = NewButtonID();
  NewRow.Open = false;
  u32 ComponentCount = ecs::GetComponentCount(GetEntityManager(), &EntityID);
  NewRow.ComponentImguiIDs = cmn::vector<entity_component_id>::Create(ComponentCount);
  
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::POSITION))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::POSITION));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::GEOMETRY))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::GEOMETRY));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::MATERIAL))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::MATERIAL));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::COLLIDER))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::COLLIDER));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::LIGHT))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::LIGHT));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::CAMERA))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::CAMERA));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::CONTROLLER))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::CONTROLLER));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::RENDER))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::RENDER));
  }
  return NewRow;
}

void PushNewEntity(me_tree* MenuTree, me_node* MenuParent, ecs::entity_id* NewEntity)
{
  // Just making sure that the parent of NewEntity in the entity Manager is the same as MenuParent
  //Assert(ecs::Compare(&(*ecs::GetEntityFromID(GetEntityManager(), NewEntity)->Node->Parent->Data)->ID, &MenuParent->Data->EntityID));

  me_node* Root = MenuParent;
  me_node* Node = MenuParent->FirstChild;
  if(Node)
  {
    do
    {
      if(ecs::Compare(&Node->Data->EntityID, NewEntity)){
        return;
      }
      Node = Node->NextSibling;
    }while((Node && Node != MenuParent->FirstChild));
  }

  ecs::entity* E = GetEntityFromID(GetEntityManager(), NewEntity);
  entity_row NewRow = MenuEntityRow(*NewEntity);
  MenuTree->NewNode(MenuParent, NewRow);
}

inline file_local b32 EntityHasChildren(ecs::entity_node* EntityNode){
  b32 Result = EntityNode->FirstChild != 0;
  return Result;
}


file_local void GetEntityName(ecs::entity_node* EntityNode, size_t BuffLen, char TextBuff[])
{
  char SuffixBuff[16] = {};
  ecs::entity* Entity = *EntityNode->Data;
  FormatString(SuffixBuff, sizeof(SuffixBuff)-1, " (%d)", EntityNode->ChildCount);
  FormatString(TextBuff, BuffLen-1, "%s%s", Entity->Name, EntityNode->ChildCount > 0 ? SuffixBuff : "");
}

file_local imgui_row CreateEntityRow(entity_row* MenuRowData, render::font& Font, r32 FontSize, r32 XOffset, r32 IconSize){
   
  ecs::entity_id* EntityID = &MenuRowData->EntityID;
  ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
  ecs::entity_node* EntityNode = Entity->Node;

  imgui_row RowRenderer = {};
  RowRenderer.Push(imgui_row::Padding(XOffset, 0));
  v2 IconPaddingSize = PixelToCanonicalSpace(V2(IconSize,IconSize));
  if(EntityHasChildren(EntityNode))
  {
    RowRenderer.Push(imgui_row::Icon(IconSize, MenuRowData->Open ? ICON_ANGLE_DOWN : ICON_ANGLE_RIGHT ), MenuRowData->ImguiID);
  }else{
    RowRenderer.Push(imgui_row::Padding(IconPaddingSize.X, IconPaddingSize.Y));
  }

  char NameBuffer[128] = {};
  GetEntityName(EntityNode,sizeof(NameBuffer),NameBuffer);
  RowRenderer.Push(imgui_row::Text(FontSize, &Font, sizeof(NameBuffer), NameBuffer));
  RowRenderer.Push(imgui_row::DivHint(XOffset+IconPaddingSize.X));
  for (int i = 0; i < MenuRowData->ComponentImguiIDs.Size(); ++i)
  {
    entity_component_id* ComponentID = &MenuRowData->ComponentImguiIDs[i];
    switch(ComponentID->Type)
    {
      case ecs::flag::POSITION: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_LOCATION ), ComponentID->ImguiID);
      } break;
      case ecs::flag::GEOMETRY: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_GEOMETRY ), ComponentID->ImguiID);
      } break;
      case ecs::flag::MATERIAL: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_MATERIAL ), ComponentID->ImguiID);
      } break;
      case ecs::flag::COLLIDER: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_COLLIDER ), ComponentID->ImguiID);
      } break;
      case ecs::flag::LIGHT: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_LIGHT ), ComponentID->ImguiID);
      } break;
      case ecs::flag::CAMERA: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_CAMERA ), ComponentID->ImguiID);
      } break;
      case ecs::flag::CONTROLLER: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_CONTROLLER ), ComponentID->ImguiID);
      } break;
      case ecs::flag::RENDER: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_UNKNOWN ), ComponentID->ImguiID);
      } break;
    }
  }

  return RowRenderer;
}


file_local void AddChildEntitiesLoadedToMenuTree(me_tree& MenuTree, me_node* MenuNode, ecs::entity_tree& EntityTree, ecs::entity_node* EntityNode){
  ecs::entity_node* EntityChild = EntityNode->FirstChild;
  do
  {
    // Is it smart to edit the me_tree while we are iterating through it....? I don't feel confident
    entity_row NewRow = MenuEntityRow((*EntityChild->Data)->ID);
    MenuTree.NewNode(MenuNode, NewRow);
    EntityChild = EntityChild->NextSibling;
  }while(EntityChild != EntityNode->FirstChild);
}


file_local void DoEntityButtonRect(context* ImguiContext, entity_row* MenuRowData) {
  if(ImguiIsActive(MenuRowData->ImguiID) && ImguiIsHot(MenuRowData->ImguiID) && jwin::Released(ImguiContext->LeftMouse))
  {
    MenuRowData->Open = !MenuRowData->Open;
  }
}

file_local cmn::vector<imgui_row> BuildTransientImguiRows(me_tree& MenuTree, ecs::entity_tree& EntityTree)
{
  render::font& Font = GlobalRenderer->Font;
  r32 TabWidth = Font.GetTextSizeCanonicalSpace(GlobalImguiContext->FontSize, (const utf8_byte*) "  ").X;

  cmn::vector<imgui_row> Result = cmn::vector<imgui_row>::CreateTransient(EntityTree.NodeCount());
  bool SkipSubTree = false;
  me_iterator It = MenuTree.PreOrderIterator(EntityTree.NodeCount());
  const r32 IconSize = 20;
  while(me_node* MenuNode = It.Next(SkipSubTree))
  {
    int Index = It.Depth() - 1;
    if(Index != 0){
      entity_row* MenuRowData = MenuNode->Data;
      r32 XOffset = (Index-1)*TabWidth;

      imgui_row RowRenderer = CreateEntityRow(MenuRowData, Font, GlobalImguiContext->FontSize, XOffset, IconSize);
      SkipSubTree = !MenuRowData->Open;
      Result.PushBack(RowRenderer);
    }
  }
  return Result;
}

struct list_add_helper_struct {
  me_node* MenuNode;
  ecs::entity_node* EntityNode;
};

file_local void PushNewlyOpenedChildEntities(me_tree& MenuTree, ecs::entity_tree& EntityTree)
{
  SCOPED_TRANSIENT_ARENA;
  cmn::list<list_add_helper_struct> NodesToAdd = cmn::list<list_add_helper_struct>::CreateTransient();
  me_iterator It = MenuTree.PreOrderIterator(EntityTree.NodeCount());
  bool SkipSubTree = false;
  while(me_node* MenuNode = It.Next(SkipSubTree))
  {
    int Index = It.Depth() - 1;
    if(Index != 0){
      entity_row* MenuRowData = MenuNode->Data;
      DoEntityButtonRect(GlobalImguiContext, MenuRowData);
      ecs::entity_id* EntityID = &MenuRowData->EntityID;
      ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
      ecs::entity_node* EntityNode = Entity->Node;
      if(MenuRowData->Open && !MenuNode->FirstChild && EntityNode->FirstChild)
      {
        SkipSubTree = !MenuRowData->Open;
        list_add_helper_struct NodesHelper = {};
        NodesHelper.MenuNode = MenuNode;
        NodesHelper.EntityNode = EntityNode;
        NodesToAdd.PushBack(NodesHelper);
      }
    }
  }

  cmn::list<list_add_helper_struct>::element* ListElement = NodesToAdd.First();
  while(!NodesToAdd.IsEnd(ListElement))
  {
    list_add_helper_struct NodesHelper = ListElement->GetCopy();
    AddChildEntitiesLoadedToMenuTree(MenuTree, NodesHelper.MenuNode, EntityTree, NodesHelper.EntityNode);
    ListElement = ListElement->Next;
  }
}



file_local v2 ImguiEntityComponentTree(entity_list* EntityList, rect2f WindowRegion) {
  SCOPED_TRANSIENT_ARENA;

  ecs::entity_tree& EntityTree = GlobalEntityManager->EntityTree;
  me_tree& MenuTree = EntityList->EntityTree;

  cmn::vector<imgui_row> ImguiRows = BuildTransientImguiRows(MenuTree, EntityTree);

  r32 ScrollbarWidth = 0.01;
  rect2f ContentRect = Rect2f(WindowRegion.X, WindowRegion.Y, WindowRegion.W-ScrollbarWidth, WindowRegion.H);
  rect2f ScrollbarRect = Rect2f(ContentRect.X + WindowRegion.W-ScrollbarWidth, ContentRect.Y, ScrollbarWidth, WindowRegion.H);

  // Reactive size keeps a stack of all the sizes of all rects for the the elements in ImguiRows.
  // They are positioned such that the bottom of the list is at Lower Left in canonical cooordinates (0,0)
  reactive_size ReactiveSizes = CreateReactiveSize(ImguiRows, ContentRect, 0);  

  b32 ScrollbarVisible = ReactiveSizes.TotalSize.Y >= WindowRegion.H;
  if(!ScrollbarVisible)
  {
    // Drawing the list takes: 
    //  the ReactiveSizes keeping the outline of the list,
    //  a region to draw in where the list is visible
    //  and a scroll ammount where
    //    0 says the top of the list should be at the top of the region
    //    1 says the bot of the list should be at the bot of the region
    // Note: - An issue with this is that the scroll ammount is the only thing telling us where to draw the list.
    //       If the list is collapsed such that before it was larger than the region and after it is smaller, the scroll ammount won't have 
    //       changed and the list rendering breaks.
    //       - A quick and hacky way to solve this (done below) is to set the scroll ammount to 0 if the list size is smaller than the region.
    //       An issue with this solution is that if the list changes size it moves around a bit in the region as the list is fixed at a percentage position interpolated from the scroll amount.
    //       - What we maybe want to do is to instead of using the scroll ammount to position the list is to have a rownumber and row offset 
    //       stored in entity_tree which makes sure that no matter how the list size changes, the first (top) part of the list which is
    //       drawn is always the same. However this makes the interaction with the scroll ammount a bit iffy.
    //       However this is good enough for now.
    EntityList->VerticalScrollbar.ScrollAmmount.Y = 0;
    DrawRowList(ReactiveSizes, ImguiRows, WindowRegion, EntityList->VerticalScrollbar.ScrollAmmount.Y);
  }else{
    DrawRowList(ReactiveSizes, ImguiRows, ContentRect, EntityList->VerticalScrollbar.ScrollAmmount.Y);
    b32 MouseScrollActive = Intersects(WindowRegion, V2(GlobalImguiContext->MouseX, GlobalImguiContext->MouseY));
    DoVerticalScrollbar(&EntityList->VerticalScrollbar, ScrollbarRect, MouseScrollActive, ReactiveSizes.TotalSize.Y);
  }

  // NOTE: There is a bug here which causes a crash when expanding the enityt-list. Especially after hot-reloading.
  //       I think this is because I change the tree while iterating. It's done very naively.
  //       A solution can be to either
  //        1 Load the entire entity tree to the MenuTree so expanding and contracting the list doesnt change the tree.
  //        2 Cache all the entitites to be added and add them after iterating.
  // NOTE2: I implemented solution number 2. I encountered the bug immediately after 'fixing it' but never again since.
  //        Maybe I fixed it, and I didnt compile properly the one time it failed. Idk, keeping theese notes around a bit
  //        longer in case it happens again. 
  PushNewlyOpenedChildEntities(MenuTree, EntityTree);
  
  return ReactiveSizes.TotalSize;
}

void DrawEntityTree(menu* Menu) {

  entity_list* EntityList = Menu->EntityList;

  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(GlobalState->ImguiContext.FontSize);
  
  rect2f ContentRect = GetContentRect(&EntityList->BorderWindow);

  v2 ScrollListPos  = LowerLeftPoint(ContentRect);
  v2 ScrollListSize = RectSize(ContentRect);

  DoImguiBorderWindow(&EntityList->BorderWindow, "Entities");

  v2 TotalSize = ImguiEntityComponentTree(EntityList, ContentRect);
}

void LoadNewEntitiesToEntityList()
{
  ecs::entity_node* TopLevelEntity = GlobalEntityManager->EntityTree.m_root->FirstChild; 
  if(TopLevelEntity)
  {
    do
    {
      ecs::entity* Entity = *TopLevelEntity->Data;
      imgui::app::me_tree* MenuTree = &GlobalState->ApplicationMenu.EntityList->EntityTree;
      imgui::app::me_node* Root = MenuTree->m_root;
      PushNewEntity(MenuTree, Root, &Entity->ID);
      TopLevelEntity = TopLevelEntity->NextSibling;
    }while(TopLevelEntity != GlobalEntityManager->EntityTree.m_root->FirstChild);
  }
}

} // namespace app
} // namespace imgui