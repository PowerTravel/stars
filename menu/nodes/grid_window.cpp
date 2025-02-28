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

internal void SetChildWidthsAndHeights(u32 NumCols, r32* ResultColWidths, r32 CellWidth, u32 NumRows, r32* ResultRowHeights, r32 CellHeight, container_node* FirstChild)
{
  container_node* Child = FirstChild;
  for (u32 j = 0; j < NumCols; ++j)
  {
    for (u32 i = 0; i < NumRows; ++i)
    {
      v2 ChildSize = {};
      if(HasAttribute(Child, ATTRIBUTE_SIZE))
      {
        size_attribute* Size = (size_attribute*) GetAttributePointer(Child, ATTRIBUTE_SIZE);
        if(Size->Width.Type == menu_size_type::ABSOLUTE_)
        {
          ChildSize.X = Size->Width.Value;
        }else{
          ChildSize.X = Size->Width.Value * CellWidth;
        }
        if(Size->Height.Type == menu_size_type::ABSOLUTE_)
        {
          ChildSize.Y = Size->Height.Value;
        }else{
          ChildSize.Y = Size->Height.Value * CellHeight;
        }
      }else{
        ChildSize.X = CellWidth;
        ChildSize.Y = CellHeight;
      }

      if(ResultRowHeights[i] < ChildSize.Y)
      {
        ResultRowHeights[i] = ChildSize.Y;
      }
      if(ResultColWidths[j] < ChildSize.X)
      { 
        ResultColWidths[j] = ChildSize.X;
      }

      Child->Region.W = ChildSize.X;
      Child->Region.H = ChildSize.Y;
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

internal r32 GetCumulativeVector(u32 Count, r32* NumVec, r32* ResultVec)
{
  r32 Result = NumVec[0];
  ResultVec[0] = Result;
  for (int i = 1; i < Count; ++i)
  {
    Result += NumVec[i];
    ResultVec[i] = Result;
  }
  return Result;
}

internal void SetChildXAndYRelativePosition(u32 NumCols, r32* CellWidths, r32 TotalWidth, u32 NumRows, r32* CellHeights, r32 TotalHeight, container_node* FirstChild)
{
  container_node* Child = FirstChild;
  r32 X = 0;
  for (u32 j = 0; j < NumCols; ++j)
  {
    r32 CellWidth = CellWidths[j];
    r32 Y = TotalHeight;
    for (u32 i = 0; i < NumRows; ++i)
    {
      r32 CellHeight = CellHeights[i];
      Y -= CellHeight;
      // Align child within the allocated grid slot
      if(HasAttribute(Child, ATTRIBUTE_SIZE))
      {
        size_attribute* Size = (size_attribute*) GetAttributePointer(Child, ATTRIBUTE_SIZE);
        switch(Size->XAlignment)
        {
          case menu_region_alignment::LEFT: {
            // Do Nothing
          }break;
          case menu_region_alignment::RIGHT: {
            X +=  CellWidth - Child->Region.W;
          }break;
          default: {
            X += (CellWidth - Child->Region.W) * 0.5f;
          }break; // CENTER
        }

        switch(Size->YAlignment)
        {
          case menu_region_alignment::TOP: {
            // Do Nothing
          }break;
          case menu_region_alignment::BOT: {
            Y -= (CellHeight-Child->Region.H);
          }break;
          default: {
            Y -= (CellHeight-Child->Region.H) * 0.5f;
          }break; // CENTER
        }
      }

      Child->Region.Y = Y;
      Child->Region.X = X;
      Child = Next(Child);
      if(!Child)
      {
        break;
      }
    }
    X += CellWidth;
    if(!Child)
    {
      break;
    }
  }
}

internal void SetChildXAndYAbsolutePosition(u32 ColCount, u32 RowCount, r32 TotalWidth, r32 TotalHeight, grid_node* GridNode, rect2f ParentRegion, container_node* FirstChild)
{
  r32 XOffset = 0;
  r32 YOffset = 0;
  switch(GridNode->StackXAlignment)
  {
    case menu_region_alignment::LEFT: {
      XOffset = ParentRegion.X;
    }break;
    case menu_region_alignment::RIGHT: {
      XOffset = ParentRegion.X + ParentRegion.W - TotalWidth;
    }break;
    default: {
      XOffset = ParentRegion.X + (ParentRegion.W - TotalWidth) * 0.5f;
    }break; // CENTER
  }

  switch(GridNode->StackYAlignment)
  {
    case menu_region_alignment::TOP: {
      YOffset = ParentRegion.H - TotalHeight;
    }break;
    case menu_region_alignment::BOT: {
      YOffset = ParentRegion.Y;
    }break;
    default: {
      YOffset = ParentRegion.Y + (ParentRegion.H-TotalHeight) * 0.5f;
    }break; // CENTER
  }

  container_node* Child = FirstChild;
  for (u32 j = 0; j < ColCount; ++j)
  {
    for (u32 i = 0; i < RowCount; ++i)
    {
      Child->Region.X += XOffset; 
      Child->Region.Y += YOffset; 
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

MENU_UPDATE_CHILD_REGIONS( UpdateGridChildRegions )
{
  SCOPED_TRANSIENT_ARENA;
  r32 Count = (r32) GetChildCount(Parent);
  if(!Count) return;

  rect2f ParentRegion = Parent->Region; 

  grid_node* GridNode = GetGridNode(Parent);
  r32 RowCount = (r32)GridNode->Row;
  r32 ColCount = (r32)GridNode->Col;

  Assert(GridNode->Row || GridNode->Col);
  
  if(RowCount == 0)
  {
    Assert(ColCount != 0);
    RowCount = Ciel(Count / ColCount);
  }else if(ColCount == 0){
    Assert(RowCount != 0);
    ColCount = Ciel(Count / RowCount);
  }else{
    Assert((RowCount * ColCount) >= Count);
  }

  r32* CellHeights = PushArray(GlobalTransientArena, RowCount, r32);
  r32* CellWidths  = PushArray(GlobalTransientArena, ColCount, r32);

  r32 EqualCellWidth = ParentRegion.W/ColCount;
  r32 EqualCellHeight = ParentRegion.H/RowCount;

  // Set node widths and calculate max height / width for each row / col
  SetChildWidthsAndHeights(ColCount, CellWidths, EqualCellWidth, RowCount, CellHeights, EqualCellHeight, Parent->FirstChild);

  r32* CumulativeHeight = PushArray(GlobalTransientArena, RowCount, r32);
  r32 TotalHeight = GetCumulativeVector(RowCount, CellHeights, CumulativeHeight);

  r32* CumulativeWidth = PushArray(GlobalTransientArena, RowCount, r32);
  r32 TotalWidth = GetCumulativeVector(ColCount, CellWidths, CumulativeWidth);

  SetChildXAndYRelativePosition(ColCount, CellWidths, TotalWidth, RowCount, CellHeights, TotalHeight, Parent->FirstChild);

  SetChildXAndYAbsolutePosition(ColCount, RowCount, TotalWidth, TotalHeight, GridNode, Parent->Region, Parent->FirstChild);

  

#if 0
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
#endif
}