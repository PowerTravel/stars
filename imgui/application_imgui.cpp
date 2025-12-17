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
  Result.MenuEntityTree->EntityTree     = me_tree::Create();
  Result.MenuEntityTree->EntityTree.NewNode(); // EmptyRoot


  Result.ColorListData->ColorList = CreateScrollableTextList();
  Result.ColorListData->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.1,0.25), V2(0.1,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  for (int i = 0; i < ColorCount; ++i)
  {
    Result.ColorListData->ImguiIDs[i] = NewButtonID();
  }

  return Result;
}

menu_entity_row MenuEntityRow(ecs::entity_id EntityID)
{
  menu_entity_row NewRow = {};
  NewRow.EntityID = EntityID;
  NewRow.ImguiID = NewButtonID();
  NewRow.Open = false;
  u32 ComponentCount = ecs::GetComponentCount(GetEntityManager(), &EntityID);
  NewRow.ComponentImguiIDs = cmn::vector<menu_entity_component_id>::Create(ComponentCount);
  
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::POSITION))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::POSITION));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::GEOMETRY))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::GEOMETRY));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::MATERIAL))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::MATERIAL));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::COLLIDER))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::COLLIDER));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::LIGHT))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::LIGHT));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::CAMERA))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::CAMERA));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::CONTROLLER))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::CONTROLLER));
  }
  if(ecs::HasComponents(GetEntityManager(), &EntityID, ecs::flag::RENDER))
  {
    NewRow.ComponentImguiIDs.PushBack(ImguiEntityComponent(ecs::flag::RENDER));
  }
  return NewRow;
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

  u32 Depth = 0;
  ecs::entity* E  = GetEntityFromID(GetEntityManager(), NewEntity);
  ecs::entity_node* EN = E->Node;
  while(EN->Parent){
    Depth++;
    EN = EN->Parent;
  }
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);
  r32 XOffset = (Depth-1)*RowHeight;
  r32 IconSize = 16;

  menu_entity_row NewRow = MenuEntityRow(*NewEntity);
  
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
    menu_entity_row NewRow = MenuEntityRow((*EntityChild->Data)->ID);
    MenuTree.NewNode(MenuNode, NewRow);
    EntityChild = EntityChild->NextSibling;
  }while(EntityChild != EntityNode->FirstChild);
}


  file_local void DoEntityButtonRect(imgui_context* ImguiContext, menu_entity_row* MenuRowData) {
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

struct reactive_row_size {
  u32 DivCount;
  imgui_row::div_hint** Divs;       // All divHints for row
  u32 SplitCount;
  u32* SplitIndeces;
  
  rect2f RowRect;// Rect holding the combined size of the total row;
  rect2f* DivRects; // Rect holding the size of each div;
  rect2f* SplitRowRects; // Rect holding the size of each split row.
};
struct reactive_size {
  u32 RowCount;       // Total number of rows to draw
  u32* DivCounts;     // Total number of divs per row
  rect2f* RowRects;   // Rects holding the position and size of each row.
  rect2f** DivRects;  // Rects holding each div within a row.
  u32* SplitCounts;   // The sizes of each list of u32 in SplitIndeces repressenting the number of divs to get a new row
  u32** SplitIndeces; // This array holds the indeces where a new row is started
  reactive_row_size* ReactiveRowSizes;
  v2 TotalSize;
};

file_local v2 SetPositionsForRow(r32 XStart, r32 YStart, reactive_row_size* ReactiveRowSize, u32 StartIndex, u32 EndIndex)
{
  r32 MaxHeight = 0;
  r32 MaxWidth  = 0;
  // Set X positions and calculate height of the biggest div.
  {
    r32 X0 = XStart;
    for (int i = StartIndex; i < EndIndex; ++i)
    {
      imgui_row::div_hint* Div = ReactiveRowSize->Divs[i];
      imgui_row::header* Header = Div->Header;

      r32 X1 = X0 + Header->Size.X;
      MaxHeight = Maximum(Header->Size.Y, MaxHeight);
      MaxWidth = X1 - XStart;

      ReactiveRowSize->DivRects[i] = Rect2f(X0, 0, Header->Size.X, 0);

      X0 = X1;
    }
  }

  {
    for (int i = StartIndex; i < EndIndex; ++i)
    {
      imgui_row::div_hint* Div = ReactiveRowSize->Divs[i];
      imgui_row::header* Header = Div->Header;

      r32 Y0 = YStart + MaxHeight - Header->Size.Y;
      r32 Y1 = Y0 + Header->Size.Y;

      ReactiveRowSize->DivRects[i].Y = Y0;
      ReactiveRowSize->DivRects[i].H = Header->Size.Y;
    }
  }

  return V2(MaxWidth, MaxHeight);
}

file_local v2 CalculateRowBreakIndeces(r32 RowYPos, imgui_row::div_hint* FirstDivHint, const rect2f& ClipRect, reactive_row_size* ReactiveRowSize)
{
  // Find out where splits will happen
  {
    const r32 RightEdge = ClipRect.X + ClipRect.W;
    ReactiveRowSize->SplitCount = 0;
    u32 DivIndex = 0;
    r32 X0 = ClipRect.X;
    imgui_row::div_hint* DivHint = FirstDivHint;
    r32 XPadding = 0;
    while(DivHint) {
      ReactiveRowSize->Divs[DivIndex] = DivHint;
      imgui_row::header* Header = DivHint->Header;
      r32 X1 = 0;
      if(DivIndex == 0){
        X1 = X0 + Header->Size.X;
      }else{
        X1 = X0 + Header->Size.X;
        if(X1 > RightEdge)
        {
          r32 XPadding = ReactiveRowSize->Divs[DivIndex-1]->Padding;
          X0 = ClipRect.X + XPadding;
          X1 = X0 + Header->Size.X;
          ReactiveRowSize->SplitIndeces[ReactiveRowSize->SplitCount++] = DivIndex;
        }
      }
      X0 = X1;
      DivIndex++;
      DivHint = DivHint->Next;
    }
    ReactiveRowSize->SplitIndeces[ReactiveRowSize->SplitCount++] = DivIndex;
  }

  v2 TotalSize = {};
  {
    r32 YPositionForRow = RowYPos;
    for (int SplitIndex = 0; SplitIndex < ReactiveRowSize->SplitCount; ++SplitIndex)
    {
      u32 StartIndex = 0;
      if(SplitIndex == 0)
      {
        StartIndex = 0;
      }else{
        StartIndex = ReactiveRowSize->SplitIndeces[SplitIndex-1];
      }
      
      u32 EndIndex = ReactiveRowSize->SplitIndeces[SplitIndex];

      // This is annoying, but each div hint header holds the size of the previous group of row-elements,
      //                   the DivHint has a x-padding for the next row __iff__ a break occured at that divHint.
      r32 RowPadding = 0;
      if(SplitIndex > 0)
      {
        RowPadding = ReactiveRowSize->Divs[SplitIndex-1]->Padding;
      }
      
      // Sets positions of divs with origin in top left. Starting at 0,0 and going down.
      v2 SizeOfSplitRow = SetPositionsForRow(RowPadding, YPositionForRow, ReactiveRowSize, StartIndex, EndIndex);
      ReactiveRowSize->SplitRowRects[SplitIndex] = Rect2f(V2(RowPadding,YPositionForRow), SizeOfSplitRow);
      YPositionForRow += SizeOfSplitRow.Y;
      TotalSize.X = Maximum(SizeOfSplitRow.X, TotalSize.X);
      TotalSize.Y += SizeOfSplitRow.Y;
    }
  }
  return TotalSize;
}

file_local r32 GetMaxHeightForRow(u32 StartIndex, u32 EndIndex, rect2f* RowDivs){
  r32 Result = 0;
  u32 i = StartIndex;
  do{
    rect2f& DivRect = RowDivs[i];
    Result = Maximum(Result, DivRect.H);  
    i++;
  }while(i < EndIndex);
  return Result;
}

file_local void GetDivRectSizes(imgui_row::div_hint* FirstDivHint, u32 DivCount, rect2f* RowDivs, u32 DivBreakCount, u32* DivBreakPointIndeces, rect2f* RowRect, const rect2f& ClipRect, reactive_row_size* ReactiveRowSize)
{
  {
    // Set Div Sizes
    u32 DivIndex = 0;
    imgui_row::div_hint* DivHint = FirstDivHint;
    while(DivHint) {
      imgui_row::header* Header = DivHint->Header;
      rect2f& DivRect = RowDivs[DivIndex];
      DivRect.W = Header->Size.X;
      DivRect.H = Header->Size.Y;
      rect2f DivRect2 = DivRect;
      ReactiveRowSize->DivRects[DivIndex] = DivRect2;
      DivIndex++;
      DivHint = DivHint->Next;
    }
    Assert(DivCount == DivIndex);
  }

  u32 StartIndex = 0;
  r32 Y0 = 0;
  r32 MaxHeight = 0;
  r32 MaxWidth  = 0;
  for(u32 DivBreakIndex = 0; DivBreakIndex < DivBreakCount; DivBreakIndex++)
  {
    u32 EndIndex = DivBreakPointIndeces[DivBreakIndex];
    r32 MaxHeightForRow = GetMaxHeightForRow(StartIndex, EndIndex, RowDivs);
    Y0 -= MaxHeightForRow;
    r32 X0 = 0;
    r32 X1 = 0;

    MaxHeight += MaxHeightForRow;
    for (int i = StartIndex; i < EndIndex; ++i)
    {
      /* code */
      rect2f& DivRect = RowDivs[i];
      DivRect.X = X0;
      DivRect.Y = Y0;
      X0 += DivRect.W;
      MaxWidth = Maximum(X0, MaxWidth);
    }
    StartIndex = EndIndex;
  }

  *RowRect = Rect2f(0, 0, MaxWidth, MaxHeight);
}

void PushDummyDiv(imgui_row& Row, imgui_row::div_hint* DummyHint, imgui_row::header* DummyHeader, imgui_row::div_hint** OriginalDivHint, imgui_row::header** OriginalTailHeader)
{
  Assert(Row.m_head && Row.m_tail); // Should not be here with a empty row i don't think

  *OriginalTailHeader = Row.m_tail;
  if(Row.m_divHead)
  {
    *OriginalDivHint = Row.m_divTail;
    
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
    *OriginalDivHint = 0;
  }

  Row.m_divCount++;
}
void PopDummyDiv(imgui_row& Row, imgui_row::div_hint* DummyHint, imgui_row::header* DummyHeader, imgui_row::div_hint* OriginalDivHint, imgui_row::header* OriginalTailHeader)
{
  if(Row.m_divTail == DummyHint)
  {
    Assert(Row.m_divCount != 0);       // Sanity Check
    Assert(Row.m_tail == DummyHeader); // Sanity Check
    Row.m_tail = OriginalTailHeader;
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

file_local v2 AlignRowRectsBotLeft(u32 RowCount, rect2f* RowRects, u32* DivCounts, rect2f** DivRects)
{
  r32 TotalHeight = 0;
  r32 TotalWidth = 0;
  for (int i = 0; i < RowCount; ++i)
  {
    TotalHeight += RowRects[i].H;
    TotalWidth = Maximum(TotalWidth,RowRects[i].W);
  }

  r32 Height = TotalHeight;
  for (int i = 0; i < RowCount; ++i)
  {
    rect2f& RowRect = RowRects[i];
    Height -= RowRect.H;
    RowRect.Y = Height;
  }
  
  for (int i = 0; i < RowCount; ++i)
  {
    rect2f RowRect = RowRects[i];
    u32 DivRowCount = DivCounts[i];
    for (int j = 0; j < DivRowCount; ++j)
    {
      rect2f& DivRowRect = DivRects[i][j];
      DivRowRect.Y = DivRowRect.Y + RowRect.Y + RowRect.H;
    }
  }
  v2 Result = V2(TotalWidth,TotalHeight);
  return Result;
}

v2 GetTotalSize(u32 RowCount, reactive_row_size* ReactiveRowSizes)
{
  v2 Result = {};
  for (int i = 0; i < RowCount; ++i)
  {
    reactive_row_size* ReactiveRowSize = &ReactiveRowSizes[i];
    Result.X = Maximum(Result.X, ReactiveRowSize->RowRect.W);
    Result.Y += ReactiveRowSize->RowRect.H;
  }
  return Result;
}


void InvertYDirection(v2 Size, u32 RowCount, reactive_row_size* ReactiveRowSizes)
{
  for (int i = 0; i < RowCount; ++i)
  {
    reactive_row_size* ReactiveRowSize = &ReactiveRowSizes[i];
    for (int j = 0; j < ReactiveRowSize->DivCount; ++j)
    {
      rect2f& DivRect = ReactiveRowSize->DivRects[j];
      DivRect.Y = - DivRect.Y - DivRect.H + Size.Y;
    }
    for (int j = 0; j < ReactiveRowSize->SplitCount; ++j)
    {
      rect2f& SplitRowRect = ReactiveRowSize->SplitRowRects[j];
      SplitRowRect.Y = - SplitRowRect.Y - SplitRowRect.H + Size.Y;
    }
    ReactiveRowSize->RowRect.Y = -ReactiveRowSize->RowRect.Y - ReactiveRowSize->RowRect.H + Size.Y;
  }
}

void DebugDrawRowRects(u32 RowCount, reactive_row_size* ReactiveRowSizes)
{

  for (int i = 0; i < RowCount; ++i)
  {
    r32 t = (r32) (i+1) / (r32) RowCount;

    reactive_row_size* ReactiveRowSize = &ReactiveRowSizes[i];
    
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ReactiveRowSize->RowRect), V4(Lerp(t,0,1),0,0,1));
    
    for (int j = 0; j < ReactiveRowSize->SplitCount; ++j)
    {
      rect2f SplitRowRect = ReactiveRowSize->SplitRowRects[j];
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(Shrink(SplitRowRect,PixelToCanonicalSpace(V2(3,3)))), V4(0,0,Lerp(t,0,1),1));
    }

    for (int j = 0; j < ReactiveRowSize->DivCount; ++j)
    {
      rect2f DivRect = ReactiveRowSize->DivRects[j];
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(Shrink(DivRect,PixelToCanonicalSpace(V2(6,6)))), V4(0,Lerp(t,0,1),0,1));
    }


  }
}
reactive_size GetFullReactiveSize(cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect, r32 ScrollAmount){

  reactive_size Result = {};

  Result.RowCount         = ImguiRows.Size();
  Result.DivCounts        = PushArray(GlobalTransientArena, Result.RowCount, u32);
  Result.RowRects         = PushArray(GlobalTransientArena, Result.RowCount, rect2f);
  Result.DivRects         = PushArray(GlobalTransientArena, Result.RowCount, rect2f*);
  Result.SplitCounts      = PushArray(GlobalTransientArena, Result.RowCount, u32);
  Result.SplitIndeces     = PushArray(GlobalTransientArena, Result.RowCount, u32*);
  Result.ReactiveRowSizes = PushArray(GlobalTransientArena, Result.RowCount, reactive_row_size);


  r32 RowPos = ClipRect.Y + ClipRect.H;
  r32 YPos = 0;
  for (int i = 0; i < Result.RowCount; ++i)
  {
    imgui_row& Row = ImguiRows[i];

    reactive_row_size* ReactiveRowSize = &Result.ReactiveRowSizes[i];

    // Prepare a dummy divhint to temporarily put at the end to help div position/size - calculations
    imgui_row::header FinalDivHeader = {};
    imgui_row::div_hint FinalDivHint = {}; // Remember to remove this later.
    FinalDivHeader.Type = imgui_row::type::DIV_HINT;
    FinalDivHeader.Data = (void*) &FinalDivHint;
    FinalDivHeader.Next = 0;
    FinalDivHeader.Size = V2(0,0);
    FinalDivHint.Header = &FinalDivHeader;
    imgui_row::header* OriginalTailHeader = 0;
    imgui_row::div_hint* OriginalDivHint = 0;

    if(Row.m_tail->Type != imgui_row::type::DIV_HINT)
    {
      // If there is no divhint at the end, we temporarily insert one (which we remove later) to 
      // simplify the div size calculations. That is we handle the special case of calculating the size
      // of the last div here so the rest of the positioning can assume every div is capped by a divHint
      PushDummyDiv(Row, &FinalDivHint, &FinalDivHeader, &OriginalDivHint, &OriginalTailHeader);
    }
    Assert(Row.m_divHead && Row.m_head); // Sanity check

    Result.DivCounts[i]    = Row.m_divCount;
    Result.DivRects[i]     = PushArray(GlobalTransientArena, Result.DivCounts[i], rect2f);

/*
struct reactive_row_size {
  rect2f RowRect;// Rect holding the combined size of the total row;
  u32 DivCount;
  rect2f* DivRects; // Rect holding the size of each div;
  imgui_row::div_hint** Divs;       // All divHints for row
  u32 SplitCount;
  u32* SplitIndeces;
  rect2f* SplitRowRects; // Rect holding the size of each split row.
};
*/
    ReactiveRowSize->DivCount      = Row.m_divCount;
    ReactiveRowSize->SplitIndeces  = PushArray(GlobalTransientArena, ReactiveRowSize->DivCount, u32);
    ReactiveRowSize->DivRects      = PushArray(GlobalTransientArena, ReactiveRowSize->DivCount, rect2f);
    ReactiveRowSize->SplitRowRects = PushArray(GlobalTransientArena, ReactiveRowSize->DivCount, rect2f);
    ReactiveRowSize->Divs          = PushArray(GlobalTransientArena, ReactiveRowSize->DivCount, imgui_row::div_hint*);

    v2 TotalSize = CalculateRowBreakIndeces(YPos, Row.m_divHead, ClipRect, ReactiveRowSize);
    ReactiveRowSize->RowRect = Rect2f(V2(0,YPos), TotalSize);
    YPos+=TotalSize.Y;

    Result.SplitCounts[i] = ReactiveRowSize->SplitCount;
    Result.SplitIndeces[i] = ReactiveRowSize->SplitIndeces;

    // Set:
    //  1: The size of RowRect positioned at (0,0)
    //  2: The size and relative position of DivRect wihtin each RowRects;
    //GetDivRectSizes(Row.m_divHead, Result.DivCounts[i], Result.DivRects[i], Result.SplitCounts[i], Result.SplitIndeces[i], &Result.RowRects[i], ClipRect, ReactiveRowSize);

    PopDummyDiv(Row, &FinalDivHint, &FinalDivHeader, OriginalDivHint, OriginalTailHeader);
  }

  Result.TotalSize = GetTotalSize(Result.RowCount, Result.ReactiveRowSizes);
  InvertYDirection(Result.TotalSize, Result.RowCount, Result.ReactiveRowSizes);

  DebugDrawRowRects(Result.RowCount, Result.ReactiveRowSizes);


  // Aligns each RowRect and DivRect such that the bottom of the list is at 0,0;
  //v2 FullListSize = AlignRowRectsBotLeft(Result.RowCount, Result.RowRects, Result.DivCounts, Result.DivRects);

  return Result;
}

void DrawRowList(const reactive_size& ReactiveSize, cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect)
{
  SCOPED_TRANSIENT_ARENA;

  for (int i = 0; i < ReactiveSize.RowCount; ++i)
  {
    r32 t = (r32) (i+1) / (r32) ReactiveSize.RowCount;

    reactive_row_size* ReactiveRowSize = &ReactiveSize.ReactiveRowSizes[i];
    
    rect2f RowRect = ReactiveRowSize->RowRect;
    RowRect.X += ClipRect.X;
    RowRect.Y += ClipRect.Y + ClipRect.H - ReactiveSize.TotalSize.Y;
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(RowRect), V4(Lerp(t,0,1),0,0,1));
    
    for (int j = 0; j < ReactiveRowSize->SplitCount; ++j)
    {
      rect2f SplitRowRect = ReactiveRowSize->SplitRowRects[j];
      SplitRowRect.X += ClipRect.X;
      SplitRowRect.Y += ClipRect.Y + ClipRect.H - ReactiveSize.TotalSize.Y;
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(Shrink(SplitRowRect,PixelToCanonicalSpace(V2(3,3)))), V4(0,0,Lerp(t,0,1),1));
    }

    for (int j = 0; j < ReactiveRowSize->DivCount; ++j)
    {
      rect2f DivRect = ReactiveRowSize->DivRects[j];
      DivRect.X += ClipRect.X;
      DivRect.Y += ClipRect.Y + ClipRect.H - ReactiveSize.TotalSize.Y;
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(Shrink(DivRect,PixelToCanonicalSpace(V2(6,6)))), V4(0,Lerp(t,0,1),0,1));
    }


  }
#if 0
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
      YPos = Row.Draw(Pos.X, YPos, Rect2f(Pos, Size));
    }
  }
#endif
}

void DebugDrawReactiveSizes(reactive_size& s)
{

#if 0

struct reactive_size {
  u32 RowCount;       // Total number of rows to draw
  u32* DivCounts;     // Total number of divs per row
  rect2f* RowRects;   // Rects holding the position and size of each row.
  rect2f** DivRects;  // Rects holding each div within a row.
  u32* SplitCounts;   // The sizes of each list of u32 in SplitIndeces repressenting the number of divs to get a new row
  u32** SplitIndeces; // This array holds the indeces where a new row is started
};

#endif
  v4 Colors[] = {
    V4(0.2,0.2,0.2,1),V4(0.3,0.3,0.3,1),
    V4(0.4,0.4,0.4,1),V4(0.5,0.5,0.5,1),
    V4(0.6,0.6,0.6,1),V4(0.7,0.7,0.7,1),
    V4(0.8,0.8,0.8,1),V4(0.9,0.9,0.9,1),
  };
  v4 Colors2[] = {
    V4(0,1,0,1)      ,V4(0.1,1,0.1,1),
    V4(0.2,1,0.2,1),V4(0.3,1,0.3,1),
    V4(0.4,1,0.4,1),V4(0.5,1,0.5,1),
    V4(0.6,1,0.6,1),V4(0.7,1,0.7,1),
    V4(0.8,1,0.8,1),V4(0.9,1,0.9,1),
  };
  u32 ColorIndex = 4;
  
  for (int i = 0; i < s.RowCount; ++i)
  {
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(s.RowRects[i]), Colors[ColorIndex++ % ArrayCount(Colors)]);
    u32 DivRowCount = s.DivCounts[i];
    for (int j = 0; j < DivRowCount; ++j){
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(s.DivRects[i][j]), Colors2[(i*3 + j) % ArrayCount(Colors2)]);
    }
  }
}

imgui_row CreateEntityRow(menu_entity_row* MenuRowData, render::font& Font, r32 FontSize, r32 XOffset, r32 RowHeight, r32 IconSize){
   
  ecs::entity_id* EntityID = &MenuRowData->EntityID;
  ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
  ecs::entity_node* EntityNode = Entity->Node;

  imgui_row RowRenderer = {};
  RowRenderer.Push(imgui_row::Padding(XOffset,RowHeight));
  v2 IconPaddingSize = PixelToCanonicalSpace(V2(IconSize,IconSize));
  if(EntityHasChildren(EntityNode))
  {
    RowRenderer.Push(imgui_row::Icon(IconSize, MenuRowData->Open ? ICON_ANGLE_DOWN : ICON_ANGLE_RIGHT ), MenuRowData->ImguiID);
  }else{
    
    RowRenderer.Push(imgui_row::Padding(IconPaddingSize.X, IconPaddingSize.Y));
  }

  char NameBuffer[128] = {};
  GetEntityName(EntityNode,sizeof(NameBuffer),NameBuffer);
  RowRenderer.Push(imgui_row::Text(FontSize, &Font, sizeof(NameBuffer), NameBuffer));
  //RowRenderer.Push(imgui_row::DivHint(IconPaddingSize.X));
  for (int i = 0; i < MenuRowData->ComponentImguiIDs.Size(); ++i)
  {
    RowRenderer.Push(imgui_row::DivHint(IconPaddingSize.X));
    menu_entity_component_id* ComponentID = &MenuRowData->ComponentImguiIDs[i];
    switch(ComponentID->Type)
    {
      case ecs::flag::POSITION: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_LOCATION ), ComponentID->ImguiID);
      } break;
      case ecs::flag::GEOMETRY: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_GEOMETRY ), ComponentID->ImguiID);
      } break;
      case ecs::flag::MATERIAL: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_MATERIAL ), ComponentID->ImguiID);
      } break;
      case ecs::flag::COLLIDER: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_COLLIDER ), ComponentID->ImguiID);
      } break;
      case ecs::flag::LIGHT: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_LIGHT ), ComponentID->ImguiID);
      } break;
      case ecs::flag::CAMERA: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_CAMERA ), ComponentID->ImguiID);
      } break;
      case ecs::flag::CONTROLLER: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_CONTROLLER ), ComponentID->ImguiID);
      } break;
      case ecs::flag::RENDER: {
        RowRenderer.Push(imgui_row::Icon( IconSize, ICON_COMPONENT_UNKNOWN ), ComponentID->ImguiID);
      } break;

    }
  }

  return RowRenderer;
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
  cmn::vector<imgui_row> ImguiRows = cmn::vector<imgui_row>::CreateTransient(EntityTree.NodeCount());
  {
    bool SkipSubTree = false;
    me_iterator It = MenuEntityTree->EntityTree.PreOrderIterator(EntityTree.NodeCount());
    const r32 IconSize = 32;
    while(me_node* MenuNode = It.Next(SkipSubTree))
    {
      int Index = It.Depth() - 1;
      if(Index != 0){
        menu_entity_row* MenuRowData = MenuNode->Data;
        r32 XOffset = (Index-1)*RowHeight;

        imgui_row RowRenderer = CreateEntityRow(MenuRowData, Font, ImguiContext->FontSize, XOffset, RowHeight, IconSize);
        SkipSubTree = !MenuRowData->Open;
        ImguiRows.PushBack(RowRenderer);
      }
    }
  }

  reactive_size ReactiveSizes = GetFullReactiveSize(ImguiRows, Rect2f(Pos,Size), 0);
  //DebugDrawReactiveSizes(ReactiveSizes);
  
  DrawRowList(ReactiveSizes, ImguiRows, Rect2f(Pos,Size));

  {
    me_iterator It = MenuEntityTree->EntityTree.PreOrderIterator(EntityTree.NodeCount());
    bool SkipSubTree = false;
    while(me_node* MenuNode = It.Next(SkipSubTree))
    {
      int Index = It.Depth() - 1;
      if(Index != 0){
        menu_entity_row* MenuRowData = MenuNode->Data;

        DoEntityButtonRect(ImguiContext, MenuRowData);
        ecs::entity_id* EntityID = &MenuRowData->EntityID;
        ecs::entity* Entity = GetEntityFromID(GetEntityManager(), EntityID);
        ecs::entity_node* EntityNode = Entity->Node;
        if(MenuRowData->Open && !MenuNode->FirstChild && EntityNode->FirstChild)
        {
          SkipSubTree = !MenuRowData->Open;
          AddChildEntitiesLoadedToMenuTree(MenuTree, MenuNode, EntityTree, EntityNode);
        }
      }
    }
  }

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
