#include "menu/tabbed_window/root_window.h"
#include "menu/menu_interface_internal.h"

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
  container_node* Result = RootWindow->FirstChild->NextSibling->NextSibling->NextSibling->NextSibling;
  return Result;
}

mouse_position_in_window* GetPositionInRootWindow(menu_interface* Interface, container_node* Node)
{
  container_node* RootBodyNode = GetBodyFromRoot(GetRoot(Node));
  mouse_position_in_window* Position = (mouse_position_in_window*) Allocate(&Interface->LinkedMemory, sizeof(mouse_position_in_window));
  Position->MousePos = Interface->MousePos;
  Position->RelativeWindow = Position->MousePos - LowerLeftPoint(RootBodyNode->Region);
  return Position;
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
  menu_tree* Menu = GetMenu(Interface, Parent);
//  if(Menu->Maximized)
//  {
//    container_node* Body = GetBodyFromRoot(Parent);
//    Body->Region = Rect2f(0,0, GetAspectRatio(Interface),1-Interface->HeaderSize);
//    Parent->Region = Rect2f(0,0, GetAspectRatio(Interface),1-Interface->HeaderSize);
//  }else{
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
  
 // }

}
