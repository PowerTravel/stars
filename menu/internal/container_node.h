#pragma once

struct menu_interface;
struct container_node;
struct menu_attribute_header;

#define MENU_UPDATE_CHILD_REGIONS(name) void name(menu_interface* Interface, container_node* Parent)
typedef MENU_UPDATE_CHILD_REGIONS( menu_get_region );

#define MENU_DRAW(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_DRAW( menu_draw );

struct menu_functions
{
  menu_get_region** UpdateChildRegions;
  menu_draw** Draw;
};

enum class container_type
{
  None,
  MainWindow,
  Root,
  Border,
  Split,
  Grid,
  Plugin,
  TabWindow,
  Tab
};

const c8* ToString(container_type Type)
{
  switch(Type)
  {
    case container_type::None: return "None";
    case container_type::MainWindow: return "MainWindow";  
    case container_type::Root: return "Root";
    case container_type::Border: return "Border";
    case container_type::Split: return "Split";
    case container_type::Grid: return "Grid";
    case container_type::Plugin: return "Plugin";
    case container_type::TabWindow: return "TabWindow";
    case container_type::Tab: return "Tab";
  }
  return "";
};

struct container_node
{
  container_type Type;
  u32 Attributes;
  menu_attribute_header* FirstAttribute;

  // Tree Links (Menu Structure)
  u32 Depth;
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;
  container_node* PreviousSibling;

  rect2f Region;

  menu_functions Functions;
};

inline u8* GetContainerPayload( container_node* Container )
{
  u8* Result = (u8*)(Container+1);
  return Result;
}


inline container_node* Previous(container_node* Node)
{
  return Node->PreviousSibling;
}

inline container_node* Next(container_node* Node)
{
  return Node->NextSibling;
}

u32 GetChildCount(container_node* Node)
{
  container_node* Child = Node->FirstChild;
  u32 Count = 0;
  while(Child)
  {
    Child = Next(Child);
    Count++;
  }
  return Count;
}

u32 GetChildIndex(container_node* Node)
{
  Assert(Node->Parent);
  container_node* Child = Node->Parent->FirstChild;
  u32 Count = 0;
  while(Child != Node)
  {
    Child = Next(Child);
    Count++;
  }
  return Count;
}

container_node* GetChildFromIndex(container_node* Parent, u32 ChildIndex)
{
  u32 Idx = 0;
  container_node* Result = Parent->FirstChild;
  while(Idx++ < ChildIndex)
  {
    Result = Next(Result);
    Assert(Result);
  }
  return Result; 
}
