#include "imgui_row.h"
namespace imgui {
file_local size_t TypeToSize(imgui_row::type Type) {
  switch(Type) {
    case imgui_row::type::PADDING: {
      return sizeof(imgui_row::padding);
    }break;
    case imgui_row::type::ICON: {
      return sizeof(imgui_row::icon);
    }break;
    case imgui_row::type::TEXT: {
      return sizeof(imgui_row::text);
    }break;
    case imgui_row::type::DIV_HINT: {
      return sizeof(imgui_row::div_hint);
    }break;
  }
  INVALID_CODE_PATH
  return 0;
}

// Sums the sizes of all elements up untill the next DivHint __or__ untill last element.
file_local v2 CalculateDivSize(imgui_row::header* H) {
  v2 DivSize = {};
  while(H && H->Type != imgui_row::type::DIV_HINT){
    DivSize.X += H->Size.X;
    DivSize.Y  = Maximum(DivSize.Y, H->Size.Y);
    H = H->Next;
  }
  return DivSize;
}

imgui_row::padding imgui_row::Padding(r32 Width, r32 Height){
  padding Result = {};
  Result.Width = Width;
  Result.Height = Height;
  return Result;
}

imgui_row::icon imgui_row::Icon(r32 Size, u32 IconType){
  icon Result = {};
  Result.Size = Size;
  Result.IconType = IconType;
  Result.Color = V4(1,1,1,1);
  return Result;
}

imgui_row::text imgui_row::Text(r32 FontSize, render::font* Font, size_t TextLen, const char* Text){
  text Result = {};
  Result.TextLen = TextLen;
  Result.Text = Text;
  Result.FontSize = FontSize;
  Result.Font = Font;
  Result.Color = V4(1,1,1,1);
  return Result;
}

imgui_row::div_hint imgui_row::DivHint(r32 Padding){
  div_hint Result = {};
  Result.Padding = Padding;
  return Result;
}

void imgui_row::Push(imgui_row::header* Header)
{
  if(!m_head)
  {
    m_head = Header;
    m_tail = Header;
  }else{
    m_tail->Next = Header;
    m_tail = Header;
  }
}

void imgui_row::Push(imgui_row::padding Padding){
  header* Header = (header*) PushStruct(GlobalTransientArena, header);
  Header->Type = type::PADDING;
  Header->Size = V2(Padding.Width, Padding.Height);
  Header->ImguiID = {};
  padding* Tmp = PushStruct(GlobalTransientArena, padding);
  *Tmp = Padding;
  Header->Data = (void*) Tmp;
  Push(Header);
}

void imgui_row::Push(imgui_row::icon Icon, id ImguiID){
  header* Header = (header*) PushStruct(GlobalTransientArena, header);
  Header->Type = type::ICON;
  Header->Size = PixelToCanonicalSpace(V2(Icon.Size,Icon.Size));
  Header->ImguiID = ImguiID;

  icon* Tmp = PushStruct(GlobalTransientArena, icon);
  *Tmp = Icon;
  Header->Data = (void*) Tmp;
  Push(Header);
}

void imgui_row::Push(imgui_row::text Text, id ImguiID) {
  header* Header = (header*) PushStruct(GlobalTransientArena, header);
  Header->Type = type::TEXT;
  Header->Size = GlobalRenderer->Font.GetTextSizeCanonicalSpace(Text.FontSize, (utf8_byte*) Text.Text);
  Header->ImguiID = ImguiID;
  text* Tmp = PushStruct(GlobalTransientArena, text);
  Tmp->TextLen = Text.TextLen;
  Tmp->FontSize = Text.FontSize;
  Tmp->Text = (char*) PushCopy(GlobalTransientArena, Text.TextLen, (void*) Text.Text);
  Tmp->Font = Text.Font;
  Tmp->Color = Text.Color;
  Header->Data = (void*) Tmp;
  Push(Header);
}

void imgui_row::Push(div_hint D){
  header* Header = (header*) PushStruct(GlobalTransientArena, header);
  Header->Type = type::DIV_HINT;
  Header->ImguiID = {};
  div_hint* DivHint = PushStruct(GlobalTransientArena, div_hint);
  DivHint->Header = Header;
  DivHint->Padding = D.Padding;

  if(!m_divTail)
  {
    Assert(!m_divHead);
    m_divHead = DivHint;
    m_divTail = DivHint;
    Assert(m_head);
    Header->Size = CalculateDivSize(m_head);
  }else{
    header* H = m_divTail->Header->Next;
    Assert(!m_divTail->Next);
    m_divTail->Next = DivHint;
    m_divTail = DivHint;
    Header->Size = CalculateDivSize(H);
  }
  Header->Data = (void*) DivHint;
  Push(Header);
  m_divCount++;
}

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

file_local v2 GetListOffset(r32 ScrollAmmount, v2 ListSize, rect2f ClipRect) {

  v2 ScreenMid = CenterPoint(ClipRect);

  // Find the point P in the list such that P will be rendered in the cetner of ClipRect if clipRect is positioned at 0,0
  r32 TopLimit = ListSize.Y - ClipRect.H / 2; // ScrollAmmount == 0
  r32 BotLimit = ClipRect.H / 2; // ScrollAmmount == 1
  v2 ContentMid = V2(ListSize.X*0.5f, Lerp(ScrollAmmount, TopLimit, BotLimit));

  v2 Diff = ScreenMid - ContentMid;

  return Diff;
}

void DrawRowList(const reactive_size& ReactiveSize, cmn::vector<imgui_row>& ImguiRows, rect2f ClipRect, r32 ScrollAmmount)
{
  SCOPED_TRANSIENT_ARENA;
  Assert(ReactiveSize.RowCount == ImguiRows.Size());

  v4 EvenColorBack = V4(0.3,0.3,0.3,1);
  v4 OddColorBack = V4(0.4,0.4,0.4,1);
  
  v2 ListSize = ReactiveSize.TotalSize;
  v2 ListOffset = GetListOffset(ScrollAmmount, ListSize, ClipRect);

  r32 StartX = ClipRect.X;
  r32 StartY = ListOffset.Y + ListSize.Y;
  if(StartY < Top(ClipRect))
  {
    StartY = Top(ClipRect);
  }
  r32 EndY   = ListOffset.Y;

  for (int i = 0; i < ImguiRows.Size(); ++i)
  {
    const imgui_row& ImguiRow = ImguiRows[i];
    const reactive_row_size& ReactiveRow = ReactiveSize.ReactiveRowSizes[i];
    
    rect2f RowRect = ReactiveRow.RowRect;
    RowRect.X += StartX;
    RowRect.Y += StartY - ListSize.Y;
    RowRect.W = ClipRect.W;

    if( (Bot(RowRect) > Top(ClipRect)) ||
        (Top(RowRect) < Bot(ClipRect)))
    {
      continue;
    }
    
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(Clip(RowRect, ClipRect)), i % 2 == 0 ? EvenColorBack : OddColorBack);

    u32 DivIndex = 0;
    const imgui_row::header* Header = ImguiRow.m_head;
    r32 X0 = 0;
    while(Header)
    {
      rect2f DivRect = ReactiveRow.DivRects[DivIndex];

      DivRect.X += StartX;
      DivRect.Y += StartY - ListSize.Y;
 
      rect2f ClipRectForDiv = Clip(DivRect, ClipRect);

      switch(Header->Type)
      {
        case imgui_row::type::PADDING: {
          imgui_row::padding* Padding = (imgui_row::padding*) Header->Data;
          rect2f PadRect = Rect2f(X0 + DivRect.X, DivRect.Y, Header->Size.X, Header->Size.Y);
          X0 += Header->Size.X;
        }break;
        case imgui_row::type::ICON: {
          imgui_row::icon* Icon = (imgui_row::icon*) Header->Data;
          rect2f IconRect = Rect2f(X0 + DivRect.X, DivRect.Y, Header->Size.X, Header->Size.Y);
          v4 TexCoords = GlobalImguiContext->Icons.Coordinates[Icon->IconType];
          ImguiButton(GlobalImguiContext, Header->ImguiID, IconRect);
          render::DrawIconCanonicalSpace2(IconRect, ClipRectForDiv, TexCoords, Icon->Color);
          X0 += Header->Size.X;
        }break;
        case imgui_row::type::TEXT: {
          imgui_row::text* Text = (imgui_row::text*) Header->Data;
          r32 DescentOffset = Text->Font->GetCanonicalFontDescenOffset(Text->FontSize);
          r32 LineHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(Text->FontSize);
          r32 DiffY = 0.5 * (DivRect.H - LineHeight);
          //ImguiTextButton(Header->ImguiID, 16, (c8*) Text->Text, X0 + DivRect.X, DivRect.Y + DescentOffset + DiffY, Header->Size.X, Header->Size.Y, 0, 0, -4, -2);
          render::DrawTextCanonicalSpace(V2(X0 + DivRect.X,  DivRect.Y + DescentOffset + DiffY), ClipRectForDiv, Text->FontSize, (const utf8_byte*) Text->Text, Text->Color);
          X0 += Header->Size.X;
        }break;
        case imgui_row::type::DIV_HINT: {
          imgui_row::div_hint* DivHint = ReactiveRow.Divs[DivIndex];
          Assert(DivHint->Header == Header);
          DivIndex++;
          X0 = 0;
        }break;
      }
      Header = Header->Next;
    }

    if(RowRect.Y < EndY){
      break;
    }
  }

  if(ReactiveSize.TotalSize.Y < ClipRect.H){
    rect2f RemainingRect = Rect2f(ClipRect.X, ClipRect.Y, ClipRect.W, ClipRect.H - ReactiveSize.TotalSize.Y);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(RemainingRect), ImguiRows.Size() % 2 == 0 ? EvenColorBack : OddColorBack);
  }

}

} // namespace imgui