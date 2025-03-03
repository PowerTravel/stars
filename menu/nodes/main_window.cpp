#include "main_window.h"
#include "menu_interface.h"

menu_tree* CreateMainWindow(menu_interface* Interface){

  menu_tree* MainWindow = NewMenuTree(Interface);
  MainWindow->Visible = true;
  MainWindow->Root = NewContainer(Interface, container_type::MainWindow);
  MainWindow->Root->Region = Rect2f(0,0,GetAspectRatio(Interface),1);

  {
    container_node* HeaderBar = ConnectNodeToBack(MainWindow->Root, NewContainer(Interface, container_type::Grid));
    grid_node* Grid = GetGridNode(HeaderBar);
    Grid->Col = 0;
    Grid->Row = 1;
    Grid->TotalMarginX = 0.0;
    Grid->TotalMarginY = 0.0;
    Grid->StackXAlignment = menu_region_alignment::LEFT;
    Grid->StackYAlignment = menu_region_alignment::TOP;
    Grid->Stack = true;

    color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, HeaderBar, ATTRIBUTE_COLOR);

    ColorAttr->Color = Interface->MenuColor;
    ColorAttr->HighlightedColor = ColorAttr->Color;
    ColorAttr->RestingColor = ColorAttr->Color;
  }

  { // Main Menu Body
    container_node* Body = ConnectNodeToBack(MainWindow->Root, NewContainer(Interface));
  }

  return MainWindow;
}

void DisplayPluginInNewWindow(menu_interface* Interface, container_node* Plugin, rect2f Region)
{
  Assert(!Plugin->Parent);

  container_node* Tab = GetPluginNode(Plugin)->Tab;
  Assert(Tab->Type == container_type::Tab);
  Assert(!Tab->Parent);

  container_node* TabWindow = CreateTabWindow(Interface);
  menu_tree* NewWindow = CreateNewRootContainer(Interface, TabWindow, Region);

  PushTab(TabWindow, Tab);
  ConnectNodeToBack(TabWindow, Plugin);

  SetFocusWindow(Interface, NewWindow);
}

void DisplayOrRemovePlugin(menu_interface* Interface, container_node* Plugin, menu_tree* TreeToDisplayIn)
{
  container_node* Tab = GetPluginNode(Plugin)->Tab;
  Assert(Tab->Type == container_type::Tab);

  if(Tab->Parent)
  {
    RemoveTabFromTabWindow(Interface, Tab);
    SetFocusWindow(Interface, Interface->MenuSentinel.Next);
  }else{

    if(!TreeToDisplayIn)
    {
      DisplayPluginInNewWindow(Interface, Plugin, Rect2f( 0.25, 0.25, 0.7, 0.5));
      //Maximize(Interface, Interface->SpawningWindow->Root);
    }else{

      container_node* TabWindow = GetBodyFromRoot(TreeToDisplayIn->Root);
      while(TabWindow->Type != container_type::TabWindow)
      {
        if(TabWindow->Type ==  container_type::Split)
        {
          TabWindow = Next(TabWindow->FirstChild);
        }else{
          INVALID_CODE_PATH;
        }
      }

      PushTab(TabWindow, Tab);

      container_node* Body = Next(TabWindow->FirstChild);
      tab_node* TabNode = GetTabNode(Tab);
      ReplaceNode(Body, TabNode->Payload);
      SetFocusWindow(Interface, Interface->SpawningWindow);
    }
  }
}

MENU_EVENT_CALLBACK(DropDownMouseUp)
{
  menu_tree* Menu = GetMenu(Interface, CallerNode);
  Assert(Menu->Visible);

  container_node* Tab = (container_node*) Data;
  tab_node* TabNode = GetTabNode(Tab);
  DisplayOrRemovePlugin(Interface, TabNode->Payload, 0);
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

menu_tree* CreateNewDropDownMenuItem(menu_interface* Interface, const c8* Name)
{
  ecs::render::system* RenderSystem = GetRenderSystem();
  ecs::render::window_size_pixel WindowSize = ecs::render::GetWindowSize(RenderSystem);
  r32 PixelFontHeight = ecs::render::GetLineSpacingPixelSpace(RenderSystem, Interface->HeaderFontSize);
  r32 CanonicalFontHeight = Interface->HeaderSize;
  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(RenderSystem, Interface->HeaderFontSize, (utf8_byte*) Name);

  container_node* DropDownContainer = Interface->MenuBar->Root->FirstChild;
  Assert(DropDownContainer->Type == container_type::Grid);

  container_node* NewMenu = ConnectNodeToBack(DropDownContainer, NewContainer(Interface));

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_COLOR);
  ColorAttr->Color = Interface->MenuColor;
  ColorAttr->RestingColor = ColorAttr->Color;
  ColorAttr->HighlightedColor = ColorAttr->Color*1.5;

  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, TextSize.X * 1.2f);
  SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, CanonicalFontHeight);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  alignment_attribute* AlignmentAttr = (alignment_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_ALIGNMENT);
  AlignmentAttr->XAlignment = menu_region_alignment::CENTER;
  AlignmentAttr->YAlignment = menu_region_alignment::CENTER;

  text_attribute* Text = (text_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(Name, Text->Text);
  Text->FontSize = Interface->HeaderFontSize;
  Text->Color = Interface->TextColor;

  menu_tree* ViewMenuRoot = NewMenuTree(Interface);
  ViewMenuRoot->Root = NewContainer(Interface);
  ViewMenuRoot->Root->Region = {};
  ViewMenuRoot->LosingFocus = DeclareFunction(menu_losing_focus, DropDownLosingFocus);
  ViewMenuRoot->GainingFocus = DeclareFunction(menu_losing_focus, DropDownGainingFocus);

  container_node* ViewMenuItems = ConnectNodeToBack(ViewMenuRoot->Root, NewContainer(Interface, container_type::Grid));
  grid_node* Grid = GetGridNode(ViewMenuItems);
  Grid->Col = 1;
  Grid->Row = 0;
  Grid->StackXAlignment = menu_region_alignment::LEFT;
  Grid->StackYAlignment = menu_region_alignment::TOP;
  Grid->TotalMarginX = 0.0;
  Grid->TotalMarginY = 0.0;

  color_attribute* ViewMenuColor = (color_attribute*) PushAttribute(Interface, ViewMenuRoot->Root, ATTRIBUTE_COLOR);
  ViewMenuColor->Color = Interface->MenuColor;
  ViewMenuColor->HighlightedColor = Interface->MenuColor;
  ViewMenuColor->RestingColor = Interface->MenuColor;

  RegisterMenuEvent(Interface, menu_event_type::MouseDown,  NewMenu, (void*) ViewMenuRoot, DropDownMenuButton, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, NewMenu, (void*) ViewMenuRoot, HeaderMenuMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  NewMenu, (void*) ViewMenuRoot, HeaderMenuMouseExit, 0);

  return ViewMenuRoot;
}

MENU_EVENT_CALLBACK(DropDownMouseEnter)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = MenuColor->HighlightedColor;
}

MENU_EVENT_CALLBACK(DropDownMouseExit)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = MenuColor->RestingColor;
}

MENU_LOSING_FOCUS(DropDownLosingFocus)
{
  Menu->Visible = false;
}

MENU_GAINING_FOCUS(DropDownGainingFocus)
{
  Menu->Visible = true;
}

void AddPlugintoMainMenu(menu_interface* Interface, menu_tree* DropDownMenu, container_node* Plugin)
{
  container_node* DropDownGrid = DropDownMenu->Root->FirstChild;
  Assert(DropDownGrid->Type == container_type::Grid);
  Assert(Plugin->Type == container_type::Plugin);

  container_node* MenuItem = ConnectNodeToBack(DropDownGrid, NewContainer(Interface));
  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(GetPluginNode(Plugin)->Title, MenuText->Text);
  MenuText->FontSize = Interface->BodyFontSize;
  MenuText->Color = Interface->TextColor;

  color_attribute* DropDownColor = (color_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_COLOR);
  DropDownColor->Color = Interface->MenuColor;
  DropDownColor->HighlightedColor = Interface->MenuColor * 1.5;
  DropDownColor->RestingColor = Interface->MenuColor;

  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), Interface->BodyFontSize, (utf8_byte*)GetPluginNode(Plugin)->Title );
  DropDownMenu->Root->Region.H += TextSize.Y*2;
  DropDownMenu->Root->Region.W = DropDownMenu->Root->Region.W >= TextSize.X*1.2 ? DropDownMenu->Root->Region.W : TextSize.X*1.2;


  plugin_node* PluginNode = GetPluginNode(Plugin);
  RegisterMenuEvent(Interface, menu_event_type::MouseUp,    MenuItem, PluginNode->Tab, DropDownMouseUp, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, MenuItem, PluginNode->Tab, DropDownMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  MenuItem, PluginNode->Tab, DropDownMouseExit, 0);

  Assert(Interface->MainMenuTabCount < ArrayCount(Interface->MainMenuTabs));
  Interface->MainMenuTabs[Interface->MainMenuTabCount++] = DropDownMenu;
}

MENU_UPDATE_CHILD_REGIONS(MainWindowUpdateChildRegions)
{
  Assert(GetChildCount(Parent) == 2);
  Assert(Parent->Type == container_type::MainWindow);

  container_node* Child = Parent->FirstChild;
  Child->Region = Rect2f(0, 1 - Interface->HeaderSize, GetAspectRatio(Interface),  Interface->HeaderSize);

  Child = Child->NextSibling;
  Child->Region = Rect2f(0, 0, GetAspectRatio(Interface),  1 - Interface->HeaderSize);
}

menu_functions GetMainWindowFunctions(){
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, MainWindowUpdateChildRegions);
  return Result;
}