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



MENU_UPDATE_CHILD_REGIONS(UpdateSplitChildRegions)
{
  container_node* Child = Parent->FirstChild;
  container_node* Body1Node = 0;
  container_node* Body2Node = 0;
  container_node* BorderNode = 0;
  while(Child)
  {
    if( Child->Type == container_type::Border)
    {
      BorderNode = Child;
    }else{
      if(!Body1Node)
      {
        Body1Node = Child;
      }else{
        Assert(!Body2Node);
        Body2Node = Child;
      }
    }
    Child = Next(Child);
  }

  Assert(Body1Node && Body2Node && BorderNode);
  border_leaf* Border = GetBorderNode(BorderNode);

  if(Border->Type == border_type::BOTTOM || Border->Type == border_type::TOP || Border->Type == border_type::SPLIT_VERTICAL)
  {
    BorderNode->Region = Rect2f(
      Parent->Region.X + (Border->Position * Parent->Region.W - 0.5f*Border->Thickness),
      Parent->Region.Y,
      Border->Thickness,
      Parent->Region.H);
    Body1Node->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y,
      Border->Position * Parent->Region.W - 0.5f*Border->Thickness,
      Parent->Region.H);
    Body2Node->Region = Rect2f(
      Parent->Region.X + (Border->Position * Parent->Region.W + 0.5f*Border->Thickness),
      Parent->Region.Y,
      Parent->Region.W - (Border->Position * Parent->Region.W+0.5f*Border->Thickness),
      Parent->Region.H);
  }else{
    BorderNode->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y + Border->Position * Parent->Region.H - 0.5f*Border->Thickness,
      Parent->Region.W,
      Border->Thickness);
    Body1Node->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y,
      Parent->Region.W,
      Border->Position * Parent->Region.H - 0.5f*Border->Thickness);
    Body2Node->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y + (Border->Position * Parent->Region.H + 0.5f*Border->Thickness),
      Parent->Region.W,
      Parent->Region.H - (Border->Position * Parent->Region.H+0.5f*Border->Thickness));
  }
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


MENU_UPDATE_CHILD_REGIONS( UpdateGridChildRegions )
{
  r32 Count = (r32) GetChildCount(Parent);
  if(!Count) return;

  rect2f ParentRegion = Parent->Region; 

  grid_node* GridNode = GetGridNode(Parent);
  r32 Row = (r32)GridNode->Row;
  r32 Col = (r32)GridNode->Col;

  Assert(GridNode->Row || GridNode->Col);
  
  if(Row == 0)
  {
    Assert(Col != 0);
    Row = Ciel(Count / Col);
  }else if(Col == 0){
    Assert(Row != 0);
    Col = Ciel(Count / Row);
  }else{
    Assert((Row * Col) >= Count);
  }

  container_node* Child = Parent->FirstChild;
  r32 MaxHeightForRow = 0;
  if(GridNode->Stack)
  {
    r32 xMargin = (GridNode->TotalMarginX / Col) * ParentRegion.W;
    r32 yMargin = (GridNode->TotalMarginY / Row) * ParentRegion.H;

    r32 Y0 = ParentRegion.Y + ParentRegion.H;
    r32 MaxWidth = 0;
    r32 MaxHeight = 0;
    for (r32 i = 0; i < Row; ++i)
    {
      r32 X0 = ParentRegion.X;
      for (r32 j = 0; j < Col; ++j)
      {
        r32 CellWidth = ParentRegion.W/Col;
        r32 CellHeight = ParentRegion.H/Row;
        size_attribute* Size = (size_attribute*) GetAttributePointer(Child, ATTRIBUTE_SIZE);
        if(Size->Width.Type == menu_size_type::ABSOLUTE_)
        {
          CellWidth = Size->Width.Value;
        }else if(Size->Height.Type == menu_size_type::ABSOLUTE_)
        {
          CellHeight = Size->Height.Value;
        }

        Child->Region = Rect2f(X0, Y0-CellHeight, CellWidth, CellHeight);
        Child->Region.X += xMargin/2.f;
        Child->Region.W -= xMargin;
        Child->Region.Y += yMargin/2.f;
        Child->Region.H -= yMargin;

        X0 += CellWidth;

        if(MaxHeightForRow < CellHeight)
        {
          MaxHeightForRow = CellHeight;
        }

        Child = Next(Child);
        if(!Child)
        {
          break;
        }
      }

      Y0 -= MaxHeightForRow;
      MaxHeightForRow = 0;
      r32 WidhtOfRow = X0 - ParentRegion.X;
      if(MaxWidth < WidhtOfRow)
      {
        MaxWidth = WidhtOfRow;
      }
      if(!Child)
      {
        break;
      }
    }
    r32 HeightOfColumn = (ParentRegion.Y + ParentRegion.H) - Y0;
    ParentRegion.W = MaxWidth;
    ParentRegion.H = HeightOfColumn;
    Parent->Region = ParentRegion;
  }else{

    r32 xMargin = (GridNode->TotalMarginX / Col) * ParentRegion.W;
    r32 yMargin = (GridNode->TotalMarginY / Row) * ParentRegion.H;

    r32 CellWidth  = ParentRegion.W / Col;
    r32 CellHeight = ParentRegion.H / Row;
    r32 X0 = ParentRegion.X;
    r32 Y0 = ParentRegion.Y + ParentRegion.H - CellHeight;
    for (r32 i = 0; i < Row; ++i)
    {
      for (r32 j = 0; j < Col; ++j)
      {
        Child->Region = Rect2f(X0 + j * CellWidth, Y0 - i* CellHeight, CellWidth, CellHeight);
        Child->Region.X += xMargin/2.f;
        Child->Region.W -= xMargin;
        Child->Region.Y += yMargin/2.f;
        Child->Region.H -= yMargin;
        Child = Next(Child);
        if(!Child)
        {
          break;
        }
      }
      if(!Child)
      {
        break;
      }
    }
  }
}

