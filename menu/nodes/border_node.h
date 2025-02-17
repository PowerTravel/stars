#pragma once

enum class border_type {
  LEFT,
  RIGHT,
  BOTTOM,
  TOP,
  SPLIT_VERTICAL,
  SPLIT_HORIZONTAL
};

struct border_leaf
{
  border_type Type;
  r32 Position;
  r32 Thickness;
  b32 Active;
};

