#pragma once

#include "asset_types.h"
#include "io/obj.h"


namespace obj {
namespace mapper {

struct tracker_element {
  int ArrayIndex;
  int VerticeIndex;
  int TextureIndex;
  int NormalIndex;
};

static tracker_element NewTrackerElement(int ArrayIndex, int VerticeIndex, int TextureIndex, int NormalIndex)
{
  tracker_element Result = {};
  Result.ArrayIndex = ArrayIndex;
  Result.VerticeIndex = VerticeIndex;
  Result.TextureIndex = TextureIndex;
  Result.NormalIndex = NormalIndex;
  return Result;
}

static inline size_t CantorPair(size_t a, size_t b)
{
  size_t Result = (a + b) * (a + b + 1) / 2 + b;
  return Result;
}

static size_t GetCantorTriplet(const tracker_element& Element)
{
  size_t CantorPair    = obj::mapper::CantorPair(Element.VerticeIndex, Element.TextureIndex);
  size_t CantorTriplet = obj::mapper::CantorPair(CantorPair, Element.NormalIndex);
  return CantorTriplet;
}

static bool IsEmpty(const tracker_element& Element)
{
  bool Result =  Element.VerticeIndex == 0 &&
                Element.TextureIndex == 0 &&
                Element.NormalIndex  == 0;
  return Result;
}

static bool Equals(const tracker_element& A, const tracker_element& B)
{
  bool Result =  A.VerticeIndex == B.VerticeIndex &&
                A.TextureIndex == B.TextureIndex &&
                A.NormalIndex  == B.NormalIndex;
  return Result;
}

static bool Exists(int ArraySize, tracker_element* TrackerArray, const tracker_element& NewElement, int* ResultIndex)
{
  size_t CantorTriplet = GetCantorTriplet(NewElement);
  int Idx = CantorTriplet % ArraySize;

  tracker_element ExistingElement = TrackerArray[Idx];
  do
  {
    if(IsEmpty(ExistingElement))
    {
      *ResultIndex = Idx;
      return false;
    }else if(Equals(ExistingElement, NewElement)){
      *ResultIndex = Idx;
      return true;
    }else{
      Idx = (Idx+1)%ArraySize;
      ExistingElement = TrackerArray[Idx];
    }
  }while(true);
  
  INVALID_CODE_PATH;

  return false;
}


static asset::geometry CreateGeometry(
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
  
  asset::geometry Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = IndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.Vertex = Vertex;
  Result.VertexNormal = VertexNormal;
  Result.TextureVertexSetCount = TextureVertexSetCount;
  Result.TextureVertices = TextureVertexSet;
  Result.Topology = asset::geometry::topology::TRIANGLES;

  return Result;
}

struct image_id_list {
  asset::image_id Image;
  image_id_list* Next;
};

struct material_map {
  int MaterialCount;
  asset::phong_material_id* Materials;
  mtl_material** Mtl_Materials;

  int ImageCount;
  image_id_list* Images;
  image_id_list* ImageTail;

  int GeometryCount;
  asset::geometry_id* GeometryIds;
  obj_group** ObjGrp;
};

inline static material_map
CreateMaterialMap(int MaterialCount, int ObjGrpCount)
{
  material_map Result = {};
  Result.MaterialCount = MaterialCount;
  Result.Materials     = PushArray(GlobalTransientArena, MaterialCount, asset::phong_material_id);
  Result.Mtl_Materials = PushArray(GlobalTransientArena, MaterialCount, mtl_material*);
  Result.GeometryIds   = PushArray(GlobalTransientArena, ObjGrpCount, asset::geometry_id);
  Result.ObjGrp        = PushArray(GlobalTransientArena, ObjGrpCount, obj_group*);
  return Result;
}

void PushImageToMap(material_map* MaterialMap, asset::image_id Image) {
  image_id_list* ImageListElement = PushStruct(GlobalTransientArena, image_id_list);
  ImageListElement->Image = Image;
  if(MaterialMap->Images)
  {
    MaterialMap->ImageTail->Next = ImageListElement;
    MaterialMap->ImageTail = ImageListElement;
  }else{
    Assert(MaterialMap->ImageCount == 0);
    MaterialMap->Images = ImageListElement;
    MaterialMap->ImageTail = MaterialMap->Images;
  }
  MaterialMap->ImageCount++;
}

inline static asset::phong_material_id
GetMaterial(material_map* MaterialMap, mtl_material* Mtl){
  for (int i = 0; i < MaterialMap->MaterialCount; ++i)
  {
    if(Mtl == MaterialMap->Mtl_Materials[i])
    {
      return MaterialMap->Materials[i];
    }
  }
  return 0;
}

inline static asset::geometry_id
GetGeometry(material_map* MaterialMap, obj_group* Obj){
  for (int i = 0; i < MaterialMap->GeometryCount; ++i)
  {
    if(Obj == MaterialMap->ObjGrp[i])
    {
      return MaterialMap->GeometryIds[i];
    }
  }
  return 0;
}


static asset::geometry ToGeometry(obj_group* ObjGrp, obj_mesh_data* MeshData)
{
  obj_mesh_indeces* Indeces = ObjGrp->Indeces;
  asset::geometry Result = CreateGeometry(Indeces->Count,
    Indeces->vi, Indeces->ni,  Indeces->ti,
    MeshData->v, MeshData->vn, MeshData->vt);

  Result.AABB = ObjGrp->aabb;

  return Result;
}

static asset::mesh::primitive ToPrimitive(obj_group* ObjectGroup,  material_map* MaterialMap)
{
  asset::mesh::primitive Result = {};
  obj_mesh_indeces* Indeces = ObjectGroup->Indeces;
  Result.Geometry = GetGeometry(MaterialMap, ObjectGroup);
  Result.PhongMaterial = GetMaterial(MaterialMap, ObjectGroup->Material);

  return Result;
}

static asset::image ToImage(const obj_bitmap* ObjBitmap)
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

static asset::phong_material ToPhongMaterial(const mtl_material* ObjMtl)
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


static asset::image_id LoadObjBitmap(const char* UniqueName, const char* Postfix, obj_bitmap* Bitmap)
{
  asset::image_id Result = 0;

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

static material_map LoadPhongMaterials(obj_mtl_data* ObjMtlGroup, u32 ObjGrpCount, const c8* UniqueName)
{
  material_map MaterialMap = CreateMaterialMap(ObjMtlGroup->MaterialCount,ObjGrpCount);
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
      asset::image_id Handle = LoadObjBitmap(UniqueMtlName,  "_BumpMap", Mtl->BumpMap);
      PushImageToMap(&MaterialMap, Handle);
      Material.BumpMap = asset::DefaultTexture(Handle);
      Material.HasBumpMap = true;
    }

    if(Mtl->MapKd)
    {
      asset::image_id Handle = LoadObjBitmap(UniqueMtlName,  "_DiffuseMap", Mtl->MapKd);
      PushImageToMap(&MaterialMap, Handle);
      Material.HasDiffuseTexture = true;
      Material.DiffuseTexture = asset::DefaultTexture(Handle);
    }

    if(Mtl->MapKs)
    {
      asset::image_id Handle = LoadObjBitmap(UniqueMtlName,  "_SpecularMap", Mtl->MapKs);
      PushImageToMap(&MaterialMap, Handle);
      Material.HasSpecularTexture = true;
      Material.SpecularTexture = asset::DefaultTexture(Handle);
    }

    asset::key Key = 0;
    asset::LoadPhongMaterial(UniqueMtlName, &Material, &Key);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = Key;
  }
  return MaterialMap;
}


static asset::mesh_id LoadMesh(const char* UniqueName, obj_loaded_file* Obj, material_map* MaterialMap)
{  
  
  asset::geometry_id* GeometryIDs = PushArray(GlobalTransientArena, Obj->ObjectCount, asset::geometry_id);
  for (int i = 0; i < Obj->ObjectCount; ++i) {
    obj_group* ObjectGroup = &Obj->ObjectGroups[i];
    c8* GeometryName = asset::CreateUniqueName(UniqueName, "_", "geometry",  i, Obj->ObjectCount);
    asset::geometry Geometry = ToGeometry(ObjectGroup, Obj->MeshData);
    asset::LoadGeometry(GeometryName, &Geometry, &GeometryIDs[i]);

    MaterialMap->ObjGrp[MaterialMap->GeometryCount] = ObjectGroup;
    MaterialMap->GeometryIds[MaterialMap->GeometryCount] = GeometryIDs[i];
    MaterialMap->GeometryCount++;
  }

  asset::mesh Mesh = {};
  Mesh.PrimitiveCount = Obj->ObjectCount;
  Mesh.Primitives = PushArray(GlobalTransientArena, Obj->ObjectCount, asset::mesh::primitive);
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    Mesh.Primitives[i] = ToPrimitive(&Obj->ObjectGroups[i], MaterialMap);
  }

  asset::mesh_id ResultKey = 0;
  c8* MeshName = asset::CreateUniqueName(UniqueName,"_", Obj->ObjectNameLength ? Obj->ObjectName : "_mesh");
  asset::LoadMesh(MeshName, &Mesh, &ResultKey);
  return ResultKey;
}

static asset::render_tree CreateRenderTree(asset::mesh_id MeshId)
{
  asset::render_tree Result = asset::render_tree::Create(true, TransientMalloc, TransientFree);
  asset::render_tree_data Data = {};
  Data.Mesh = MeshId;
  Result.NewNode(NULL, Data);
  return Result;
}

static void* TransientAllocator(uint32_t MemorySize) {
  void* Result = PushSize(GlobalTransientArena, MemorySize);
  return Result;
}

asset::package_id LoadObj(const c8* Path, const c8* UniqueName)
{
  Assert(Path && *Path != '\0');
  if(!UniqueName || *UniqueName == '\0')
  {
    UniqueName = Path;
  }

  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  asset::package Package = {};

  material_map MaterialMap = LoadPhongMaterials(Obj->MaterialData, Obj->ObjectCount, UniqueName);
  Package.PhongMaterialCount = MaterialMap.MaterialCount;
  Package.PhongMaterials = MaterialMap.Materials;

  Package.ImageCount = MaterialMap.ImageCount;
  Package.Images = PushArray(GlobalTransientArena, Package.ImageCount, asset::image_id);
  image_id_list* ImageElement = MaterialMap.Images;
  int ImageIndex = 0;
  while(ImageElement)
  {
    Package.Images[ImageIndex++] = ImageElement->Image;
    ImageElement = ImageElement->Next;
  }
  Assert(ImageIndex == Package.ImageCount);
  
  Package.MeshCount = 1;
  Package.Meshes = PushArray(GlobalTransientArena, Package.MeshCount, asset::mesh_id);
  Package.Meshes[0] = LoadMesh(UniqueName, Obj, &MaterialMap);

  Package.RenderTreeCount = 1;
  Package.RenderTrees = PushArray(GlobalTransientArena, Package.RenderTreeCount, asset::render_tree_id);
  asset::render_tree RenderTree = CreateRenderTree(Package.Meshes[0]);
  asset::LoadRenderTree(UniqueName, Path, &RenderTree, Package.RenderTrees);
  
  asset::package_id ResultKey = 0;
  asset::LoadPackage(UniqueName, Path, &Package, &ResultKey);

  return ResultKey;
}

} // namespace mapper
} // namespace obj
