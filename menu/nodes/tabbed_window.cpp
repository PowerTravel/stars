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
  Assert(TabbedWindow->Type == container_type::TabWindow);
  container_node* TabRegion = TabbedWindow->FirstChild;
  Assert(TabRegion->Type == container_type::None);
  container_node* TabHeader = TabRegion->FirstChild;
  Assert(TabHeader->Type == container_type::Grid);
  return TabHeader;
}

internal void ReduceSplitWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  Assert(WindowToRemove->Parent->Type == container_type::Split);
  container_node* SplitNodeToSwapOut = WindowToRemove->Parent;

  split_windows SplitWindows = GetSplitWindows(SplitNodeToSwapOut);

  container_node* WindowToRemain = WindowToRemove == SplitWindows.FirstWindow ? SplitWindows.SecondWindow : SplitWindows.FirstWindow;

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


internal b32 TabDrag(menu_interface* Interface, container_node* Tab)
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


MENU_UPDATE_FUNCTION(TabDragUpdate)
{
  b32 Continue = TabDrag(Interface, CallerNode);
  return Interface->MouseLeftButton.Active && Continue;
}

MENU_EVENT_CALLBACK(TabMouseDown)
{
  container_node* Tab = CallerNode;

  // Initiate Tab Drag
  container_node* TabWindow = GetTabWindowFromTab(Tab);
  container_node* TabGrid = GetTabGridFromWindow(TabWindow);
  if(GetChildCount(TabGrid) > 1 || TabWindow->Parent->Type == container_type::Split)
  {
    PushToUpdateQueue(Interface, CallerNode, TabDragUpdate, 0, false);
  }
}

MENU_EVENT_CALLBACK(TabMouseUp)
{
  
}

MENU_EVENT_CALLBACK(TabMouseEnter)
{
  tab_node* TabNode = GetTabNode(CallerNode);
  if(!IsPluginSelected(Interface, TabNode->Payload))
  {
    color_attribute* TabColor =(color_attribute*) GetAttributePointer(CallerNode,ATTRIBUTE_COLOR);
    TabColor->Color = TabColor->HighlightedColor;
  }
}

MENU_EVENT_CALLBACK(TabMouseExit)
{
  tab_node* TabNode = GetTabNode(CallerNode);
  if(!IsPluginSelected(Interface, TabNode->Payload))
  {
    color_attribute* TabColor =(color_attribute*) GetAttributePointer(CallerNode,ATTRIBUTE_COLOR);
    TabColor->Color = TabColor->RestingColor;
  }
}

container_node* CreateTab(menu_interface* Interface, container_node* Plugin)
{
  plugin_node* PluginNode = GetPluginNode(Plugin);

  container_node* Tab = NewContainer(Interface, container_type::Tab);  

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_COLOR);
  ColorAttr->Color = Interface->MenuColor;
  ColorAttr->RestingColor = Interface->MenuColor;
  ColorAttr->HighlightedColor = Interface->MenuColor * 1.5;

  GetTabNode(Tab)->Payload = Plugin;
  PluginNode->Tab = Tab;

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Tab, 0, TabMouseDown, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseUp, Tab, 0, TabMouseUp, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, Tab, 0, TabMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit,  Tab, 0, TabMouseExit, 0);
  
  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_TEXT);
  jstr::CopyStringsUnchecked(PluginNode->Title, MenuText->Text);
  MenuText->FontSize = Interface->BodyFontSize;
  MenuText->Color = Interface->TextColor;

  v2 TextSize = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), Interface->HeaderFontSize, (utf8_byte*) PluginNode->Title);
  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, TextSize.X * 1.05);
  SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE_, 1);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->XAlignment = menu_region_alignment::LEFT;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;

  return Tab;
}

plugin_node* GetPluginNode(container_node* Container)
{
  Assert(Container->Type == container_type::Plugin);
  plugin_node* Result = (plugin_node*) GetContainerPayload(Container);
  return Result;
}

tab_window_node* GetTabWindowNode(container_node* Container)
{
  Assert(Container->Type == container_type::TabWindow);
  tab_window_node* Result = (tab_window_node*) GetContainerPayload(Container);
  return Result;
}

tab_node* GetTabNode(container_node* Container)
{
  Assert(Container->Type == container_type::Tab);
  tab_node* Result = (tab_node*) GetContainerPayload(Container);
  return Result;
}


internal u32 FillArrayWithTabs(u32 MaxArrSize, container_node* TabArr[], menu_tree* Menu)
{
  container_node* StartNode = GetBodyFromRoot(Menu->Root);
  SCOPED_TRANSIENT_ARENA;
  u32 StackCount = 0;
  u32 TabCount = 0;

  container_node** ContainerStack = PushArray(GlobalTransientArena, MaxArrSize, container_node*);

  // Push StartNode
  ContainerStack[StackCount++] = StartNode;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    if(Parent->Type == container_type::Split)
    {
      ContainerStack[StackCount++] = Next(Parent->FirstChild);
      ContainerStack[StackCount++] = Next(Next(Parent->FirstChild));
    }else if(Parent->Type == container_type::TabWindow)
    {
      ContainerStack[StackCount++] = GetTabGridFromWindow(Parent);
    }else if(Parent->Type == container_type::Grid)
    {
      container_node* Tab = Parent->FirstChild;
      Assert(Tab->Type == container_type::Tab);
      while(Tab)
      {
        container_node* TabToMove = Tab;
        Tab = Next(Tab);
        
        tab_node* TabNode = GetTabNode(TabToMove);
        if(TabNode->Payload->Type == container_type::Plugin)
        {
          PopTab(TabToMove);
          TabArr[TabCount++] = TabToMove;
        }else{
          ContainerStack[StackCount++] = TabNode->Payload;
        }
      }
      Assert(TabCount < MaxArrSize);
    }else{
      INVALID_CODE_PATH;
    }
  }
  return TabCount;
}

internal void UpdateMergableAttribute( menu_interface* Interface, container_node* Node )
{
  container_node* TabWindow = GetTabWindowFromOtherMenu(Interface, Node);
  if(!TabWindow)
  {
    return;
  }
  tab_window_node* TabWindowNode = GetTabWindowNode(TabWindow);
  TabWindowNode->HotMergeZone = merge_zone::NONE;
  if(TabWindow)
  {
    rect2f Rect = TabWindow->Region;
    r32 W = Rect.W;
    r32 H = Rect.H;
    r32 S = Minimum(W,H)/4;

    v2 CP = CenterPoint(Rect);  // Middle Point
    v2 LS = V2(CP.X-S, CP.Y);   // Left Square
    v2 RS = V2(CP.X+S, CP.Y);   // Right Square
    v2 BS = V2(CP.X,   CP.Y-S); // Bot Square
    v2 TS = V2(CP.X,   CP.Y+S); // Top Square

    TabWindowNode->MergeZone[(u32) merge_zone::CENTER] = Rect2f(CP.X-S/2.f, CP.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::LEFT]   = Rect2f(LS.X-S/2.f, LS.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::RIGHT]  = Rect2f(RS.X-S/2.f, RS.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::TOP]    = Rect2f(BS.X-S/2.f, BS.Y-S/2.f,S/1.1f,S/1.1f);
    TabWindowNode->MergeZone[(u32) merge_zone::BOT]    = Rect2f(TS.X-S/2.f, TS.Y-S/2.f,S/1.1f,S/1.1f);

    if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::CENTER)], Interface->MousePos))
    {
      TabWindowNode->HotMergeZone = merge_zone::CENTER;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::LEFT)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::LEFT;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::RIGHT)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::RIGHT;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::TOP)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::TOP;
    }else if(Intersects(TabWindowNode->MergeZone[EnumToIdx(merge_zone::BOT)], Interface->MousePos)){
      TabWindowNode->HotMergeZone = merge_zone::BOT;
    }else{
      TabWindowNode->HotMergeZone = merge_zone::HIGHLIGHTED;
    }
  }

  if(Interface->MouseLeftButton.Edge &&
    !Interface->MouseLeftButton.Active )
  {
    merge_zone MergeZone = TabWindowNode->HotMergeZone;
    container_node* TabWindowToAccept = TabWindow;
    container_node* NodeToInsert = Node;
    switch(TabWindowNode->HotMergeZone)
    {
      case merge_zone::CENTER:
      {
        menu_tree* MenuToRemove = GetMenu(Interface, NodeToInsert);

        Assert(TabWindowToAccept->Type == container_type::TabWindow);

        container_node* TabArr[64] = {};
        u32 TabCount = FillArrayWithTabs(ArrayCount(TabArr), TabArr, MenuToRemove);

        for(u32 Index = 0; Index < TabCount; ++Index)
        {
          container_node* TabToMove = TabArr[Index];
          PushTab(TabWindowToAccept, TabToMove);
        }

        SetTabAsActive(TabArr[TabCount-1]);
        
        FreeMenuTree(Interface, MenuToRemove);
        TabWindowNode->HotMergeZone = merge_zone::NONE;
      }break;
      case merge_zone::LEFT:    // Fallthrough 
      case merge_zone::RIGHT:   // Fallthrough
      case merge_zone::TOP:     // Fallthrough
      case merge_zone::BOT:
      {
        menu_tree* MenuToRemove = GetMenu(Interface, NodeToInsert);
        menu_tree* MenuToRemain = GetMenu(Interface, TabWindowToAccept);

        while(NodeToInsert && NodeToInsert->Type != container_type::Split)
        {
          NodeToInsert = NodeToInsert->Parent;
        }
        if(!NodeToInsert)
        {
          NodeToInsert = GetBodyFromRoot(MenuToRemove->Root);
          Assert(NodeToInsert->Type == container_type::TabWindow);
        }else{
          Assert(NodeToInsert->Type == container_type::Split);
        }
        
        Assert(TabWindowToAccept->Type == container_type::TabWindow);

        DisconnectNode(NodeToInsert);

        b32 Vertical = (TabWindowNode->HotMergeZone == merge_zone::LEFT || TabWindowNode->HotMergeZone == merge_zone::RIGHT);

        container_node* SplitNode = CreateSplitWindow(Interface, Vertical, 0.5);
        ReplaceNode(TabWindowToAccept, SplitNode);

        b32 VisitorIsFirstChild = (TabWindowNode->HotMergeZone == merge_zone::LEFT || TabWindowNode->HotMergeZone == merge_zone::TOP);
        if(VisitorIsFirstChild)
        {
          ConnectNodeToBack(SplitNode, NodeToInsert);
          ConnectNodeToBack(SplitNode, TabWindowToAccept);
        }else{
          ConnectNodeToBack(SplitNode, TabWindowToAccept);
          ConnectNodeToBack(SplitNode, NodeToInsert);
        }

        FreeMenuTree(Interface, MenuToRemove);
        TabWindowNode->HotMergeZone = merge_zone::NONE;
      }break;
    }
  }

  if(!Interface->MouseLeftButton.Active)
  {
    TabWindowNode->HotMergeZone = merge_zone::NONE;
  }
}


MENU_UPDATE_FUNCTION(WindowDragUpdate)
{
  mouse_position_in_window* PosInWindow = (mouse_position_in_window*) Data;
  container_node* RootContainer = GetRoot(CallerNode);
  if(GetRootNode(RootContainer)->Maximized)
  {
    return false;
  }

  root_border_collection Borders = GetRoorBorders(RootContainer);

  border_leaf* LeftBorder  = GetBorderNode(Borders.Left);
  border_leaf* RightBorder = GetBorderNode(Borders.Right);
  border_leaf* BotBorder   = GetBorderNode(Borders.Bot);
  border_leaf* TopBorder   = GetBorderNode(Borders.Top);
  
  r32 Width  = RightBorder->Position - LeftBorder->Position;
  r32 Height = TopBorder->Position   - BotBorder->Position;

  r32 AspectRatio = GetAspectRatio(Interface);
  v2 MousePos = V2(
    Clamp(Interface->MousePos.X, 0, AspectRatio),
    Clamp(Interface->MousePos.Y, 0, 1-Interface->HeaderSize - (Height - PosInWindow->RelativeWindow.Y)));
  v2 BotLeftWindowPos = MousePos - PosInWindow->RelativeWindow;

  LeftBorder->Position  = BotLeftWindowPos.X - LeftBorder->Thickness * 0.5;
  RightBorder->Position = BotLeftWindowPos.X + Width - RightBorder->Thickness * 0.5;
  BotBorder->Position   = BotLeftWindowPos.Y - LeftBorder->Thickness * 0.5;
  TopBorder->Position   = BotLeftWindowPos.Y + Height - TopBorder->Thickness * 0.5;
  
  UpdateMergableAttribute(Interface, CallerNode);

  return Interface->MouseLeftButton.Active;
}

container_node* CreatePlugin(menu_interface* Interface, menu_tree* WindowsDropDownMenu, c8* HeaderName, container_node* BodyNode)
{
  container_node* Plugin = NewContainer(Interface, container_type::Plugin);
  plugin_node* PluginNode = GetPluginNode(Plugin);
  jstr::CopyStringsUnchecked(HeaderName, PluginNode->Title);

  ConnectNodeToBack(Plugin, BodyNode);

  AddPlugintoMainMenu(Interface, WindowsDropDownMenu, Plugin);

  return Plugin;
}
