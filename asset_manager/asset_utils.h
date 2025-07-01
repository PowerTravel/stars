#pragma once;
#include "asset_manager.h"
#include "renderer/render_push_buffer/application_render_push_buffer.h"
#include "../utils.h"

extern memory_arena* GlobalTransientArena;

void SendAssetToGpu(render_group* RenderGroup, asset::type Type, u32 Key) {

  opengl_buffer_data Data =  MapObjToOpenGLMesh(GlobalTransientArena, Data);
  
}