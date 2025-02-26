#include "grid_window.h"

grid_node* GetGridNode(container_node* Container)
{
  Assert(Container->Type == container_type::Grid);
  grid_node* Result = (grid_node*) GetContainerPayload(Container);
  return Result;
}

menu_functions GetGridFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region,UpdateGridChildRegions);
  return Result;
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