#include "tabbed_window.h"
#include "root_window.h"

container_node* CreateTabWindow(menu_interface* Interface)
{
  container_node* TabWindow = NewContainer(Interface, container_type::TabWindow);
  GetTabWindowNode(TabWindow)->HeaderSize = Interface->HeaderSize;

  container_node* TabWindowHeader = ConnectNodeToBack(TabWindow, NewContainer(Interface, container_type::None));

  color_attribute* Color = (color_attribute*) PushAttribute(Interface, TabWindowHeader, ATTRIBUTE_COLOR);
  Color->Color = menu::GetColor(GetColorTable(),"café noir");
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, TabWindowHeader, 0, TabWindowHeaderMouseDown, 0);

  container_node* TabItemCollection = ConnectNodeToBack(TabWindowHeader, NewContainer(Interface, container_type::Grid));
  grid_node* Grid = GetGridNode(TabItemCollection);
  Grid->Row = 1;
  Grid->Stack = true;

  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, TabItemCollection, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, 0.7);
  SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE_, 1);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->XAlignment = menu_region_alignment::LEFT;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;
  
  return TabWindow;
}


internal container_node* GetTabGridFromWindow(container_node* TabbedWindow)
{
  Assert(TabbedWindow->Type == container_type::TabWindow); // Could also be split window
  container_node* TabRegion = TabbedWindow->FirstChild;
  Assert(TabRegion->Type == container_type::None);
  container_node* TabHeader = TabRegion->FirstChild;
  Assert(TabHeader->Type == container_type::Grid);
  return TabHeader;
}

void SetRootWindowRegion(menu_tree* MenuTree, rect2f Region)
{
  root_border_collection Borders = GetRoorBorders(MenuTree->Root);

  GetBorderNode(Borders.Left)->Position = Region.X;
  GetBorderNode(Borders.Right)->Position = Region.X + Region.W;
  GetBorderNode(Borders.Bot)->Position = Region.Y;
  GetBorderNode(Borders.Top)->Position = Region.Y + Region.H;
}
/*
container_node* GetTabWindow(container_node* Node)
{
  while(Node->Type != container_type::TabWindow)
  {
    Node = Node->Parent;
  }
  return Node;
}
*/

MENU_EVENT_CALLBACK(TabWindowHeaderMouseDown)
{
  container_node* WindowHeader = CallerNode;
  if(Interface->DoubleKlick)
  {
    menu_tree* MenuTree = GetMenu(Interface, CallerNode);
    ToggleMaximizeWindow(Interface, MenuTree,WindowHeader);
  }else{
      Assert(WindowHeader->Type == container_type::None);
      container_node* TabWindow = WindowHeader->Parent;

      mouse_position_in_window* Position = (mouse_position_in_window*) Allocate(&Interface->LinkedMemory, sizeof(mouse_position_in_window));
      *Position = GetPositionInRootWindow(Interface->MousePos, WindowHeader);

      if(TabWindow->Parent->Type == container_type::Split)
      {
        if(!Intersects(WindowHeader->FirstChild->Region, Interface->MousePos))
        {
          InitiateRootWindowDrag(Interface, WindowHeader);
        }
      }else{
        Assert(TabWindow->Type == container_type::TabWindow);
        container_node* TabGrid = GetTabGridFromWindow(TabWindow);
        u32 ChildCount = GetChildCount(TabGrid);
        if(ChildCount == 1)
        {
          InitiateRootWindowDrag(Interface, WindowHeader);
        }else if(!Intersects(WindowHeader->FirstChild->Region, Interface->MousePos)){
          InitiateRootWindowDrag(Interface, WindowHeader);
        }
      }
  }
}
