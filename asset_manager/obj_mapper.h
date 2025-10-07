#pragma once

#include "asset_types.h"
#include "io/obj.h"

struct material_map {
  u32 MaterialCount;
  asset::phong_material** Materials;
  mtl_material** Mtl_Materials;
};

static material_map CreateMaterialMap(u32 MaterialCount)
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

asset::render_group_element CreateRenderGroupElement(const c8* Key, const c8* Name, const c8* Path, obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
{  
  asset::render_group_element Result = {};

  obj_mesh_indeces* ObjIndeces = ObjGrp->Indeces;
  Result.SmoothingGroup = -1;
  if(ObjGrp->SmoothingGroup) {
    Result.SmoothingGroup =  *ObjGrp->SmoothingGroup;
  }

  Result.Mesh = asset::CreateMesh(
      Key,
      ObjGrp->GroupName,
      Path,
      ObjIndeces->Count,
      ObjIndeces->vi,
      ObjIndeces->ni,
      ObjIndeces->ti,
      MeshData->nv,
      MeshData->nvn,
      MeshData->nvt,
      MeshData->v,
      MeshData->vn,
      MeshData->vt);

  Result.Material = GetMaterial(MaterialMap, ObjGrp->Material);
  return Result;
}

static void* TransientAllocator(u32 MemorySize) {
  void* Result = PushSize(GlobalTransientArena, MemorySize);
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


static u32 LoadObjBitmap(const char* UniqueName, const char* Postfix, obj_bitmap* Bitmap)
{
  u32 Result = 0;

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
      u32 Handle = LoadObjBitmap(UniqueName,  "_BumpMap", Mtl->BumpMap);
      Material.BumpMap = asset::DefaultTexture(Handle);
      Material.HasBumpMap = true;
    }

    if(Mtl->MapKd)
    {
      u32 Handle = LoadObjBitmap(UniqueName,  "_DiffuseMap", Mtl->MapKd);
      Material.HasDiffuseTexture = true;
      Material.DiffuseTexture = asset::DefaultTexture(Handle);
    }

    if(Mtl->MapKs)
    {
      u32 Handle = LoadObjBitmap(UniqueName,  "_SpecularMap", Mtl->MapKs);
      Material.HasSpecularTexture = true;
      Material.SpecularTexture = asset::DefaultTexture(Handle);
    }

    u32 Key = 0;
    
    asset::phong_material* LoadedMaterial = asset::LoadMaterial(UniqueName, &Material, &Key);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = LoadedMaterial;
  }
  return MaterialMap;
}

static u32 LoadObj(const c8* Path, const c8* UniqueName)
{
  Assert(Path && *Path != '\0');
  if(!UniqueName || *UniqueName == '\0')
  {
    UniqueName = Path;
  }

  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  // Upload MATERIAL and IMAGES related to material
  material_map MaterialMap = LoadPhongMaterial(Obj->MaterialData, UniqueName);

  // RENDER_GROUP
  midx RenderGroupMemSize = sizeof(asset::render_group) + Obj->ObjectCount * sizeof(asset::render_group_element);
  asset::header* Header = CreateHeader(asset::type::RENDER_GROUP, UniqueName, Obj->ObjectName, Path, RenderGroupMemSize);
  asset::render_group* RenderGroup = (asset::render_group*) Header->Data;
  
  u32 ActualElementCount = {};
  RenderGroup->Elements = (asset::render_group_element*) AdvanceBytePointer(RenderGroup, sizeof(asset::render_group));

  // RENDER_GROUP_ELEMENT
  u32 ElementCount = 0;
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    obj_group* ObjGrp = Obj->ObjectGroups + i;
    if(ObjGrp->Indeces->Count)
    {
      // Note: The reason we have to check for ObjGrp->Indeces.Count is because we are not handling splines and surfaces in the obj_loader
      //       if we see a spline or a surface we create a new empty object group. For now we are fine allocating a bit of extra space 
      //       but this should be taken care of once we implement surfaces and splines etc.
      c8 MeshNameBuff[ASSET_MAX_NAME_LENGTH] = {};
      c8* MeshName = asset::CreateUniqueName("", UniqueName, "", i, Obj->ObjectCount);
      RenderGroup->Elements[i] = CreateRenderGroupElement(UniqueName, MeshName, Path, ObjGrp, Obj->MeshData, &MaterialMap);
      ElementCount++;
    }
  }
  RenderGroup->ElementCount = ElementCount;

  return Header->Key;
}


asset::gltf_tmp::mesh* MapObjMesh( obj_loaded_file* ObjFile )
{
  return 0;
}
