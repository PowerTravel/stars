#pragma once

#include "asset_types.h"
#include "renderer/render_push_buffer/render_push_buffer.h"
#include "platform/jwin_platform_memory.h"

extern memory_arena* GlobalTransientArena;

namespace asset::mapper{

void MeshToGlVertexBuffer(memory_arena* Arena, const mesh * Mesh, gl_vertex_buffer* Result)
{
  Result->IndexCount  = Mesh->IndexCount;
  Result->Indeces     = (u32*) PushCopy(Arena, Mesh->IndexCount * sizeof(u32), Mesh->Indeces);
  Result->VertexCount = Mesh->VertexCount;
  Result->VertexData  = PushArray(Arena, Mesh->VertexCount, opengl_vertex);

  for (int i = 0; i < Result->VertexCount; ++i) {
    Result->VertexData[i].v  = Mesh->v[i];
    Result->VertexData[i].vn = Mesh->vn ? Mesh->vn[i] : V3(0,0,0);
    Result->VertexData[i].vt = Mesh->vt ? Mesh->vt[i] : V2(0,0);
  }
}

gl_vertex_buffer* MeshToGlVertexBuffer(memory_arena* Arena, const mesh * Mesh)
{
  gl_vertex_buffer* Result = PushStruct(Arena, gl_vertex_buffer);
  MeshToGlVertexBuffer(Arena, Mesh, Result);
  return Result;
}

opengl_buffer_data MeshToGlVertexBuffer(memory_arena* Arena, render_group * Group)
{
  opengl_buffer_data Result = {};
  Result.BufferCount = Group->ElementCount;
  Result.BufferData  = PushArray(Arena, Group->ElementCount, gl_vertex_buffer);
  for (int i = 0; i < Group->ElementCount; ++i)
  {
    MeshToGlVertexBuffer(Arena, Group->Elements[i].Mesh, Result.BufferData + i);
  }
  return Result;
}

}
