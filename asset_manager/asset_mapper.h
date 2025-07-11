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


struct tracker_element {
  u32 ArrayIndex;
  u32 VerticeIndex;
  u32 TextureIndex;
  u32 NormalIndex;
};

tracker_element NewTrackerElement(u32 ArrayIndex, u32 VerticeIndex, u32 TextureIndex, u32 NormalIndex)
{
  tracker_element Result = {};
  Result.ArrayIndex = ArrayIndex;
  Result.VerticeIndex = VerticeIndex;
  Result.TextureIndex = TextureIndex;
  Result.NormalIndex = NormalIndex;
  return Result;
}
int GetCantorPair2(int a, int b)
{
  u32 Result = (a + b) * (a + b + 1) / 2 + b;
  return Result;
}

u32 GetCantorTriplet(const tracker_element& Element)
{
  u32 CantorPair    = GetCantorPair2(Element.VerticeIndex, Element.TextureIndex);
  u32 CantorTriplet = GetCantorPair2(CantorPair, Element.NormalIndex);
  return CantorTriplet;
}

b32 IsEmpty(const tracker_element& Element)
{
  b32 Result =  Element.VerticeIndex == 0 &&
                Element.TextureIndex == 0 &&
                Element.NormalIndex  == 0;
  return Result;
}

b32 Equals(const tracker_element& A, const tracker_element& B)
{
  b32 Result =  A.VerticeIndex == B.VerticeIndex &&
                A.TextureIndex == B.TextureIndex &&
                A.NormalIndex  == B.NormalIndex;
  return Result;
}

global_variable r32 G_CollisionCount = 0;

b32 Exists(u32 ArraySize, tracker_element* TrackerArray, const tracker_element& NewElement, u32* ResultIndex)
{
  u32 CantorTriplet = GetCantorTriplet(NewElement);
  u32 Idx = CantorTriplet % ArraySize;

  tracker_element ExistingElement = TrackerArray[Idx];
  b32 Collision = false;
  do
  {
    if(IsEmpty(ExistingElement))
    {
      *ResultIndex = Idx;
      if(Collision)
      {
        G_CollisionCount++;
      }
      return false;
    }else if(Equals(ExistingElement, NewElement)){
      *ResultIndex = Idx;
      return true;
    }else{
      Collision = true;
      Idx = (Idx+1)%ArraySize;
      ExistingElement = TrackerArray[Idx];
    }
  }while(true);
  
  INVALID_CODE_PATH;

  return false;
}

global_variable u32 G_PrimeNumberList[] =  {23, 47, 97, 193, 383, 769, 1531, 3079, 6043};
gl_vertex_buffer CreateGLVertexBuffer(memory_arena* Arena,
                     const u32  IndexCount,
                     const u32* VerticeIndeces, const u32* NormalIndeces, const u32* TextureIndeces,
                     const v3*  VerticeData,    const v3*  NormalData,    const v2*  TextureData)
{
  u32* GLVerticeIndexArray  = PushArray(Arena, 3*IndexCount, u32);
  u32* GLIndexArray         = PushArray(Arena, IndexCount, u32);



  u32 TrackerCount = 3*IndexCount;
  for(u32 i = 0; i < ArrayCount(G_PrimeNumberList); i++)
  {
    if(TrackerCount < G_PrimeNumberList[i])
    {
      TrackerCount = G_PrimeNumberList[i];
      break;
    }
  }
  tracker_element* TrackerArray  = PushArray(Arena, TrackerCount, tracker_element);
  G_CollisionCount = 0;
  u32 VerticeArrayCount = 0;
  for( u32 i = 0; i < IndexCount; ++i )
  {
    const u32 vidx = VerticeIndeces[i];
    const u32 tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const u32 nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    tracker_element Element = NewTrackerElement(0, vidx+1, tidx+1, nidx+1);


    u32 TrackerIndex = 0;
    if(!Exists(TrackerCount, TrackerArray, Element, &TrackerIndex))
    {
      u32 NewIndex = VerticeArrayCount++;
      Element.ArrayIndex = NewIndex;
      GLIndexArray[i] = NewIndex;

      GLVerticeIndexArray[3*NewIndex+0] = vidx;
      GLVerticeIndexArray[3*NewIndex+1] = tidx;
      GLVerticeIndexArray[3*NewIndex+2] = nidx;
      TrackerArray[TrackerIndex] = Element;
    }else{
      tracker_element ExistingElement = TrackerArray[TrackerIndex];
      GLIndexArray[i] = ExistingElement.ArrayIndex;
    }
  }
  //Platform.DEBUGPrint("%f %% %d %d Unique Collision Frequencey\n", G_CollisionCount/(r32)IndexCount,IndexCount, TrackerCount);
  opengl_vertex* VertexData = PushArray(Arena, VerticeArrayCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  for( u32 i = 0; i < VerticeArrayCount; ++i )
  {
    const u32 vidx = GLVerticeIndexArray[3*i + 0];
    const u32 tidx = GLVerticeIndexArray[3*i + 1];
    const u32 nidx = GLVerticeIndexArray[3*i + 2];
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
