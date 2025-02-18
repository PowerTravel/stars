#include "nodes/border_node.h"

border_leaf* GetBorderNode(container_node* Container)
{
  Assert(Container->Type == container_type::Border);
  border_leaf* Result = (border_leaf*) GetContainerPayload(Container);
  return Result;
}

container_node* CreateBorderNode(menu_interface* Interface, v4 Color)
{
  container_node* Result = NewContainer(Interface, container_type::Border);
 
  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Result, ATTRIBUTE_COLOR);
  ColorAttr->Color = Color;

  Assert(Result->Type == container_type::Border);
  return Result;
}

void UpdateBorderPosition(container_node* BorderNode, v2 NewPosition, rect2f MinimumWindowSize, rect2f MaximumWindowSize)
{
  border_leaf* Border = GetBorderNode(BorderNode);
  switch(Border->Type)
  {
    // A Root border work with canonical positions
    case border_type::LEFT: {
      Border->Position = Clamp(
        NewPosition.X,
        MaximumWindowSize.X + Border->Thickness * 0.5f,
        MinimumWindowSize.X + MinimumWindowSize.W - Border->Thickness * 0.5f);
    }break;
    case border_type::RIGHT:{
      Border->Position = Clamp(
        NewPosition.X,
        MinimumWindowSize.X + Border->Thickness * 0.5f,
        MaximumWindowSize.X + MaximumWindowSize.W - Border->Thickness * 0.5f);
    }break;
    case border_type::TOP:{
      Border->Position = Clamp(
        NewPosition.Y,
        MinimumWindowSize.Y + Border->Thickness * 0.5f,
        MaximumWindowSize.Y + MaximumWindowSize.H - Border->Thickness * 0.5f);
    }break;
    case border_type::BOTTOM:{
      Border->Position = Clamp(
        NewPosition.Y,
        MaximumWindowSize.Y + Border->Thickness * 0.5f,
        MinimumWindowSize.Y + MinimumWindowSize.H - Border->Thickness * 0.5f);
    }break;
    // A SPLIT border don't work with canonical positions, they work with percentage of parent region.
    // That means if a border position is at 10% it means 10% of parent window is for left split region and 90% is for right split region
    case border_type::SPLIT_VERTICAL:{
      Border->Position = Clamp(
        NewPosition.X,
        MinimumWindowSize.X,
        MinimumWindowSize.W);
    }break;
    case border_type::SPLIT_HORIZONTAL:{
      Border->Position = Clamp(
        NewPosition.Y,
        MinimumWindowSize.Y,
        MinimumWindowSize.H);
    }break;
  }
}

void SetRootWindowRegion(menu_tree* MenuTree, rect2f Region)
{
  root_border_collection Borders = GetRoorBorders(MenuTree->Root);

  GetBorderNode(Borders.Left)->Position = Region.X;
  GetBorderNode(Borders.Right)->Position = Region.X + Region.W;
  GetBorderNode(Borders.Bot)->Position = Region.Y;
  GetBorderNode(Borders.Top)->Position = Region.Y + Region.H;
}


void SetBorderData(container_node* Border, r32 Thickness, r32 Position, border_type Type)
{
  Assert(Border->Type == container_type::Border);
  border_leaf* BorderLeaf = GetBorderNode(Border);
  BorderLeaf->Type        = Type;
  BorderLeaf->Position    = Position;
  BorderLeaf->Thickness   = Thickness;
  BorderLeaf->Active      = true;
}