#include "split_window.h"

menu_functions GetSplitFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region,UpdateSplitChildRegions);
  return Result;
}

container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos)
{
  container_node* SplitNode  = NewContainer(Interface, container_type::Split);
  container_node* BorderNode = CreateBorderNode(Interface, Interface->BorderColor);
  SetBorderData(BorderNode, Interface->BorderSize, BorderPos, Vertical ? border_type::SPLIT_VERTICAL : border_type::SPLIT_HORIZONTAL);
  ConnectNodeToBack(SplitNode, BorderNode);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, BorderNode, 0, InitiateSplitWindowBorderDrag, 0);
  return SplitNode;
}

split_windows GetSplitWindows(container_node* SplitWindowContainer)
{
  Assert(SplitWindowContainer->Type == container_type::Split);
  split_windows Result = {};
  Result.Border = SplitWindowContainer->FirstChild;
  Result.FirstWindow  = Next(Result.Border);
  Result.SecondWindow = Next(Result.FirstWindow);
  return Result;
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