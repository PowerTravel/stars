#include "menu_tree.h"

menu_tree* NewMenuTree(menu_interface* Interface)
{
  menu_tree* Result = (menu_tree*) Allocate(&Interface->LinkedMemory, sizeof(menu_tree));
  Result->LosingFocus = DeclareFunction(menu_losing_focus, DefaultLosingFocus);
  Result->GainingFocus = DeclareFunction(menu_gaining_focus, DefaultGainingFocus);
  ListInsertBefore(&Interface->MenuSentinel, Result);
  return Result;
}

//  PostOrder (Left, Right, Root),  Depth first.
u32_pair UpdateSubTreeDepthAndCount( u32 ParentDepth, container_node* SubTreeRoot )
{
  u32 TotalDepth = 0;
  u32 CurrentDepth = ParentDepth;
  u32 NodeCount = 0;

  // Make SubTreeRoot look like an actual root node
  container_node* SubTreeParent = SubTreeRoot->Parent;
  container_node* SubTreeSibling = Next(SubTreeRoot);

  SubTreeRoot->Parent = 0;
  SubTreeRoot->NextSibling = 0;

  container_node* CurrentNode = SubTreeRoot;

  while(CurrentNode != SubTreeRoot->Parent)
  {
    // Set the depth of the current Node
    CurrentNode->Depth = CurrentDepth++;
    ++NodeCount;

    // Step all the way down (setting depth as you go along)
    while(CurrentNode->FirstChild)
    {
      CurrentNode = CurrentNode->FirstChild;
      CurrentNode->Depth = CurrentDepth++;
      ++NodeCount;
    }

    // The depth is now set until the leaf.
    TotalDepth = Maximum(CurrentDepth, TotalDepth);

    // Step up until you find another sibling or we reach root
    while(!Next(CurrentNode) && CurrentNode->Parent)
    {
      CurrentNode = CurrentNode->Parent;
      CurrentDepth--;
      Assert(CurrentDepth >= 0)
    }

    CurrentDepth--;

    // Either we found another sibling and we can traverse that part of the tree
    //  or we are at root and root has no siblings and we are done.
    CurrentNode = Next(CurrentNode);
  }

  // Restore the Root
  SubTreeRoot->Parent = SubTreeParent;
  SubTreeRoot->NextSibling = SubTreeSibling;

  u32_pair Result = {};
  Result.a = NodeCount;
  Result.b = TotalDepth;

  return Result;
}

void TreeSensus( menu_tree* Menu )
{
  u32_pair Pair =  UpdateSubTreeDepthAndCount( 0, Menu->Root );

  Menu->NodeCount = Pair.a;
  Menu->Depth = Pair.b;
}

inline internal menu_tree* GetNextSpawningWindow(menu_interface* Interface)
{ 
  menu_tree* Result = 0;
  menu_tree* SpawningMenu = Interface->MenuSentinel.Next;
  while(SpawningMenu != &(Interface->MenuSentinel))
  {
    if(SpawningMenu->Root->Type == container_type::Root &&
       SpawningMenu != Interface->SpawningWindow )
    {
      Result = SpawningMenu;
      break;
    }
    SpawningMenu = SpawningMenu->Next;
  }

  return Result;
}

void FreeMenuTree(menu_interface* Interface, menu_tree* MenuToFree)
{
  TreeSensus(MenuToFree);
  if(MenuToFree == Interface->SpawningWindow)
  {
    Interface->SpawningWindow = GetNextSpawningWindow(Interface);
  }
  if(MenuToFree == Interface->MenuInFocus)
  {
    CallFunctionPointer(MenuToFree->LosingFocus, Interface, MenuToFree);
    Interface->MenuInFocus = 0;
  }


  Assert(MenuToFree != &Interface->MenuSentinel);

  ListRemove( MenuToFree );
  container_node* Root = MenuToFree->Root;

  FreeMemory(&Interface->LinkedMemory, (void*)MenuToFree);

  DeleteMenuSubTree(Interface, Root);
}


MENU_LOSING_FOCUS(DefaultLosingFocus)
{

}

MENU_GAINING_FOCUS(DefaultGainingFocus)
{

}

