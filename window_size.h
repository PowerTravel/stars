#pragma once

#include "commons/macros.h"
#include "platform/jwin_platform.h"

struct window_size_pixel {
  r32 WindowWidth;
  r32 WindowHeight;
  r32 MonitorWidth;
  r32 MonitorHeight;
  r32 MonitorDPI;
  r32 EffectiveDPI;
  r32 ApplicationAspectRatio;
  r32 ApplicationWidth;
  r32 ApplicationHeight;
};


extern window_size_pixel GlobalWindowSize;

static window_size_pixel WindowSizePixel(application_render_commands* RenderCommands, r32 ApplicationWidth, r32 ApplicationHeight)
{
  window_size_pixel Result = {};
  Result.WindowWidth       = (r32) RenderCommands->WindowInfo.Width;
  Result.WindowHeight      = (r32) RenderCommands->WindowInfo.Height;
  Result.MonitorWidth      = (r32) RenderCommands->MonitorInfo.Width;
  Result.MonitorHeight     = (r32) RenderCommands->MonitorInfo.Height;
  Result.MonitorDPI        = (r32) RenderCommands->MonitorInfo.RawDPI;
  Result.EffectiveDPI      = (r32) RenderCommands->MonitorInfo.EffectiveDPI;
  Result.ApplicationWidth  = (r32) ApplicationWidth;
  Result.ApplicationHeight = (r32) ApplicationHeight;
  Result.ApplicationAspectRatio = Result.ApplicationWidth / Result.ApplicationHeight;
  return Result;
}

inline r32 PixelToCanonicalWidth(const r32 X)
{
  r32 Result = LinearRemap(X, 0, GlobalWindowSize.ApplicationWidth,  0, GlobalWindowSize.ApplicationAspectRatio);
  return Result;
}

inline r32 PixelToCanonicalHeight(const r32 Y)
{
  r32 Result = LinearRemap(Y, 0, GlobalWindowSize.ApplicationHeight, 0, 1);
  return Result;
}

inline v2 PixelToCanonicalSpace(const v2& PixelPos)
{
  v2 CanonicalPos = V2( PixelToCanonicalWidth(PixelPos.X),
                        PixelToCanonicalHeight(PixelPos.Y));
  return CanonicalPos;
}

inline r32 CanonicalToPixelWidth(const r32 X)
{
  r32 Result = LinearRemap(X, 0, GlobalWindowSize.ApplicationAspectRatio,  0, GlobalWindowSize.ApplicationWidth);
  return Result;
}

inline r32 CanonicalToPixelHeight(const r32 Y)
{
  r32 Result = LinearRemap(Y, 0, 1, 0, GlobalWindowSize.ApplicationHeight);
  return Result;
}

inline v2 CanonicalToPixelSpace(const v2& CanPos)
{
  v2 PixelPos = V2( CanonicalToPixelWidth(CanPos.X),
                    CanonicalToPixelHeight(CanPos.Y));
  return PixelPos;
}

inline rect2f CanonicalToPixelSpace(const rect2f& CanRect)
{
  rect2f PixelRect = Rect2f(CanonicalToPixelSpace(V2(CanRect.X, CanRect.Y)),
                        CanonicalToPixelSpace(V2(CanRect.W, CanRect.H)));
  return PixelRect;
}