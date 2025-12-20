#include "menu.h"
#include "platform/jwin_platform_memory.h"
#include "entity_list.h"
#include "color_list.h"

namespace imgui {
namespace app {

menu_bar* CreateMenuBar(memory_arena* Arena){

  menu_bar::item ColorList = {};
  FormatString(ColorList.Name, sizeof(ColorList.Name), "%s", "ColorList");
  ColorList.ButtonID = NewButtonID();

  menu_bar::item EntityTree = {};
  FormatString(EntityTree.Name, sizeof(EntityTree.Name), "%s", "EntityTree");
  EntityTree.ButtonID = NewButtonID();

  menu_bar::item Windows = {};
  FormatString(Windows.Name, sizeof(Windows.Name), "%s", "Windows");
  Windows.ButtonID = NewButtonID();
  Windows.Items = cmn::vector<menu_bar::item>::Create(2);
  Windows.Items.PushBack(ColorList);
  Windows.Items.PushBack(EntityTree);

  menu_bar* Result = PushStruct(Arena, menu_bar);
  Result->TopItems = cmn::vector<menu_bar::item>::Create(1);
  Result->TopItems.PushBack(Windows);

  r32 HeaderSize = render::GetLineSpacing(0,GlobalImguiContext->FontSize);
  Result->HeaderBarRegion = Rect2f(0, 1.0-HeaderSize, GetAspectRatio(), HeaderSize);

  return Result;
}

b32 DoMenuBar(){
  if(GlobalState->ApplicationMenu.MenuBarActive)
  {
    menu_bar* MenuBar = GlobalState->ApplicationMenu.MenuBar;
    const r32 FontSize = GlobalImguiContext->FontSize;
    const r32 DescentOffset = render::GetDescenOffset(0, FontSize);
    rect2f BackgroundRect = MenuBar->HeaderBarRegion;

    v4 HeaderColor = GetColor(&GlobalState->ColorTable, "taupe");
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(BackgroundRect), HeaderColor);

    r32 X0 = 0;
    for (int i = 0; i < MenuBar->TopItems.Size(); ++i)
    {
      menu_bar::item* Item = &MenuBar->TopItems[i];
      
      char NameBuff[128] = {};
      FormatString(NameBuff, sizeof(NameBuff), " %s ", Item->Name);
      v2 ItemSize = render::GetTextSize(0, FontSize,  NameBuff);
      rect2f ButtonRect = Rect2f(X0, BackgroundRect.Y, ItemSize.X, BackgroundRect.H);

      if(ImguiIsSelected(Item->ButtonID)){
        r32 LineSpacing = render::GetLineSpacing(0,FontSize);
        r32 MaxWidth = 0;
        for (int j = 0; j < Item->Items.Size(); ++j)
        {
          menu_bar::item* SubItem = &Item->Items[j];
          char SubItemBuff[128] = {};
          FormatString(SubItemBuff, sizeof(SubItemBuff), " %s ", SubItem->Name);
          r32 Width = render::GetTextSize(0, FontSize, SubItemBuff).X;
          MaxWidth = Maximum(MaxWidth,Width);
        }
        rect2f DropDownRect = Rect2f(Left(ButtonRect), Bot(ButtonRect) - Item->Items.Size() * LineSpacing, MaxWidth, LineSpacing*Item->Items.Size());
        r32 Y0 = Top(DropDownRect) - LineSpacing + DescentOffset;
        for (int j = 0; j < Item->Items.Size(); ++j)
        {
          menu_bar::item* SubItem = &Item->Items[j];
          rect2f ButtonBackgroundRect = Rect2f(DropDownRect.X, DropDownRect.Y + DropDownRect.H - (1+j)*LineSpacing, DropDownRect.W, LineSpacing);
          if(ImguiButton(GlobalImguiContext, SubItem->ButtonID, ButtonBackgroundRect)){
            switch(j)
            {
              case 0: { GlobalState->ApplicationMenu.ColorListActive = !GlobalState->ApplicationMenu.ColorListActive; } break;
              case 1: { GlobalState->ApplicationMenu.EntityListActive = !GlobalState->ApplicationMenu.EntityListActive; } break;
            }
          };
          v4 ButtonColor = ImguiGetButtonColor(SubItem->ButtonID, ImguiDefaultButtonColor());
          render::DrawOverlayQuadCanonicalSpace(CenteredRect(ButtonBackgroundRect), ButtonColor);

          char SubItemBuff[128] = {};
          FormatString(SubItemBuff, sizeof(SubItemBuff), " %s ", SubItem->Name);
          render::DrawTextCanonicalSpace(V2(DropDownRect.X, Y0), FontSize, (const utf8_byte*) SubItemBuff, v4(1,1,1,1));
          Y0 -= LineSpacing;
        }
      }

      ImguiSelectabeRegion(GlobalImguiContext, Item->ButtonID, ButtonRect, GlobalInput);

      v4 TopButtonColor = ImguiGetButtonColor(Item->ButtonID, ImguiDefaultButtonColor());
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(ButtonRect), TopButtonColor);
      render::DrawTextCanonicalSpace(V2(BackgroundRect.X + X0, BackgroundRect.Y + DescentOffset), FontSize, (const utf8_byte*) NameBuff, v4(1,1,1,1));
    

      X0 += ItemSize.X;
    }


  }
  return false;
}

bool ToggleMenu(){
  GlobalState->ApplicationMenu.MenuBarActive = !GlobalState->ApplicationMenu.MenuBarActive;
  return GlobalState->ApplicationMenu.MenuBarActive;
}

menu CreateAppllicationMenu(memory_arena* Arena, context* ImguiContext, u32 ColorCount) {
  menu Result = {};

  Result.MenuBar = CreateMenuBar(Arena);
  Result.ColorList = CreateColorList(Arena, ColorCount);
  Result.EntityList = CreateEntityList(Arena);
  Result.MenuBarActive = false;
  Result.ColorListActive = false;
  Result.EntityListActive = false;
  Result.EnclosingRegion = Rect2f(0,0,GetAspectRatio(), 1 - Result.MenuBar->HeaderBarRegion.H);
  return Result;
}

void DoMenu(){

  if(GlobalState->ApplicationMenu.MenuBarActive)
  {
    render::NewOverlayLevel();
    imgui::app::DoMenuBar();
    if(GlobalState->ApplicationMenu.ColorListActive)
    {
      render::NewOverlayLevel();
      DrawColorList(&GlobalState->ApplicationMenu);
    }
    if(GlobalState->ApplicationMenu.EntityListActive)
    {
      render::NewOverlayLevel();
      DrawEntityTree(&GlobalState->ApplicationMenu);
    }
  }
}

} // namespace app
} // namespace imgui

