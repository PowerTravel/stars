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