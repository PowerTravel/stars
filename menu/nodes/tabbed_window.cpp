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


container_node* GetTabGridFromWindow(container_node* TabbedWindow)
{
  Assert(TabbedWindow->Type == container_type::TabWindow); // Could also be split window
  container_node* TabRegion = TabbedWindow->FirstChild;
  Assert(TabRegion->Type == container_type::None);
  container_node* TabHeader = TabRegion->FirstChild;
  Assert(TabHeader->Type == container_type::Grid);
  return TabHeader;
}


internal void ReduceSplitWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  container_node* SplitNodeToSwapOut = WindowToRemove->Parent;
  container_node* WindowToRemain = WindowToRemove->NextSibling;
  if(!WindowToRemain)
  {
    WindowToRemain = WindowToRemove->Parent->FirstChild->NextSibling;
    Assert(WindowToRemain);
    Assert(WindowToRemain->NextSibling == WindowToRemove);
  }

  DisconnectNode(WindowToRemove);
  DeleteMenuSubTree(Interface, WindowToRemove);

  DisconnectNode(WindowToRemain);

  ReplaceNode(SplitNodeToSwapOut, WindowToRemain);
  DeleteMenuSubTree(Interface, SplitNodeToSwapOut);
}

void ReduceWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  // Just to let us know if we need to handle another case
  Assert(WindowToRemove->Type == container_type::TabWindow);

  // Make sure the tab window is empty
  Assert(GetChildCount(GetTabGridFromWindow(WindowToRemove)) == 0);

  container_node* ParentContainer = WindowToRemove->Parent;
  switch(ParentContainer->Type)
  {
    case container_type::Split:
    {
      ReduceSplitWindowTree(Interface, WindowToRemove);
    }break;
    case container_type::Root:
    {
      FreeMenuTree(Interface, GetMenu(Interface,ParentContainer));
    }break;
    default:
    {
      INVALID_CODE_PATH;
    } break;
  }
}



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

menu_functions GetTabWindowFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, UpdateTabWindowChildRegions);
  Result.Draw = DeclareFunction(menu_draw, TabWindowDraw);
  return Result; 
}



inline internal container_node* GetTabWindowFromTab(container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);
  container_node* TabHeader = Tab->Parent;
  Assert(TabHeader->Type == container_type::Grid);
  container_node* TabRegion = TabHeader->Parent;
  Assert(TabRegion->Type == container_type::None);
  container_node* TabWindow = TabRegion->Parent;

  Assert(TabWindow->Type == container_type::TabWindow);
  return TabWindow;
}


internal void SetTabAsActive(container_node* Tab)
{
  tab_node* TabNode = GetTabNode(Tab);
  container_node* TabWindow = GetTabWindowFromTab(Tab);
  container_node* TabBody = TabWindow->FirstChild->NextSibling;
  if(!TabBody)
  {
    ConnectNodeToBack(TabWindow, TabNode->Payload);
  }else{
    ReplaceNode(TabBody, TabNode->Payload);  
  }
}


internal b32 SplitWindowSignal(menu_interface* Interface, container_node* HeaderNode)
{
  rect2f HeaderSplitRegion = Shrink(HeaderNode->Region, -2*HeaderNode->Region.H);
  if(!Intersects(HeaderSplitRegion, Interface->MousePos))
  {
    return true;
  }
  return false;
}

internal container_node* PushTab(container_node* TabbedWindow,  container_node* Tab)
{
  container_node* TabContainer = GetTabGridFromWindow(TabbedWindow);
  ConnectNodeToFront(TabContainer, Tab);
  return Tab;
}


internal container_node* PopTab(container_node* Tab)
{
  tab_node* TabNode = GetTabNode(Tab);
  DisconnectNode(TabNode->Payload);
  if(Next(Tab))
  {
    SetTabAsActive(Next(Tab));  
  }else if(Previous(Tab)){
    SetTabAsActive(Previous(Tab));
  }

  DisconnectNode(Tab);
  return Tab;
}

internal void SplitTabToNewWindow(menu_interface* Interface, container_node* Tab, rect2f TabbedWindowRegion)
{
  rect2f NewRegion = Move(TabbedWindowRegion, V2(-Interface->BorderSize, Interface->BorderSize)*0.5); 
  
  container_node* NewTabWindow = CreateTabWindow(Interface);
  CreateNewRootContainer(Interface, NewTabWindow, NewRegion);

  PopTab(Tab);
  PushTab(NewTabWindow, Tab);

  ConnectNodeToBack(NewTabWindow, GetTabNode(Tab)->Payload);

  // Trigger the window-drag instead of this tab-drag
  mouse_position_in_window* Position = (mouse_position_in_window*) Allocate(&Interface->LinkedMemory, sizeof(mouse_position_in_window));
  *Position = GetPositionInRootWindow(Interface->MousePos, NewTabWindow);
  PushToUpdateQueue(Interface, Tab, WindowDragUpdate, Position, true);
}


b32 TabDrag(menu_interface* Interface, container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);

  container_node* TabWindow = GetTabWindowFromTab(Tab);
  container_node* TabHeader = GetTabGridFromWindow(TabWindow);


  b32 Continue = Interface->MouseLeftButton.Active; // Tells us if we should continue to call the update function;

  SetTabAsActive(Tab);

  u32 Count = GetChildCount(TabHeader);
  if(Count > 1)
  {
    r32 dX = Interface->MousePos.X - Interface->PreviousMousePos.X;
    u32 Index = GetChildIndex(Tab);
    if(dX < 0 && Index > 0)
    {
      container_node* PreviousTab = Previous(Tab);
      Assert(PreviousTab);
      if(Interface->MousePos.X < (PreviousTab->Region.X + PreviousTab->Region.W/2.f))
      {
        // Note: A bit of a hacky way to handle an issue. When the user pushes the mouse down, we store that mouse location in the Interface.
        // If a person shifts the tab that mouse down position no longer holds the distance within the tab where the button was pushed.
        // This means if a user Shifts a tab then splits it into a new window, the calculation below for NewRegion is wrong because 
        // Mouse Left Button Push will no longer hold the information about where in the tab the user klicked.
        // Therefore we update the "MouseLeftButtonPush" here to keep that information. It's hacky because it's no longer hold the actual mouseDownPosition.
        // This is not an issue as long as we don't make use of the original mouseLeftButtonPush location somewehre else before the user klicks again.
        // Should not be an issue since a tab-trag ends with a mouse-up event. Next mouse klick should not need to know where the previous klick was,
        // but if any weird behaviour occurs later related to MouseLeftButtonPush, know that this is a possible cause of the issue.
        Interface->MouseLeftButtonPush.X -= PreviousTab->Region.W;
        ShiftLeft(Tab);
      }
    }else if(dX > 0 && Index < Count-1){
      container_node* NextTab = Next(Tab);
      Assert(NextTab);
      if(Interface->MousePos.X > (NextTab->Region.X + NextTab->Region.W/2.f))
      {
        // Note: See above
        Interface->MouseLeftButtonPush.X += NextTab->Region.W;
        ShiftRight(Tab);
      }
    }else if(SplitWindowSignal(Interface, Tab->Parent))
    {
      // Note: See above
      v2 MouseDiffFromClick = Interface->MousePos - Interface->MouseLeftButtonPush - (LowerLeftPoint(Tab->Parent->Region) - LowerLeftPoint(Tab->Region));
      rect2f NewRegion = Move(TabWindow->Region, MouseDiffFromClick);
      SplitTabToNewWindow(Interface, Tab, NewRegion);
      Continue = false;
    }
  }else if(TabWindow->Parent->Type == container_type::Split)
  {
    if(SplitWindowSignal(Interface, Tab->Parent))
    {
      // Note: See above
      v2 MouseDiffFromClick = Interface->MousePos - Interface->MouseLeftButtonPush - (LowerLeftPoint(Tab->Parent->Region) - LowerLeftPoint(Tab->Region));
      rect2f NewRegion = Move(TabWindow->Region, MouseDiffFromClick);
      SplitTabToNewWindow(Interface, Tab, NewRegion);
      ReduceWindowTree(Interface, TabWindow);
      Continue = false;
    }  
  }
  return Continue;
}
