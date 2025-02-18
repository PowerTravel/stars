#include "root_window.h"
#include "border_node.h"
#include "menu_interface.h"

container_node* GetRoot(container_node* Node)
{
  while(Node->Parent)
  {
    Node = Node->Parent;
  }
  return Node;
}

container_node* GetBodyFromRoot(container_node* RootWindow)
{
  Assert(RootWindow->Type == container_type::Root);
  
  container_node* Result = RootWindow->FirstChild;
  while(Result->Type == container_type::Border)
  {
    Result = Result->NextSibling;
  }
  
  return Result;
}

root_node* GetRootNode(container_node* Container)
{
  Assert(Container->Type == container_type::Root);
  root_node* Result = (root_node*) GetContainerPayload(Container);
  return Result;
}

mouse_position_in_window GetPositionInRootWindow(v2 Pos, container_node* Node)
{
  container_node* RootBodyNode = GetBodyFromRoot(GetRoot(Node));
  mouse_position_in_window Result = {};
  Result.MousePos = Pos;
  Result.RelativeWindow = Result.MousePos - LowerLeftPoint(RootBodyNode->Region);
  return Result;
}

root_border_collection GetRoorBorders(container_node* RootContainer)
{
  Assert(RootContainer->Type == container_type::Root);
  root_border_collection Borders = {};
  Borders.Left = RootContainer->FirstChild;
  Borders.Right = Borders.Left->NextSibling;
  Borders.Bot = Borders.Right->NextSibling;
  Borders.Top = Borders.Bot->NextSibling;

  Assert(Borders.Left->Type  == container_type::Border);
  Assert(Borders.Right->Type == container_type::Border);
  Assert(Borders.Bot->Type   == container_type::Border);
  Assert(Borders.Top->Type   == container_type::Border);

  return Borders;
}


MENU_UPDATE_CHILD_REGIONS(RootUpdateChildRegions)
{
  root_node* Root = GetRootNode(Parent);
  if(Root->Maximized)
  {
    Parent->FirstChild->Region = Parent->Region;
  }else{
    container_node* Child = Parent->FirstChild;
    u32 BorderIndex = 0;
    container_node* Body = 0;
    container_node* Header = 0;
    container_node* BorderNodes[4] = {};
    border_leaf* Borders[4] = {};
    while(Child)
    {
      if(Child->Type == container_type::Border)
      {
        // Left->Right->Bot->Top;
        BorderNodes[BorderIndex] = Child;
        Borders[BorderIndex] = GetBorderNode(Child);
        BorderIndex++;
      }else{
        if(!Body)
        {
          Assert(!Body);
          Body = Child;
        }
        
      }
      Child = Next(Child);
    }

    r32 Width  = (Borders[1]->Position + 0.5f*Borders[1]->Thickness) - ( Borders[0]->Position - 0.5f*Borders[0]->Thickness);
    r32 Height = (Borders[3]->Position + 0.5f*Borders[3]->Thickness) - ( Borders[2]->Position - 0.5f*Borders[2]->Thickness);
    for (BorderIndex = 0; BorderIndex < ArrayCount(Borders); ++BorderIndex)
    {
      switch(BorderIndex)
      {
        case 0: // Left
        {
          BorderNodes[0]->Region = Rect2f(
            Borders[0]->Position - 0.5f * Borders[0]->Thickness,
            Borders[2]->Position - 0.5f * Borders[2]->Thickness,
            Borders[0]->Thickness,
            Height);
        }break;
        case 1: // Right
        {
          BorderNodes[1]->Region = Rect2f(
            Borders[1]->Position - 0.5f * Borders[1]->Thickness,
            Borders[2]->Position - 0.5f * Borders[2]->Thickness,
            Borders[1]->Thickness,
            Height);
        }break;
        case 2: // Bot
        {
          BorderNodes[2]->Region = Rect2f(
            Borders[0]->Position - 0.5f * Borders[0]->Thickness,
            Borders[2]->Position - 0.5f * Borders[2]->Thickness,
            Width,
            Borders[2]->Thickness);
        }break;
        case 3: // Top
        {
          BorderNodes[3]->Region = Rect2f(
            Borders[0]->Position - 0.5f * Borders[0]->Thickness,
            Borders[3]->Position - 0.5f * Borders[3]->Thickness,
            Width,
            Borders[3]->Thickness);
        }break;
      }
    }

    Assert(BorderIndex == 4);
    Assert(Body);

    rect2f InteralRegion = Rect2f(
      Borders[0]->Position + 0.5f*Borders[0]->Thickness,
      Borders[2]->Position + 0.5f*Borders[2]->Thickness,
      Width  - (Borders[0]->Thickness + Borders[1]->Thickness),
      Height - (Borders[2]->Thickness + Borders[3]->Thickness));

    r32 BodyHeight = InteralRegion.H;

    Body->Region   = Rect2f(InteralRegion.X, InteralRegion.Y, InteralRegion.W, BodyHeight);

    Parent->Region = Rect2f(
      Borders[0]->Position - 0.5f*Borders[0]->Thickness,
      Borders[2]->Position - 0.5f*Borders[2]->Thickness,
      Width,
      Height);
  }
}

menu_tree* GetAlreadyMaximizedMenuTree(menu_interface* Interface)
{
  menu_tree* Menu = Interface->MenuSentinel.Next;
  while(Menu != &Interface->MenuSentinel)
  {
    container_node* Root = Menu->Root;
    if(Root->Type == container_type::Root)
    {
      root_node* RootNode = GetRootNode(Root);
      if(RootNode->Maximized)
      {
        return Menu;
      }
    }
    Menu = Menu->Next;
  }
  return 0;
}

void InitiateRootWindowDrag(menu_interface* Interface, container_node* Node)
{
  Assert(Node->Parent->Type == container_type::TabWindow);
  mouse_position_in_window* Position = (mouse_position_in_window*) Allocate(&Interface->LinkedMemory, sizeof(mouse_position_in_window));
  *Position = GetPositionInRootWindow(Interface->MousePos, Node);
  PushToUpdateQueue(Interface, Node, WindowDragUpdate, Position, true);
}

void Maximize(menu_interface* Interface, container_node* Root)
{
  root_node* RootNode = GetRootNode(Root);
  root_border_collection Borders = GetRoorBorders(Root);
  container_node* Body = GetBodyFromRoot(Root);

  Body->PreviousSibling = 0;
  Root->FirstChild = Body;
  
  DeleteContainer(Interface, Borders.Left);
  DeleteContainer(Interface, Borders.Right);
  DeleteContainer(Interface, Borders.Top);
  DeleteContainer(Interface, Borders.Bot);

  RootNode->CachedRegion = Root->Region;
  Root->Region = Rect2f(0, 0, GetAspectRatio(Interface), 1-Interface->HeaderSize);
  RootNode->Maximized = true;
  UpdateFocusWindow(Interface);
}

void Minimize(menu_interface* Interface, container_node* Root)
{
  root_node* RootNode = GetRootNode(Root);
  rect2f Region = RootNode->CachedRegion;
  r32 Thickness = Interface->BorderSize;
  
  container_node* Border1 = CreateBorderNode(Interface, Interface->BorderColor);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border1, 0, InitiateBorderDrag, 0);
  SetBorderData(Border1, Thickness, Region.X, border_type::LEFT);

  container_node* Border2 = CreateBorderNode(Interface, Interface->BorderColor);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border2, 0, InitiateBorderDrag, 0);
  SetBorderData(Border2, Thickness, Region.X + Region.W, border_type::RIGHT);

  container_node* Border3 = CreateBorderNode(Interface, Interface->BorderColor);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border3, 0, InitiateBorderDrag, 0);
  SetBorderData(Border3, Thickness, Region.Y,  border_type::BOTTOM);
  
  container_node* Border4 = CreateBorderNode(Interface, Interface->BorderColor);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border4, 0, InitiateBorderDrag, 0);
  SetBorderData(Border4, Thickness, Region.Y + Region.H, border_type::TOP);
  
  
  ConnectNodeToFront(Root, Border4);
  ConnectNodeToFront(Root, Border3);
  ConnectNodeToFront(Root, Border2);
  ConnectNodeToFront(Root, Border1);
  RootNode->Maximized = false;
  UpdateFocusWindow(Interface);
}

void ToggleMaximizeWindow(menu_interface* Interface, menu_tree* Menu, container_node* TabHeader)
{
  container_node* Root = Menu->Root;
  root_node* RootNode = GetRootNode(Menu->Root);
  if(!RootNode->Maximized)
  {
    menu_tree* MaximizedMenu = GetAlreadyMaximizedMenuTree(Interface);
    if(!MaximizedMenu)
    {
      Maximize(Interface, Root);
    }else{
      InitiateRootWindowDrag(Interface, TabHeader);
    }
  }else{
    Minimize(Interface, Root)
  }
}

internal rect2f GetMinimumRootWindowSize(root_border_collection* BorderCollection, r32 MinimumRegionWidth, r32 MinimumRegionHeight)
{
  rect2f Result = {};
  Result.X = GetBorderNode(BorderCollection->Left)->Position  + MinimumRegionWidth*0.5;
  Result.W = GetBorderNode(BorderCollection->Right)->Position - MinimumRegionWidth*0.5 - Result.X;
  Result.Y = GetBorderNode(BorderCollection->Bot)->Position   + MinimumRegionHeight*0.5;
  Result.H = GetBorderNode(BorderCollection->Top)->Position   - MinimumRegionHeight*0.5 - Result.Y;
  return Result;
}

MENU_UPDATE_FUNCTION(RootBorderDragUpdate)
{
  Assert(CallerNode->Parent->Type == container_type::Root);

  r32 AspectRatio = GetAspectRatio(Interface);
  rect2f MaximumWindowSize = Rect2f(0,0,AspectRatio, 1 - Interface->HeaderSize);
  root_border_collection BorderCollection = GetRoorBorders(CallerNode->Parent);
  rect2f MinimumWindowSize = GetMinimumRootWindowSize(&BorderCollection, Interface->MinSize, Interface->MinSize);
  UpdateBorderPosition(CallerNode, Interface->MousePos, MinimumWindowSize, MaximumWindowSize);
  return Interface->MouseLeftButton.Active;
}

MENU_EVENT_CALLBACK(InitiateBorderDrag)
{
  PushToUpdateQueue(Interface, CallerNode, RootBorderDragUpdate, 0, false);
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, RootUpdateChildRegions);
  return Result;
}

container_node* CreateRootContainer(menu_interface* Interface, container_node* BodyContainer, rect2f RootRegion)
{ 
  container_node* Root = NewContainer(Interface, container_type::Root);

  r32 Thickness = Interface->BorderSize;

  container_node* Border1 = CreateBorderNode(Interface, Interface->BorderColor);
  ConnectNodeToBack(Root, Border1);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border1, 0, InitiateBorderDrag, 0);
  SetBorderData(Border1, Thickness, RootRegion.X, border_type::LEFT);

  container_node* Border2 = CreateBorderNode(Interface, Interface->BorderColor);
  ConnectNodeToBack(Root, Border2);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border2, 0, InitiateBorderDrag, 0);
  SetBorderData(Border2, Thickness, RootRegion.X + RootRegion.W, border_type::RIGHT);

  container_node* Border3 = CreateBorderNode(Interface, Interface->BorderColor);
  ConnectNodeToBack(Root, Border3);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border3, 0, InitiateBorderDrag, 0);
  SetBorderData(Border3, Thickness, RootRegion.Y,  border_type::BOTTOM);
  
  container_node* Border4 = CreateBorderNode(Interface, Interface->BorderColor);
  ConnectNodeToBack(Root, Border4);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border4, 0, InitiateBorderDrag, 0);
  SetBorderData(Border4, Thickness, RootRegion.Y + RootRegion.H,  border_type::TOP);

  ConnectNodeToBack(Root, BodyContainer);

  return Root;
}
