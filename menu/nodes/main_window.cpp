#include "main_window.h"
#include "menu_interface.h"

menu_tree* CreateMainWindow(menu_interface* Interface){

  menu_tree* MainWindow = NewMenuTree(Interface);
  MainWindow->Visible = true;

  {
    container_node* HeaderBar = ConnectNodeToBack(MainWindow->Root, NewContainer(Interface, container_type::None));
    HeaderBar->StackHorizontal = true;

    color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, HeaderBar, ATTRIBUTE_COLOR);
    ColorAttr->Color = Interface->MenuColor;
    ColorAttr->HighlightedColor = ColorAttr->Color;
    ColorAttr->RestingColor = ColorAttr->Color;

    absolute_size_attribute* Size = (absolute_size_attribute*) PushAttribute(Interface, HeaderBar, ATTRIBUTE_ABS_SIZE);
    Size->Width = GetAspectRatio(Interface);
    Size->Height = Interface->HeaderSize;
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
  position_attribute* Pos = (position_attribute*) GetAttributePointer(NewWindow->Root, ATTRIBUTE_POSITION);

  Pos->X = 0.5;
  Pos->Y = 0.5;

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
  Menu->Visible = !Menu->Visible;
  position_attribute* Position = (position_attribute*) GetAttributePointer(Menu->Root, ATTRIBUTE_POSITION);
  Position->X = CallerNode->Region.X;
  Position->Y = CallerNode->Region.Y - Menu->Root->Region.H;
}

MENU_EVENT_CALLBACK(HeaderMenuMouseEnter)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
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
  Assert(DropDownContainer->Type == container_type::None);
  Assert(DropDownContainer->StackHorizontal == true);

  container_node* NewMenu = ConnectNodeToBack(DropDownContainer, NewContainer(Interface));

  container_node* MenuButton = ConnectNodeToBack(NewMenu, NewContainer(Interface));

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, MenuButton, ATTRIBUTE_COLOR);
  ColorAttr->Color = Interface->MenuColor;
  ColorAttr->RestingColor = ColorAttr->Color;
  ColorAttr->HighlightedColor = ColorAttr->Color*1.5;

  absolute_size_attribute* SizeAttr = (absolute_size_attribute*) PushAttribute(Interface, MenuButton, ATTRIBUTE_ABS_SIZE);
  SizeAttr->Width = TextSize.X * 1.2f;
  SizeAttr->Height = CanonicalFontHeight;

  text_attribute* Text = (text_attribute*) PushAttribute(Interface, MenuButton, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(Name, Text->Text);
  Text->FontSize = Interface->HeaderFontSize;
  Text->Color = Interface->TextColor;


  menu_tree* MenuDropDown = NewMenuTree(Interface);
  MenuDropDown->Visible = false;
  position_attribute* PositionAttr = (position_attribute*) GetAttributePointer(MenuDropDown->Root, ATTRIBUTE_POSITION);
  PositionAttr->X = 0;
  PositionAttr->Y = 1-Interface->HeaderSize;

  color_attribute* ViewMenuColor = (color_attribute*) PushAttribute(Interface, MenuDropDown->Root, ATTRIBUTE_COLOR);
  ViewMenuColor->Color = Interface->MenuColor;
  ViewMenuColor->HighlightedColor = Interface->MenuColor;
  ViewMenuColor->RestingColor = Interface->MenuColor;

  RegisterMenuEvent(Interface, menu_event_type::MouseDown,  MenuButton, (void*) MenuDropDown, DropDownMenuButton, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, MenuButton, 0, HeaderMenuMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  MenuButton, 0, HeaderMenuMouseExit, 0);

  return MenuDropDown;
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

void AddPlugintoMainMenu(menu_interface* Interface, menu_tree* MenuItemContainer, container_node* Plugin)
{
  Assert(Plugin->Type == container_type::Plugin);

  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), Interface->BodyFontSize, (utf8_byte*) GetPluginNode(Plugin)->Title );
  r32 Height = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), Interface->BodyFontSize);

  r32 Width = TextSize.X*1.2;
  container_node* Child = GetFirstChild(MenuItemContainer->Root);
  while(Child)
  {
    absolute_size_attribute* ChildSize = (absolute_size_attribute*) GetAttributePointer(Child, ATTRIBUTE_ABS_SIZE);
    if(Width < ChildSize->Width)
    {
      Width = ChildSize->Width;
    }
    Child = Next(Child);
  }

  container_node* MenuItem = ConnectNodeToBack(MenuItemContainer->Root, NewContainer(Interface));
  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(GetPluginNode(Plugin)->Title, MenuText->Text);
  MenuText->FontSize = Interface->BodyFontSize;
  MenuText->Color = Interface->TextColor;

  color_attribute* DropDownColor = (color_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_COLOR);
  DropDownColor->Color = Interface->MenuColor;
  DropDownColor->HighlightedColor = Interface->MenuColor * 1.5;
  DropDownColor->RestingColor = Interface->MenuColor;

  absolute_size_attribute* Size = (absolute_size_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_ABS_SIZE);

  Child = GetFirstChild(MenuItemContainer->Root);
  u32 ContainerCount = GetChildCount(MenuItemContainer->Root)+1;
  while(Child)
  {
    absolute_size_attribute* ChildSize = (absolute_size_attribute*) GetAttributePointer(Child, ATTRIBUTE_ABS_SIZE);
    Size->Height = Height;
    Size->Width = Width;
    ContainerCount += GetChildCount(Child);
    Child = Next(Child);  

  }

  UpdateRegionsOfContainerTree2(Interface, ContainerCount, MenuItemContainer->Root);
  


  plugin_node* PluginNode = GetPluginNode(Plugin);
  RegisterMenuEvent(Interface, menu_event_type::MouseUp,    MenuItem, PluginNode->Tab, DropDownMouseUp, 0);
  //RegisterMenuEvent(Interface, menu_event_type::MouseEnter, MenuItem, PluginNode->Tab, DropDownMouseEnter, 0);
  //RegisterMenuEvent(Interface, menu_event_type::MouseExit,  MenuItem, PluginNode->Tab, DropDownMouseExit, 0);
//
 // Assert(Interface->MainMenuTabCount < ArrayCount(Interface->MainMenuTabs));
 // Interface->MainMenuTabs[Interface->MainMenuTabCount++] = DropDownMenu;
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