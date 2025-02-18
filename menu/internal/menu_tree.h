#pragma once

#define MENU_LOSING_FOCUS(name) void name(struct menu_interface* Interface, struct menu_tree* Menu)
typedef MENU_LOSING_FOCUS( menu_losing_focus );
#define MENU_GAINING_FOCUS(name) void name(struct menu_interface* Interface, struct menu_tree* Menu)
typedef MENU_GAINING_FOCUS( menu_gaining_focus );

struct draw_queue_entry{
  container_node* CallerNode;
  menu_draw** DrawFunction;
};

struct menu_tree
{
  b32 Visible;

  u32 NodeCount;
  u32 Depth;
  container_node* Root;

  u32 HotLeafCount;
  u32 NewLeafOffset;  
  container_node* HotLeafs[64];
  u32 RemovedHotLeafCount;
  container_node* RemovedHotLeafs[64];

  u32 FinalRenderCount; 
  draw_queue_entry FinalRenderFunctions[64];

  menu_tree* Next;
  menu_tree* Previous;

  menu_losing_focus** LosingFocus;
  menu_gaining_focus** GainingFocus;
};

void DeleteMenuSubTree(menu_interface* Interface, container_node* Root)
{
  DisconnectNode(Root);
  // Free the nodes;
  // 1: Go to the bottom
  // 2: Step up Once
  // 3: Delete FirstChild
  // 4: Set NextSibling as FirstChild
  // 5: Repeat from 1
  container_node* Node = Root->FirstChild;
  while(Node)
  {

    while(Node->FirstChild)
    {
      Node = Node->FirstChild;
    }

    Node = Node->Parent;
    if(Node)
    {
      container_node* NodeToDelete = Node->FirstChild;
      Node->FirstChild = Next(NodeToDelete);
      DeleteContainer(Interface, NodeToDelete);
    }
  }
  DeleteContainer(Interface, Root);
}

menu_tree* NewMenuTree(menu_interface* Interface);
void TreeSensus( menu_tree* Menu );
void FreeMenuTree(menu_interface* Interface, menu_tree* MenuToFree);