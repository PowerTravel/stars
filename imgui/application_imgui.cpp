#include "application_imgui.h"
#include "platform/jwin_platform_memory.h"

application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount) {
  application_imgui Result = {};

  u32 InputLen = 512;
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);

  u32 IDCount  = 512;
  u32 EntityChunkCount = 32;
  Result.MenuEntityList = PushStruct(Arena, menu_entity_list);
  Result.MenuEntityList->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  Result.MenuEntityList->EntityList   = CreateScrollableTextList();
  Result.MenuEntityList->EntityData   = NewChunkList(Arena, sizeof(imgui_entity_data), EntityChunkCount);
  Result.MenuEntityList->PositionComponentData = NewChunkList(Arena, sizeof(position_component_data), EntityChunkCount);

  Result.ColorListData = PushStruct(Arena, color_list_data);
  Result.ColorListData->TextInputBuffer  = ImguiNewTextInputBuffer(InputLen, PushArray(Arena, InputLen, utf8_byte));
  Result.ColorListData->TextInputID      = NewButtonID();
  Result.ColorListData->ImguiIDs         = PushArray(Arena, ColorCount, imgui_id);
  Result.ColorListData->ColorIDs         = PushArray(Arena, ColorCount, s32);

  Result.MenuEntityTree = PushStruct(Arena, menu_entity_tree);
  Result.MenuEntityTree->BorderWindow   = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  Result.MenuEntityTree->EntityList     = CreateScrollableTextList();
  Result.MenuEntityTree->EntityTree = me_tree::Create();
  Result.MenuEntityTree->EntityTree.NewNode(); // EmptyRoot


  Result.ColorListData->ColorList = CreateScrollableTextList();
  Result.ColorListData->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.1,0.25), V2(0.1,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  for (int i = 0; i < ColorCount; ++i)
  {
    Result.ColorListData->ImguiIDs[i] = NewButtonID();
  }

  return Result;
}

void PushNewEntity(me_tree* MenuEntityTree, me_node* MenuParent, ecs::entity_id* NewEntity)
{
  // Just making sure that the parent of NewEntity in the entity Manager is the same as MenuParent
  //Assert(ecs::Compare(&(*ecs::GetEntityFromID(GetEntityManager(), NewEntity)->Node->Parent->Data)->ID, &MenuParent->Data->EntityID));

  me_node* Root = MenuParent;
  me_node* Node = MenuParent->FirstChild;
  if(Node)
  {
    do
    {
      if(ecs::Compare(&Node->Data->EntityID, NewEntity)){
        return;
      }
      Node = Node->NextSibling;
    }while((Node && Node != MenuParent->FirstChild));
  }

  menu_entity_row NewRow = {};
  NewRow.EntityID = *NewEntity;
  NewRow.ImguiID = NewButtonID();
  NewRow.Open = false;

  MenuEntityTree->NewNode(MenuParent, NewRow);
}

void DrawColorRow(imgui_context* ImguiContext, imgui_id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data)
{
  color_list_data* ColorListData = (color_list_data*) Data;
  umm ColorIndex = (umm) ColorListData->ColorIDs[ListIndex];
  menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, (umm) ColorIndex);
  char* ColorName = NamedColor->Name;
  v4 ColorValue   = HexCodeToColorV4(NamedColor->Color);

  v2 Padding = PixelToCanonicalSpace(V2(1,1));

  r32 RowWidth = RowRect.W;
  r32 ColorSquareWidth = RowRect.H;
  r32 TextWidth = RowRect.W - ColorSquareWidth;

  if(ImguiIsHot(ButtonID) && ImguiIsInactive() && RowRect.H == ClippedRowRect.H){
    r32 TextWidthTmp = GlobalRenderer->Font.GetTextSizeCanonicalSpace(ImguiContext->FontSize, (utf8_byte*) ColorName).X;
    if(TextWidthTmp > TextWidth)
    {
      TextWidth = TextWidthTmp + 2*Padding.X;
      RowWidth = TextWidthTmp + ColorSquareWidth + 2*Padding.X;
    }
  }

  // Button Background
  v4 ButtonColor = ImguiGetButtonColor(ButtonID, ImguiDefaultButtonColor());
  rect2f ButtonBackgroundRect = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, RowWidth, ClippedRowRect.H);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(ButtonBackgroundRect), ButtonColor);

  // Colored Square
  rect2f ColorSquare = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, ColorSquareWidth, ClippedRowRect.H);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(Shrink(ColorSquare,Padding)), ColorValue);

  // Color Name
  rect2f TextRect = Rect2f(RowRect.X + ColorSquareWidth, ClippedRowRect.Y, TextWidth, ClippedRowRect.H);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(ImguiContext->FontSize);
  v2 TextPos = V2(RowRect.X + ColorSquareWidth, RowRect.Y + DescentOffset);
  utf8_string_buffer StringBuffer = SetStringToFit(ImguiContext->FontSize, TextWidth, ColorName);
  render::DrawTextCanonicalSpace(TextPos, TextRect, ImguiContext->FontSize, StringBuffer.Buffer, V4(1.0,1.0,1.0,1.0));
}

void DrawColorList(application_imgui* AppImgui) {
  
  u32 ColorCount = GlobalState->ColorTable.ColorCount;
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);

  color_list_data* ColorListData = AppImgui->ColorListData;

  s32 RowCount = 0;
  ZeroArray(ColorCount, ColorListData->ColorIDs);
  imgui_id* ImguiIDs = PushArray(GlobalTransientArena, ColorCount, imgui_id);
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
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(SearchIconRectBackground), SearchBoxBackgroundColor);
  rect2f SearchIconRect = Shrink(SearchIconRectBackground, 0.1*SearchIconRectBackground.W);
  render::DrawIconCanonicalSpace(CenteredRect(SearchIconRect),  TexCoord, V4(1,1,1,1));

  
  v2 FilterBarDialogPos  = V2(BorderWindow->Region.X + RowHeight, BorderWindow->Region.Y);
  v2 FilterBarDialogSize = V2(BorderWindow->Region.W - RowHeight, RowHeight);
  if(ImguiTextDialog(&ColorListData->TextInputBuffer, ColorListData->TextInputID, FilterBarDialogPos, FilterBarDialogSize, SearchBoxBackgroundColor))
  {
    ImguiReadInput(&ColorListData->TextInputBuffer, ColorListData->TextInputID, GlobalInput, FilterBarDialogPos);
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

void DrawNumber(c8* Number, r32 FontSize, rect2f Rect)
{
  v2 TextSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, (utf8_byte const *) Number);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
  v2 TextPos  = V2(Rect.X + (Rect.W - TextSize.X), Rect.Y);
  Rect.Y -= DescentOffset;
  render::DrawTextCanonicalSpace(TextPos, Rect, FontSize, (utf8_byte const *) Number, V4(1.0,1.0,1.0,1.0));
}

void DrawNumber(r32 Number, r32 FontSize, rect2f Rect)
{
  c8 NumBuf[32] = {};
  jstr::Ftoa( Number, 2, 31, NumBuf);
  DrawNumber(NumBuf, FontSize, Rect);
}


float PosXToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  return Position->RelativePosition.X;
}

void StoreXPos(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  v3 Pos = Position->RelativePosition;
  Pos.X = Val;
  ecs::position::Set(Position, Pos, Position->RelativeRotation, Position->Scale);
}

float PosYToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  return Position->RelativePosition.Y;
}

void StoreYPos(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  v3 Pos = Position->RelativePosition;
  Pos.Y = Val;
  ecs::position::Set(Position, Pos, Position->RelativeRotation, Position->Scale);
}

float PosZToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  return Position->RelativePosition.Z;
}

void StoreZPos(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  v3 Pos = Position->RelativePosition;
  Pos.Z = Val;
  ecs::position::Set(Position, Pos, Position->RelativeRotation, Position->Scale);
}

float RollToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  return EulerAngle.Roll * 180.f/Pi32;
}

void StoreRoll(float Val, void* Data)
{
  r32 Roll = Val * Pi32 / 180.f;
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  EulerAngle.Roll = Roll;
  ecs::position::Set(Position, Position->RelativePosition, EulerAngle, Position->Scale);
}

float YawToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  return EulerAngle.Yaw * 180.f/Pi32;
}

void StoreYaw(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  EulerAngle.Yaw = Val * Pi32 / 180.f;
  ecs::position::Set(Position, Position->RelativePosition, EulerAngle, Position->Scale);
}

float PitchToFloat(void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  return EulerAngle.Pitch * 180.f/Pi32;
}

void StorePitch(float Val, void* Data)
{
  ecs::position::component* Position = (ecs::position::component*) Data;
  ecs::entity_id EntityID = ecs::GetEntityIDFromComponent( (bptr) Position );
  euler_angle EulerAngle = QuaternionToEuler(Position->RelativeRotation);
  EulerAngle.Pitch = Val * Pi32 / 180.f;
  ecs::position::Set(Position, Position->RelativePosition, EulerAngle, Position->Scale);
}

void ReadNumber(imgui_context* ImguiContext, imgui_id ID, imgui_text_input_buffer* TextInputBuffer, rect2f DialogRect, jwin::device_input* Input,
  float (*DataToFloat)(void* Data),
  void (*StoreFloat)(float Val, void* Data), void* Data) {
  b32 SelectAll = !ImguiIsSelected(ID);
  v2 DialogPosition = V2(DialogRect.X,DialogRect.Y);
  v2 DialogSize     = V2(DialogRect.W,DialogRect.H);
  float DataValue   = DataToFloat(Data);
  if(ImguiSelectabeRegion(ImguiContext, ID, DialogRect, Input))
  {
    if(ImguiContext->SelectedID.idEdge)
    {
      ClearBuffer(TextInputBuffer);
      c8 NumBuf[32] = {};
      midx Len = jstr::Ftoa(DataValue, 2, 31, NumBuf);
      PushString(TextInputBuffer, NumBuf);
    }

    ImguiReadInput(TextInputBuffer, ID, GlobalInput, DialogPosition, SelectAll);  

    ImguiRenderTextDialog(TextInputBuffer, ID, DialogPosition, DialogSize, V4(0.5,0.5,0.5,1));
  } else {
    if(ImguiWasDeselected(ID))
    {
      if(!jwin::Pushed(Input->Keyboard.Key_ESCAPE))
      {
        float Val = jstr::StringToReal64((c8*) TextInputBuffer->Buffer.Buffer);
        StoreFloat(Val, Data);
      }
    }

    r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(ImguiContext->FontSize);
    rect2f TextRect = DialogRect;
    TextRect.Y +=DescentOffset;
    DrawNumber(DataValue, ImguiContext->FontSize, TextRect);
  }
}

r32 RenderPositionComponent(imgui_context* ImguiContext, position_component_data* PosCompData, ecs::position::component* Position, v2 TopLeft, rect2f ClipArea)
{
  r32 RowHeight1 = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( ImguiContext->FontSize);
  r32 DescentOffset1 = GlobalRenderer->Font.GetCanonicalFontDescenOffset(ImguiContext->FontSize);
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( ImguiContext->FontSize);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(ImguiContext->FontSize);
  v2 TextPos = V2(TopLeft.X, TopLeft.Y + DescentOffset1 - RowHeight1 );

  r32 Height = RowHeight1;
  {
    c8 Header[] = "Position Component";
    v2 TextSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(ImguiContext->FontSize, (utf8_byte const *) Header);
    render::DrawTextCanonicalSpace(V2(TopLeft.X + (ClipArea.W - TextSize.X)/2.f, TextPos.Y), ClipArea, ImguiContext->FontSize, (utf8_byte const *) Header, V4(1.0,1.0,1.0,1.0));
    Height += RowHeight;
    TextPos.Y -= RowHeight;
  }

  { 
    c8 Preamble[] = "  Pos:";
    v2 TextSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(ImguiContext->FontSize, (utf8_byte const *) Preamble);
    render::DrawTextCanonicalSpace(TextPos, ClipArea, ImguiContext->FontSize, (utf8_byte const *) Preamble, V4(1.0,1.0,1.0,1.0));
    
    v2 LeftOverSize = V2(ClipArea.W - TextSize.X - 0.01, TextSize.Y);
    rect2f LeftoverRect = Rect2f(V2(TextPos.X + TextSize.X, TextPos.Y), LeftOverSize);
    r32 SubrectWidth = LeftoverRect.W/3.f;

    rect2f Rect1 = Rect2f(LeftoverRect.X, LeftoverRect.Y - DescentOffset, SubrectWidth, RowHeight);
    ReadNumber(ImguiContext,  PosCompData->PosX, &PosCompData->TextBufferPosX, Rect1, GlobalInput,
      PosXToFloat, StoreXPos, (void*) Position );

    rect2f Rect2 = Rect2f(LeftoverRect.X + SubrectWidth, LeftoverRect.Y - DescentOffset, SubrectWidth, RowHeight);
    ReadNumber(ImguiContext,  PosCompData->PosY, &PosCompData->TextBufferPosY, Rect2, GlobalInput,
      PosYToFloat, StoreYPos, (void*) Position );

    rect2f Rect3 = Rect2f(LeftoverRect.X + 2.f*SubrectWidth, LeftoverRect.Y - DescentOffset, SubrectWidth, RowHeight);
    ReadNumber(ImguiContext,  PosCompData->PosZ, &PosCompData->TextBufferPosZ, Rect3, GlobalInput,
      PosZToFloat, StoreZPos, (void*) Position );
    

    Height += RowHeight;
    TextPos.Y -= RowHeight;
  }
  {
    c8 Preamble[] = "  Rot:";
    v2 TextSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(ImguiContext->FontSize, (utf8_byte const *) Preamble);
    render::DrawTextCanonicalSpace(TextPos, ClipArea, ImguiContext->FontSize, (utf8_byte const *) Preamble, V4(1.0,1.0,1.0,1.0));
    v2 LeftOverSize = V2(ClipArea.W - TextSize.X - 0.01, TextSize.Y);

    rect2f LeftoverRect = Rect2f(V2(TextPos.X + TextSize.X, TextPos.Y), LeftOverSize);
    r32 SubrectWidth = LeftoverRect.W/3.f;

    rect2f Rect1 = Rect2f(LeftoverRect.X, LeftoverRect.Y-DescentOffset, SubrectWidth, RowHeight);
    ReadNumber(ImguiContext,  PosCompData->RotX, &PosCompData->TextBufferPosX, Rect1, GlobalInput,
      RollToFloat, StoreRoll, (void*) Position );

    rect2f Rect2 = Rect2f(LeftoverRect.X + SubrectWidth, LeftoverRect.Y-DescentOffset, SubrectWidth, RowHeight);
    ReadNumber(ImguiContext,  PosCompData->RotY, &PosCompData->TextBufferPosY, Rect2, GlobalInput,
      YawToFloat, StoreYaw, (void*) Position );

    rect2f Rect3 = Rect2f(LeftoverRect.X + 2.f*SubrectWidth, LeftoverRect.Y-DescentOffset, SubrectWidth, RowHeight);
    ReadNumber(ImguiContext,  PosCompData->RotZ, &PosCompData->TextBufferPosZ, Rect3, GlobalInput,
      PitchToFloat, StorePitch, (void*) Position );


    //Height += RowHeight;
    TextPos.Y -= RowHeight;
  }

  return Height;
}

r32 DrawEntityRow(imgui_context* ImguiContext, v2 TopLeft, rect2f ClipArea, imgui_entity_data* Data)
{
  // Button Background
  r32 Height = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( ImguiContext->FontSize);
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
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(ImguiContext->FontSize);

  v4 TexCoord = Data->Open ? GlobalImguiContext->Icons.Coordinates[ICON_ANGLE_DOWN] : GlobalImguiContext->Icons.Coordinates[ICON_ANGLE_RIGHT];
  render::DrawIconCanonicalSpace(CenteredRect(Rect2f(ButtonBackgroundRect.X, ButtonBackgroundRect.Y, Height,Height)), TexCoord, V4(1,1,1,1));
  v2 TextPos = V2(ButtonBackgroundRect.X + Height, ButtonBackgroundRect.Y + DescentOffset);
  c8 LineBuffer[128] = {};
  ecs::entity_id EntityID = Data->EntityID;
  midx LineBufferPos = jstr::Itoa(EntityID.EntityID, 31, LineBuffer);
  LineBufferPos += jstr::CopyStringsUnchecked(": ", LineBuffer + LineBufferPos);
  LineBufferPos += jstr::CopyStringsUnchecked(ecs::GetName(GetEntityManager(), &EntityID), LineBuffer + LineBufferPos);

  r32 TextWidth = GlobalRenderer->Font.GetTextSizeCanonicalSpace(ImguiContext->FontSize, (utf8_byte const *) LineBuffer).X;
  render::DrawTextCanonicalSpace(TextPos, TextRect, ImguiContext->FontSize, (utf8_byte const *) LineBuffer, V4(1.0,1.0,1.0,1.0));

  if(Data->Open)
  {
    ecs::position::component* Position = GetPositionComponent(&EntityID);
    if(Position)
    {
      Height += RenderPositionComponent(ImguiContext, Data->PositionComponentData, Position, V2(TopLeft.X, TopLeft.Y - Height), ClipArea);
    }
    //ecs::render::component* Position = GetRenderComponent(&EntityID);
    //if(Position)
    //{
    //  Height += RenderPositionComponent(ImguiContext, Data->PositionComponentData, Position, V2(TopLeft.X, TopLeft.Y - Height), ClipArea);
    //}
  }else{

  }
  
  return Height;
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

void UpdateListWithEntities(menu_entity_list* MenuEntity)
{
  chunk_list* MenuEntityList = &MenuEntity->EntityData;
  chunk_list* PositionComponentList = &MenuEntity->PositionComponentData;

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
      if(EntityData->PositionComponentData)
      {
        FreeBlock(PositionComponentList, (bptr) EntityData->PositionComponentData);
      }
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
      if(ecs::position::component* Pos = (ecs::position::component*) GetComponent(GetEntityManager(), &EntityData->ID, ecs::flag::POSITION)) {
        position_component_data* PosComp = (position_component_data*) GetNewBlock(GlobalPersistentArena, PositionComponentList); 
        PosComp->ComponentID = NewButtonID();
        u32 TextDataLen = 512;
        PosComp->PosX = NewButtonID();
        PosComp->TextBufferPosX = ImguiNewTextInputBuffer(TextDataLen, &PosComp->TextData[0]);
        PosComp->PosY = NewButtonID();
        PosComp->TextBufferPosY = ImguiNewTextInputBuffer(TextDataLen, &PosComp->TextData[TextDataLen]);
        PosComp->PosZ = NewButtonID();
        PosComp->TextBufferPosZ = ImguiNewTextInputBuffer(TextDataLen, &PosComp->TextData[2*TextDataLen]);
        PosComp->RotX = NewButtonID();
        PosComp->TextBufferRotX = ImguiNewTextInputBuffer(TextDataLen, &PosComp->TextData[3*TextDataLen]);
        PosComp->RotY = NewButtonID();
        PosComp->TextBufferRotY = ImguiNewTextInputBuffer(TextDataLen, &PosComp->TextData[4*TextDataLen]);
        PosComp->RotZ = NewButtonID();
        PosComp->TextBufferRotZ = ImguiNewTextInputBuffer(TextDataLen, &PosComp->TextData[5*TextDataLen]);

        Data.PositionComponentData  = PosComp;
      }
      Push(GlobalPersistentArena, MenuEntityList, (bptr) &Data);
    }
  }
}

b32 ImguiEntityComponentList(menu_entity_list* MenuEntityList, v2 Pos, v2 Size) {

  // List Background
  rect2f BackgroundRect = Rect2f(Pos, Size);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(BackgroundRect), ImguiDefaultButtonColor().InactiveColor);

  v2 ListContentSize = Size;
  rect2f ListRect = Rect2f(Pos, ListContentSize);
  b32 Result = 0;
  v2 TopLeft = UpperLeftPoint(BackgroundRect);
  s32 Index = 0;

  UpdateListWithEntities(MenuEntityList);
  chunk_list_iterator IT = BeginIterator(&MenuEntityList->EntityData);
  while (imgui_entity_data* Entity = (imgui_entity_data*) Next(&IT))
  {
    r32 HeightOfRow = DrawEntityRow(&GlobalState->ImguiContext, TopLeft, BackgroundRect, Entity);
    TopLeft.Y -= HeightOfRow;
  }

  return Result;
}


void DrawEntityList(application_imgui* AppImgui) {

  menu_entity_list* MenuEntityList = AppImgui->MenuEntityList;

  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(GlobalState->ImguiContext.FontSize);
  
  v2 ScrollListPos  = V2(MenuEntityList->BorderWindow.Region.X, MenuEntityList->BorderWindow.Region.Y);
  v2 ScrollListSize = V2(MenuEntityList->BorderWindow.Region.W, MenuEntityList->BorderWindow.Region.H - MenuEntityList->BorderWindow.HeaderSize);

  ImguiBorderWindow(&MenuEntityList->BorderWindow, "Entities");

  ImguiEntityComponentList(MenuEntityList, ScrollListPos, ScrollListSize);
}

/// Start Entity Tree

file_local b32 IsClippingTop(rect2f DrawRect, rect2f ClipRect){
  r32 ClipTop = Top(ClipRect);
  b32 TopOutside = Top(DrawRect) > ClipTop;
  b32 BotInside  = Bot(DrawRect) < ClipTop;
  b32 Result = TopOutside && BotInside;
  return Result;
}

file_local b32 IsClippingBot(rect2f DrawRect, rect2f ClipRect){
  r32 ClipBot = Bot(ClipRect);
  b32 TopInside  = Top(DrawRect) > ClipBot;
  b32 BotOutside = Bot(DrawRect) < ClipBot;
  b32 Result = TopInside && BotOutside;
  return Result;
}

//DoEntityButtonRect(Row->ImguiID, rect2f ButtonRect, ButtonColor)
 
inline file_local b32 MenuHasChildren(me_node* MenuEntityNode){
  b32 Result = MenuEntityNode->FirstChild != 0;
  return Result;
}

;
inline file_local b32 EntityHasChildren(ecs::entity_node* EntityNode){
  b32 Result = EntityNode->FirstChild != 0;
  return Result;
}


file_local void AddChildEntitiesLoadedToMenuTree(me_tree& MenuTree, me_node* MenuNode, ecs::entity_tree& EntityTree, ecs::entity_node* EntityNode){
  ecs::entity_node* EntityChild = EntityNode->FirstChild;
  do
  {
    // Is it smart to edit the me_tree while we are iterating through it....? I don't feel confident
    menu_entity_row NewRow = {};
    NewRow.EntityID = (*EntityChild->Data)->ID;
    NewRow.ImguiID = NewButtonID();
    NewRow.Open = false;
    MenuTree.NewNode(MenuNode, NewRow);
    EntityChild = EntityChild->NextSibling;
  }while(EntityChild != EntityNode->FirstChild);
}


file_local void DoEntityButtonRect(imgui_context* ImguiContext, rect2f ButtonRect, imgui_button_color& ButtonColor, menu_entity_row* MenuRowData) {
  ImguiPlainButton(ImguiContext, MenuRowData->ImguiID, ButtonRect, ButtonColor);
  if(ImguiIsActive(MenuRowData->ImguiID) && ImguiIsHot(MenuRowData->ImguiID) && jwin::Released(ImguiContext->LeftMouse))
  {
    MenuRowData->Open = !MenuRowData->Open;
  }
}

inline file_local rect2f GetRectForRow(rect2f ContentRect, r32 RowStart, r32 RowHeight){
  rect2f Result = Rect2f(ContentRect.X, RowStart, ContentRect.W, RowHeight);
  return Result;
}


void DrawIcon(b32 RowOpen, v4 TexCoord, rect2f RowRect) {
  rect2f IconRect = Rect2f(RowRect.X, RowRect.Y, RowRect.H, RowRect.H);
  //rect2f SearchIconRect = Shrink(SearchIconRectBackground, 0.1*SearchIconRectBackground.W);
  render::DrawIconCanonicalSpace(CenteredRect(IconRect), TexCoord, V4(1,1,1,1));
}

void DrawText(ecs::entity_node* EntityNode, v2 TextPos, rect2f ButtonRect){
  char SuffixBuff[16] = {};
  ecs::entity* Entity = *EntityNode->Data;
  FormatString(SuffixBuff, sizeof(SuffixBuff)-1, " (%d)", EntityNode->ChildCount);
  char ButtonBuff[128] = {};
  FormatString(ButtonBuff, sizeof(ButtonBuff)-1, "%s%s", Entity->Name, EntityNode->ChildCount > 0 ? SuffixBuff : "");
  render::DrawTextCanonicalSpace(TextPos, ButtonRect, GlobalState->ImguiContext.FontSize, (utf8_byte const *) ButtonBuff, V4(1.0,1.0,1.0,1.0));
}

void GetEntityName(ecs::entity_node* EntityNode, size_t BuffLen, char TextBuff[])
{
  char SuffixBuff[16] = {};
  ecs::entity* Entity = *EntityNode->Data;
  FormatString(SuffixBuff, sizeof(SuffixBuff)-1, " (%d)", EntityNode->ChildCount);
  FormatString(TextBuff, BuffLen-1, "%s%s", Entity->Name, EntityNode->ChildCount > 0 ? SuffixBuff : "");
}

struct reactive_size {
  u32 RowCount;       // Total number of rows to draw
  u32* DivCounts;     // Total number of divs per row
  rect2f* RowRects;   // Rects holding the position and size of each row.
  rect2f** DivRects;  // Rects holding each div within a row.
  u32* SplitCounts;   // Each row can be divided into several rows if the bounding rect is too small
  u32** SplitIndeces; // Within each DivSizes This array holds the indeces where a new row is started
};


void CalculateRowBreakIndeces(imgui_row::div_hint* FirstDivHint, u32* DivBreakCount, u32* DivBreakPointIndeces, const rect2f& ClipRect)
{
  imgui_row::div_hint* DivHint = FirstDivHint;
  *DivBreakCount = 0;
  u32 DivIndex = 0;
  r32 RightEdge = ClipRect.X + ClipRect.W;
  r32 X0 = ClipRect.X;
  while(DivHint) {
    imgui_row::header* Header = DivHint->Header;
    r32 X1 = 0;
    if(DivIndex == 0){
      X1 = X0 + Header->Size.X;
    }else{
      X1 = X0 + Header->Size.X;
      if(X1 > RightEdge)
      {
        X0 = ClipRect.X;
        X1 = ClipRect.X + Header->Size.X;
        DivBreakPointIndeces[*DivBreakCount] = DivIndex;
        *DivBreakCount++;
      }
    }
    X0 = X1;
    DivHint = DivHint->Next;
    DivIndex++;
  }
}
#if 0

void StackDivsVertically(imgui_row::div_hint* FirstDivHint, u32 DivCount, rect2f* RowDivs, const rect2f& ClipRect)
{
  imgui_row::div_hint* DivHint = FirstDivHint;
  int DivIndex = 0;
    
  while(DivHint) {
    imgui_row::head* Header = DivHint->Header;
    if(DivIndex == 0){
      RowDivs[0] = Rect2f(ClipRect.X, 0, Header.Size.X, 0);
    }else{
      rect2f NewDivRect = Rect2f(RowDivs[DivIndex-1].X + RowDivs[DivIndex-1].W, 0, Header.Size.X, 0);
      if(NewDivRect.X + NewDivRect.W > ClipRect.X + ClipRect.W)
      {
        RowDivs[DivIndex] = Rect2f(ClipRect.X, 0, Header.Size.X, 0);
      }else{
        RowDivs[DivIndex] = NewDivRect;
      }
    }
    DivHint = DivHint->Next;
    DivIndex++;
  }
  Assert(DivCount == DivIndex);
}

void StackDivsHorizontally(imgui_row::div_hint* FirstDivHint, u32 DivCount, rect2f* RowDivs, const rect2f& ClipRect)
{
  imgui_row::div_hint* DivHint = FirstDivHint;
  int DivIndex = 0;
    
  r32 MaxHeight = 0;
  r32 YPos = 0;
  while(DivHint){
    imgui_row::head* Header = DivHint->Header;

    bool NewRow = false;

    if(DivIndex == 0){
      RowDivs[0].H =  Header.Size.Y;
      MaxHeight = Header.Size.Y;
      YPos = -Header.Size.Y;
    }else{
      if(RowDivs.X == ClipRect.X)
      {
        // New Row

        MaxHeight = 0;
      }else{
        if(MaxHeight < Header->Size.Y)
        {
          MaxHeight = Header->Size.Y;
        }
        RowDivs[0].Y == Minimum(RowDivs[0].Y, Header->Size.Y);
      }
    }

    if(RowDivs[DivIndex].X + RowDivs[DivIndex].W == ClipRect.X){
      RowDivs[0] = Rect2f(ClipRect.X, 0, Header.Size.X, 0);
    }else{
      rect2f NewDivRect = Rect2f(RowDivs[DivIndex-1].X + RowDivs[DivIndex-1].W, 0, Header.Size.X, 0);
      if(NewDivRect.X + NewDivRect.W > ClipRect.X + ClipRect.W)
      {
        RowDivs[DivIndex] = Rect2f(ClipRect.X, 0, Header.Size.X, 0);
      }else{
        RowDivs[DivIndex] = NewDivRect;
      }
    }
    DivHint = DivHint->Next;
    DivIndex++;
  }
  Assert(DivCount == DivIndex);
}
#endif

void PushDummyDiv(imgui_row& Row, imgui_row::div_hint* DummyHint, imgui_row::header* DummyHeader, imgui_row::div_hint** OriginalDivHint, imgui_row::header** OriginalDivHeader)
{
  Assert(Row.m_head && Row.m_tail); // Should not be here with a empty row i don't think

  
  *OriginalDivHint = Row.m_divTail;
  *OriginalDivHeader = Row.m_divTail->Header;
  if(Row.m_divHead)
  {
    Assert(Row.m_divCount != 0); // Sanity Check
    Assert(Row.m_divTail); // Sanity Check
    DummyHeader->Size = CalculateDivSize(Row.m_divTail->Header->Next);
    Row.m_tail->Next    = DummyHeader;
    Row.m_tail          = DummyHeader;
    Row.m_divTail->Next = DummyHint;
    Row.m_divTail       = DummyHint;
  }else{
    Assert(Row.m_divCount == 0);  // Sanity Check
    Assert(!Row.m_divTail); // Sanity Check
    DummyHeader->Size = CalculateDivSize(Row.m_head);
    Row.m_tail->Next = DummyHeader;
    Row.m_tail = DummyHeader;
    Row.m_divHead = DummyHint;
    Row.m_divTail = DummyHint;
  }
  Row.m_divCount++;
}
void PopDummyDiv(imgui_row& Row, imgui_row::div_hint* DummyHint, imgui_row::header* DummyHeader, imgui_row::div_hint* OriginalDivHint, imgui_row::header* OriginalDivHeader)
{
  if(Row.m_divTail == DummyHint)
  {
    Assert(Row.m_divCount != 0);       // Sanity Check
    Assert(Row.m_tail == DummyHeader); // Sanity Check
    Row.m_tail = OriginalDivHeader;
    Row.m_tail->Next = 0;
    if(Row.m_divCount == 1)
    {
      Row.m_divHead = 0;
      Row.m_divTail = 0;
    }else{
      Row.m_divTail = OriginalDivHint;
      Row.m_divTail->Next = 0;
    }
    Row.m_divCount--;
  }
}

reactive_size GetFullReactiveSize(cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect){

  reactive_size Result = {};
/*
  u32 RowCount;       // Total number of rows to draw
  u32* DivCounts;     // Total number of divs per row
  rect2f* RowRects;   // Rects holding the position and size of each row.
  rect2f** DivRects;  // Rects holding each div within a row.
  u32* SplitCounts;   // Each row can be divided into several rows if the bounding rect is too small
  u32** SplitIndeces; // Within each DivSizes This array holds the indeces where a new row is started
*/
  Result.RowCount      = ImguiRows.Size();
  Result.DivCounts     = PushArray(GlobalTransientArena, Result.RowCount, u32);
  Result.RowRects      = PushArray(GlobalTransientArena, Result.RowCount, rect2f);
  Result.DivRects      = PushArray(GlobalTransientArena, Result.RowCount, rect2f*);
  Result.SplitCounts   = PushArray(GlobalTransientArena, Result.RowCount, u32);
  Result.SplitIndeces  = PushArray(GlobalTransientArena, Result.RowCount, u32*);

  // Step One: Figure out where the 

  for (int i = 0; i < Result.RowCount; ++i)
  {
    imgui_row& Row = ImguiRows[i];

#if 0 
    r32 XRow = ClipRect.X;
    r32 YRow = ClipRect.Y + ClipRect.H;
    r32 XSize = 0;
    r32 YSize = 0;
    r32 XPos = ClipRect.X;
    r32 YPos = ClipRect.Y;
    rect2f TotalRowRect = {};
#endif

    // Prepare a dummy divhint to temporarily put at the end to help div position/size - calculations
    imgui_row::header FinalDivHeader = {};
    imgui_row::div_hint FinalDivHint = {}; // Remember to remove this later.
    FinalDivHeader.Type = imgui_row::type::DIV_HINT;
    FinalDivHeader.Data = (void*) &FinalDivHint;
    FinalDivHeader.Next = 0;
    FinalDivHeader.Size = V2(0,0);
    imgui_row::header* OriginalDivHeader = 0;
    imgui_row::div_hint* OriginalDivHint = 0;

    if(Row.m_tail->Type != imgui_row::type::DIV_HINT)
    {
      Assert(Row.m_divTail->Header->Next);
      PushDummyDiv(Row, &FinalDivHint, &FinalDivHeader, &OriginalDivHint, &OriginalDivHeader);
    }

    if(Row.m_divHead) {
      rect2f DivRect = {};
      // Sum up all pushed m_divHeads


      // If there is no divhint at the end, we temporarily insert one (which we remove later) to 
      // simplify the div size calculations. That is we handle the special case of calculating the size
      // of the last div here so the rest of the positioning can assume every div is capped by a divHint
      

      Result.DivCounts[i] = Row.m_divCount;
      Result.SplitIndeces[i] = PushArray(GlobalTransientArena, Result.DivCounts[i], u32);
      CalculateRowBreakIndeces(Row.m_divHead, &Result.SplitCounts[i], Result.SplitIndeces[i], ClipRect);
      #if 0
      StackDivsVertically(ImguiRows->m_divHead, Result.DivSizes[i], ClipRect);
      StackDivsHorizontally(ImguiRows->m_divHead, Result.DivSizes[i], ClipRect);

      int DivIndex = 0;
      bool NewRow = true;
      r32 MaxYSizeForRow = 0;
      while(DivHint) {
        imgui_row::head* Header = DivHint->Header;

        // First Pile them up horizontally and figure out if and where we need to break for a new row.
        if(DivRect.X == 0)
        {
          Assert(DivRect.X == 0 && DivRect.Y == 0 && DivRect.W == 0, DivRect.H == 0);
          DivRect = Rect2f(ClipRect.X, ClipRect.Y - Header.Size.Y, Header.Size.X, Header.Size.Y);
        }else{
          rect2f NewDivRect = Rect2f(DivRect.X + DivRect.W, DivRect.Y)
        }

        // Then we can figure out the max Y-size for each div and row and position them vertically.


        if(DivIndex == 0)
        {

        }


        if(DivIndex == 0 || XSize + Header.Size.X < ClipRect.W) {
          NewRow = true;
        }else{
          NewRow = false;
        }

        if(NewRow) {
          MaxYSizeForRow = Header.Size.Y;
          XSize += Maximum(XSize, Header.Size.X);
          YSize += MaxYSizeForRow;
          YPos -= MaxYSizeForRow;
          XPos =  ClipRect.X;
        }else{
          Assert(MaxYSizeForRow);
          MaxYSizeForRow = Maximum(MaxYSizeForRow, Header.Size.Y);
          XPos  +=  ClipRect.X;
          XSize += Header.Size.X;
          YSize += Maximum(YSize, Header.Size.Y);
        }

      #endif
        PopDummyDiv(Row, &FinalDivHint, &FinalDivHeader, OriginalDivHint, OriginalDivHeader);
      }
    }

    #if 0
    if(ImguiRows->m_divTail != ImguiRows->m_tail)
    {
      // Sum the last one
    }

      imgui_row::div_hint* DivHint = ImguiRows->m_divHead;
      int DivIndex = 0;
      while(DivHint){
        if(DivIndex == 0){
          DivSizes[i][DivIndex] = CountDivSize();
          Assert(0);
        }else{
          DivSizes[i][DivIndex] = CountDivSize() - DivSizes[i][DivIndex-1];
          Assert(0);
        }
        DivHint = DivHint->Next;
        DivIndex++;
      }

      imgui_row::header* H = DivHint->Header;

    }else{
      imgui_row::header* H = Row->m_head;
    }
    for (int j = 0; j < Result.DivCounts[i]; ++j)
    {
      DivSizes[i][j] = 
    }
  }
  #endif
  return Result;
}

void DrawRowList(cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect)
{
  SCOPED_TRANSIENT_ARENA;
  v2 Pos = V2(ClipRect.X,ClipRect.Y);
  v2 Size = V2(ClipRect.W,ClipRect.H);
  const r32 ScrollAmmount = 0;
  r32 TotalHeight = 0;
  u32 MaxDivCount = 1;
  u32 RowCount = ImguiRows.Size();
  r32* RowHeights = PushArray(GlobalTransientArena, RowCount, r32);
  for (int i = 0; i < RowCount; ++i)
  {
    imgui_row& Row = ImguiRows[i];
    v2* DivLengths = PushArray(GlobalTransientArena, Row.m_divCount+1, v2);
    v2 RowSize = Row.GetSize(DivLengths);
    r32 RowWidth = 0;
    r32 RowHeight = 0;
    for (int j = 0; j < Row.m_divCount+1; ++j)
    {
      v2 DivSize = DivLengths[j];
      if(RowWidth + DivSize.X > Size.X)
      {
        RowHeight += DivSize.Y;
        RowWidth = Maximum(RowWidth, DivSize.X);
      }else{
        RowHeight = Maximum(RowHeight, DivSize.Y);
        RowWidth += DivSize.X;
      }
    }
    if(i == 0){
      
      RowHeights[0] = RowHeight;
    }else{
      r32 Height = Row.GetSize(DivLengths).Y;
      RowHeights[i] = RowHeight + RowHeights[i-1];
    }
    TotalHeight += RowHeights[i];
  }

  r32 YPos = Pos.Y + Size.Y;
  for (int i = 0; i < RowCount; ++i)
  {
    r32 Percentage = RowHeights[i] / TotalHeight;
    if(Percentage >= ScrollAmmount && Percentage < 1)
    {
      imgui_row& Row = ImguiRows[i];
      YPos = Row.Draw(Pos.X, YPos, Rect2f(Pos, Size), RowCount);
    }
  }
}

b32 ImguiEntityComponentTree(menu_entity_tree* MenuEntityTree, v2 Pos, v2 Size) {
  SCOPED_TRANSIENT_ARENA;
  imgui_context* ImguiContext = &GlobalState->ImguiContext;
  imgui_button_color ButtonColor = {};
  ButtonColor.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "taupe");
  ButtonColor.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "persian indigo");
  ButtonColor.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "egyptian blue");
  ButtonColor.HotColor =  menu::GetColor(&GlobalState->ColorTable, "rich black");

  render::font& Font = GlobalRenderer->Font;

  rect2f ContentRect = Rect2f(Pos,Size);

  v2 Padding = PixelToCanonicalSpace(V2(2,2));

  r32 FontSize = ImguiContext->FontSize;
  r32 RowHeight = Font.GetLineSpacingCanonicalSpace(FontSize);
  r32 DescentOffset = Font.GetCanonicalFontDescenOffset(FontSize);
  r32 TabWidth  = Font.GetTextSizeCanonicalSpace(FontSize, (utf8_byte const *) "  ").X;

  ecs::entity_tree& EntityTree = GlobalEntityManager->EntityTree;
  me_tree& MenuTree = MenuEntityTree->EntityTree;
  r32 Y = Pos.Y + Size.Y - RowHeight;
  bool SkipSubTree = false;
  me_iterator It = MenuEntityTree->EntityTree.PreOrderIterator(EntityTree.NodeCount());
  cmn::vector<imgui_row> ImguiRows = cmn::vector<imgui_row>::CreateTransient(EntityTree.NodeCount());
  const r32 IconSize = 32;
  while(me_node* MenuNode = It.Next(SkipSubTree))
  {
    int Index = It.Depth() - 1;
    if(Index != 0){
      menu_entity_row* MenuRowData = MenuNode->Data;
      ecs::entity_id* EntityID = &MenuRowData->EntityID;
      ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
      ecs::entity_node* EntityNode = Entity->Node;

      r32 XOffset = (Index-1)*RowHeight;

      imgui_row RowRenderer = {};
      RowRenderer.Push(imgui_row::Padding(XOffset,RowHeight));
      RowRenderer.Push(imgui_row::Icon( IconSize, MenuRowData->Open ? ICON_ANGLE_DOWN : ICON_ANGLE_RIGHT ));

      char NameBuffer[128] = {};
      GetEntityName(EntityNode,sizeof(NameBuffer),NameBuffer);
      RowRenderer.Push(imgui_row::Text(FontSize, &Font, sizeof(NameBuffer), NameBuffer));
      RowRenderer.Push(imgui_row::DivHint()); // DivHint is a hint to the row-render that if it needs to break things into several rows it will do so along divs
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::POSITION))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_LOCATION ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::GEOMETRY))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_GEOMETRY ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::MATERIAL))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_MATERIAL ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::COLLIDER))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_COLLIDER ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::LIGHT))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_LIGHT ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::CAMERA))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_CAMERA ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::CONTROLLER))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_CONTROLLER ));
      }
      if(ecs::HasComponents(GetEntityManager(), &Entity->ID, ecs::flag::RENDER))
      {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_UNKNOWN ));
      }
      SkipSubTree = !MenuRowData->Open;
      ImguiRows.PushBack(RowRenderer);
    }
    if(Y+RowHeight < Pos.Y)
    {
      break;
    }
  }

  DrawRowList(ImguiRows, Rect2f(Pos,Size));
  return false;
}

void DrawEntityTree(application_imgui* AppImgui) {

#if 1
  menu_entity_tree* MenuEntityTree = AppImgui->MenuEntityTree;

  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(GlobalState->ImguiContext.FontSize);
  
  v2 ScrollListPos  = V2(MenuEntityTree->BorderWindow.Region.X, MenuEntityTree->BorderWindow.Region.Y);
  v2 ScrollListSize = V2(MenuEntityTree->BorderWindow.Region.W, MenuEntityTree->BorderWindow.Region.H - MenuEntityTree->BorderWindow.HeaderSize);

  ImguiBorderWindow(&MenuEntityTree->BorderWindow, "Entities");

  ImguiEntityComponentTree(MenuEntityTree, ScrollListPos, ScrollListSize);
#endif
}
