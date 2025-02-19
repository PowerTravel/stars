#pragma once
#include "menu_interface.h"

MENU_UPDATE_CHILD_REGIONS(UpdateChildRegions)
{
  container_node* Child = Parent->FirstChild;
  while(Child)
  {
    Child->Region = Parent->Region;
    Child = Next(Child);
  }
}

MENU_UPDATE_CHILD_REGIONS(MainHeaderUpdate)
{
  container_node* Header = Parent->FirstChild;
  Header->Region = Rect2f(0,1-Interface->HeaderSize, GetAspectRatio(Interface), Interface->HeaderSize);
  container_node* Body = Header->NextSibling;
  Body->Region = Rect2f(0,0,GetAspectRatio(Interface), 1-Interface->HeaderSize);
}




MENU_UPDATE_CHILD_REGIONS(UpdateTabWindowChildRegions)
{
  container_node* Child = Parent->FirstChild;
  u32 Index = 0;
  tab_window_node* TabWindow = GetTabWindowNode(Parent);
  while(Child)
  {
    switch(Index)
    {
      case 0: // Header
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y + Parent->Region.H - TabWindow->HeaderSize,
          Parent->Region.W,
          TabWindow->HeaderSize);
      }break;
      case 1: // Body
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y,
          Parent->Region.W,
          Parent->Region.H - TabWindow->HeaderSize);
      }break;
      default:
      {
        INVALID_CODE_PATH
      }break;
    }
    ++Index;
    Child = Next(Child);
  }
}

MENU_DRAW(DrawMergeSlots)
{
  tab_window_node* TabWindowNode = GetTabWindowNode(Node);
  for (u32 Index = 0; Index < ArrayCount(TabWindowNode->MergeZone); ++Index)
  {
    rect2f DrawRegion = ecs::render::RectCenterBotLeft(TabWindowNode->MergeZone[Index]);
    ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), DrawRegion, Index == EnumToIdx(TabWindowNode->HotMergeZone) ? V4(0,1,0,0.5) : V4(0,1,0,0.3));
  }
  TabWindowNode->HotMergeZone = merge_zone::NONE;
}

MENU_DRAW(TabWindowDraw)
{
  tab_window_node* TabWindowNode = GetTabWindowNode(Node);
  if(TabWindowNode->HotMergeZone != merge_zone::NONE)
  {
    PushToFinalDrawQueue(Interface, Node, DrawMergeSlots);
  }
}



