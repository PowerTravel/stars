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
        ChildSize = GetSize(Size, V2(CellWidth, CellHeight));
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

internal r32 GetVectorSum(u32 Count, r32* NumVec)
{
  r32 Result = 0;
  for (int i = 0; i < Count; ++i)
  {
    Result += NumVec[i];
  }
  return Result;
}

internal void SetChildXAndYRelativePosition(u32 NumCols, r32* CellWidths, r32 TotalWidth, u32 NumRows, r32* CellHeights, r32 TotalHeight, container_node* FirstChild)
{
  container_node* Child = FirstChild;
  r32 XCol = 0;
  for (u32 j = 0; j < NumCols; ++j)
  {
    r32 CellWidth = CellWidths[j];
    r32 YRow = TotalHeight;
    for (u32 i = 0; i < NumRows; ++i)
    {
      r32 CellHeight = CellHeights[i];
      // Align child within the allocated grid slot
      r32 X = XCol;
      r32 Y = YRow - CellHeight;
      v2 AlignedPos = V2(X,Y);
      if(HasAttribute(Child, ATTRIBUTE_ALIGNMENT))
      {
        rect2f BaseRegion = Rect2f(X,Y,CellWidth,CellHeight);
        alignment_attribute* AlignmentAttr = (alignment_attribute*) GetAttributePointer(Child, ATTRIBUTE_ALIGNMENT);
        AlignedPos = GetAlignedPosition(AlignmentAttr, Child->Region, BaseRegion);       
      }
      Child->Region.X = AlignedPos.X;
      Child->Region.Y = AlignedPos.Y;
      Child = Next(Child);
      YRow -= CellHeight;
      if(!Child)
      {
        break;
      }
    }
    XCol += CellWidth;
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
      YOffset = ParentRegion.Y + ParentRegion.H - TotalHeight;
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

  r32 TotalHeight = GetVectorSum(RowCount, CellHeights);
  r32 TotalWidth = GetVectorSum(ColCount, CellWidths);

  SetChildXAndYRelativePosition(ColCount, CellWidths, TotalWidth, RowCount, CellHeights, TotalHeight, Parent->FirstChild);

  SetChildXAndYAbsolutePosition(ColCount, RowCount, TotalWidth, TotalHeight, GridNode, Parent->Region, Parent->FirstChild);
}