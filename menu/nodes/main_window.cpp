#include "main_window.h"

menu_functions GetMainWindowFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, MainWindowUpdate);
  return Result;
}

menu_tree* CreateMainWindow(menu_interface* Interface){

  menu_tree* MainWindow = NewMenuTree(Interface);
  MainWindow->Visible = true;
  MainWindow->Maximized = true;
  MainWindow->Root = NewContainer(Interface, container_type::MainWindow);

  main_window_node* MainWindowNode = GetMainWindowNode(MainWindow->Root);
  MainWindowNode->HeaderSize = Interface->HeaderSize;
  {
    container_node* HeaderBar = ConnectNodeToBack(MainWindow->Root, NewContainer(Interface));

    color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, HeaderBar, ATTRIBUTE_COLOR);

    ColorAttr->Color = Interface->MenuColor;
    ColorAttr->HighlightedColor = ColorAttr->Color;
    ColorAttr->RestingColor = ColorAttr->Color;

    container_node* DropDownContainer = ConnectNodeToBack(HeaderBar, NewContainer(Interface, container_type::Grid));
    grid_node* Grid = GetGridNode(DropDownContainer);
    Grid->Col = 0;
    Grid->Row = 1;
    Grid->TotalMarginX = 0.0;
    Grid->TotalMarginY = 0.0;
    Grid->Stack = true;
  }

  { // Main Menu Body
    container_node* Body = ConnectNodeToBack(MainWindow->Root, NewContainer(Interface));
  }

  return MainWindow;
}