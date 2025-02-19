#include "main_window.h"
#include "menu_interface.h"

menu_tree* CreateMainWindow(menu_interface* Interface){

  menu_tree* MainWindow = NewMenuTree(Interface);
  MainWindow->Visible = true;
  MainWindow->Root = NewContainer(Interface, container_type::MainWindow);
  MainWindow->Root->Region = Rect2f(0,0,GetAspectRatio(Interface),1);

  {
    container_node* HeaderBar = ConnectNodeToBack(MainWindow->Root, NewContainer(Interface));

    size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, HeaderBar, ATTRIBUTE_SIZE);
    SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, 1);
    SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, Interface->HeaderSize);
    SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
    SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
    SizeAttr->XAlignment = menu_region_alignment::LEFT;
    SizeAttr->YAlignment = menu_region_alignment::TOP;

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
    size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, Body, ATTRIBUTE_SIZE);
    SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, 1);
    SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, 1-Interface->HeaderSize);
    SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
    SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
    SizeAttr->XAlignment = menu_region_alignment::LEFT;
    SizeAttr->YAlignment = menu_region_alignment::BOT;
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

menu_tree* CreateNewDropDownMenuItem(menu_interface* Interface, const c8* Name)
{
  ecs::render::system* RenderSystem = GetRenderSystem();
  ecs::render::window_size_pixel WindowSize = ecs::render::GetWindowSize(RenderSystem);
  r32 PixelFontHeight = ecs::render::GetLineSpacingPixelSpace(RenderSystem, Interface->HeaderFontSize);
  r32 CanonicalFontHeight = Interface->HeaderSize;
  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(RenderSystem, Interface->HeaderFontSize, (utf8_byte*) Name);

  container_node* MainSettingBar =  Interface->MenuBar->Root->FirstChild;
  container_node* DropDownContainer = MainSettingBar->FirstChild;
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
  SizeAttr->XAlignment = menu_region_alignment::CENTER;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;

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

  container_node* Tab = CreateTab(Interface, Plugin);

  Interface->PermanentWindows[Interface->PermanentWindowCount++] = Tab;

  RegisterMenuEvent(Interface, menu_event_type::MouseUp,    MenuItem, Tab, DropDownMouseUp, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, MenuItem, Tab, DropDownMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  MenuItem, Tab, DropDownMouseExit, 0);

  Assert(Interface->MainMenuTabCount < ArrayCount(Interface->MainMenuTabs));
  Interface->MainMenuTabs[Interface->MainMenuTabCount++] = DropDownMenu;
}