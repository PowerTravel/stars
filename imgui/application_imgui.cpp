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

  Result.ColorListData->ColorList = CreateScrollableTextList();
  Result.ColorListData->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.1,0.25), V2(0.1,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
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
