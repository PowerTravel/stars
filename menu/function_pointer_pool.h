#pragma once
#include "menu/menu_interface.h"
#include "commons/string.h"

MENU_LOSING_FOCUS(DefaultLosingFocus);
MENU_LOSING_FOCUS(DropDownLosingFocus);

MENU_LOSING_FOCUS(DefaultGainingFocus);
MENU_GAINING_FOCUS(DropDownGainingFocus);

MENU_UPDATE_CHILD_REGIONS(UpdateChildRegions);
MENU_UPDATE_CHILD_REGIONS(MainHeaderUpdate);
MENU_UPDATE_CHILD_REGIONS(RootUpdateChildRegions);
MENU_UPDATE_CHILD_REGIONS(UpdateSplitChildRegions);
MENU_UPDATE_CHILD_REGIONS(UpdateGridChildRegions);
MENU_UPDATE_CHILD_REGIONS(UpdateTabWindowChildRegions);

MENU_DRAW(DrawMergeSlots);
MENU_DRAW(TabWindowDraw);
MENU_DRAW(DrawFunctionTimeline);
MENU_DRAW(DrawStatistics);
MENU_DRAW(DrawFrameFunctions);
MENU_DRAW(RenderScene);
MENU_UPDATE_FUNCTION(SceneTakeInput);

MENU_EVENT_CALLBACK(DebugToggleButton);
MENU_EVENT_CALLBACK(DebugRecompileButton);

MENU_EVENT_CALLBACK(TabMouseDown);
MENU_EVENT_CALLBACK(TabMouseUp);
MENU_EVENT_CALLBACK(TabMouseEnter);
MENU_EVENT_CALLBACK(TabMouseExit);
MENU_EVENT_CALLBACK(TabWindowHeaderMouseDown);
MENU_EVENT_CALLBACK(InitiateSplitWindowBorderDrag);
MENU_EVENT_CALLBACK(InitiateBorderDrag);

MENU_UPDATE_FUNCTION(TabDragUpdate);
MENU_UPDATE_FUNCTION(WindowDragUpdate);
MENU_UPDATE_FUNCTION(RootBorderDragUpdate);
MENU_UPDATE_FUNCTION(SplitWindowBorderUpdate);

MENU_EVENT_CALLBACK(DropDownMouseEnter);
MENU_EVENT_CALLBACK(DropDownMouseExit);
MENU_EVENT_CALLBACK(DropDownMouseUp);
MENU_EVENT_CALLBACK(DropDownMenuButton);


MENU_EVENT_CALLBACK(HeaderMenuMouseEnter);
MENU_EVENT_CALLBACK(HeaderMenuMouseExit);


#ifdef JWIN_INTERNAL

#define RedeclareFunction(Name) _DeclareFunction((func_ptr_void) (&Name), #Name );
#define NewFunPtr(FunctionName) if(jstr::ExactlyEquals(Function->Name, #FunctionName)) { RedeclareFunction(FunctionName); } else

void _ReinitiatePool(function_pool* Pool)
{
  u32 PoolSize = Pool->Count;
  for (u32 Index = 0; Index < PoolSize; ++Index)
  {
    function_ptr* Function = &Pool->Functions[Index];
    NewFunPtr(DefaultLosingFocus)
    NewFunPtr(DropDownLosingFocus)
    NewFunPtr(DefaultGainingFocus)
    NewFunPtr(DropDownGainingFocus)
    NewFunPtr(UpdateChildRegions)
    NewFunPtr(MainHeaderUpdate)
    NewFunPtr(RootUpdateChildRegions)
    NewFunPtr(UpdateSplitChildRegions)
    NewFunPtr(UpdateGridChildRegions)
    NewFunPtr(UpdateTabWindowChildRegions)
    NewFunPtr(DrawMergeSlots)
    NewFunPtr(TabWindowDraw)
    //NewFunPtr(DrawFunctionTimeline)
    //NewFunPtr(DrawStatistics)
    //NewFunPtr(DrawFrameFunctions)
    NewFunPtr(RenderScene)
    NewFunPtr(SceneTakeInput)
    //NewFunPtr(DebugToggleButton)
    //NewFunPtr(DebugRecompileButton)
    NewFunPtr(TabMouseDown)
    NewFunPtr(TabMouseUp)
    NewFunPtr(TabMouseEnter)
    NewFunPtr(TabMouseExit)
    NewFunPtr(TabWindowHeaderMouseDown)
    NewFunPtr(InitiateSplitWindowBorderDrag)
    NewFunPtr(InitiateBorderDrag)
    NewFunPtr(TabDragUpdate)
    NewFunPtr(WindowDragUpdate)
    NewFunPtr(RootBorderDragUpdate)
    NewFunPtr(SplitWindowBorderUpdate)
    NewFunPtr(DropDownMouseEnter)
    NewFunPtr(DropDownMouseExit)
    NewFunPtr(DropDownMouseUp)
    NewFunPtr(DropDownMenuButton)
    NewFunPtr(HeaderMenuMouseEnter)
    NewFunPtr(HeaderMenuMouseExit)
    {
      INVALID_CODE_PATH;
    }    
  }

}

#define ReinitiatePool() _ReinitiatePool(GlobalState->FunctionPool)

#else

#define ReinitiatePool()

#endif
