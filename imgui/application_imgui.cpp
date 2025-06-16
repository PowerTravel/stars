#include "application_imgui.h"
#include "platform/jwin_platform_memory.h"

application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount) {
  application_imgui Result = {};

  r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), GlobalState->ImguiContext.FontSize);

  u32 IDCount  = 512;
  u32 EntityChunkCount = 32;
  Result.MenuEntityList = PushStruct(Arena, menu_entity_list);
  Result.MenuEntityList->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), ecs::render::PixelToCanonicalSpace(GetRenderSystem(), V2(3,3)), RowHeight);
  Result.MenuEntityList->EntityList   = CreateScrollableTextList();
  Result.MenuEntityList->EntityData = NewChunkList(Arena, sizeof(imgui_entity_data), EntityChunkCount);

  
  u32 InputLen = 512;

  Result.ColorListData = PushStruct(Arena, color_list_data);
  Result.ColorListData->TextInputBuffer = ImguiNewTextInputBuffer(InputLen, PushArray(Arena, InputLen, utf8_byte));
  Result.ColorListData->ImguiIDs         = PushArray(Arena, ColorCount, imgui_id);
  Result.ColorListData->ColorIDs         = PushArray(Arena, ColorCount, s32);

  Result.ColorListData->ColorList = CreateScrollableTextList();
  Result.ColorListData->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.1,0.25), V2(0.1,0.5)), ecs::render::PixelToCanonicalSpace(GetRenderSystem(), V2(3,3)), RowHeight);
  for (int i = 0; i < ColorCount; ++i)
  {
    Result.ColorListData->ImguiIDs[i] = NewButtonID();
  }

  return Result;
}

void DrawColorRow(imgui_context* ImguiContext, imgui_id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data)
{
  color_list_data* ColorListData = (color_list_data*) Data;
  umm ColorIndex = (umm) ColorListData->ColorIDs[ListIndex];
  menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, (umm) ColorIndex);
  char* ColorName = NamedColor->Name;
  v4 ColorValue   = HexCodeToColorV4(NamedColor->Color);

  v2 Padding = V2(ecs::render::PixelToCanonicalWidth(GetRenderSystem(), 1),ecs::render::PixelToCanonicalHeight(GetRenderSystem(),1));

  r32 RowWidth = RowRect.W;
  r32 ColorSquareWidth = RowRect.H;
  r32 TextWidth = RowRect.W - ColorSquareWidth;

  if(ImguiIsHot(ButtonID) && ImguiIsInactive() && RowRect.H == ClippedRowRect.H){
    r32 TextWidthTmp = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize, (utf8_byte*) ColorName).X;
    if(TextWidthTmp > TextWidth)
    {
      TextWidth = TextWidthTmp + 2*Padding.X;
      RowWidth = TextWidthTmp + ColorSquareWidth + 2*Padding.X;
    }
  }

  // Button Background
  v4 ButtonColor = ImguiGetButtonColor(ButtonID, ImguiDefaultButtonColor());
  rect2f ButtonBackgroundRect = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, RowWidth, ClippedRowRect.H);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ButtonBackgroundRect), ButtonColor);

  // Colored Square
  rect2f ColorSquare = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, ColorSquareWidth, ClippedRowRect.H);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(Shrink(ColorSquare,Padding)), ColorValue);

  // Color Name
  rect2f TextRect = Rect2f(RowRect.X + ColorSquareWidth, ClippedRowRect.Y, TextWidth, ClippedRowRect.H);
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), ImguiContext->FontSize);
  v2 TextPos = V2(RowRect.X + ColorSquareWidth, RowRect.Y + DescentOffset);
  utf8_string_buffer StringBuffer = SetStringToFit(ImguiContext->FontSize, TextWidth, ColorName);
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextPos, TextRect, ImguiContext->FontSize, StringBuffer.Buffer, V4(1.0,1.0,1.0,1.0));
}

void DrawColorList(application_imgui* AppImgui) {
  
  u32 ColorCount = GlobalState->ColorTable.ColorCount;
  r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), GlobalState->ImguiContext.FontSize);

  color_list_data* ColorListData = AppImgui->ColorListData;

  s32 RowCount = 0;
  ZeroArray(ColorCount, ColorListData->ColorIDs);
  imgui_id* ImguiIDs = PushArray(GlobalPersistentArena, ColorCount, imgui_id);
  for (u32 i = 0; i < ColorCount; ++i) {
    menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, (umm) i);
    char ColorNameLower[512] = {};
    Utf8ToLower( (utf8_byte*) NamedColor->Name, (utf8_byte*) ColorNameLower);
    char InputStringLower[512] = {};
    Utf8ToLower( ColorListData->TextInputBuffer.Buffer.Buffer, (utf8_byte*) InputStringLower);
    if(ColorListData->TextInputBuffer.CharCount == 0 || jstr::Contains( InputStringLower, ColorNameLower))
    {
      ColorListData->ColorIDs[RowCount] = i;
      ImguiIDs[RowCount] = ColorListData->ImguiIDs[i];
      RowCount++;
    }
  }

  imgui_bordered_window* BorderWindow = &ColorListData->BorderWindow;

  // SearchIcon
  v4 SearchBoxBackgroundColor = menu::GetColor(&GlobalState->ColorTable, "bole");
  v2 SearchIconPos = V2(BorderWindow->Region.X, BorderWindow->Region.Y);
  v2 SearchIconSize = V2(RowHeight, RowHeight);
  v4 TexCoord = GlobalImguiContext->Icons.Coordinates[ICON_SEARCH];
  rect2f SearchIconRectBackground = Rect2f(SearchIconPos, SearchIconSize);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(SearchIconRectBackground), SearchBoxBackgroundColor);
  rect2f SearchIconRect = Shrink(SearchIconRectBackground, 0.1*SearchIconRectBackground.W);
  ecs::render::DrawIconCanonicalSpace(GetRenderSystem(), CenteredRect(SearchIconRect),  TexCoord, V4(1,1,1,1));

  
  v2 FilterBarDialogPos  = V2(BorderWindow->Region.X + RowHeight, BorderWindow->Region.Y);
  v2 FilterBarDialogSize = V2(BorderWindow->Region.W - RowHeight, RowHeight);
  if(ImguiTextDialog(&ColorListData->TextInputBuffer, ColorListData->TextInputBuffer.ID, FilterBarDialogPos, FilterBarDialogSize, SearchBoxBackgroundColor))
  {
    if(ImguiIsSelected(ColorListData->TextInputBuffer.ID))
    {
      ImguiReadInput(&ColorListData->TextInputBuffer, GlobalInput);
    }
  }

  v2 ScrollListPos  = V2(BorderWindow->Region.X, BorderWindow->Region.Y + RowHeight);
  v2 ScrollListSize = V2(BorderWindow->Region.W, BorderWindow->Region.H - 2* RowHeight);
  
  ImguiBorderWindow(BorderWindow, "Colors");

  if(ImguiScrollableButtonList(&ColorListData->ColorList, ScrollListPos, ScrollListSize, RowCount, RowHeight, ImguiIDs, (void*) ColorListData, DrawColorRow))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, ColorListData->ColorIDs[ColorListData->ColorList.SelectedRow] );
      v4 Color =  HexCodeToColorV4(NamedColor->Color);
      v4 HexColor = 255 * Color;
      Platform.DEBUGPrint("V4(%f, %f, %f, %f) - %s\n", Color.X, Color.Y, Color.Z, Color.W, 
        NamedColor->Name);  
    }
  }
}


r32 RenderPositionComponent(imgui_context* ImguiContext, ecs::position::component* Position, v2 TopLeft, rect2f ClipArea)
{
  r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize);
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), ImguiContext->FontSize);
  v2 TextPos  = V2(TopLeft.X, TopLeft.Y + DescentOffset - RowHeight ); 
  v2 TextSize = V2(ClipArea.W, RowHeight);
  rect2f LineRect = Rect2f(TextPos, TextSize);

  world_coordinate Pos = Position->FirstChild->RelativePosition;

  c8 LineToPrint[512] = {};
  midx Len = jstr::CopyStringsUnchecked("  Position: ", LineToPrint);
  rect2f TextRect = Rect2f(TopLeft.X, TopLeft.Y-RowHeight, (ClipArea.X + ClipArea.W) - TextPos.X, RowHeight);
  u32 idx = 0;
  c8 NumBuf[32] = {};
  c8* Scan = NumBuf;
  if(Pos.X >= 0)
  {
    *Scan++ = ' ';  
  }
  Scan += jstr::Ftoa( Pos.X, 2, 255, Scan);
  *Scan++ = ' ';
  if(Pos.Y >= 0)
  {
    *Scan++ = ' ';  
  }
  Scan += jstr::Ftoa( Pos.Y, 2, 255, Scan);
  *Scan++ = ' ';
  if(Pos.Z >= 0)
  {
    *Scan++ = ' ';
  }
  Scan += jstr::Ftoa( Pos.Z, 2, 255, Scan);

  Len = jstr::CopyStringsUnchecked(NumBuf, LineToPrint+Len);
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextPos, TextRect, ImguiContext->FontSize, (utf8_byte const *) LineToPrint, V4(1.0,1.0,1.0,1.0));
  return RowHeight;
}

r32 DrawEntityRow(imgui_context* ImguiContext, v2 TopLeft, rect2f ClipArea, imgui_entity_data* Data)
{
  // Button Background
  r32 Height = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize);
  r32 ResultHeight = Height;
  rect2f ButtonBackgroundRect = Rect2f(TopLeft.X, TopLeft.Y - Height, ClipArea.W, Height);

  imgui_button_color ButtonColor = {};
  ButtonColor.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "taupe");
  ButtonColor.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "persian indigo");
  ButtonColor.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "egyptian blue");
  ButtonColor.HotColor =  menu::GetColor(&GlobalState->ColorTable, "rich black");
  ImguiPlainButton(ImguiContext, Data->ImguiID, ButtonBackgroundRect, ButtonColor);
  if(ImguiIsActive(Data->ImguiID) && ImguiIsHot(Data->ImguiID) && jwin::Released(ImguiContext->LeftMouse))
  {
    Data->Open = !Data->Open;
  }

  rect2f TextRect = Rect2f(ButtonBackgroundRect.X, ButtonBackgroundRect.Y, ButtonBackgroundRect.W, ButtonBackgroundRect.H);
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), ImguiContext->FontSize);

  v4 TexCoord = Data->Open ? GlobalImguiContext->Icons.Coordinates[ICON_ANGLE_DOWN] : GlobalImguiContext->Icons.Coordinates[ICON_ANGLE_RIGHT];
  ecs::render::DrawIconCanonicalSpace(GetRenderSystem(), CenteredRect(Rect2f(ButtonBackgroundRect.X, ButtonBackgroundRect.Y, Height,Height)), TexCoord, V4(1,1,1,1));
  v2 TextPos = V2(ButtonBackgroundRect.X + Height, ButtonBackgroundRect.Y + DescentOffset);
  c8 LineBuffer[128] = {};
  ecs::entity_id EntityID = Data->EntityID;
  midx LineBufferPos = jstr::Itoa(EntityID.EntityID, 31, LineBuffer);
  LineBufferPos += jstr::CopyStringsUnchecked(": ", LineBuffer + LineBufferPos);
  LineBufferPos += jstr::CopyStringsUnchecked(ecs::GetName(GetEntityManager(), &EntityID), LineBuffer + LineBufferPos);

  r32 TextWidth = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize, (utf8_byte const *) LineBuffer).X;
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextPos, TextRect, ImguiContext->FontSize, (utf8_byte const *) LineBuffer, V4(1.0,1.0,1.0,1.0));

  if(Data->Open)
  {
    ecs::position::component* Position = GetPositionComponent(&EntityID);
    if(Position)
    {
      ResultHeight += RenderPositionComponent(ImguiContext, Position, V2(TopLeft.X, TopLeft.Y - Height), ClipArea);
    }
  }else{

  }
  
  return ResultHeight;
}

struct list_map_pair {
  rb_tree* MapToCheckAgainst;
  chunk_list* ResultList;
  memory_arena* Arena;
};

void PopulateWithDataNotInMap(red_black_tree_node const * MenuEntryNode, void* ListMapPairPtr)
{
  list_map_pair* ListMapPair = (list_map_pair*) ListMapPairPtr;
  rb_tree* MapToCheckAgainst = (rb_tree*) ListMapPair->MapToCheckAgainst;
  chunk_list* ResultList = ListMapPair->ResultList;
  memory_arena* Arena = ListMapPair->Arena;
  midx Key = MenuEntryNode->Key;
  if(Find(MapToCheckAgainst, Key) == 0)
  {
    Push(Arena, ResultList, (bptr) MenuEntryNode->Data->Data);
  }
}

void UpdateListWithEntities(chunk_list* MenuEntityList)
{
  // Insert all current real entities into a search tree
  u32 EntityCount = GetEntityManager()->EntityList.BlockCount;
  u32 MenuEntityCount = MenuEntityList->BlockCount;
  chunk_list ItemsToRemove = NewChunkList(GlobalTransientArena, sizeof(imgui_entity_data*), EntityCount);
  chunk_list ItemsToAdd    = NewChunkList(GlobalTransientArena, sizeof(ecs::entity*), EntityCount);

  // Create Existing Menu entities search tree
  rb_tree MenuEntitiesMap = NewRBTree(GlobalTransientArena, MenuEntityCount, MenuEntityCount);
  {
    u32 Index = 0;
    chunk_list_iterator IT = BeginIterator(MenuEntityList);
    while(imgui_entity_data* EntityData = (imgui_entity_data*) Next(&IT) )
    {
      u32 EntityID = EntityData->EntityID.EntityID;
      Insert(&MenuEntitiesMap, EntityID, (void*) EntityData);
    }
  }

  // Create Existing Entities search tree
  rb_tree ExistingEntitiesMap = NewRBTree(GlobalTransientArena, EntityCount, EntityCount);
  {
    u32 Count = 0;
    chunk_list_iterator IT = BeginIterator(&GetEntityManager()->EntityList);
    while(ecs::entity* Entity = (ecs::entity*) Next(&IT))
    {
      u32 EntityID = Entity->ID.EntityID;
      Insert(&ExistingEntitiesMap, (midx) EntityID, Entity);
    }
  }

  // Fill ItemsToRemove with items in MenuEntitiesMap which are not in ExistingEntitiesMap. 
  list_map_pair LMP1 = {};
  LMP1.MapToCheckAgainst = &ExistingEntitiesMap;
  LMP1.ResultList = &ItemsToRemove;
  LMP1.Arena = GlobalTransientArena;
  PostOrderTraverse(&MenuEntitiesMap.Tree,     (void*) &LMP1, PopulateWithDataNotInMap);

  // Fill ItemsToAdd with items in ExistingEntitiesMap which are not in MenuEntitiesMap. 
  list_map_pair LMP2 = {};
  LMP2.MapToCheckAgainst = &MenuEntitiesMap;
  LMP2.ResultList = &ItemsToAdd;
  LMP2.Arena = GlobalTransientArena;
  PostOrderTraverse(&ExistingEntitiesMap.Tree, (void*) &LMP2, PopulateWithDataNotInMap);

  {
    chunk_list_iterator IT = BeginIterator(&ItemsToRemove);
    while(imgui_entity_data* EntityData = (imgui_entity_data*) Next(&IT))
    {
      FreeBlock(MenuEntityList, (bptr) EntityData);
    }
  }

  {
    chunk_list_iterator IT = BeginIterator(&ItemsToAdd);
    while(ecs::entity* EntityData = (ecs::entity*) Next(&IT))
    {
      imgui_entity_data Data = {};
      Data.ImguiID = NewButtonID();
      Data.EntityID = EntityData->ID;
      Data.Open = 0;
      Data.MenuBoxHeight = 0;
      Push(GlobalPersistentArena, MenuEntityList, (bptr) &Data);
    }
  }
}

b32 ImguiEntityComponentList(imgui_scrollable_list* ScrollableList, v2 Pos, v2 Size, chunk_list* MenuEntityList) {

  // List Background
  rect2f BackgroundRect = Rect2f(Pos, Size);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(BackgroundRect), ImguiDefaultButtonColor().InactiveColor);

  v2 ListContentSize = Size;
  rect2f ListRect = Rect2f(Pos, ListContentSize);
  b32 Result = 0;
  v2 TopLeft = UpperLeftPoint(BackgroundRect);
  s32 Index = 0;

  UpdateListWithEntities(MenuEntityList);
  chunk_list_iterator IT = BeginIterator(MenuEntityList);
  while (imgui_entity_data* Entity = (imgui_entity_data*) Next(&IT))
  {
    r32 HeightOfRow = DrawEntityRow(&GlobalState->ImguiContext, TopLeft, BackgroundRect, Entity);
    TopLeft.Y -= HeightOfRow;
  }
  return Result;
}


void DrawEntityList(application_imgui* AppImgui) {

  menu_entity_list* MenuEntityList = AppImgui->MenuEntityList;

  r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(),GlobalState->ImguiContext.FontSize);
  
  v2 ScrollListPos  = V2(MenuEntityList->BorderWindow.Region.X, MenuEntityList->BorderWindow.Region.Y);
  v2 ScrollListSize = V2(MenuEntityList->BorderWindow.Region.W, MenuEntityList->BorderWindow.Region.H - MenuEntityList->BorderWindow.HeaderSize);

  ImguiBorderWindow(&MenuEntityList->BorderWindow, "Entities");

  ImguiEntityComponentList(&MenuEntityList->EntityList, ScrollListPos, ScrollListSize, &MenuEntityList->EntityData);
}
