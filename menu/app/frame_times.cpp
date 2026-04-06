#include "frame_times.h"

namespace imgui { 
namespace app {


#if JWIN_PROFILE

static void DrawText(v2 Pos, const char* Text, r32 FontSize, b32 Background){
  u32 DummyFontEnum = 0;
  r32 LineSpacing = render::GetLineSpacing(DummyFontEnum, FontSize);
  if(Background) {
    r32 TextWidth = render::GetTextSize(DummyFontEnum, FontSize, Text).X;
    v2 TextSize =V2(TextWidth, LineSpacing);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(Rect2f(Pos, TextSize)), V4(0.3,0.3,0.3,1));
  }
  v2 TextPos = Pos + V2(0, render::GetDescenOffset(DummyFontEnum, FontSize));
  render::DrawTextCanonicalSpace(TextPos, FontSize, (utf8_byte const *) Text, V4(0.9,0.9,0.9,1));
}

static b32 MouseOver(rect2f Rect, v2* MousePosResult = 0){
  v2 MousePos = V2(GlobalInput->Mouse.X, GlobalInput->Mouse.Y);
  r32 Result = Intersects(Rect, MousePos);
  if(MousePosResult){
    *MousePosResult = MousePos;
  }
  return Result;
}

void DrawThread(debug_thread* Thread, u64 FrameLength, rect2f Rect){
  debug_block* Block = Thread->FirstBlock;
  v2 MousePos = {};
  debug_block* HotBlock = 0;
  while(Block) {
    r32 X0 = LinearRemap(Block->BeginClock, 0, FrameLength, Left(Rect), Right(Rect));
    r32 X1 = LinearRemap(Block->EndClock,   0, FrameLength, Left(Rect), Right(Rect));
    r32 T = Unlerp(X0, Left(Rect), Right(Rect));
    rect2f OverlayRect = Rect2f(X0, Rect.Y, X1 - X0, Rect.H);
    if(MouseOver(OverlayRect, &MousePos)){
       HotBlock = Block;
    }
    v2 PixelSize = PixelToCanonicalSpace(V2(1,1));

    render::DrawOverlayQuadCanonicalSpace(CenteredRect(OverlayRect), V4(1,T,1,1));
    //Platform.DEBUGPrint("%1.2f, %1.2f, %1.2f, %1.2f\n", OverlayRect.X, OverlayRect.Y, OverlayRect.W,OverlayRect.H);
    //Platform.DEBUGPrint("%1.2f, %1.2f\n", Block->BeginClock-Start, End-Block->EndClock);
    Block = Block->Next;
  }
  if(HotBlock)
  {
    DrawText(MousePos, HotBlock->Record->BlockName, 16, true);
    if(jwin::Pushed(GlobalInput->Mouse.Button[jwin::MouseButton_Left])) {
      Thread->SelectedBlock = HotBlock;
    }
  }else{
    if(jwin::Pushed(GlobalInput->Mouse.Button[jwin::MouseButton_Left])) {
      Thread->SelectedBlock = 0;
    }
  }
  Platform.DEBUGPrint("\n");
}

void DrawFrame(debug_frame* SelectedFrame){
  const r32 Width = 1.0;
  const r32 Height = 0.4;
  const r32 X0 = 0.6;
  const r32 Y0 = 0.5;

  const u64 FrameStart  = SelectedFrame->BeginClock;
  const u64 FrameEnd    = SelectedFrame->EndClock;
  const u64 FrameLength = FrameEnd-FrameStart;

  const rect2f FrameRect = Rect2f(X0, Y0, Width, Height);

//  Platform.DEBUGPrint("Frame      %1.2lld - %1.2lld = %1.2lld\n", FrameEnd, FrameStart, FrameLength);
/*
  const u64 BlockStart  = SelectedBlock->BeginClock;
  const u64 BlockEnd    = SelectedBlock->EndClock;
  const u64 BlockLength = BlockEnd-BlockStart;
  Platform.DEBUGPrint("Block      %1.2lld - %1.2lld = %1.2lld\n", BlockEnd, BlockStart, BlockLength);

  r32 BlockStartPercentage  = Unlerp(BlockStart,  0, FrameLength);
  r32 BlockEndPercentage    = Unlerp(BlockEnd,    0, FrameLength);
  r32 BlockLengthPercentage = Unlerp(BlockLength, 0, FrameLength);
  Platform.DEBUGPrint("Percentage %1.2f - %1.2f = %1.2f\n", BlockEndPercentage, BlockStartPercentage, BlockLengthPercentage);
*/

  v2 MousePos = {};
  u32 TotFrames = SelectedFrame->Threads.Reserved();
  r32 FrameLaneHeight = Height / TotFrames;
  r32 Y = 0;
  render::DrawOverlayQuadCanonicalSpace(CenteredRect(FrameRect), V4(1,0,0,1));
  for (int i = 0; i < TotFrames; ++i)
  {
    debug_thread* Thread = &SelectedFrame->Threads[i];
    u32 TotThreads = SelectedFrame->Threads.Reserved();
    rect2f ThreadRect = Split(FrameRect, i, 0, TotThreads, 1);
    render::DrawOverlayQuadCanonicalSpace(CenteredRect(ThreadRect), V4(0,1,0,1));
    const u64 FrameStart  = SelectedFrame->BeginClock;
    const u64 FrameEnd    = SelectedFrame->EndClock;
    const u64 FrameLength = FrameEnd-FrameStart;

    
    DrawThread(Thread, FrameLength, FrameRect);
  }
}

rect2f InterpolateRect(r32 t, rect2f StartRect, rect2f EndRect){
  r32 X = Lerp(t, StartRect.X, EndRect.X);
  r32 Y = Lerp(t, StartRect.Y, EndRect.Y);
  r32 W = Lerp(t, StartRect.W, EndRect.W);
  r32 H = Lerp(t, StartRect.H, EndRect.H);

  rect2f Result = Rect2f(X,Y,W,H);
  return Result;
}

v4 InterpolateColor(r32 t, v4 StartColor, v4 EndColor){
  r32 R = Lerp(t, StartColor.X, EndColor.X);
  r32 G = Lerp(t, StartColor.Y, EndColor.Y);
  r32 B = Lerp(t, StartColor.Z, EndColor.Z);
  r32 A = Lerp(t, StartColor.W, EndColor.W);

  v4 Result = V4(R,G,B,A);
  return Result;
}


void DrawFunctionLanes(rect2f MenuRegion)
{
  const r32 BotEdge = Bot(MenuRegion);
  const r32 TopEdge = Top(MenuRegion);
  const r32 LeftEdge = Left(MenuRegion);
  const r32 RightEdge = Right(MenuRegion);
  const r32 LaneWidth = (RightEdge - LeftEdge) / (r32) MAX_DEBUG_FRAME_COUNT;
  const r32 TimeBot = 0;
  const r32 TimeTop = 1/60.0;

  static r32 tp = 0;
  tp += GlobalInput->deltaTime;
  if(tp > Tau32){
    tp -= Tau32;
  }
  r32 ts = 0.5 * (Sin(tp) + 1);

  render::DrawOverlayQuadCanonicalSpace(CenteredRect(Rect2f(LeftEdge, BotEdge, RightEdge-LeftEdge, TopEdge-BotEdge)), V4(0,0,0,1));
  if(GlobalDebugState)
  {
    v2 MousePos = V2(GlobalInput->Mouse.X, GlobalInput->Mouse.Y);
    debug_state* DebugState = GlobalDebugState;

    r32 X = LeftEdge;

    size_t StartFrame = DebugState->CurrentFrameIndex+1;
    if(StartFrame >= MAX_DEBUG_FRAME_COUNT){
      StartFrame = 0;
    }

    debug_frame* HotFrame = 0;
    debug_block* HotBlock = 0;
    v2 PixelSize = PixelToCanonicalSpace(V2(1,1));
    size_t FrameIndex = 0;
    r32 MaxTime = 0;
    while (FrameIndex < MAX_DEBUG_FRAME_COUNT)
    {
      if(FrameIndex != DebugState->CurrentFrameIndex)
      {
        debug_frame* Frame = &DebugState->Frames[FrameIndex];
        r32 T = 0;
        debug_thread* Thread = &Frame->Threads[0];
        debug_block* Block = Thread->FirstBlock;
        while(Block) {
          size_t ClockCount = Block->EndClock - Block->BeginClock;
          r64 WallCount = (ClockCount) / (GlobalDebugTable->PerfCounterFrequency*8);
          if(WallCount > MaxTime){
            MaxTime = WallCount;
          }
          r32 dt = WallCount;
          r32 Bot = Lerp(T, BotEdge, TopEdge);
          T+=dt;


          r32 FrameTop = Lerp(T, BotEdge, TopEdge);
          r32 MaxTop = Top(MenuRegion);
          r32 TopP = Minimum(FrameTop, MaxTop);

          rect2f OverlayRect = Rect2f(X, Bot, LaneWidth, TopP - Bot);
          if(GlobalDebugState->SelectedFrame && GlobalDebugState->SelectedFrame == Frame){
            render::DrawOverlayQuadCanonicalSpace(Shrink(CenteredRect(OverlayRect),-PixelSize*0.5), V4(1,1,0,1));
          }
          render::DrawOverlayQuadCanonicalSpace(Shrink(CenteredRect(OverlayRect),PixelSize*0.5), V4(1,T,1,1));

          if(MouseOver(OverlayRect))
          {
            HotFrame = Frame;
            HotBlock = Block;
          }

          Block = Block->Next;
        }
      }
      X+=LaneWidth;
      FrameIndex++;
    }

    if(HotBlock){
      DrawText(MousePos, HotBlock->Record->BlockName, 16, true);
    }

    if(HotFrame) {
      if(jwin::Pushed(GlobalInput->Mouse.Button[jwin::MouseButton_Left])) {
        GlobalDebugState->SelectedFrame = HotFrame;
      }
    }else{
      if(jwin::Pushed(GlobalInput->Mouse.Button[jwin::MouseButton_Left])) {
        GlobalDebugState->SelectedFrame = 0;
      }
    }

    if(GlobalDebugState->SelectedFrame) {
      //DrawText(MousePos, HotBlock->Record->BlockName, 16, true);
      DrawFrame(GlobalDebugState->SelectedFrame);
      GlobalDebugState->Paused = true;
    }else{
      GlobalDebugState->Paused = false;
    }
  }
}
#else
void DrawFunctionLanes(rect2f MenuRegion){

}
#endif

frame_times* CreateFrameTimes(memory_arena* Arena, debug_state* DebugState){
  frame_times* Result = PushStruct(Arena, frame_times);
  r32 RowHeight = GlobalRenderer->Font.GetLineSpacingCanonicalSpace(GlobalImguiContext->FontSize);
  Result->BorderWindow      = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.3,0.5)), PixelToCanonicalSpace(V2(3,3)), RowHeight);
  return Result;
}

void DrawFrameTimes(menu* AppImgui){
  DoImguiBorderWindow(&AppImgui->FrameTimes->BorderWindow, GlobalState->ApplicationMenu.EnclosingRegion, "FrameTimes");
  rect2f ContentRect = GetContentRect(&AppImgui->FrameTimes->BorderWindow);
  DrawFunctionLanes(ContentRect);
}

} //namespace app
} // namespace imgui