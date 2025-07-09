#pragma once

#include "asset_types.h"
#include "renderer/render_push_buffer/render_push_buffer.h"
#include "platform/jwin_platform_memory.h"

extern memory_arena* GlobalTransientArena;

namespace asset::mapper{


u32 PushUnique( u8* Array, const u32 ElementCount, const u32 ElementByteSize,
               u8* NewElement, b32 (*CompareFunction)(const u8* DataA, const u8* DataB))
{
  for( u32 i = 0; i < ElementCount; ++i )
  {
    if( CompareFunction(NewElement, Array) )
    {
      return i;
    }
    Array += ElementByteSize;
  }
  
  // If we didn't find the element we push it to the end
  utils::Copy(ElementByteSize, NewElement, Array);
  
  return ElementCount;
}


gl_vertex_buffer CreateGLVertexBuffer(memory_arena* Arena,
                     const u32  IndexCount,
                     const u32* VerticeIndeces, const u32* NormalIndeces, const u32* TextureIndeces,
                     const v3*  VerticeData,    const v3*  NormalData,    const v2*  TextureData)
{
  u32* GLVerticeIndexArray  = PushArray(Arena, 3*IndexCount, u32);
  u32* GLIndexArray         = PushArray(Arena, IndexCount, u32);
  
  u32 VerticeArrayCount = 0;
  for( u32 i = 0; i < IndexCount; ++i )
  {
    const u32 vidx = VerticeIndeces[i];
    const u32 tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const u32 nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    u32 NewElement[3] = {vidx, tidx, nidx};
    u32 Index = PushUnique((u8*)GLVerticeIndexArray, VerticeArrayCount, sizeof(NewElement), (u8*) NewElement,
                           [](const u8* DataA, const u8* DataB) {
                             u32* U32A = (u32*) DataA;
                             const u32 A1 = *(U32A+0);
                             const u32 A2 = *(U32A+1);
                             const u32 A3 = *(U32A+2);
                             u32* U32B = (u32*) DataB;
                             const u32 B1 = *(U32B+0);
                             const u32 B2 = *(U32B+1);
                             const u32 B3 = *(U32B+2);
                             b32 result = (A1 == B1) && (A2 == B2) && (A3 == B3);
                             return result;
                           });
    if(Index == VerticeArrayCount)
    {
      VerticeArrayCount++;
    }
    
    GLIndexArray[i] = Index;
  }
  
  opengl_vertex* VertexData = PushArray(Arena, VerticeArrayCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  for( u32 i = 0; i < VerticeArrayCount; ++i )
  {
    const u32 vidx = *(GLVerticeIndexArray + 3 * i + 0);
    const u32 tidx = *(GLVerticeIndexArray + 3 * i + 1);
    const u32 nidx = *(GLVerticeIndexArray + 3 * i + 2);
    Vertice->v  = VerticeData[vidx];
    Vertice->vt = TextureData ? TextureData[tidx] : V2(0,0);
    Vertice->vn = NormalData  ? NormalData[nidx]  : V3(0,0,0);
    ++Vertice;
  }
  
  gl_vertex_buffer Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = GLIndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.VertexData = VertexData;
  return Result;
}

void MeshToGlVertexBuffer(memory_arena* Arena, const mesh * Mesh, gl_vertex_buffer* Result)
{
  Assert(Mesh->IndexCount && Mesh->vi && Mesh->vCount && Mesh->v);
  *Result = CreateGLVertexBuffer(
      Arena,
      Mesh->IndexCount,
      Mesh->vi,
      Mesh->vni,
      Mesh->vti,
      Mesh->v,
      Mesh->vn,
      Mesh->vt
    );
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
