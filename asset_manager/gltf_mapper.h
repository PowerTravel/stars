#pragma once
#include "asset_types.h"
#include "io/gltf.h"
#include "ecs/systems/system_render.h" // For Aspect Ratio

namespace gltf {
namespace mapper {
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
    Result.Queue = PushArray(GlobalTransientArena,Size, node_queue::pair);
    return Result;
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

  void Copy(size_t ByteCount, uint8_t* Src, uint8_t* Dst)
  {
    uint8_t* SrcScan = (uint8_t*) Src;
    uint8_t* DstScan = (uint8_t*) Dst;
    while (ByteCount--) { *DstScan++ = *SrcScan++;}
  }

  asset::image Map(const gltf::raw_image& Raw)
  { 
    asset::image Result = {};
    Result.Width    = Raw.Width;
    Result.Height   = Raw.Height;
    Result.Channels = Raw.Channels;
    Result.Pixels   = Raw.Pixels;
    return Result;  
  }

  asset::texture::filter MapFilter(gltf::raw_sampler::filter Raw)
  {
    asset::texture::filter Result = asset::texture::filter::NEAREST;
    switch(Raw)
    {
      case gltf::raw_sampler::filter::NEAREST:                Result = asset::texture::filter::NEAREST; break;
      case gltf::raw_sampler::filter::LINEAR:                 Result = asset::texture::filter::LINEAR; break;
      case gltf::raw_sampler::filter::NEAREST_MIPMAP_NEAREST: Result = asset::texture::filter::NEAREST_MIPMAP_NEAREST; break;
      case gltf::raw_sampler::filter::LINEAR_MIPMAP_NEAREST:  Result = asset::texture::filter::LINEAR_MIPMAP_NEAREST; break;
      case gltf::raw_sampler::filter::NEAREST_MIPMAP_LINEAR:  Result = asset::texture::filter::NEAREST_MIPMAP_LINEAR; break;
      case gltf::raw_sampler::filter::LINEAR_MIPMAP_LINEAR:   Result = asset::texture::filter::LINEAR_MIPMAP_LINEAR; break;
    }
    return Result;
  }

  asset::texture::wrap MapWrap(gltf::raw_sampler::wrap Raw)
  {
    asset::texture::wrap Result = asset::texture::wrap::REPEAT;
    switch(Raw)
    {
      case gltf::raw_sampler::wrap::CLAMP_TO_EDGE:   Result = asset::texture::wrap::CLAMP_TO_EDGE; break;
      case gltf::raw_sampler::wrap::MIRRORED_REPEAT: Result = asset::texture::wrap::MIRRORED_REPEAT; break;
      case gltf::raw_sampler::wrap::REPEAT:          Result = asset::texture::wrap::REPEAT; break;
    }
    return Result;
  }

  asset::texture MapTexture(gltf::raw_texture_info* BaseColorTexture, gltf::raw_gltf_data* RawGltfData, asset::key* Images) {
    asset::texture Result = {};

    gltf::raw_texture* RawTexture = &RawGltfData->RawTextures[BaseColorTexture->Index];

    if(RawTexture->Sampler)
    {
      gltf::raw_sampler* RawSampler = &RawGltfData->RawSamplers[*RawTexture->Sampler];
      Result.MagFilter = MapFilter(RawSampler->MagFilter);
      Result.MinFilter = MapFilter(RawSampler->MinFilter);
      Result.WrapS = MapWrap(RawSampler->WrapS);
      Result.WrapT = MapWrap(RawSampler->WrapT);
    }else{
      Result.MagFilter = asset::texture::filter::NEAREST;
      Result.MinFilter = asset::texture::filter::NEAREST;
      Result.WrapS = asset::texture::wrap::REPEAT;
      Result.WrapT = asset::texture::wrap::REPEAT;  
    }

    Assert(RawTexture->Source);
    Result.Image = Images[*RawTexture->Source];
    Result.TexCoord = BaseColorTexture->TexCoord;

    return Result;
  };

  asset::pbr_material::metallic_roughness MapMetallicRoughness(gltf::raw_pbr_metallic_roughness* RawPbrMetallicRoughness, gltf::raw_gltf_data* RawGltfData,  asset::key* Images) {
    asset::pbr_material::metallic_roughness Result = {};

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

  asset::pbr_material::normal_texture MapNormalTexture() {
    asset::pbr_material::normal_texture Result = {};

    // Implement if we hit this
    Assert(0);
    return Result;
  };

  asset::pbr_material::occlusion_texture MapOcclusionTexture() {
    asset::pbr_material::occlusion_texture Result = {};
    // Implement if we hit this
    Assert(0);
    return Result;
  };


  asset::pbr_material MapMaterial(gltf::raw_material* RawMaterial, gltf::raw_gltf_data* RawGltfData,  asset::key* Images)
  {
    asset::pbr_material Result = {};

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

  asset::mesh::primitive::topology ModeToTopology(gltf::primitive_mode Mode)
  {  
    switch(Mode)
    {
      case gltf::primitive_mode::POINTS: return asset::mesh::primitive::topology::POINTS;
      case gltf::primitive_mode::LINES: return asset::mesh::primitive::topology::LINES;
      case gltf::primitive_mode::LINE_LOOP: return asset::mesh::primitive::topology::LINE_LOOP;
      case gltf::primitive_mode::LINE_STRIP: return asset::mesh::primitive::topology::LINE_STRIP;
      case gltf::primitive_mode::TRIANGLES: return asset::mesh::primitive::topology::TRIANGLES;
      case gltf::primitive_mode::TRIANGLE_STRIP: return asset::mesh::primitive::topology::TRIANGLE_STRIP;
      case gltf::primitive_mode::TRIANGLE_FAN: return asset::mesh::primitive::topology::TRIANGLE_FAN;
    };
    return asset::mesh::primitive::topology::TRIANGLES;
  }

  void MapChildNodes(
    size_t NodeIndex,
    size_t ChildCount,
    asset::render_tree::node* NodeArray,
    asset::render_tree::node* Parent) 
  {

    u32 FirstChildIndex = NodeIndex;
    u32 LastChildIndex  = NodeIndex + ChildCount;

    for (int i = FirstChildIndex; i < LastChildIndex; ++i)
    {
      asset::render_tree::node* Child = &NodeArray[i];
      Child->Parent = Parent;
      if(i == FirstChildIndex)
      {
        Child->Parent->FirstChild = Child;
      }
      if(i < LastChildIndex-1)
      {
        Child->NextSibling = &NodeArray[i];
        Child->NextSibling->PreviousSibling = Child;
      }
      if(i == FirstChildIndex-1)
      {
        Child->PreviousSibling = &NodeArray[i-1];
      }
    }
  }

  void CopyTransforms(asset::render_tree::node* Node, gltf::raw_node* RawNode)
  {
    switch(RawNode->TransformationType){
      case gltf::raw_node::transformation_type::TRS:{
        Node->HasTransform = true;
        Node->Transform = GetModelMatrix(RawNode->Translation, RawNode->Rotation, RawNode->Scale);
      }break;
      case gltf::raw_node::transformation_type::MATRIX:{
        Node->HasTransform = true;
        Node->Transform = RawNode->Matrix;
      }break;
      default :{
        Node->HasTransform = false;
        Node->Transform = M4Identity();
      } break;
    }
  }


  asset::render_tree::node* ToNodes(size_t NodeCount, asset::render_tree::node* Nodes, int RawRootNodeIndex, gltf::raw_node* RawNodes, asset::key* LoadedMeshes, asset::key* LoadedCameras)
  {
    node_queue Queue = NodeQueue(NodeCount);
    Push(Queue, RawRootNodeIndex, 0);
    
    size_t NodeHeadIndex = 1;
    while(!IsEmpty(Queue))
    {
      node_queue::pair NodeIndexPair = Pop(Queue);
      int RawNodeIndex = NodeIndexPair.RawNodeIndex;
      gltf::raw_node* RawNode = &RawNodes[RawNodeIndex];

      int NodeIndex = NodeIndexPair.NodeIndex;
      asset::render_tree::node* Node = &Nodes[NodeIndex];

      CopyTransforms(Node,RawNode);
      if(RawNode->Mesh)
      {
        Node->Mesh = LoadedMeshes[*RawNode->Mesh];
      }

      if(RawNode->Camera)
      {
        Node->Camera = LoadedCameras[*RawNode->Camera];
      }

      Node->ChildCount = RawNode->ChildCount;
      InitiateChildNodes(Node, Node->ChildCount, Nodes+NodeHeadIndex);

      

      for (int i = 0; i < RawNode->ChildCount; ++i)
      {
        int RawChildIndex = RawNode->Children[i];
        Push(Queue, RawChildIndex, NodeHeadIndex++);
      }
    }

    return Nodes;
  }

  size_t GetTreeNodeCount(int RootNodeIndex, size_t RawNodeCount, gltf::raw_node* RawNodes)
  {
    node_queue Queue = NodeQueue(RawNodeCount);
    Push(Queue, RootNodeIndex);
    size_t Result = 1;
    while(!IsEmpty(Queue))
    {
      const int RawNodeIndex = Pop(Queue).RawNodeIndex;
      const int ChildCount = RawNodes[RawNodeIndex].ChildCount;
      Result += ChildCount;
      for (int i = 0; i < ChildCount; ++i)
      {
        const int ChildIndex = RawNodes[RawNodeIndex].Children[i];
        Push(Queue, ChildIndex);
      }
    }

    return Result;
  }

  // Note: The node hierarchy make up a set of disjoint strict trees which means they are free of cycles and each node must have zero or one parent node.
  //       Nodes with 0 parents are root nodes. The same root node may appear in multiple scenes.
  //       I'm assuming this means each child node only appears once.
  asset::key* ToRenderTrees(const c8* Name, const c8* Path, gltf::raw_gltf_data* RawGltfData, size_t* RetKeyCount, asset::key* LoadedMeshes, asset::key* LoadedCameras)
  {
    const size_t RawNodeCount = RawGltfData->RawNodeCount;
    gltf::raw_node* RawNodes = RawGltfData->RawNodes;

    size_t RootCount = 0;
    int* RootNodeIndeces = PushArray(GlobalTransientArena,RawNodeCount, int);
    bool* RootNodeTracker = PushArray(GlobalTransientArena,RawNodeCount, bool);
    for (int i = 0; i < RawGltfData->RawSceneCount; ++i)
    {
      gltf::raw_scene* RawScene = &RawGltfData->RawScenes[i];
      for (int j = 0; j < RawScene->NodeCount; ++j)
      {
        int RootNodeIndex = RawScene->Nodes[j];
        if(!RootNodeTracker[RootNodeIndex])
        {
          RootNodeIndeces[RootCount++] = RootNodeIndex;
          RootNodeTracker[RootNodeIndex] = true;
        }
      }
    }

    asset::key* Result = PushArray(GlobalTransientArena,RootCount, asset::key);
    *RetKeyCount = RootCount;
    for (int i = 0; i < RootCount; ++i)
    {
      asset::render_tree Tree = {};
      int RootNodeIndex = RootNodeIndeces[i];
      Tree.NodeCount   = GetTreeNodeCount(RootNodeIndex, RawNodeCount, RawNodes);
      Tree.Nodes       = PushArray(GlobalTransientArena,Tree.NodeCount, asset::render_tree::node);
      Tree.Root        = ToNodes(Tree.NodeCount, Tree.Nodes, RootNodeIndex, RawNodes, LoadedMeshes, LoadedCameras);

      c8* UnqName = asset::CreateUniqueName("",Name,"", i, RootCount);

      asset::render_tree* RT = asset::LoadRenderTree(UnqName, Path, &Tree, &Result[i]);
      int a = 10;
    }

    return Result;
  }

  asset::mesh ToMesh(gltf::raw_mesh* RawMesh, asset::key* LoadedMaterials){
    
    asset::mesh Result = {};
    Result.PrimitiveCount = RawMesh->ExtractedPrimitiveCount;
    Result.Primitives = PushArray(GlobalTransientArena, Result.PrimitiveCount, asset::mesh::primitive);
    for (int i = 0; i < RawMesh->ExtractedPrimitiveCount; ++i)
    {
      gltf::extracted_primitive* ExtractedPrimitive = &RawMesh->ExtractedPrimitives[i];  
      asset::mesh::primitive* Primitive = &Result.Primitives[i];
      Primitive->IndexCount            = ExtractedPrimitive->IndexCount;
      Primitive->Indeces               = ExtractedPrimitive->Indeces;
      Primitive->VertexCount           = ExtractedPrimitive->vCount;
      Primitive->Vertex                = ExtractedPrimitive->v;
      Primitive->VertexNormal          = ExtractedPrimitive->vn;
      Primitive->TextureVertexSetCount = ExtractedPrimitive->vtSetCount;
      Primitive->TextureVertices       = ExtractedPrimitive->vt;
      Primitive->Topology              = ModeToTopology(ExtractedPrimitive->Mode);
      Primitive->AABB                  = AABB3f(ExtractedPrimitive->vMin,ExtractedPrimitive->vMax);
      Assert(ExtractedPrimitive->MaterialIndex); // Not required but fix once we find a mesh without material
      Primitive->PbrMaterial           = LoadedMaterials[*ExtractedPrimitive->MaterialIndex];
    }
    return Result;
  }

  asset::camera ToCamera(gltf::raw_camera* RawCamera)
  {
    asset::camera Result = {};
    switch(RawCamera->Type)
    {
      case gltf::raw_camera::type::ORTHOGRAPHIC:
      {
        Result.Type = asset::camera::type::ORTHOGRAPHIC;
        Result.Orthographic.XMag = RawCamera->Orthographic.XMag;
        Result.Orthographic.YMag = RawCamera->Orthographic.YMag;
        Result.Orthographic.ZFar = RawCamera->Orthographic.ZFar;
        Result.Orthographic.ZNear = RawCamera->Orthographic.ZNear;
      }break;
      case gltf::raw_camera::type::PERSPECTIVE:
      {
        Result.Type = asset::camera::type::PERSPECTIVE;
        if(RawCamera->Perspective.AspectRatio)
        {
          Result.Perspective.AspectRatio = *RawCamera->Perspective.AspectRatio;
        }else{
          ecs::render::window_size_pixel WindowSize = ecs::render::GetWindowSize(GetRenderSystem());
          Result.Perspective.AspectRatio = WindowSize.ApplicationAspectRatio;
        }
        Result.Perspective.YFov = RawCamera->Perspective.YFov;
        if(RawCamera->Perspective.AspectRatio)
        {
          Result.Perspective.ZFar = *RawCamera->Perspective.ZFar;
        }else{ 
          Result.Perspective.ZFar = R32Max;
        }

        Result.Perspective.ZNear = RawCamera->Perspective.ZNear;
        
      }break;
    }
    return Result;
  }

  asset::key* LoadGltf(const c8* UniqueName,const  c8* Path, gltf::raw_gltf_data* RawGltfData, size_t* RenderTreeCount) {

    size_t LoadedImageCount = RawGltfData->RawImageCount;
    asset::key* LoadedImagesTracker = PushArray(GlobalTransientArena,LoadedImageCount, asset::key);
    for (int i = 0; i < RawGltfData->RawImageCount; ++i)
    {
      gltf::raw_image& RawImage = RawGltfData->RawImages[i];
      asset::image TmpImage = Map(RawImage);
      asset::LoadImage(RawImage.Uri.data, RawImage.Name.data, RawImage.Uri.data, &TmpImage, &LoadedImagesTracker[i]);
    }

    size_t LoadedMaterialCount = RawGltfData->RawMaterialCount;
    asset::key* LoadedMaterialTracker = PushArray(GlobalTransientArena,LoadedImageCount, asset::key);
    for (int i = 0; i < RawGltfData->RawMaterialCount; ++i)
    {
      gltf::raw_material* RawMaterial = &RawGltfData->RawMaterials[i];
      asset::pbr_material TmpMaterial = MapMaterial(RawMaterial, RawGltfData, LoadedImagesTracker);

      c8* Name = 0;
      if(cmn::IsEmpty(RawMaterial->Name))
      {
        Name =  asset::CreateUniqueName("", "material", "", i, RawGltfData->RawMaterialCount);
      }else{
        Name =  asset::CreateUniqueName("", RawMaterial->Name.data, "", i, RawGltfData->RawMaterialCount);
      }

      asset::LoadPbrMaterial(Name, &TmpMaterial, &LoadedMaterialTracker[i]);
    }
    
    size_t LoadedMeshCount = RawGltfData->RawMeshCount;
    asset::key* LoadedMeshTracker = PushArray(GlobalTransientArena,LoadedMeshCount, asset::key);
    for (int i = 0; i < LoadedMeshCount; ++i)
    {
      gltf::raw_mesh* RawMesh = &RawGltfData->RawMeshes[i];
      asset::mesh Mesh = ToMesh(RawMesh, LoadedMaterialTracker);

      c8* Name = 0;
      if(cmn::IsEmpty(RawMesh->Name))
      {
        Name =  asset::CreateUniqueName("", "mesh", "", i, LoadedMeshCount);
      }else{
        Name =  asset::CreateUniqueName("", RawMesh->Name.data, "", i, LoadedMeshCount);
      }

      asset::LoadMesh(Name, &Mesh, &LoadedMeshTracker[i]);
    }

    size_t LoadedCameraCount = RawGltfData->RawCameraCount;
    asset::key* LoadedCameraTracker = PushArray(GlobalTransientArena, LoadedCameraCount, asset::key);
    for(int i = 0; i < RawGltfData->RawCameraCount; ++i)
    {
      gltf::raw_camera* RawCamera = &RawGltfData->RawCameras[i];
      asset::camera Camera = ToCamera(RawCamera);

      c8* Name = 0;
      if(cmn::IsEmpty(RawCamera->Name))
      {
        Name =  asset::CreateUniqueName("", "camera", "", i, LoadedCameraCount);
      }else{
        Name =  asset::CreateUniqueName("", RawCamera->Name.data, "", i, LoadedCameraCount);
      }

      asset::LoadCamera(Name, &Camera, &LoadedCameraTracker[i]);
    }

    asset::key* Result = ToRenderTrees(UniqueName, Path, RawGltfData, RenderTreeCount, LoadedMeshTracker, LoadedCameraTracker);
////

    return Result;
  }


} // namespace mapper
} // namespace gltf
