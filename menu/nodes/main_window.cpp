#include "main_window.h"
#include "menu_interface.h"

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


void DisplayOrRemovePluginTab(menu_interface* Interface, container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);
  container_node* TabHeader = Tab->Parent;
  if(TabHeader)
  {
    Assert(TabHeader->Type == container_type::Grid);

    container_node* TabWindow = GetTabWindowFromTab(Tab);

    PopTab(Tab);
    if(GetChildCount(TabHeader) == 0)
    {
      ReduceWindowTree(Interface, TabWindow);
    }

  }else{

    container_node* TabWindow = 0;
    if(!Interface->SpawningWindow)
    {
      TabWindow = CreateTabWindow(Interface);
      Interface->SpawningWindow = CreateNewRootContainer(Interface, TabWindow, Rect2f( 0.25, 0.25, 0.7, 0.5));

    }else{
      TabWindow = GetBodyFromRoot(Interface->SpawningWindow->Root);
      while(TabWindow->Type != container_type::TabWindow)
      {
        if(TabWindow->Type ==  container_type::Split)
        {
          TabWindow = Next(TabWindow->FirstChild);
        }else{
          INVALID_CODE_PATH;
        }
      }
    }

    PushTab(TabWindow, Tab);

    container_node* Body = Next(TabWindow->FirstChild);
    tab_node* TabNode = GetTabNode(Tab);
    if(Body)
    {
      ReplaceNode(Body, TabNode->Payload);
      container_node* Plugin = TabNode->Payload;
      Assert(Plugin->Type == container_type::Plugin);
      plugin_node* PluginNode = GetPluginNode(Plugin);
    }else{
      ConnectNodeToBack(TabWindow, TabNode->Payload);
    }
  }

  SetFocusWindow(Interface, Interface->SpawningWindow);
}

MENU_EVENT_CALLBACK(DropDownMouseUp)
{
  menu_tree* Menu = GetMenu(Interface, CallerNode);
  Assert(Menu->Visible);
  //Menu->Visible = false;
  DisplayOrRemovePluginTab(Interface, (container_node*) Data);
}


MENU_EVENT_CALLBACK(DropDownMenuButton)
{
  menu_tree* Menu = (menu_tree*) Data;
  Menu->Root->Region.X = CallerNode->Region.X;
  Menu->Root->Region.Y = CallerNode->Region.Y - Menu->Root->Region.H;

  if(!Menu->Visible)
  {
    SetFocusWindow(Interface, Menu);
  }
}

MENU_EVENT_CALLBACK(HeaderMenuMouseEnter)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);

  menu_tree* DropDownMenu = (menu_tree*) Data;
  menu_tree* MenuBar = GetMenu(Interface, CallerNode);
  menu_tree* VisibleDropDownMenu = 0;
  
  // See if any drop down menu is visible
  for (int i = 0; i < Interface->MainMenuTabCount; ++i)
  {
    menu_tree* DropDown = Interface->MainMenuTabs[i];
    if(DropDown->Visible && DropDown != DropDownMenu)
    {
      VisibleDropDownMenu = DropDown;
      break;
    }
  }
  if(VisibleDropDownMenu)
  {
    DropDownMenu->Root->Region.X = CallerNode->Region.X;
    DropDownMenu->Root->Region.Y = CallerNode->Region.Y - DropDownMenu->Root->Region.H;
    SetFocusWindow(Interface, DropDownMenu);
  }
  MenuColor->Color = MenuColor->HighlightedColor;
}

MENU_EVENT_CALLBACK(HeaderMenuMouseExit)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = MenuColor->RestingColor;
}
