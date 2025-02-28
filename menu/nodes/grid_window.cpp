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
  SCOPED_TRANSIENT_ARENA;
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

  r32* RowHeights = PushArray(GlobalTransientArena, Row, r32);

  r32 CellWidth = ParentRegion.W/Col;
  r32 CellHeight = ParentRegion.H/Row;

  container_node* Child = Parent->FirstChild;
  if(GridNode->Stack)
  {
    r32 X = ParentRegion.X;
    r32 MaxCellWidthForColumn = 0;
    r32 TotalGridWidth = 0;
    r32 MaxCellHeightForRow = 0;
    r32 TotalGridHeight = 0;
    for (u32 j = 0; j < Col; ++j)
    {
      r32 Y = ParentRegion.Y + ParentRegion.H;
      for (u32 i = 0; i < Row; ++i)
      {
        rect2f CellRegion = {};
        if(HasAttribute(Child, ATTRIBUTE_SIZE))
        {
          size_attribute* Size = (size_attribute*) GetAttributePointer(Child, ATTRIBUTE_SIZE);
          if(Size->Width.Type == menu_size_type::ABSOLUTE_)
          {
            CellRegion.W = Size->Width.Value;
            CellRegion.X = X;
          }else{
            CellRegion.W = Size->Width.Value * CellWidth;
            CellRegion.X = X;
          }
          if(Size->Height.Type == menu_size_type::ABSOLUTE_)
          {
            CellRegion.H = Size->Height.Value;
            CellRegion.Y = Y - CellRegion.H;
          }else{
            CellRegion.H = Size->Height.Value * CellHeight;
            CellRegion.Y = Y - CellRegion.H;
          }
        }else{
          CellRegion = Rect2f(
            X, 
            Y,
            CellWidth,
            CellHeight);
        }

        if(MaxCellWidthForColumn < CellRegion.W)
        {
          MaxCellWidthForColumn = CellRegion.W;
        }
        if(RowHeights[i] < CellRegion.H)
        {
          RowHeights[i] = CellRegion.H;
        }

        Child->Region = CellRegion;
        Child = Next(Child);

        Y -= CellRegion.H;
        if(!Child)
        {
          break;
        }
      }

      X += MaxCellWidthForColumn;
      TotalGridWidth  += MaxCellWidthForColumn;
      MaxCellWidthForColumn = 0;
      if(!Child)
      {
        break;
      }
    }


    r32* RowYPositions = PushArray(GlobalTransientArena, Row, r32);
    r32 YP = ParentRegion.Y+ParentRegion.H;
    RowYPositions[0] = YP - RowHeights[0];
    TotalGridHeight = RowHeights[0];
    for (u32 i = 1; i < Row; ++i)
    {
      TotalGridHeight += RowHeights[i];
      RowYPositions[i] = RowYPositions[i-1] - RowHeights[i];
    }

    container_node* Child = Parent->FirstChild;
    for (u32 j = 0; j < Col; ++j)
    {
      for (u32 i = 0; i < Row; ++i)
      {
        Child->Region.Y = RowYPositions[i]; 
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

    r32 XOffset = 0;
    r32 YOffset = 0;
    switch(GridNode->StackXAlignment)
    {
      case menu_region_alignment::LEFT: {
        XOffset = 0;
      }break;
      case menu_region_alignment::RIGHT: {
        XOffset = ParentRegion.W - TotalGridWidth;
      }break;
      default: {
        XOffset = (ParentRegion.W - TotalGridWidth) * 0.5f;
      }break; // CENTER
    }

    switch(GridNode->StackYAlignment)
    {
      case menu_region_alignment::TOP: {
        YOffset = 0;
      }break;
      case menu_region_alignment::BOT: {
        YOffset = (ParentRegion.H-TotalGridHeight);
      }break;
      default: {
        YOffset = (ParentRegion.H-TotalGridHeight) * 0.5f;
      }break; // CENTER
    }

    Child = Parent->FirstChild;
    for (u32 j = 0; j < Col; ++j)
    {
      for (u32 i = 0; i < Row; ++i)
      {
        Child->Region.X += XOffset; 
        Child->Region.Y -= YOffset; 
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


  }else{

    r32 xMargin = (GridNode->TotalMarginX / Col) * ParentRegion.W;
    r32 yMargin = (GridNode->TotalMarginY / Row) * ParentRegion.H;

    r32 X0 = ParentRegion.X;
    r32 Y0 = ParentRegion.Y + ParentRegion.H - CellHeight;
    for (r32 i = 0; i < Row; ++i)
    {
      for (r32 j = 0; j < Col; ++j)
      {
        Child->Region = Rect2f(X0 + j * CellWidth, Y0 - i* CellHeight, CellWidth, CellHeight);
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