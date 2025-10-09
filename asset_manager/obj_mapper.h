#pragma once

#include "asset_types.h"
#include "io/obj.h"

struct material_map {
  int MaterialCount;
  asset::phong_material** Materials;
  mtl_material** Mtl_Materials;
};

static material_map CreateMaterialMap(int MaterialCount)
{
  material_map Result = {};
  Result.MaterialCount = MaterialCount;
  Result.Materials     = PushArray(GlobalTransientArena, MaterialCount, asset::phong_material*);
  Result.Mtl_Materials = PushArray(GlobalTransientArena, MaterialCount, mtl_material*);
  return Result;
}

static asset::phong_material* GetMaterial(material_map* MaterialMap, mtl_material* Mtl){
  for (int i = 0; i < MaterialMap->MaterialCount; ++i)
  {
    if(Mtl == MaterialMap->Mtl_Materials[i])
    {
      return MaterialMap->Materials[i];
    }
  }
  return 0;
}

struct tracker_element {
  int ArrayIndex;
  int VerticeIndex;
  int TextureIndex;
  int NormalIndex;
};

tracker_element NewTrackerElement(int ArrayIndex, int VerticeIndex, int TextureIndex, int NormalIndex)
{
  tracker_element Result = {};
  Result.ArrayIndex = ArrayIndex;
  Result.VerticeIndex = VerticeIndex;
  Result.TextureIndex = TextureIndex;
  Result.NormalIndex = NormalIndex;
  return Result;
}
size_t GetCantorPair2(size_t a, size_t b)
{
  size_t Result = (a + b) * (a + b + 1) / 2 + b;
  return Result;
}

size_t GetCantorTriplet(const tracker_element& Element)
{
  size_t CantorPair    = GetCantorPair2(Element.VerticeIndex, Element.TextureIndex);
  size_t CantorTriplet = GetCantorPair2(CantorPair, Element.NormalIndex);
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

b32 Exists(int ArraySize, tracker_element* TrackerArray, const tracker_element& NewElement, int* ResultIndex)
{
  size_t CantorTriplet = GetCantorTriplet(NewElement);
  int Idx = CantorTriplet % ArraySize;

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


asset::gltf_tmp::mesh CreateMesh(memory_arena* Arena,
                     const int  IndexCount,
                     const unsigned int* VerticeIndeces, const unsigned int* NormalIndeces, const unsigned  int* TextureIndeces,
                     const v3*  VerticeData,    const v3*  NormalData,    const v2*  TextureData)
{
  int* VerticeIndexArray        = PushArray(Arena, IndexCount, int);
  int* VerticeNormalIndexArray  = PushArray(Arena, IndexCount, int);
  int* TextureVerticeIndexArray = PushArray(Arena, IndexCount, int);
  int* IndexArray               = PushArray(Arena, IndexCount, int);

  int TrackerCount = utils::GetHashListSize(IndexCount, 3);
  tracker_element* TrackerArray  = PushArray(Arena, TrackerCount, tracker_element);
  G_CollisionCount = 0;
  int VerticeArrayCount = 0;
  for( int i = 0; i < IndexCount; ++i )
  {
    const int vidx = VerticeIndeces[i];
    const int tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const int nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    tracker_element Element = NewTrackerElement(0, vidx+1, tidx+1, nidx+1);

    int TrackerIndex = 0;
    if(!Exists(TrackerCount, TrackerArray, Element, &TrackerIndex))
    {
      int NewIndex = VerticeArrayCount++;
      Element.ArrayIndex = NewIndex;
      IndexArray[i] = NewIndex;

      VerticeIndexArray[NewIndex] = vidx;
      VerticeNormalIndexArray[NewIndex] = nidx;
      TextureVerticeIndexArray[NewIndex] = tidx;
      TrackerArray[TrackerIndex] = Element;
    }else{
      tracker_element ExistingElement = TrackerArray[TrackerIndex];
      IndexArray[i] = ExistingElement.ArrayIndex;
    }
  }
  //Platform.DEBUGPrint("%f %% %d %d Unique Collision Frequencey\n", G_CollisionCount/(r32)IndexCount,IndexCount, TrackerCount);


  Assert(VerticeData);
  v3* Vertex = PushArray(Arena, VerticeArrayCount, v3);
  for( int i = 0; i < VerticeArrayCount; ++i )
  {
    const int idx = VerticeIndexArray[i];
    Vertex[i]  = VerticeData[idx];
  }

  v3* VertexNormal = 0;
  if(NormalData)
  {
    VertexNormal = PushArray(Arena, VerticeArrayCount, v3);
    for( int i = 0; i < VerticeArrayCount; ++i )
    {
      const int idx = VerticeNormalIndexArray[i];
      VertexNormal[i] = NormalData[idx];
    }
  }

  v2** TextureVertexSet = 0;
  int TextureVertexSetCount = 0;
  if(TextureData)
  {
    TextureVertexSetCount = 1;
    TextureVertexSet = PushArray(Arena, 1, v2*);
    v2* TextureVertex = PushArray(Arena, VerticeArrayCount, v2);
    for( int i = 0; i < VerticeArrayCount; ++i )
    {
      const int idx = TextureVerticeIndexArray[i];
      TextureVertex[i] = TextureData[idx];
    }
    *TextureVertexSet = TextureVertex;
  }
  
  asset::gltf_tmp::mesh Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = IndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.Vertex = Vertex;
  Result.VertexNormal = VertexNormal;
  Result.TextureVertexSetCount = TextureVertexSetCount;
  Result.TextureVertices = TextureVertexSet;
  Result.Topology = asset::gltf_tmp::mesh::topology::TRIANGLES;

  return Result;
}


asset::gltf_tmp::mesh ToMesh(obj_group* ObjGrp, obj_mesh_data* MeshData)
{
  obj_mesh_indeces* Indeces = ObjGrp->Indeces;
  asset::gltf_tmp::mesh Result = CreateMesh(GlobalTransientArena,
    Indeces->Count, Indeces->vi, Indeces->ni, Indeces->ti,
    MeshData->v,MeshData->vn, MeshData->vt);

  Result.AABB = ObjGrp->aabb;

  return Result;
}

asset::gltf_tmp::render_tree::mesh_info CreateMeshInfo(const c8* UniqueName, const c8* Name, const c8* Path, obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
{  
  asset::gltf_tmp::render_tree::mesh_info Result = {};
  Result.PhongMaterial = GetMaterial(MaterialMap, ObjGrp->Material);

  obj_mesh_indeces* ObjIndeces = ObjGrp->Indeces;

  const asset::gltf_tmp::mesh Mesh = ToMesh(ObjGrp, MeshData);

  unsigned int ResultKey = 0;
  Result.Mesh = asset::LoadMesh2(UniqueName, &Mesh, &ResultKey);

  return Result;
}

file_local asset::image ToImage(const obj_bitmap* ObjBitmap)
{
  asset::image Result = {};
  if(!ObjBitmap){return Result;};

  Assert(ObjBitmap->BPP == 32);

  Result.Channels = 4;
  Result.Width    = ObjBitmap->Width;
  Result.Height   = ObjBitmap->Height;
  Result.Pixels   = (uint8_t*) ObjBitmap->Pixels;
  return Result;
}

asset::phong_material ToPhongMaterial(const mtl_material* ObjMtl)
{
  asset::phong_material Result = {};

  if(ObjMtl->Ka) {
    Result.Ka = ObjMtl->Ka;
  }
  if(ObjMtl->Kd) {
    Result.Kd = ObjMtl->Kd;
  }
  if(ObjMtl->Ks) {
    Result.Ks = ObjMtl->Ks;
  }
  if(ObjMtl->Tf) {
    Result.Tf = ObjMtl->Tf;
  }
  if(ObjMtl->Ke) {
    Result.Ke = ObjMtl->Ke;
  }
  if(ObjMtl->d) {
    Result.d = ObjMtl->d;
  }
  if(ObjMtl->Ni) {
    Result.Ni = ObjMtl->Ni;
  }
  if(ObjMtl->Ns) {
    Result.Ns = ObjMtl->Ns;
  }

  return Result;
}


static int LoadObjBitmap(const char* UniqueName, const char* Postfix, obj_bitmap* Bitmap)
{
  uint32_t Result = 0;

  if(Bitmap)
  {
    c8* UniqueName = Bitmap->Name;
    if(!UniqueName)
    {
      UniqueName = asset::CreateUniqueName("", UniqueName, Postfix, 0, 1);
    }

    asset::image* LoadedImage = (asset::image*) asset::Find(asset::type::IMAGE, UniqueName);

    if(!LoadedImage){
      asset::image Image = ToImage(Bitmap);
      LoadedImage = asset::LoadImage(UniqueName, Bitmap->Name, Bitmap->Path, &Image, &Result);
    }
  }

  return Result;
}

static material_map LoadPhongMaterial(obj_mtl_data* ObjMtlGroup, const c8* UniqueName)
{
  material_map MaterialMap = CreateMaterialMap(ObjMtlGroup->MaterialCount);
  for (int i = 0; i < ObjMtlGroup->MaterialCount; ++i)
  {
    mtl_material* Mtl = ObjMtlGroup->Materials + i;
    c8* UniqueMtlName = Mtl->Name;
    if(!Mtl->NameLength){
      UniqueMtlName = asset::CreateUniqueName("", UniqueName, "", i, ObjMtlGroup->MaterialCount);
    }

    asset::phong_material Material = ToPhongMaterial(Mtl);
    if(Mtl->BumpMap)
    {
      int Handle = LoadObjBitmap(UniqueName,  "_BumpMap", Mtl->BumpMap);
      Material.BumpMap = asset::DefaultTexture(Handle);
      Material.HasBumpMap = true;
    }

    if(Mtl->MapKd)
    {
      int Handle = LoadObjBitmap(UniqueName,  "_DiffuseMap", Mtl->MapKd);
      Material.HasDiffuseTexture = true;
      Material.DiffuseTexture = asset::DefaultTexture(Handle);
    }

    if(Mtl->MapKs)
    {
      int Handle = LoadObjBitmap(UniqueName,  "_SpecularMap", Mtl->MapKs);
      Material.HasSpecularTexture = true;
      Material.SpecularTexture = asset::DefaultTexture(Handle);
    }

    uint32_t Key = 0;
    asset::phong_material* LoadedMaterial = asset::LoadMaterial(UniqueName, &Material, &Key);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = LoadedMaterial;
  }
  return MaterialMap;
}

static void* TransientAllocator(uint32_t MemorySize) {
  void* Result = PushSize(GlobalTransientArena, MemorySize);
  return Result;
}

static asset::gltf_tmp::render_tree* LoadObj(const c8* Path, const c8* UniqueName)
{
  Assert(Path && *Path != '\0');
  if(!UniqueName || *UniqueName == '\0')
  {
    UniqueName = Path;
  }

  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  // Upload MATERIAL and IMAGES related to material
  material_map MaterialMap = LoadPhongMaterial(Obj->MaterialData, UniqueName);
  asset::gltf_tmp::render_tree::mesh_info* MeshInfos = PushArray(GlobalTransientArena, Obj->ObjectCount, asset::gltf_tmp::render_tree::mesh_info);
  // MESH
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    c8* MeshName = asset::CreateUniqueName("", UniqueName, "_Mesh", i, Obj->ObjectCount);
    MeshInfos[i] = CreateMeshInfo(UniqueName, MeshName, Path, &Obj->ObjectGroups[i], Obj->MeshData, &MaterialMap);
  }

  /// RENDER_TREE
  Assert(Obj->ObjectCount>0);

  asset::gltf_tmp::render_tree RenderTree = {};
  if(Obj->ObjectCount==1)
  {
    RenderTree.NodeCount = 1;
    RenderTree.Nodes = PushArray(GlobalTransientArena, RenderTree.NodeCount, asset::gltf_tmp::render_tree::node);
    RenderTree.MeshInfoCount = 1;
    RenderTree.MeshInfos = MeshInfos;
    RenderTree.Root = RenderTree.Nodes;
    RenderTree.Root->HasMeshInfo = true;
    RenderTree.Root->MeshInfo = RenderTree.MeshInfos[0];

  }else{
    RenderTree.NodeCount = Obj->ObjectCount+1;
    RenderTree.Nodes = PushArray(GlobalTransientArena, RenderTree.NodeCount, asset::gltf_tmp::render_tree::node);
    RenderTree.MeshInfoCount = Obj->ObjectCount;
    RenderTree.MeshInfos = MeshInfos;
    RenderTree.Root = RenderTree.Nodes;
    RenderTree.Root->FirstChild = &RenderTree.Nodes[1];
    for (int i = 1; i <= Obj->ObjectCount; ++i)
    {
      asset::gltf_tmp::render_tree::node* Node = &RenderTree.Nodes[i];
      Node->Parent = &RenderTree.Nodes[0];
      if(i < Obj->ObjectCount){
        Node->NextSibling = &RenderTree.Nodes[i+1];
        Node->NextSibling->PreviousSibling = Node;
      }
      Node->HasMeshInfo = true;
      Node->MeshInfo = RenderTree.MeshInfos[i-1];
    }
    RenderTree.Root->HasMeshInfo = false;
    RenderTree.Root->MeshInfo = {};
  }



  uint32_t ResultKey = 0;
  asset::gltf_tmp::render_tree* Result = asset::LoadRenderTree(UniqueName, Path, &RenderTree, &ResultKey);

  return Result;
}


asset::gltf_tmp::mesh* MapObjMesh( obj_loaded_file* ObjFile )
{
  return 0;
}
