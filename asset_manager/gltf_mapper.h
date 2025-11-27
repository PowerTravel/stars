#pragma once
#include "asset_types.h"
#include "io/gltf.h"

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

  void CopyTransforms2(asset::render_tree_data* Data, gltf::raw_node& RawNode)
  {
    switch(RawNode.TransformationType){
      case gltf::raw_node::transformation_type::TRS:{
        Data->HasTransform = true;
        Data->Transform = GetModelMatrix(RawNode.Translation, RawNode.Rotation, RawNode.Scale);
      }break;
      case gltf::raw_node::transformation_type::MATRIX:{
        Data->HasTransform = true;
        Data->Transform = RawNode.Matrix;
      }break;
      default :{
        Data->HasTransform = false;
        Data->Transform = M4Identity();
      } break;
    }
  }

  inline asset::render_tree_data CreateRenderTreeData(gltf::raw_node& RawNode, asset::key* LoadedMeshes, asset::key* LoadedCameras) {
    asset::render_tree_data Result = {};
    CopyTransforms2(&Result, RawNode);
    if(RawNode.Mesh)
    {
      Result.Mesh = LoadedMeshes[*RawNode.Mesh];
    }
    if(RawNode.Camera)
    {
      Result.Camera = LoadedCameras[*RawNode.Camera];
    }
    return Result;
  }

  struct node_pair {
    int RawNodeIndex;
    asset::render_tree::node* Node;
  };

  asset::render_tree ToRenderTree(size_t NodeCount, int RawRootNodeIndex, gltf::raw_node* RawNodes, asset::key* LoadedMeshes, asset::key* LoadedCameras)
  {
    asset::render_tree Result = asset::render_tree::Create(true,TransientMalloc, TransientFree);
    cmn::vector<node_pair> NodeQueue = cmn::vector<node_pair>::CreateTransient(NodeCount);
    
    node_pair RootPair = {};
    RootPair.RawNodeIndex = RawRootNodeIndex;
    RootPair.Node = Result.NewNode();
    NodeQueue.PushBack(RootPair);

    while(NodeQueue.Size() > 0)
    {
      node_pair NodeIndexPair = NodeQueue.PopBack();
      gltf::raw_node* RawNode = &RawNodes[NodeIndexPair.RawNodeIndex];

      asset::render_tree_data Data = CreateRenderTreeData(*RawNode, LoadedMeshes, LoadedCameras);
      Result.SetData(NodeIndexPair.Node,&Data);
      for (int i = 0; i < RawNode->ChildCount; ++i)
      {
        node_pair Pair = {};
        Pair.RawNodeIndex = RawNode->Children[i];
        Pair.Node = Result.NewNode(NodeIndexPair.Node);
        NodeQueue.PushBack(Pair);
      }
    }

    return Result;
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

  asset::key* ToRenderTrees(const c8* Name, const c8* Path, gltf::raw_gltf_data* RawGltfData, size_t* RetKeyCount, asset::key* LoadedMeshes, asset::key* LoadedCameras){
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

    asset::key* Result = PushArray(GlobalTransientArena, RootCount, asset::key);
    *RetKeyCount = RootCount;
    for (int i = 0; i < RootCount; ++i)
    {
      temporary_memory TempMem = BeginTemporaryMemory(GlobalTransientArena);
      int RootNodeIndex = RootNodeIndeces[i];
      size_t NodeCount = GetTreeNodeCount(RootNodeIndex, RawNodeCount, RawNodes);
      asset::render_tree Tree = ToRenderTree(NodeCount, RootNodeIndex, RawNodes, LoadedMeshes, LoadedCameras);
      c8* UnqName = asset::CreateUniqueName("",Name,"", i, RootCount);

      asset::LoadRenderTree(UnqName, Path, &Tree, &Result[i]);
      EndTemporaryMemory(TempMem);
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
      //Assert(ExtractedPrimitive->MaterialIndex); // Not required but fix once we find a mesh without material
      if(ExtractedPrimitive->MaterialIndex)
      {
        Primitive->PbrMaterial           = LoadedMaterials[*ExtractedPrimitive->MaterialIndex];
      }else{
        Primitive->PbrMaterial = 0;
      }
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
          Result.Perspective.AspectRatio = GlobalWindowSize.ApplicationAspectRatio;
        }
        Result.Perspective.YFov = RawCamera->Perspective.YFov;
        if(RawCamera->Perspective.ZFar)
        {
          Result.Perspective.HasZFar = true;
          Result.Perspective.ZFar = *RawCamera->Perspective.ZFar;
        }else{ 
          Result.Perspective.HasZFar = false;
          Result.Perspective.ZFar = 0;
        }

        Result.Perspective.ZNear = RawCamera->Perspective.ZNear;
        
      }break;
    }
    return Result;
  }
  
  c8* SetName(const c8* UniqueName, const cmn::string Name, const c8* TypeName, int Index, int IndexCount)
  {
    c8* Result = 0;
    if(cmn::IsEmpty(Name))
    {
      Result =  asset::CreateUniqueName(UniqueName, TypeName, "", Index, IndexCount);
    }else{
      Result =  asset::CreateUniqueName(UniqueName, Name.data, "", Index, IndexCount);
    }
    return Result;
  }

  asset::package_id LoadGltf(const c8* UniqueName, const  c8* Path, gltf::raw_gltf_data* RawGltfData) {

    asset::package Package = {};

    if(RawGltfData->RawImageCount)
    {
      Package.ImageCount = RawGltfData->RawImageCount;
      Package.Images = PushArray(GlobalTransientArena, Package.ImageCount, asset::image_id);
      for (int i = 0; i < RawGltfData->RawImageCount; ++i)
      {
        gltf::raw_image& RawImage = RawGltfData->RawImages[i];
        asset::image TmpImage = Map(RawImage);
        asset::LoadImage(RawImage.Uri.data, RawImage.Name.data, RawImage.Uri.data, &TmpImage, &Package.Images[i]);
      }
    }

    if(RawGltfData->RawMaterialCount)
    {
      Package.PBRMaterialCount = RawGltfData->RawMaterialCount;
      Package.PBRMaterials = PushArray(GlobalTransientArena, Package.PBRMaterialCount, asset::pbr_material_id);
      for (int i = 0; i < Package.PBRMaterialCount; ++i)
      {
        gltf::raw_material* RawMaterial = &RawGltfData->RawMaterials[i];
        asset::pbr_material TmpMaterial = MapMaterial(RawMaterial, RawGltfData, Package.Images);

        c8* Name = SetName(UniqueName, RawMaterial->Name, "material", i, Package.PBRMaterialCount);

        asset::LoadPbrMaterial(Name, &TmpMaterial, &Package.PBRMaterials[i]);
      }
    }
    
    if(RawGltfData->RawMeshCount)
    {
      Package.MeshCount = RawGltfData->RawMeshCount;
      Package.Meshes = PushArray(GlobalTransientArena, Package.MeshCount, asset::mesh_id);
      for (int i = 0; i < Package.MeshCount; ++i)
      {
        gltf::raw_mesh* RawMesh = &RawGltfData->RawMeshes[i];
        asset::mesh Mesh = ToMesh(RawMesh, Package.PBRMaterials);

        c8* Name = SetName(UniqueName, RawMesh->Name, "mesh", i, Package.MeshCount);

        asset::LoadMesh(Name, &Mesh, &Package.Meshes[i]);
      }
    }

    if(RawGltfData->RawCameraCount)
    {
      Package.CameraCount = RawGltfData->RawCameraCount;
      Package.Cameras = PushArray(GlobalTransientArena, Package.CameraCount, asset::camera_id);
      for(int i = 0; i < Package.CameraCount; ++i)
      {
        gltf::raw_camera* RawCamera = &RawGltfData->RawCameras[i];
        asset::camera Camera = ToCamera(RawCamera);

        c8* Name = SetName(UniqueName, RawCamera->Name, "camera", i, Package.CameraCount);
        
        asset::LoadCamera(Name, &Camera, &Package.Cameras[i]);
      }
    }

    Package.RenderTrees = ToRenderTrees(UniqueName, Path,  RawGltfData, &Package.RenderTreeCount, Package.Meshes, Package.Cameras);  
    asset::package_id Result = 0;
    asset::LoadPackage(UniqueName, Path, &Package, &Result);

    return Result;
  }


} // namespace mapper
} // namespace gltf

