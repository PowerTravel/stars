#pragma once

#include "asset_types.h"
#include "renderer/render_push_buffer/render_push_buffer.h"
#include "platform/jwin_platform_memory.h"

extern memory_arena* GlobalTransientArena;

struct gl_vertex_buffer
{
  u32 IndexCount;
  u32* Indeces;
  u32 VertexCount;
  opengl_vertex* VertexData;
};


struct opengl_buffer_data{
  u32 BufferCount;
  gl_vertex_buffer* BufferData;
};
   
namespace asset {
namespace mapper{

gl_vertex_buffer CreateGLVertexBuffer2(memory_arena* Arena,
                     const int IndexCount, const int* Indeces, const int VertexCount,
                     const v3* VerticeData, const v3* NormalData, const v2* TextureData)
{
  int* GLIndexArray         = PushArray(Arena, IndexCount, int);
  opengl_vertex* VertexData = PushArray(Arena, VertexCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  utils::Copy(IndexCount*sizeof(int), (void*) Indeces, (void*)GLIndexArray);

  for (int i = 0; i < VertexCount; ++i)
  {
    opengl_vertex* GlVertice = &VertexData[i];
    Vertice->v  = VerticeData[i];
    Vertice->vt = TextureData ? TextureData[i] : V2(0,0);
    Vertice->vn = NormalData  ? NormalData[i]  : V3(0,0,0);
    ++Vertice;
  }
  
  gl_vertex_buffer Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces =  (u32*) GLIndexArray;
  Result.VertexCount = VertexCount;
  Result.VertexData = VertexData;
  return Result;
}

static void PrimitiveToGlVertexBuffer(memory_arena* Arena, const asset::gltf_tmp::mesh::primitive * Primitive, gl_vertex_buffer* Result)
{
  Assert(Primitive->IndexCount && Primitive->Indeces && Primitive->VertexCount && Primitive->Vertex);
  // We are only handling 1 set of texture vertices atm. Increase if we find the need
  Assert(Primitive->TextureVertexSetCount== 0 || Primitive->TextureVertexSetCount ==1);
  *Result = CreateGLVertexBuffer2(
      Arena,
      Primitive->IndexCount,
      Primitive->Indeces,
      Primitive->VertexCount,
      Primitive->Vertex,
      Primitive->VertexNormal,
      Primitive->TextureVertices[0]
    );
}

opengl_buffer_data MeshToGlVertexBuffer(memory_arena* Arena, const asset::gltf_tmp::mesh * Mesh)
{ 
  opengl_buffer_data Result = {};
  Assert(Mesh->PrimitiveCount == 1); // Deal wiht several primitives per mesh when we run into them.
  Result.BufferCount = Mesh->PrimitiveCount;
  Result.BufferData  = PushArray(Arena, Result.BufferCount, gl_vertex_buffer);
  for (int i = 0; i < Result.BufferCount; ++i)
  {
    PrimitiveToGlVertexBuffer(Arena, &Mesh->Primitives[i], &Result.BufferData[i]);
  }

  return Result;
}


}
}