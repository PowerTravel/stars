#include "application_imgui_entity_list.h"


file_local imgui_row CreateEntityRow(menu_entity_row* MenuRowData, render::font& Font, r32 FontSize, r32 XOffset, r32 RowHeight, r32 IconSize){
   
  ecs::entity_id* EntityID = &MenuRowData->EntityID;
  ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
  ecs::entity_node* EntityNode = Entity->Node;

  imgui_row RowRenderer = {};
  RowRenderer.Push(imgui_row::Padding(XOffset,RowHeight));
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

  for (int i = 0; i < MenuRowData->ComponentImguiIDs.Size(); ++i)
  {
    RowRenderer.Push(imgui_row::DivHint(XOffset+IconPaddingSize.X));
    menu_entity_component_id* ComponentID = &MenuRowData->ComponentImguiIDs[i];
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



file_local v2 ImguiEntityComponentTree(menu_entity_tree* MenuEntityTree, v2 Pos, v2 Size) {
  SCOPED_TRANSIENT_ARENA;
  imgui_context* ImguiContext = &GlobalState->ImguiContext;
  imgui_button_color ButtonColor = {};
  ButtonColor.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "taupe");
  ButtonColor.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "persian indigo");
  ButtonColor.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "egyptian blue");
  ButtonColor.HotColor =  menu::GetColor(&GlobalState->ColorTable, "rich black");

  render::font& Font = GlobalRenderer->Font;

  v2 Padding = PixelToCanonicalSpace(V2(2,2));

  r32 FontSize = ImguiContext->FontSize;
  r32 RowHeight = Font.GetLineSpacingCanonicalSpace(FontSize);
  r32 DescentOffset = Font.GetCanonicalFontDescenOffset(FontSize);
  r32 TabWidth  = Font.GetTextSizeCanonicalSpace(FontSize, (utf8_byte const *) "  ").X;

  ecs::entity_tree& EntityTree = GlobalEntityManager->EntityTree;
  me_tree& MenuTree = MenuEntityTree->EntityTree;
  cmn::vector<imgui_row> ImguiRows = cmn::vector<imgui_row>::CreateTransient(EntityTree.NodeCount());
  {
    bool SkipSubTree = false;
    me_iterator It = MenuEntityTree->EntityTree.PreOrderIterator(EntityTree.NodeCount());
    const r32 IconSize = 32;
    while(me_node* MenuNode = It.Next(SkipSubTree))
    {
      int Index = It.Depth() - 1;
      if(Index != 0){
        menu_entity_row* MenuRowData = MenuNode->Data;
        r32 XOffset = (Index-1)*RowHeight;

        imgui_row RowRenderer = CreateEntityRow(MenuRowData, Font, ImguiContext->FontSize, XOffset, RowHeight, IconSize);
        SkipSubTree = !MenuRowData->Open;
        ImguiRows.PushBack(RowRenderer);
      }
    }
  }
  r32 ScrollbarWidth = 0.03;
  rect2f WindowRegion = Rect2f(Pos,Size);
  rect2f ContentRect = Rect2f(Pos,V2(Size.X-ScrollbarWidth,Size.Y));

  reactive_size ReactiveSizes = CreateReactiveSize(ImguiRows, ContentRect, 0);
  
  r32 SizePercentage = Size.Y / ReactiveSizes.TotalSize.Y;
  rect2f ScrollbarRect = Rect2f(Pos.X + Size.X-ScrollbarWidth, Pos.Y, ScrollbarWidth, Size.Y);
  
  b32 MouseScrollActive = Intersects(WindowRegion, V2(GlobalImguiContext->MouseX, GlobalImguiContext->MouseY));
  DoVerticalScrollbar(&MenuEntityTree->VerticalScrollbar, ScrollbarRect, MouseScrollActive, ReactiveSizes.TotalSize.Y);


  DrawRowList(ReactiveSizes, ImguiRows, ContentRect);


  {
    me_iterator It = MenuEntityTree->EntityTree.PreOrderIterator(EntityTree.NodeCount());
    bool SkipSubTree = false;
    while(me_node* MenuNode = It.Next(SkipSubTree))
    {
      int Index = It.Depth() - 1;
      if(Index != 0){
        menu_entity_row* MenuRowData = MenuNode->Data;

        DoEntityButtonRect(ImguiContext, MenuRowData);
        ecs::entity_id* EntityID = &MenuRowData->EntityID;
        ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
        ecs::entity_node* EntityNode = Entity->Node;
        if(MenuRowData->Open && !MenuNode->FirstChild && EntityNode->FirstChild)
        {
          SkipSubTree = !MenuRowData->Open;
          AddChildEntitiesLoadedToMenuTree(MenuTree, MenuNode, EntityTree, EntityNode);
        }
      }
    }
  }

  return ReactiveSizes.TotalSize;
}



void DrawEntityTree(application_imgui* AppImgui) {

  menu_entity_tree* MenuEntityTree = AppImgui->MenuEntityTree;

  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(GlobalState->ImguiContext.FontSize);
  
  v2 ScrollListPos  = V2(MenuEntityTree->BorderWindow.Region.X, MenuEntityTree->BorderWindow.Region.Y);
  v2 ScrollListSize = V2(MenuEntityTree->BorderWindow.Region.W, MenuEntityTree->BorderWindow.Region.H - MenuEntityTree->BorderWindow.HeaderSize);

  ImguiBorderWindow(&MenuEntityTree->BorderWindow, "Entities");

  v2 TotalSize = ImguiEntityComponentTree(MenuEntityTree, ScrollListPos, ScrollListSize);
}