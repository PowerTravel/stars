#pragma once

#include "renderer/render_push_buffer/application_render_push_buffer.h"
#include "platform/obj_loader.h"

u32 Push32BitColorTexture(render_group* RenderGroup,  obj_bitmap* BitMap)
{
  texture_params Params = DefaultColorTextureParams();
  Params.TextureFormat = texture_format::RGBA_U8;
  Params.InputDataType = OPEN_GL_UNSIGNED_BYTE;
  u32 Result = PushNewTexture(RenderGroup, BitMap->Width, BitMap->Height, Params, BitMap->Pixels);
  return Result;
}