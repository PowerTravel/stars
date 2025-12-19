#include "menu.h"
#include "platform/jwin_platform_memory.h"
#include "entity_list.h"
#include "color_list.h"

namespace imgui {
namespace app {
menu CreateAppllicationMenu(memory_arena* Arena, context* ImguiContext, u32 ColorCount) {
  menu Result = {};

  Result.ColorList = CreateColorList(Arena, ColorCount);
  Result.EntityList = CreateEntityList(Arena);

  return Result;
}

} // namespace app
} // namespace imgui

