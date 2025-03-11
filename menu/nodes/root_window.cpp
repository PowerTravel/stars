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
  Borders.Top = Borders.Right->NextSibling;
  Borders.Bot = Borders.Top->NextSibling;
  Borders.Body = Borders.Bot->NextSibling;

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
  if(RootNode->Maximized)
  {
    return;
  }

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

container_node* GetClosestTabWindow(container_node* Node)
{
  container_node* NodeToIterate = Node;
  while(NodeToIterate)
  {
    if(NodeToIterate->Type == container_type::TabWindow)
    {
      return NodeToIterate;
    }
    NodeToIterate = NodeToIterate->Parent;
  }

  return 0;
}

void Minimize(menu_interface* Interface, container_node* Node)
{
  container_node* Root = GetRoot(Node);

  root_node* RootNode = GetRootNode(Root);
  if(!RootNode->Maximized)
  {
    return;
  }

  container_node* TabWindow = GetClosestTabWindow(Node);
  if(TabWindow)
  {
    // TODO: Take whats in the TabWindow and put in a new menu tree
    //rect2f NewRegion = GetTabWindowNode(TabWindow)->CachedRegion;
    //NewRegion = Rect2f(0.25,0.25,0.25,0.25);
    //SplitTabToNewWindow(Interface, TabWindow->FirstChild->FirstChild, NewRegion);
    //return;
  }


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
    Minimize(Interface, TabHeader);
  }
}

struct border_drag_initiated
{
  v2 OriginalSize;
  v2 MousePosInRect;
};
MENU_UPDATE_FUNCTION(RootBorderDragUpdate)
{
  Assert(CallerNode->Parent->Type == container_type::Root);

  r32 AspectRatio = GetAspectRatio(Interface);
  
  root_border_collection BorderCollection = GetRoorBorders(CallerNode->Parent);

  position_attribute* Pos = (position_attribute*) GetAttributePointer(CallerNode->Parent, ATTRIBUTE_POSITION);
  absolute_size_attribute* Size = (absolute_size_attribute*) GetAttributePointer(CallerNode->Parent, ATTRIBUTE_ABS_SIZE);
  border_drag_initiated* BorderDrag = (border_drag_initiated*)Data;


  v2 MouseDownPos = Interface->MouseLeftButtonPush;
  v2 OriginalSize = BorderDrag->OriginalSize;
  v2 BorderSize = RectSize(CallerNode->Region);

  v2 MouseDownBotLeft = BorderDrag->MousePosInRect - MouseDownPos;
  v2 ControlBotLeft   = Interface->MousePos - BorderDrag->MousePosInRect;
  v2 MaximumWindowSize = V2(AspectRatio, 1 - Interface->HeaderSize);
  v2 MinimumWindowSize = V2(0.2,0.2);
  
  if(CallerNode == BorderCollection.Left)
  {
    r32 BoxRight = Pos->X + Size->Width;
    r32 NewPos = Clamp(ControlBotLeft.X, 0, BoxRight - MinimumWindowSize.X);
    Pos->X = NewPos;
    r32 MouseMovementDistance = NewPos + MouseDownBotLeft.X;
    Size->Width = OriginalSize.X - MouseMovementDistance;
  }else if(CallerNode == BorderCollection.Right){
    r32 BoxLeft = Pos->X;
    r32 NewPosX = Clamp(ControlBotLeft.X, BoxLeft + MinimumWindowSize.X - BorderSize.X, MaximumWindowSize.X - BorderSize.X);
    r32 MouseMovementDistance = NewPosX + MouseDownBotLeft.X;
    Size->Width = OriginalSize.X + MouseMovementDistance;
  }else if(CallerNode == BorderCollection.Top){
    r32 BoxBot = Pos->Y;
    r32 NewPosY = Clamp(ControlBotLeft.Y, BoxBot + MinimumWindowSize.Y - BorderSize.Y, MaximumWindowSize.Y - BorderSize.Y);
    r32 MouseMovementDistance = NewPosY + MouseDownBotLeft.Y;
    Size->Height = OriginalSize.Y + MouseMovementDistance;
  }else if(CallerNode == BorderCollection.Bot){
    r32 BoxTop = Pos->Y + Size->Height;
    r32 NewPos = Clamp(ControlBotLeft.Y, 0, BoxTop - MinimumWindowSize.Y);
    Pos->Y = NewPos;
    r32 MouseMovementDistance = NewPos + MouseDownBotLeft.Y;
    Size->Height = OriginalSize.Y - MouseMovementDistance;
  }else{
    INVALID_CODE_PATH;
  }
  return Interface->MouseLeftButton.Active;
}


MENU_EVENT_CALLBACK(InitiateBorderDrag)
{
  border_drag_initiated* BorderDrag =  (border_drag_initiated*) Allocate(&Interface->LinkedMemory, sizeof(border_drag_initiated));

  absolute_size_attribute* Size = (absolute_size_attribute*) GetAttributePointer(CallerNode->Parent, ATTRIBUTE_ABS_SIZE);
  BorderDrag->OriginalSize = V2(Size->Width, Size->Height);
  BorderDrag->MousePosInRect = Interface->MousePos - LowerLeftPoint(CallerNode->Region);
  PushToUpdateQueue(Interface, CallerNode, RootBorderDragUpdate, BorderDrag, true);
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, RootUpdateChildRegions);
  return Result;
}

container_node* CreateBorderNode(menu_interface* Interface)
{
  container_node* Result = NewContainer(Interface);
  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Result, ATTRIBUTE_COLOR);
  ColorAttr->Color = Interface->BorderColor;
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Result, 0, InitiateBorderDrag, 0);
  return Result;
}

container_node* CreateRootContainer(menu_interface* Interface, container_node* BodyContainer, rect2f RootRegion)
{ 
  container_node* Root = NewContainer(Interface, container_type::Root);
  position_attribute* Position = (position_attribute*) PushAttribute(Interface, Root, ATTRIBUTE_POSITION);
  Position->X = 0;
  Position->Y = 1 - Interface->HeaderSize;

  color_attribute* Color = (color_attribute*) PushAttribute(Interface, Root, ATTRIBUTE_COLOR);
  Color->Color = V4(0.5,0.5,0.5,1);

  absolute_size_attribute* Size = (absolute_size_attribute*) PushAttribute(Interface, Root, ATTRIBUTE_ABS_SIZE);
  Size->Width = 0.25;
  Size->Height = 0.25;

  ConnectNodeToBack(Root, CreateBorderNode(Interface));
  ConnectNodeToBack(Root, CreateBorderNode(Interface));
  ConnectNodeToBack(Root, CreateBorderNode(Interface));
  ConnectNodeToBack(Root, CreateBorderNode(Interface));
  ConnectNodeToBack(Root, BodyContainer);

  return Root;
}
