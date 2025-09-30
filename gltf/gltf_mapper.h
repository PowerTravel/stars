#pragma once
#include "../asset_manager/asset_types.h"
#include "gltf_loader.h"

#ifndef JWIN_ALLOC_FUNCTIONS
#define JWIN_ALLOC_FUNCTIONS
#include <cstdlib>
#define ALLOC_MEMORY(size) malloc(size)
#define FREE_MEMORY(ResultFromAlloc) free(ResultFromAlloc)
#endif


namespace asset {
namespace gltf_tmp {

  void Zero(size_t Size, void* Mem)
  {
    uint8_t* Scan = (uint8_t*) Mem;
    while(Size--)
    {
      *Scan++ = 0;
    }
  }

  void* _AllocSize(size_t Size){
    void* Result = ALLOC_MEMORY(Size);
    Zero(Size, Result);
    return Result;
  }

  #define AllocSize(Size) _AllocSize(Size)
  #define AllocStruct(Type) (Type*) _AllocSize(sizeof(Type))
  #define AllocArray(Count, Type) (Type*) _AllocSize((Count)*sizeof(Type))
  #define FreeMemory(ResultFromAlloc) FREE_MEMORY(ResultFromAlloc)

  struct node_queue {

    struct pair {
      int RawNodeIndex;
      int NodeIndex;
    };

    size_t Count;
    size_t TotCount;
    pair* Queue;
  };

  

  node_queue NodeQueue(size_t Size)
  {
    node_queue Result = {}; 
    Result.Count = 0;
    Result.TotCount = Size;
    Result.Queue = AllocArray(Size, node_queue::pair);
    return Result;
  }
  void DeleteNodeQueue(node_queue& Queue)
  {
    FreeMemory(Queue.Queue);
  }

  bool IsEmpty(node_queue& Queue)
  {
    bool Result = Queue.Count == 0;
    return Result;
  }

  void Push(node_queue& Queue, int RawNodeIndex, int NodeIndex = 0)
  {
    node_queue::pair* Pair = &Queue.Queue[Queue.Count++];
    Pair->RawNodeIndex = RawNodeIndex;
    Pair->NodeIndex = NodeIndex;
  }

  node_queue::pair Pop(node_queue& Queue)
  {
    Assert(Queue.Count > 0);
    if(Queue.Count == 0) return {};
    node_queue::pair Result = Queue.Queue[--Queue.Count];
    Queue.Queue[Queue.Count+1] = {};
    return Result;
  }

//  Idea now:
//  The mapper will split up a raw_gltf_data struct into:
//
//    Each  raw_image, raw_mesh_primitive and raw_material will be converted and loaded into the asset_manager.
//
//    Each scene will create a separate render_asset. Render asset is a tree-structure with transformations and mesh-material pairs.
//
//  Reason: Im unsure if I want the tree-structure to be made up of entities or if they are internal to the render asset, so we keep it internal for now
//          and we can convert them to entities-components later if we want.

  // MeshIDMap has same order as meshes in the raw_gltf_data, but the values are in the asset manager.
  render_asset::node ConvertPayload(gltf::raw_node* RawNode, mesh_id* MeshIDMap)
  {
    render_asset::node Result = {};
    switch(RawNode->TransformationType){
      case gltf::raw_node::transformation_type::NONE: {
        Result.Transform = M4Identity();
      } break;
      case gltf::raw_node::transformation_type::TRS:{
        Result.Transform = GetModelMatrix(RawNode->Translation, RawNode->Rotation, RawNode->Scale);
      }break;
      case gltf::raw_node::transformation_type::MATRIX:{
        Result.Transform = RawNode->Matrix;
      }break;
    }

    if(RawNode->Mesh)
    {
      Result.HasMesh = true;
      Result.MeshInfo.Mesh = MeshIDMap[*RawNode->Mesh];
    }

    return Result;
  }

  bool Exists(size_t ArrayCount, int* Array, int Val)
  {
    for (int i = 0; i < ArrayCount; ++i)
    {
      if(Array[i] == Val)
      {
        return true;
      }
    }

    return false;
  }

  size_t GetRootNodeCount(size_t RawSceneCount, gltf::raw_scene* RawScenes, size_t TotalNodeCount, int** UniqueRootNodes)
  {
    int* UniqueRootNodeTracker = AllocArray(TotalNodeCount, int);

    size_t UniqueRootNodeCount = 0;
    for (int SceneIndex = 0; SceneIndex < RawSceneCount; ++SceneIndex)
    {
      gltf::raw_scene* RawScene = &RawScenes[SceneIndex];
      for (int i = 0; i < RawScene->NodeCount; ++i)
      {
        int RootNodeIndex = RawScene->Nodes[i] + 1;
        if(!Exists(UniqueRootNodeCount, UniqueRootNodeTracker, RootNodeIndex))
        {
          UniqueRootNodeTracker[UniqueRootNodeCount++] = RootNodeIndex;
        }
      }
    }
    
    int* Result = AllocArray(UniqueRootNodeCount, int);
    for (int i = 0; i < UniqueRootNodeCount; ++i)
    {
      Result[i] = UniqueRootNodeTracker[i]-1;
      Assert(Result[i]>=0);
    }

    FreeMemory(UniqueRootNodeTracker);
    *UniqueRootNodes = Result;
    return UniqueRootNodeCount;
  }

  size_t GetNodeCount(int RootNodeIndex, size_t RawNodeCount, gltf::raw_node* RawNodes)
  {
    size_t Result = 1;
    node_queue Queue = NodeQueue(RawNodeCount);
    Push(Queue, RootNodeIndex);
    while(!IsEmpty(Queue))
    {
      int NodeIndex = Pop(Queue).RawNodeIndex;
      gltf::raw_node* RawNode = &RawNodes[NodeIndex];
      Result += RawNode->ChildCount;
      for (int i = 0; i < RawNode->ChildCount; ++i)
      {
        int ChildIndex = RawNode->Children[i];
        Push(Queue, ChildIndex);
      }
    }
    DeleteNodeQueue(Queue);
    return Result;
  }

  render_asset ConvertTree(int RootNodeIndex, size_t RawNodeCount, gltf::raw_node* RawNodes, mesh_id* MeshIDMap)
  {
    render_asset Result = {};
    Result.NodeCount = GetNodeCount(RootNodeIndex, RawNodeCount, RawNodes);
    Result.Nodes = AllocArray(Result.NodeCount, render_asset::node);
    Result.Root = Result.Nodes;

    int NodeArrayIndex = 0;
    node_queue Queue = NodeQueue(Result.NodeCount);

    render_asset::node* RootNode = &Result.Nodes[NodeArrayIndex];
    gltf::raw_node* RawRootNode = &RawNodes[RootNodeIndex];
    *RootNode = ConvertPayload(RawRootNode, MeshIDMap);
    Push(Queue, RootNodeIndex, NodeArrayIndex++);

    while(!IsEmpty(Queue))
    {
      node_queue::pair NodeIndexPair = Pop(Queue);
      gltf::raw_node* RawNode = &RawNodes[NodeIndexPair.RawNodeIndex];
      render_asset::node* ParentNode = &Result.Nodes[NodeIndexPair.NodeIndex];
      ParentNode->ChildCount = RawNode->ChildCount;
      for (int i = 0; i < RawNode->ChildCount; ++i)
      {
        int RawChildIndex = RawNode->Children[i];
        gltf::raw_node* RawChildNode = &RawNodes[RawChildIndex];

        int ChildIndex = NodeArrayIndex++;
        render_asset::node* ChildNode = &Result.Nodes[ChildIndex];

        *ChildNode = ConvertPayload(RawChildNode, MeshIDMap);
        ChildNode->Parent = ParentNode;

        if(i == 0)
        {
          ParentNode->FirstChild = ChildNode;
        } else {
          ChildNode->PreviousSibling = &Result.Nodes[ChildIndex-1];
          ChildNode->PreviousSibling->NextSibling = ChildNode;
        }

        Push(Queue, RawChildIndex, ChildIndex);
      }
    }

    DeleteNodeQueue(Queue);
    return Result;
  }

  render_asset* ToRenderAssets(gltf::raw_gltf_data* GltfData, gltf::raw_scene* RawScene, mesh_id* MeshIDMap, mesh_id* MaterialIDMap)
  {
    int* UniqueRootNodes = 0;
    size_t UniqueRootNodeCount = GetRootNodeCount(GltfData->RawSceneCount, GltfData->RawScenes, GltfData->RawNodeCount, &UniqueRootNodes);

    render_asset* Result = AllocArray(UniqueRootNodeCount, render_asset);
    for (int i = 0; i < UniqueRootNodeCount; ++i)
    {
      int UniqueRootNodeIndex = UniqueRootNodes[i];
      gltf::raw_node* RawRoot = &GltfData->RawNodes[UniqueRootNodeIndex];

      render_asset* RenderAsset = &Result[i];
      *RenderAsset = ConvertTree(UniqueRootNodeIndex, GltfData->RawNodeCount, GltfData->RawNodes, MeshIDMap);
    }

    FreeMemory(UniqueRootNodes);

    return Result;
  }


  render_asset Map(gltf::raw_gltf_data* GltfData) {
    render_asset Result = {};


#if 0

    Result.ImageCount = RawGltfData.RawImageCount;
    Result.Images = AllocArray( Result.ImageCount, image);
    for (int i = 0; i < Result.ImageCount; ++i)
    {
      Result.Images[i] = ToImage(&RawGltfData.RawImages[i], PersistentAllocator);
    }

    Result.SamplerCount = RawGltfData.RawSamplerCount;
    Result.Samplers = AllocArray( Result.SamplerCount, sampler);
    for (int i = 0; i < Result.SamplerCount; ++i)
    {
      Result.Samplers[i] = ToSampler(&RawGltfData.RawSamplers[i], PersistentAllocator);
    }

    Result.TextureCount = RawGltfData.RawTextureCount;
    Result.Textures = AllocArray( Result.TextureCount, texture);
    for (int i = 0; i < Result.TextureCount; ++i)
    {
      Result.Textures[i] = ToTexture(&RawGltfData.RawTextures[i], Result.Samplers, Result.Images, PersistentAllocator);
    }

    Result.MaterialCount = RawGltfData.RawMaterialCount;
    Result.Materials = AllocArray( Result.MaterialCount, material);
    for (int i = 0; i < Result.MaterialCount; ++i)
    {
      Result.Materials[i] = ToMaterial(&RawGltfData.RawMaterials[i], Result.Textures, PersistentAllocator);
    }

    Result.MeshCount = RawGltfData.RawMeshCount;
    Result.Meshes = AllocArray( Result.MeshCount, mesh);
    for (int i = 0; i < Result.MeshCount; ++i)
    {
      Result.Meshes[i] = ToMesh(i, &RawGltfData, Result.Materials, PersistentAllocator);
    }

    Result.SceneCount = RawGltfData.RawSceneCount;
    Result.Scenes = ToScenes(&RawGltfData, Result.Meshes,  TmpAllocator);

    for (int i = 0; i < RawGltfData.BufferCount; ++i)
    {
      if(RawGltfData.RawBuffers[i].LoadedData)
      {
        FreeFile(RawGltfData.RawBuffers[i].LoadedData);
      }
    }
#endif
    return Result;
  }

} // namespace gltf_tmp
} // namespace asset