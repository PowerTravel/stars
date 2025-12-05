#pragma once

#include "asset_manager/asset_types.h"

namespace ecs { 
namespace controller {

enum class type {
  KEYBOARD_MOUSE,
  GAMEPAD
};

struct component
{
  int a;
};

} // namespace controller
} // namespace ecs
