#include "menu/tabbed_window/tabbed_window.h"
#include "menu/menu_interface_internal.h"

container_node* CreateTabWindow(menu_interface* Interface)
{
  container_node* TabWindow = NewContainer(Interface, container_type::TabWindow);
  GetTabWindowNode(TabWindow)->HeaderSize = Interface->HeaderSize;

  container_node* TabWindowHeader = ConnectNodeToBack(TabWindow, NewContainer(Interface, container_type::None));

  color_attribute* Color = (color_attribute*) PushAttribute(Interface, TabWindowHeader, ATTRIBUTE_COLOR);
  Color->Color = menu::GetColor(GetColorTable(),"café noir");
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, TabWindowHeader, 0, TabWindowHeaderMouseDown, 0);
  PushAttribute(Interface, TabWindowHeader, ATTRIBUTE_MERGE);

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
