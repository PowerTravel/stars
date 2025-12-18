#include "application_imgui.h"
#include "platform/jwin_platform_memory.h"
#include "application_imgui_entity_list.h"
#include "application_imgui_color_list.h"

application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount) {
  application_imgui Result = {};

  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace( GlobalState->ImguiContext.FontSize);

  Result.ColorListData = CreateColorList(Arena, ColorCount);

  Result.MenuEntityTree = PushStruct(Arena, menu_entity_tree);
  Result.MenuEntityTree->BorderWindow      = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  Result.MenuEntityTree->VerticalScrollbar = CreateVerticalScrollbar();
  Result.MenuEntityTree->EntityTree     = me_tree::Create();
  Result.MenuEntityTree->EntityTree.NewNode(); // EmptyRoot

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
  rect2f RowRect;// Rect holding the combined size of the total row;

  u32 DivCount;
  imgui_row::div_hint** Divs;       // All divHints for row
  rect2f* DivRects; // Rect holding the size of each div;

  u32 SplitCount;
  rect2f* SplitRowRects; // Rect holding the size of each split row.
};

struct reactive_size {
  u32 RowCount;       // Total number of rows to draw
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

file_local reactive_row_size CreateReactiveRow(u32 DivCount, r32 RowYPos, imgui_row::div_hint* FirstDivHint, const rect2f& ClipRect)
{
  reactive_row_size Result = {};
  Result.DivCount           = DivCount;
  Result.DivRects           = PushArray(GlobalTransientArena, Result.DivCount, rect2f);
  Result.SplitRowRects      = PushArray(GlobalTransientArena, Result.DivCount, rect2f);
  Result.Divs               = PushArray(GlobalTransientArena, Result.DivCount, imgui_row::div_hint*);

  SCOPED_TRANSIENT_ARENA;

  // Find out where splits will happen
  u32* SplitIndeces  = PushArray(GlobalTransientArena, Result.DivCount, u32);
  {
    const r32 RightEdge = ClipRect.X + ClipRect.W;
    Result.SplitCount = 0;
    u32 DivIndex = 0;
    r32 X0 = ClipRect.X;
    imgui_row::div_hint* DivHint = FirstDivHint;
    r32 XPadding = 0;
    while(DivHint) {
      Result.Divs[DivIndex] = DivHint;
      imgui_row::header* Header = DivHint->Header;
      r32 X1 = 0;
      if(DivIndex == 0){
        X1 = X0 + Header->Size.X;
      }else{
        X1 = X0 + Header->Size.X;
        if(X1 > RightEdge)
        {
          r32 XPadding = Result.Divs[DivIndex-1]->Padding;
          X0 = ClipRect.X + XPadding;
          X1 = X0 + Header->Size.X;
          SplitIndeces[Result.SplitCount++] = DivIndex;
        }
      }
      X0 = X1;
      DivIndex++;
      DivHint = DivHint->Next;
    }
    SplitIndeces[Result.SplitCount++] = DivIndex;
  }

  v2 TotalSize = {};
  {
    r32 YPositionForRow = RowYPos;
    for (int SplitIndex = 0; SplitIndex < Result.SplitCount; ++SplitIndex)
    {
      u32 StartIndex = 0;
      if(SplitIndex == 0)
      {
        StartIndex = 0;
      }else{
        StartIndex = SplitIndeces[SplitIndex-1];
      }
      
      u32 EndIndex = SplitIndeces[SplitIndex];

      // This is annoying, but each div hint header holds the size of the previous group of row-elements,
      //                   the DivHint has a x-padding for the next row __iff__ a break occured at that divHint.
      r32 RowPadding = 0;
      if(SplitIndex > 0)
      {
        RowPadding = Result.Divs[SplitIndex-1]->Padding;
      }
      
      // Sets positions of divs with origin in top left. Starting at 0,0 and going down.
      v2 SizeOfSplitRow = SetPositionsForRow(RowPadding, YPositionForRow, &Result, StartIndex, EndIndex);
      Result.SplitRowRects[SplitIndex] = Rect2f(V2(RowPadding,YPositionForRow), SizeOfSplitRow);
      YPositionForRow += SizeOfSplitRow.Y;
      TotalSize.X = Maximum(SizeOfSplitRow.X, TotalSize.X);
      TotalSize.Y += SizeOfSplitRow.Y;
    }
  }
  Result.RowRect = Rect2f(V2(0,RowYPos), TotalSize);
  return Result;
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

file_local v2 GetTotalSize(u32 RowCount, reactive_row_size* ReactiveRowSizes)
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


file_local void InvertYDirection(v2 Size, u32 RowCount, reactive_row_size* ReactiveRowSizes)
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
reactive_size CreateReactiveSize(cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect, r32 ScrollAmount){

  reactive_size Result = {};

  Result.RowCount         = ImguiRows.Size();
  Result.ReactiveRowSizes = PushArray(GlobalTransientArena, Result.RowCount, reactive_row_size);

  r32 RowPos = ClipRect.Y + ClipRect.H;
  r32 YPos = 0;
  for (int i = 0; i < Result.RowCount; ++i)
  {
    imgui_row& Row = ImguiRows[i];

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

    Result.ReactiveRowSizes[i] = CreateReactiveRow(Row.m_divCount, YPos, Row.m_divHead, ClipRect);
    
    YPos+=Result.ReactiveRowSizes[i].RowRect.H;

    PopDummyDiv(Row, &FinalDivHint, &FinalDivHeader, OriginalDivHint, OriginalTailHeader);
  }

  Result.TotalSize = GetTotalSize(Result.RowCount, Result.ReactiveRowSizes);
  InvertYDirection(Result.TotalSize, Result.RowCount, Result.ReactiveRowSizes);


  return Result;
}

void DrawRowList(const reactive_size& ReactiveSize, cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect)
{
  SCOPED_TRANSIENT_ARENA;
  Assert(ReactiveSize.RowCount == ImguiRows.Size());
  
  imgui_button_color ButtonColorEven = {};
  ButtonColorEven.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "sap green");
  ButtonColorEven.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "persian indigo");
  ButtonColorEven.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "egyptian blue");
  ButtonColorEven.HotColor =  menu::GetColor(&GlobalState->ColorTable, "rich black");

  imgui_button_color ButtonColorOdd = {};
  ButtonColorOdd.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "fern green");
  ButtonColorOdd.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "persian indigo");
  ButtonColorOdd.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "egyptian blue");
  ButtonColorOdd.HotColor =  menu::GetColor(&GlobalState->ColorTable, "rich black");

  v4 EvenColorBack = V4(0.3,0.3,0.3,1);
  v4 OddColorBack = V4(0.4,0.4,0.4,1);
  for (int i = 0; i < ImguiRows.Size(); ++i)
  {
    const imgui_row& ImguiRow = ImguiRows[i];
    const reactive_row_size& ReactiveRow = ReactiveSize.ReactiveRowSizes[i];
    
    r32 t = (r32) (i+1) / (r32) ReactiveSize.RowCount;
    

    rect2f RowRect = ReactiveRow.RowRect;
    RowRect.X += ClipRect.X;
    RowRect.Y += ClipRect.Y + ClipRect.H - ReactiveSize.TotalSize.Y;
    RowRect.W = ClipRect.W;
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(RowRect), i % 2 == 0 ? EvenColorBack : OddColorBack);
    u32 DivIndex = 0;

    const imgui_row::header* Header = ImguiRow.m_head;

    r32 X0 = 0;
    r32 Y0 = 0;
    while(Header)
    {
      rect2f DivRect = ReactiveRow.DivRects[DivIndex];

      DivRect.X += ClipRect.X;
      DivRect.Y += ClipRect.Y + ClipRect.H - ReactiveSize.TotalSize.Y;

      switch(Header->Type)
      {
        case imgui_row::type::PADDING: {
          imgui_row::padding* Padding = (imgui_row::padding*) Header->Data;
          rect2f PadRect = Rect2f(X0 + DivRect.X, Y0 + DivRect.Y, Header->Size.X, Header->Size.Y);
          X0 += Header->Size.X;
        }break;
        case imgui_row::type::ICON: {
          imgui_row::icon* Icon = (imgui_row::icon*) Header->Data;
          rect2f IconRect = Rect2f(X0 + DivRect.X, Y0 + DivRect.Y, Header->Size.X, Header->Size.Y);
          v4 TexCoords = GlobalImguiContext->Icons.Coordinates[Icon->IconType];
          ImguiPlainButton(GlobalImguiContext, Header->ImguiID, IconRect, i % 2 == 0 ? ButtonColorEven : ButtonColorOdd);
          render::DrawIconCanonicalSpace(CenteredRect(IconRect), TexCoords, Icon->Color);
          X0 += Header->Size.X;
        }break;
        case imgui_row::type::TEXT: {
          imgui_row::text* Text = (imgui_row::text*) Header->Data;
          r32 DescentOffset = Text->Font->GetCanonicalFontDescenOffset(Text->FontSize);
          r32 LineHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(Text->FontSize);

          r32 DiffY = 0.5 * (DivRect.H - LineHeight);

          render::DrawTextCanonicalSpace(V2(X0 + DivRect.X,  Y0 + DivRect.Y + DescentOffset + DiffY), Text->FontSize, (const utf8_byte*) Text->Text, Text->Color);
          X0 += Header->Size.X;
        }break;
        case imgui_row::type::DIV_HINT: {
          imgui_row::div_hint* DivHint = ReactiveRow.Divs[DivIndex];
          Assert(DivHint->Header == Header);
          DivIndex++;
          X0 = 0;
          Y0 = 0;
        }break;
      }

      Header = Header->Next;
    }
  }
}
