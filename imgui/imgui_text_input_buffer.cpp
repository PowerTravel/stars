#include "imgui_text_input_buffer.h"


namespace imgui { 

imgui_text_input_buffer ImguiNewTextInputBuffer(s32 InputLen, utf8_byte* InputBuffer)
{
  imgui_text_input_buffer Result = {};
  Result.Buffer = Utf8StringBuffer(InputLen, InputBuffer);
  Result.CaretPosition = 0;
  Result.CharCount = 0;
  return Result;
}

void PushString(imgui_text_input_buffer* TextInputBuffer, c8* String)
{
  utf8_byte* Utf8String = (utf8_byte*) String;
  u32 CharCount = utf8_GetCharCountOfString(Utf8String);
  AppendStringToBuffer(Utf8String, &TextInputBuffer->Buffer);
  TextInputBuffer->CaretPosition += CharCount;
  TextInputBuffer->CharCount += CharCount;
}

void ClearBuffer(imgui_text_input_buffer* TextInputBuffer)
{
  ClearBuffer(&TextInputBuffer->Buffer);
  TextInputBuffer->CaretPosition = 0;
  TextInputBuffer->SelectMode = false;
  TextInputBuffer->SelectionStart = 0;
  TextInputBuffer->CharCount = 0;
}

b32 ImguiTextDialog(imgui_text_input_buffer* TextInputBuffer, id DialogID, v2 DialogPos, v2 TextWidth, v4 BackgroundColor)
{
  rect2f DialogRect = Rect2f(DialogPos.X, DialogPos.Y, TextWidth.X, TextWidth.Y);
  b32 Result = ImguiSelectabeRegion(&GlobalState->ImguiContext, DialogID, DialogRect, GlobalInput);

  r32 FontSize = 14;
  s32 InputLen = 512;
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(DialogRect), BackgroundColor);
  render::DrawTextCanonicalSpace(V2(DialogPos.X, DialogPos.Y +DescentOffset), FontSize, TextInputBuffer->Buffer.Buffer, V4(1,1,1,1));

  if(Result)
  {
    if(TextInputBuffer->SelectMode)
    {
      s32 Start = Minimum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
      s32 End   = Maximum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);

      utf8_string_buffer TempBuffer1 = CreateTempStringBuffer(InputLen);
      CopySubstring(&TextInputBuffer->Buffer, &TempBuffer1, 0, Start);

      utf8_string_buffer TempBuffer2 = CreateTempStringBuffer(InputLen);
      CopySubstring(&TextInputBuffer->Buffer, &TempBuffer2, 0, End);

      v2 TextSizeSelectStart = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer1.Buffer);
      v2 TextSizeSelectEnd = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer2.Buffer);

      rect2f HighlightRect = Rect2f(DialogPos.X + TextSizeSelectStart.X, DialogPos.Y, TextSizeSelectEnd.X - TextSizeSelectStart.X, TextWidth.Y);
      render::DrawOverlayQuadCanonicalSpace(CenteredRect(HighlightRect), BackgroundColor * 1.2);
    }

    utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
    v2 TextSizeToCaret = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer.Buffer);
    r32 CaretWidth = PixelToCanonicalWidth(1);
    rect2f CaretBox = Rect2f(DialogPos.X + CaretWidth/2.f + TextSizeToCaret.X, DialogPos.Y, CaretWidth, GlobalRenderer->Font.GetLineSpacingCanonicalSpace(14));
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(CaretBox), V4(1,1,1,1));
  }

  return Result;
}


file_local void DrawNumber(c8* Number, r32 FontSize, rect2f Rect)
{
  v2 TextSize = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, (utf8_byte const *) Number);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
  v2 TextPos  = V2(Rect.X + (Rect.W - TextSize.X), Rect.Y);
  Rect.Y -= DescentOffset;
  render::DrawTextCanonicalSpace(TextPos, Rect, FontSize, (utf8_byte const *) Number, V4(1.0,1.0,1.0,1.0));
}

file_local void DrawNumber(r32 Number, r32 FontSize, rect2f Rect)
{
  c8 NumBuf[32] = {};
  jstr::Ftoa( Number, 2, 31, NumBuf);
  DrawNumber(NumBuf, FontSize, Rect);
}

file_local void ImguiRenderTextDialog(imgui_text_input_buffer* TextInputBuffer, id DialogID, v2 DialogPos, v2 DialogSize, v4 BackgroundColor)
{
  r32 FontSize = 14;
  s32 InputLen = 512;
  rect2f DialogRect = Rect2f(DialogPos, DialogSize);
  r32 DescentOffset = GlobalRenderer->Font.GetCanonicalFontDescenOffset(FontSize);
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(DialogRect), BackgroundColor);
  render::DrawTextCanonicalSpace(V2(DialogPos.X, DialogPos.Y +DescentOffset), FontSize, TextInputBuffer->Buffer.Buffer, V4(1,1,1,1));
  if(TextInputBuffer->SelectMode)
  {
    s32 Start = Minimum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
    s32 End   = Maximum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
    
    utf8_string_buffer TempBuffer1 = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer1, 0, Start);

    utf8_string_buffer TempBuffer2 = CreateTempStringBuffer(InputLen);
    CopySubstring(&TextInputBuffer->Buffer, &TempBuffer2, 0, End);

    v2 TextSizeSelectStart = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer1.Buffer);
    v2 TextSizeSelectEnd = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer2.Buffer);

    rect2f HighlightRect = Rect2f(DialogPos.X + TextSizeSelectStart.X, DialogPos.Y, TextSizeSelectEnd.X - TextSizeSelectStart.X, DialogSize.Y);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(HighlightRect), BackgroundColor * 1.2);
  }

  utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
  CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
  v2 TextSizeToCaret = GlobalRenderer->Font.GetTextSizeCanonicalSpace(FontSize, TempBuffer.Buffer);
  r32 CaretWidth = PixelToCanonicalWidth(1);
  rect2f CaretBox = Rect2f(DialogPos.X + CaretWidth/2.f + TextSizeToCaret.X, DialogPos.Y, CaretWidth, GlobalRenderer->Font.GetLineSpacingCanonicalSpace(14));
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(CaretBox), V4(1,1,1,1));  
}

file_local void DeleteSelection(imgui_text_input_buffer* TextInputBuffer){
  u32 InputLen = 512;
  utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
  u32 Start = Minimum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
  u32 End = Maximum(TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart);
  CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, End);
  EraseFromBuffer(&TempBuffer,End-Start);
  CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, End, TextInputBuffer->CharCount - End);
  ClearBuffer(&TextInputBuffer->Buffer);
  CopyBufferContent(&TempBuffer, &TextInputBuffer->Buffer);
  TextInputBuffer->CaretPosition = Start;
  TextInputBuffer->CharCount -= End-Start;
}

void ReadNumber(context* ImguiContext, id ID, imgui_text_input_buffer* TextInputBuffer, rect2f DialogRect, jwin::device_input* Input, float (*DataToFloat)(void* Data), void (*StoreFloat)(float Val, void* Data), void* Data) {
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

void ImguiReadInput(imgui_text_input_buffer* TextInputBuffer, id DialogID, jwin::device_input* Input, v2 TextPos, b32 HighlightAll)
{
  u32 InputLen = 512;
  r32 FontSize = 14;

  b32 ShiftDown = jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT);

  v2 MousePos = V2(Input->Mouse.X, Input->Mouse.Y);
  v2 MousePosRelText = MousePos - TextPos;
  if(jwin::Active(Input->Mouse.Button[jwin::MouseButton_Left]))
  {
    size_t CharCount = 0;
    GlobalRenderer->Font.GetCharsCountToFitCanonicalSpace(FontSize, MousePosRelText.X, TextInputBuffer->Buffer.Buffer, 0, &CharCount);
    
    if(HighlightAll)
    {
      if(jwin::Pushed(Input->Mouse.Button[jwin::MouseButton_Left]))
      {
        TextInputBuffer->SelectMode = true;
        TextInputBuffer->SelectionStart = 0;
        TextInputBuffer->CaretPosition = TextInputBuffer->CharCount;
      }
    }else{  
      if(jwin::Pushed(Input->Mouse.Button[jwin::MouseButton_Left]))
      {
        TextInputBuffer->SelectionStart = CharCount;
      }
      TextInputBuffer->CaretPosition = CharCount;
    }
    
    if(TextInputBuffer->SelectionStart != TextInputBuffer->CaretPosition)
    {
      TextInputBuffer->SelectMode = true;
    }
  }

  b32 KeyClicked = false;
  if(jwin::Pushed(Input->Keyboard.Key_BACK)){
    KeyClicked = true;
    if(TextInputBuffer->SelectMode)
    {        
      DeleteSelection(TextInputBuffer);
      TextInputBuffer->SelectMode = false;
    }else{
      if(TextInputBuffer->CaretPosition > 0)
      {
        if(TextInputBuffer->CaretPosition == TextInputBuffer->CharCount)
        {
          EraseFromBuffer(&TextInputBuffer->Buffer, 1);
        }else{
          utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
          CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
          EraseFromBuffer(&TempBuffer,1);
          CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, TextInputBuffer->CaretPosition, TextInputBuffer->CharCount - TextInputBuffer->CaretPosition);
          ClearBuffer(&TextInputBuffer->Buffer);
          CopyBufferContent(&TempBuffer, &TextInputBuffer->Buffer);
        }
        TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->CaretPosition-1, 0);
        TextInputBuffer->CharCount = Maximum(TextInputBuffer->CharCount-1, 0);;  
      }
    }

  }else if(jwin::Pushed(Input->Keyboard.Key_LEFT)){
    KeyClicked = true;

    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
      TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->CaretPosition-1, 0);
    }else{
      if(TextInputBuffer->SelectMode){
        TextInputBuffer->CaretPosition = Minimum(TextInputBuffer->SelectionStart, TextInputBuffer->CaretPosition);
        TextInputBuffer->SelectMode = false;
      }else{
        TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->CaretPosition-1, 0);
      }
    }
  }else if(jwin::Pushed(Input->Keyboard.Key_RIGHT)){
    KeyClicked = true;
    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
      TextInputBuffer->CaretPosition = Minimum(TextInputBuffer->CaretPosition+1, TextInputBuffer->CharCount);
    }else{
      if(TextInputBuffer->SelectMode){
        TextInputBuffer->CaretPosition = Maximum(TextInputBuffer->SelectionStart, TextInputBuffer->CaretPosition);
        TextInputBuffer->SelectMode = false;
      }else{
        TextInputBuffer->CaretPosition = Minimum(TextInputBuffer->CaretPosition+1, TextInputBuffer->CharCount);
      }
    }
  }else if(jwin::Pushed(Input->Keyboard.Key_END)){
    KeyClicked = true;
    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
    }else{
      TextInputBuffer->SelectMode = false;
    }
    TextInputBuffer->CaretPosition = TextInputBuffer->CharCount;
  }else if(jwin::Pushed(Input->Keyboard.Key_HOME)){
    KeyClicked = true;
    if(ShiftDown)
    {
      if(!TextInputBuffer->SelectMode)
      {
        TextInputBuffer->SelectionStart = TextInputBuffer->CaretPosition;
        TextInputBuffer->SelectMode = true;
      }
    }else{
      TextInputBuffer->SelectMode = false;
    }
    TextInputBuffer->CaretPosition = 0;
  }else{
    utf8_string_buffer CharBuffer = CreateTempStringBuffer(InputLen);
    if(PushInputToBuffer(&Input->Keyboard, &CharBuffer, ENGLISH))
    {
      KeyClicked = true;
      if(TextInputBuffer->SelectMode)
      {
        DeleteSelection(TextInputBuffer);
        TextInputBuffer->SelectMode = false;
      }
      u32 CharCount = utf8_GetCharCountOfString(CharBuffer.Buffer);
      if(TextInputBuffer->CaretPosition == TextInputBuffer->CharCount)
      {
        CopyBufferContent(&CharBuffer, &TextInputBuffer->Buffer);
      }else{
        utf8_string_buffer TempBuffer = CreateTempStringBuffer(InputLen);
        CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, 0, TextInputBuffer->CaretPosition);
        CopyBufferContent(&CharBuffer, &TempBuffer);
        CopySubstring(&TextInputBuffer->Buffer, &TempBuffer, TextInputBuffer->CaretPosition, TextInputBuffer->CharCount - TextInputBuffer->CaretPosition);
        ClearBuffer(&TextInputBuffer->Buffer);
        CopyBufferContent(&TempBuffer, &TextInputBuffer->Buffer);
      }
      TextInputBuffer->CaretPosition += CharCount;
      TextInputBuffer->CharCount += CharCount;
    }
  }
  if(KeyClicked)
  {
    Platform.DEBUGPrint("Len %d, Caret Pos %d, Selection Start %d Selection Mode %s\n",
      TextInputBuffer->CharCount, TextInputBuffer->CaretPosition, TextInputBuffer->SelectionStart, TextInputBuffer->SelectMode ? "'On'" : "'off'");
  }
}
} // namespace imgui