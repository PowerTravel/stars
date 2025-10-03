#pragma once
#include "../asset_manager/asset_types.h"
#include "gltf_loader.h"
#include "commons/memory.h"

namespace asset {
namespace gltf_tmp {


  cmn::string GetUniqueName(const char* Prefix, size_t Size, char* Memory)
  {
    static int UniqueIndex = 0;
    cmn::string Result = cmn::String(Size, Memory);
    cmn::PushBack(Result, Prefix);
    char NumBuf[32] = {};
    cmn::Itoa(UniqueIndex++, ArrayCount(NumBuf), NumBuf);
    cmn::PushBack(Result, NumBuf);
    return Result;
  }

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
    Result.Queue = JwinAllocArray(Size, node_queue::pair);
    return Result;
  }
  void DeleteNodeQueue(node_queue& Queue)
  {
    JwinFreeMemory(Queue.Queue);
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
//    Each scene will create a separate render_tree. Render asset is a tree-structure with transformations and mesh-material pairs.
//
//  Reason: Im unsure if I want the tree-structure to be made up of entities or if they are internal to the render asset, so we keep it internal for now
//          and we can convert them to entities-components later if we want.

  // MeshIDMap has same order as meshes in the raw_gltf_data, but the values are in the asset manager.

  // This function is broken. each raw_mesh has a set of mesh_primitives. Our type for mesh is mesh_info which has an array of mesh(_primitive) + material pair.
  render_tree::node ConvertPayload(gltf::raw_node* RawNode, mesh_id* MeshIdMap )
  {
    render_tree::node Result = {};

#if 0
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
      for (int i = 0; i < RawNode->Mesh->PrimitiveCount; ++i)
      {
        
      }
      Result.MeshInfo.Mesh = &MeshMap[*RawNode->Mesh];
      Result.MeshInfo.PbrMaterial = &MeshMap[*RawNode->Material];
    }
#endif
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
    int* UniqueRootNodeTracker = JwinAllocArray(TotalNodeCount, int);

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
    
    int* Result = JwinAllocArray(UniqueRootNodeCount, int);
    for (int i = 0; i < UniqueRootNodeCount; ++i)
    {
      Result[i] = UniqueRootNodeTracker[i]-1;
      Assert(Result[i]>=0);
    }

    JwinFreeMemory(UniqueRootNodeTracker);
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

  render_tree ConvertTree(int RootNodeIndex, size_t RawNodeCount, gltf::raw_node* RawNodes, mesh_id* MeshIDMap)
  {
    render_tree Result = {};
    Result.NodeCount = GetNodeCount(RootNodeIndex, RawNodeCount, RawNodes);
    Result.Nodes = JwinAllocArray(Result.NodeCount, render_tree::node);
    Result.Root = Result.Nodes;

    int NodeArrayIndex = 0;
    node_queue Queue = NodeQueue(Result.NodeCount);

    render_tree::node* RootNode = &Result.Nodes[NodeArrayIndex];
    gltf::raw_node* RawRootNode = &RawNodes[RootNodeIndex];
    *RootNode = ConvertPayload(RawRootNode, MeshIDMap);
    Push(Queue, RootNodeIndex, NodeArrayIndex++);

    while(!IsEmpty(Queue))
    {
      node_queue::pair NodeIndexPair = Pop(Queue);
      gltf::raw_node* RawNode = &RawNodes[NodeIndexPair.RawNodeIndex];
      render_tree::node* ParentNode = &Result.Nodes[NodeIndexPair.NodeIndex];
      ParentNode->ChildCount = RawNode->ChildCount;
      for (int i = 0; i < RawNode->ChildCount; ++i)
      {
        int RawChildIndex = RawNode->Children[i];
        gltf::raw_node* RawChildNode = &RawNodes[RawChildIndex];

        int ChildIndex = NodeArrayIndex++;
        render_tree::node* ChildNode = &Result.Nodes[ChildIndex];

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

  render_tree* ToRenderAssets(gltf::raw_gltf_data* GltfData, gltf::raw_scene* RawScene, mesh_id* MeshIDMap, mesh_id* MaterialIDMap)
  {
    int* UniqueRootNodes = 0;
    size_t UniqueRootNodeCount = GetRootNodeCount(GltfData->RawSceneCount, GltfData->RawScenes, GltfData->RawNodeCount, &UniqueRootNodes);

    render_tree* Result = JwinAllocArray(UniqueRootNodeCount, render_tree);
    for (int i = 0; i < UniqueRootNodeCount; ++i)
    {
      int UniqueRootNodeIndex = UniqueRootNodes[i];
      gltf::raw_node* RawRoot = &GltfData->RawNodes[UniqueRootNodeIndex];

      render_tree* RenderAsset = &Result[i];
      *RenderAsset = ConvertTree(UniqueRootNodeIndex, GltfData->RawNodeCount, GltfData->RawNodes, MeshIDMap);
    }

    JwinFreeMemory(UniqueRootNodes);

    return Result;
  }


  void Copy(size_t ByteCount, uint8_t* Src, uint8_t* Dst)
  {
    uint8_t* SrcScan = (uint8_t*) Src;
    uint8_t* DstScan = (uint8_t*) Dst;
    while (ByteCount--) { *DstScan++ = *SrcScan++;}
  }

  image Map(const gltf::raw_image& Raw)
  { 
    image Result = {};
    Result.Width    = Raw.Width;
    Result.Height   = Raw.Height;
    Result.Channels = Raw.Channels;
    Result.Pixels   = Raw.Pixels;
    return Result;  
  }

  texture::filter MapFilter(gltf::raw_sampler::filter Raw)
  {
    texture::filter Result = texture::filter::NEAREST;
    switch(Raw)
    {
      case gltf::raw_sampler::filter::NEAREST:                Result = texture::filter::NEAREST; break;
      case gltf::raw_sampler::filter::LINEAR:                 Result = texture::filter::LINEAR; break;
      case gltf::raw_sampler::filter::NEAREST_MIPMAP_NEAREST: Result = texture::filter::NEAREST_MIPMAP_NEAREST; break;
      case gltf::raw_sampler::filter::LINEAR_MIPMAP_NEAREST:  Result = texture::filter::LINEAR_MIPMAP_NEAREST; break;
      case gltf::raw_sampler::filter::NEAREST_MIPMAP_LINEAR:  Result = texture::filter::NEAREST_MIPMAP_LINEAR; break;
      case gltf::raw_sampler::filter::LINEAR_MIPMAP_LINEAR:   Result = texture::filter::LINEAR_MIPMAP_LINEAR; break;
    }
    return Result;
  }

  texture::wrap MapWrap(gltf::raw_sampler::wrap Raw)
  {
    texture::wrap Result = texture::wrap::REPEAT;
    switch(Raw)
    {
      case gltf::raw_sampler::wrap::CLAMP_TO_EDGE:   Result = texture::wrap::CLAMP_TO_EDGE; break;
      case gltf::raw_sampler::wrap::MIRRORED_REPEAT: Result = texture::wrap::MIRRORED_REPEAT; break;
      case gltf::raw_sampler::wrap::REPEAT:          Result = texture::wrap::REPEAT; break;
    }
    return Result;
  }

  texture MapTexture(gltf::raw_texture_info* BaseColorTexture, gltf::raw_gltf_data* RawGltfData, image** Images) {
    texture Result = {};

    gltf::raw_texture* RawTexture = &RawGltfData->RawTextures[BaseColorTexture->Index];

    if(RawTexture->Sampler)
    {
      gltf::raw_sampler* RawSampler = &RawGltfData->RawSamplers[*RawTexture->Sampler];
      Result.MagFilter = MapFilter(RawSampler->MagFilter);
      Result.MinFilter = MapFilter(RawSampler->MinFilter);
      Result.WrapS = MapWrap(RawSampler->WrapS);
      Result.WrapT = MapWrap(RawSampler->WrapT);
    }else{
      Result.MagFilter = texture::filter::NEAREST;
      Result.MinFilter = texture::filter::NEAREST;
      Result.WrapS = texture::wrap::REPEAT;
      Result.WrapT = texture::wrap::REPEAT;  
    }

    Assert(RawTexture->Source);
    Result.Image = Images[*RawTexture->Source];
    Result.TexCoord = BaseColorTexture->TexCoord;

    return Result;
  };

  pbr_material::metallic_roughness MapMetallicRoughness(gltf::raw_pbr_metallic_roughness* RawPbrMetallicRoughness, gltf::raw_gltf_data* RawGltfData, image** Images) {
    pbr_material::metallic_roughness Result = {};

    Result.BaseColorFactor = RawPbrMetallicRoughness->BaseColorFactor;

    if(RawPbrMetallicRoughness->BaseColorTexture)
    {
      Result.HasBaseColorTexture = true;
      Result.BaseColorTexture = MapTexture(RawPbrMetallicRoughness->BaseColorTexture, RawGltfData, Images);
    }

    Result.MetallicFactor = RawPbrMetallicRoughness->MetallicFactor; 
    Result.RoughnessFactor = RawPbrMetallicRoughness->RoughnessFactor; 

    if(RawPbrMetallicRoughness->MetallicRoughnessTexture)
    {
      Result.HasMetallicRoughnessTexture = true;
      Result.MetallicRoughnessTexture = MapTexture(RawPbrMetallicRoughness->MetallicRoughnessTexture, RawGltfData, Images);
    }

    return Result;
  }

  pbr_material::normal_texture MapNormalTexture() {
    pbr_material::normal_texture Result = {};

    // Implement if we hit this
    Assert(0);
    return Result;
  };

  pbr_material::occlusion_texture MapOcclusionTexture() {
    pbr_material::occlusion_texture Result = {};
    // Implement if we hit this
    Assert(0);
    return Result;
  };


  pbr_material MapMaterial(gltf::raw_material* RawMaterial, gltf::raw_gltf_data* RawGltfData, image** Images)
  {
    pbr_material Result = {};

    if(RawMaterial->PbrMetallicRoughness)
    {
      Result.HasMetallicRoughness = true;
      Result.MetallicRoughness = MapMetallicRoughness(RawMaterial->PbrMetallicRoughness, RawGltfData, Images);
    }

    if(RawMaterial->NormalTexture)
    {
      Result.HasNormalTexture = true;
      Result.NormalTexture = MapNormalTexture();
    }

    if(RawMaterial->OcclusionTexture)
    {
      Result.HasOcclusionTexture = true;
      Result.OcclusionTexture = MapOcclusionTexture();
    }

    if(RawMaterial->EmissiveTexture)
    {
      Result.HasEmissiveTexture = true;
      Result.EmissiveTexture = MapTexture(RawMaterial->EmissiveTexture, RawGltfData, Images);
    }

    Result.EmissiveFactor = RawMaterial->EmissiveFactor;
    if(!cmn::IsEmpty(RawMaterial->AlphaMode)){
      Result.AlphaMode = cmn::Copy(RawMaterial->AlphaMode);
    }
    Result.AlphaCutoff = RawMaterial->AlphaCutoff;
    Result.DoubleSided = RawMaterial->DoubleSided;

    return Result;
  }

  render_tree LoadToAssetManager(gltf::raw_gltf_data* RawGltfData) {
    render_tree Result = {};

    size_t LoadedImageCount = RawGltfData->RawImageCount;
    image** LoadedImagesTracker = JwinAllocArray(LoadedImageCount, image*);
    for (int i = 0; i < RawGltfData->RawImageCount; ++i)
    {
      gltf::raw_image& RawImage = RawGltfData->RawImages[i];
      image TmpImage = Map(RawImage);
      LoadedImagesTracker[i]  = asset::LoadImage(RawImage.Uri.data, RawImage.Name.data, RawImage.Uri.data, &TmpImage);
    }

    size_t LoadedMaterialCount = RawGltfData->RawImageCount;
    pbr_material** LoadedMaterialTracker = JwinAllocArray(LoadedImageCount, pbr_material*);
    for (int i = 0; i < RawGltfData->RawImageCount; ++i)
    {
      gltf::raw_material* RawMaterial = &RawGltfData->RawMaterials[i];
      pbr_material TmpMaterial = MapMaterial(RawMaterial, RawGltfData, LoadedImagesTracker);

      char NameBuf[256] = {};
      cmn::string Name = {};
      if(cmn::IsEmpty(RawMaterial->Name))
      {
        Name = GetUniqueName("MATERIAL_", sizeof(NameBuf), NameBuf);
      }else{
        Name = RawMaterial->Name;
      }

      LoadedMaterialTracker[i]  = asset::LoadPbrMaterial(Name.data, &TmpMaterial);
    }
    

////
    size_t LoadedMeshCount = RawGltfData->RawMeshCount;
    mesh** LoadedMeshTracker = JwinAllocArray(LoadedImageCount, mesh*);
    for (int i = 0; i < LoadedMeshCount; ++i)
    {
      /* code */
    }




////

    JwinFreeMemory(LoadedMeshTracker);
    JwinFreeMemory(LoadedMaterialTracker);
    JwinFreeMemory(LoadedImagesTracker);

#if 0

    Result.ImageCount = RawGltfData.RawImageCount;
    Result.Images = JwinAllocArray( Result.ImageCount, image);
    for (int i = 0; i < Result.ImageCount; ++i)
    {
      Result.Images[i] = ToImage(&RawGltfData.RawImages[i], PersistentAllocator);
    }

    Result.SamplerCount = RawGltfData.RawSamplerCount;
    Result.Samplers = JwinAllocArray( Result.SamplerCount, sampler);
    for (int i = 0; i < Result.SamplerCount; ++i)
    {
      Result.Samplers[i] = ToSampler(&RawGltfData.RawSamplers[i], PersistentAllocator);
    }

    Result.TextureCount = RawGltfData.RawTextureCount;
    Result.Textures = JwinAllocArray( Result.TextureCount, texture);
    for (int i = 0; i < Result.TextureCount; ++i)
    {
      Result.Textures[i] = ToTexture(&RawGltfData.RawTextures[i], Result.Samplers, Result.Images, PersistentAllocator);
    }

    Result.MaterialCount = RawGltfData.RawMaterialCount;
    Result.Materials = JwinAllocArray( Result.MaterialCount, material);
    for (int i = 0; i < Result.MaterialCount; ++i)
    {
      Result.Materials[i] = ToMaterial(&RawGltfData.RawMaterials[i], Result.Textures, PersistentAllocator);
    }

    Result.MeshCount = RawGltfData.RawMeshCount;
    Result.Meshes = JwinAllocArray( Result.MeshCount, mesh);
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