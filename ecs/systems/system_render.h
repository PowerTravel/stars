#pragma once

#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "containers/chunk_list.h"

namespace ecs {

namespace render {

  namespace data {
  enum class programs{
    PHONG,
    TRANSPARENT_PHONG,
    TRANSPARENT_COMPOSITION
  };

    struct font
    {
      int TextPixelSize;
      int OnedgeValue;
      float PixelDistanceScale;
      float FontRelativeScale;
      jfont::sdf_font Font;
      jfont::sdf_atlas FontAtlas;
    };

  }// namespace data


  struct system {
    memory_arena Arena;
    render_group* RenderGroup;
    chunk_list SolidObjects;
    chunk_list TransparentObjects;
    chunk_list OverlayText;
    data::font Font;
    u32 FontTextureHandle;
  };

  system* CreateRenderSystem(render_group* RenderGroup);
  void Draw(entity_manager* EntityManager, system* RenderSystem, m4 ProjectionMatrix, m4 ViewMatrix);
}
}
