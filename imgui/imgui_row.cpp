#include "imgui_row.h"

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
v2 CalculateDivSize(imgui_row::header* H) {
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

void imgui_row::Push(imgui_row::icon Icon, imgui_id ImguiID){
  header* Header = (header*) PushStruct(GlobalTransientArena, header);
  Header->Type = type::ICON;
  Header->Size = PixelToCanonicalSpace(V2(Icon.Size,Icon.Size));
  Header->ImguiID = ImguiID;

  icon* Tmp = PushStruct(GlobalTransientArena, icon);
  *Tmp = Icon;
  Header->Data = (void*) Tmp;
  Push(Header);
}

void imgui_row::Push(imgui_row::text Text, imgui_id ImguiID) {
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

imgui_row ImguiRow(){
  imgui_row Result = {};
  return Result;
};
