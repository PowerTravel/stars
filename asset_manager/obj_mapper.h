#pragma once

#include "asset_types.h"
#include "io/obj.h"

struct material_map {
  int MaterialCount;
  asset::gltf_tmp::phong_material_id* Materials;
  mtl_material** Mtl_Materials;
};

static material_map CreateMaterialMap(int MaterialCount)
{
  material_map Result = {};
  Result.MaterialCount = MaterialCount;
  Result.Materials     = PushArray(GlobalTransientArena, MaterialCount, asset::gltf_tmp::phong_material_id);
  Result.Mtl_Materials = PushArray(GlobalTransientArena, MaterialCount, mtl_material*);
  return Result;
}

static asset::gltf_tmp::phong_material_id GetMaterial(material_map* MaterialMap, mtl_material* Mtl){
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


asset::gltf_tmp::mesh::primitive CreateMesh(
  const int  IndexCount,
  const unsigned int* VerticeIndeces, const unsigned int* NormalIndeces, const unsigned  int* TextureIndeces,
  const v3*  VerticeData,    const v3*  NormalData,    const v2*  TextureData)
{
  int* VerticeIndexArray        = PushArray(GlobalTransientArena, IndexCount, int);
  int* VerticeNormalIndexArray  = PushArray(GlobalTransientArena, IndexCount, int);
  int* TextureVerticeIndexArray = PushArray(GlobalTransientArena, IndexCount, int);
  int* IndexArray               = PushArray(GlobalTransientArena, IndexCount, int);

  int TrackerCount = utils::GetHashListSize(IndexCount, 3);
  tracker_element* TrackerArray  = PushArray(GlobalTransientArena, TrackerCount, tracker_element);
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
  v3* Vertex = PushArray(GlobalTransientArena, VerticeArrayCount, v3);
  for( int i = 0; i < VerticeArrayCount; ++i )
  {
    const int idx = VerticeIndexArray[i];
    Vertex[i]  = VerticeData[idx];
  }

  v3* VertexNormal = 0;
  if(NormalData)
  {
    VertexNormal = PushArray(GlobalTransientArena, VerticeArrayCount, v3);
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
    TextureVertexSet = PushArray(GlobalTransientArena, 1, v2*);
    v2* TextureVertex = PushArray(GlobalTransientArena, VerticeArrayCount, v2);
    for( int i = 0; i < VerticeArrayCount; ++i )
    {
      const int idx = TextureVerticeIndexArray[i];
      TextureVertex[i] = TextureData[idx];
    }
    *TextureVertexSet = TextureVertex;
  }
  
  asset::gltf_tmp::mesh::primitive Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = IndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.Vertex = Vertex;
  Result.VertexNormal = VertexNormal;
  Result.TextureVertexSetCount = TextureVertexSetCount;
  Result.TextureVertices = TextureVertexSet;
  Result.Topology = asset::gltf_tmp::mesh::primitive::topology::TRIANGLES;

  return Result;
}


asset::gltf_tmp::mesh::primitive ToMesh(obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
{
  obj_mesh_indeces* Indeces = ObjGrp->Indeces;
  asset::gltf_tmp::mesh::primitive Result = CreateMesh(Indeces->Count,
    Indeces->vi, Indeces->ni,  Indeces->ti,
    MeshData->v, MeshData->vn, MeshData->vt);

  Result.AABB = ObjGrp->aabb;

  Result.PhongMaterial = GetMaterial(MaterialMap, ObjGrp->Material);


  return Result;
}

#if 0
asset::gltf_tmp::render_tree::mesh_info CreateMeshInfo(const c8* UniqueName, const c8* Name, const c8* Path, obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
{  
  asset::gltf_tmp::render_tree::mesh_info Result = {};
  Result.PhongMaterial = GetMaterial(MaterialMap, ObjGrp->Material);

  obj_mesh_indeces* ObjIndeces = ObjGrp->Indeces;

  const asset::gltf_tmp::mesh::primitive MeshPrimitive = ToMesh(ObjGrp, MeshData);
  const asset::gltf_tmp::mesh Mesh = {};
  Mesh->PrimitiveCount = 1;
  Mesh->Primitives = &MeshPrimitive;
  

  unsigned int ResultKey = 0;
  Result.Mesh = asset::LoadMesh2(UniqueName, &Mesh, &ResultKey);

  return Result;
}
#endif

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


static asset::key LoadObjBitmap(const char* UniqueName, const char* Postfix, obj_bitmap* Bitmap)
{
  asset::key Result = 0;

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
      UniqueMtlName = asset::CreateUniqueName(UniqueName, "_" , Mtl->Name, i, ObjMtlGroup->MaterialCount);
    }else{
      UniqueMtlName = asset::CreateUniqueName(UniqueName, "_" , "material", i, ObjMtlGroup->MaterialCount);
    }

    asset::phong_material Material = ToPhongMaterial(Mtl);
    if(Mtl->BumpMap)
    {
      int Handle = LoadObjBitmap(UniqueMtlName,  "_BumpMap", Mtl->BumpMap);
      Material.BumpMap = asset::DefaultTexture(Handle);
      Material.HasBumpMap = true;
    }

    if(Mtl->MapKd)
    {
      int Handle = LoadObjBitmap(UniqueMtlName,  "_DiffuseMap", Mtl->MapKd);
      Material.HasDiffuseTexture = true;
      Material.DiffuseTexture = asset::DefaultTexture(Handle);
    }

    if(Mtl->MapKs)
    {
      int Handle = LoadObjBitmap(UniqueMtlName,  "_SpecularMap", Mtl->MapKs);
      Material.HasSpecularTexture = true;
      Material.SpecularTexture = asset::DefaultTexture(Handle);
    }

    asset::key Key = 0;
    asset::LoadMaterial(UniqueMtlName, &Material, &Key);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = Key;
  }
  return MaterialMap;
}

static void* TransientAllocator(uint32_t MemorySize) {
  void* Result = PushSize(GlobalTransientArena, MemorySize);
  return Result;
}

asset::key LoadMesh(const char* UniqueName, obj_loaded_file* Obj, material_map* MaterialMap)
{  
  c8* MeshName = asset::CreateUniqueName(UniqueName,"_", Obj->ObjectNameLength ? Obj->ObjectName : "_mesh");
  asset::gltf_tmp::mesh Mesh = {};
  Mesh.PrimitiveCount = Obj->ObjectCount;
  Mesh.Primitives = PushArray(GlobalTransientArena, Obj->ObjectCount, asset::gltf_tmp::mesh::primitive);
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    obj_group* ObjectGroup = &Obj->ObjectGroups[i];
    Mesh.Primitives[i] = ToMesh(ObjectGroup, Obj->MeshData, MaterialMap);
  }

  asset::key ResultKey = 0;
  asset::LoadMesh(MeshName, &Mesh, &ResultKey);
  asset::gltf_tmp::mesh* LoadedMesh = (asset::gltf_tmp::mesh*) asset::Find(asset::type::MESH, ResultKey);
  return ResultKey;
}

asset::gltf_tmp::render_tree CreateRenderTree(asset::key MeshId)
{
  asset::gltf_tmp::render_tree Result = {};
  Result.NodeCount  = 1;
  Result.Nodes      = PushArray(GlobalTransientArena, Result.NodeCount, asset::gltf_tmp::render_tree::node);
  Result.Root       = Result.Nodes;
  Result.Root->Mesh = MeshId;
  return Result;
}

static asset::key LoadObj(const c8* Path, const c8* UniqueName)
{
  Assert(Path && *Path != '\0');
  if(!UniqueName || *UniqueName == '\0')
  {
    UniqueName = Path;
  }

  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  // Upload MATERIAL and IMAGES related to material

  material_map MaterialMap = LoadPhongMaterial(Obj->MaterialData, UniqueName);
  
  // MESH
  asset::key MeshKey = LoadMesh(UniqueName, Obj, &MaterialMap);
  
  asset::gltf_tmp::render_tree RenderTree = CreateRenderTree(MeshKey);

  asset::key ResultKey = 0;
  asset::LoadRenderTree(UniqueName, Path, &RenderTree, &ResultKey);

  return ResultKey;
}