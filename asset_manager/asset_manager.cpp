#include "asset_manager.h"
#include "platform/obj_loader.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

extern asset::manager* GlobalAssetManager;
extern memory_arena* GlobalTransientArena;

namespace asset {

u32 ToKey(type Type, c8* UniqueName)
{
  c8 TempKeyString[ASSET_MAX_KEY_LENGTH] = {};

  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  u32 UniqueNameLength = jstr::StringLength(UniqueName);
  
  string KeyString = {};
  u32 KeyStringLength = TypeLength + UniqueNameLength + 2;
  u32 FormattedLength = FormatString(TempKeyString, KeyStringLength+1, "%s::%s", TypeString, UniqueName);
  Assert(FormattedLength <= KeyStringLength);
  u32 Result = utils::djb2_hash(TempKeyString);
  return Result;
}

header* CreateHeader(type Type, c8* UniqueName, c8* Name, c8* Path, u32 DataSize)
{
  Assert(UniqueName && *UniqueName != '\0');
  // Layout of any memory allocated in the asset_manager is -> | HEADER | DATA | NAME | PATH | KEY |
  // This way, anyone who has a pointer to DATA can get the header by just rewinding the pointer sizeof(header) bytes.

  u32 NameLength = jstr::StringLength(Name);
  u32 NameSize = (NameLength+1)* sizeof(c8);
  u32 PathLength = jstr::StringLength(Path);
  u32 PathSize = (PathLength+1)* sizeof(c8);
  u32 UniqueNameLength = jstr::StringLength(UniqueName);
  u32 UniqueNameSize = (UniqueNameLength+1)* sizeof(c8);

  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  u32 KeyStringLength = TypeLength + UniqueNameLength + 4;
  u32 KeyStringSize   = (KeyStringLength+1) * sizeof(c8);

  u32 HeaderSize = sizeof(header) + NameSize + PathSize + KeyStringLength;
  u32 MemorySize = HeaderSize + DataSize;
  header* Result = (header*) Allocate(&GlobalAssetManager->Memory, MemorySize);

  Result->Type = Type;
  Result->DataSize = DataSize;
  Result->Name.Length = NameLength;
  Result->FilePath.Length = PathLength;
  Result->KeyString.Length = KeyStringLength;

  Result->Data             = AdvanceBytePointer(Result, sizeof(header));
  Result->Name.String      = (c8*) AdvanceBytePointer(Result, sizeof(header) + DataSize);
  Result->FilePath.String  = (c8*) AdvanceBytePointer(Result, sizeof(header) + DataSize + NameSize);
  Result->KeyString.String = (c8*) AdvanceBytePointer(Result, sizeof(header) + DataSize + NameSize + PathSize);

  jstr::CopyStrings(Result->Name.Length, Name, Result->Name.Length+1, Result->Name.String);
  jstr::CopyStrings(Result->FilePath.Length, Path, Result->FilePath.Length+1, Result->FilePath.String);
  FormatString(Result->KeyString.String, KeyStringSize, "%s::%s", TypeString, UniqueName);

  Result->Key  = utils::djb2_hash(Result->KeyString.String);
  Insert(&GlobalAssetManager->Headers, Result->Key, (void*) Result);

  return Result;
}

midx GetMeshSize(u32 IndexCount, u32 VertexCount, u32 NormalCount, u32 TextureCount) {
  u32  TypeCount = ((VertexCount>0) + (NormalCount>0) + (TextureCount>0));
  midx IndexMemSize   = TypeCount * IndexCount * sizeof(u32);
  midx VerticeMemSize = VertexCount  * sizeof(v3);
  midx NormalMemSize  = NormalCount  * sizeof(v3);
  midx TextureMemSize = TextureCount * sizeof(v2);
  midx TotalMeshSize = sizeof(mesh) + IndexMemSize + VerticeMemSize + NormalMemSize + TextureMemSize;
  return TotalMeshSize;
}

internal inline midx GetMeshSize(const mesh* Mesh) {
  midx Result = GetMeshSize(Mesh->IndexCount, Mesh->vCount, Mesh->vnCount, Mesh->vtCount);
  return Result;
}


mesh* InitializeMesh(u32 IndexCount, u32 VertexCount, u32 NormalCount, u32 TextureCount, void* Memory) {
  Assert(IndexCount);
  Assert(VertexCount);

  midx IndexMemSize = IndexCount  * sizeof(u32);

  mesh* Result = (mesh*) Memory;
  Result->IndexCount = IndexCount;

  bptr MemScan = AdvanceBytePointer(Result, sizeof(mesh));
  {
    Result->vCount = VertexCount;
    Result->vi = (u32*) MemScan;
    Result->v  = (v3*)  AdvanceBytePointer(Result->vi, IndexMemSize);
    MemScan = AdvanceBytePointer(Result->v, VertexCount * sizeof(v3));
  }


  if(NormalCount) {
    Result->vnCount = NormalCount;
    Result->vni = (u32*) MemScan;
    Result->vn  = (v3*)  AdvanceBytePointer(Result->vni, IndexMemSize);
    MemScan = AdvanceBytePointer(Result->vn, NormalCount * sizeof(v3));
  }

  if(TextureCount) {
    Result->vtCount = TextureCount;
    Result->vti = (u32*) MemScan;
    Result->vt = (v2*) AdvanceBytePointer(Result->vti, IndexMemSize);
  }
  return Result;
}

b32 IntCompareFunction(const bptr DataA, const bptr DataB)
{
  u32* A =  (u32*) DataA;
  u32* B =  (u32*) DataB;
  return *A == *B;
}

b32 V3CompareFunction(const bptr DataA, const bptr DataB)
{
  v3 A =  *((v3*) DataA);
  v3 B =  *((v3*) DataB);
  return A == B;
}

b32 V2CompareFunction(const bptr DataA, const bptr DataB)
{
  v2 A =  *((v2*) DataA);
  v2 B =  *((v2*) DataB);
  return A == B;
}


u32 PushUnique(bptr Array, const u32 ElementCount, const u32 ElementByteSize,
               bptr NewElement, b32 (*CompareFunction)(const bptr DataA, const bptr DataB))
{
  bptr Scan = Array;
  for( u32 i = 0; i < ElementCount; ++i )
  {
    if( CompareFunction(NewElement, Scan) )
    {
      return i;
    }
    Scan += ElementByteSize;
  }
  
  // If we didn't find the element we push it to the end
  utils::Copy(ElementByteSize, NewElement, Scan);
  
  return ElementCount;
}
/*

// -Z
VerticeIndeces = {4 5 6 4 6 7};
Vertices = {
  {-1,-1, 1},
  {-1, 1, 1},
  { 1, 1, 1},
  { 1,-1, 1},
  {-1,-1,-1},
  {-1, 1,-1},
  { 1, 1,-1},
  { 1,-1,-1},
}


UniqueVerticeIndeces = {4,5,6,7}
UniqueVertices = {
    {-1,-1,-1},
    {-1, 1,-1},
    { 1, 1,-1},
    { 1,-1,-1},
}

VerticeIndeceMap = {4,0} {5,1} {6,2} {6,3}
NewVerticeIndece = {0, 1, 2, 0, 2, 3};


/////
NormalIndeces = {5,5,5,5,5,5}
Normals = {
  {
    { 1,0,0},
    {-1,0,0},
    {0, 1,0},
    {0,-1,0},
    {0,0, 1},
    {0,0,-1},
  }

UniqueNormalIndeces = {5}
UniqueNormals = {0,0,-1},
NormalMap = {5,0}
NewNormalIndeces = {0}


TextureCount = {0,2,3,0,3,1}
Texture = {
  {
    {0,0},
    {1,0},
    {0,1},
    {1,1}
  }

UniqueTextureIndeces = {0,2,3,1}
UniqueTexture = {
  {
    {0,0},
    {0,1},
    {1,1},
    {1,0},
  }
TextureIndeceMap = {0, 0}, {2, 1}, {3,2}, {1,3}
NewTextureIndeces = {0,1,2,0,2,3}

*/

struct indexed_array {
  u32 IndexCount;
  u32* IndexArray;

  midx ValueSize;
  u32 ValueCount;
  bptr ValueArray;
};

indexed_array IndexedArray(memory_arena* Arena, u32 IndexCount, midx ValueSize, u32 ValueCount)
{
  indexed_array Result = {};
  Result.IndexCount = IndexCount;
  Result.ValueSize  = ValueSize;
  Result.ValueCount = ValueCount;
  Result.IndexArray = PushArray(Arena, IndexCount, u32);
  Result.ValueArray = (bptr) PushSize(Arena, ValueCount*ValueSize);
  return Result;
}


indexed_array CreateNewIndexedArray(memory_arena* Arena, u32 IndexCount, const u32* IndexArray, midx ValueSize, bptr ValueArray,
  b32 (*CompareFunction)(const bptr DataA, const bptr DataB))
{
  indexed_array Result = IndexedArray(Arena, IndexCount, ValueSize, IndexCount);
  u32 ValueCount = 0;
  for (int i = 0; i < IndexCount; ++i)
  {
    u32 OldIndex = IndexArray[i];
    bptr Value = ValueArray + OldIndex*ValueSize;
    u32 NewIndex = PushUnique(Result.ValueArray, ValueCount, ValueSize, Value, CompareFunction);
    if(NewIndex == ValueCount)
    {
      ValueCount++;
    }
    Result.IndexArray[i] = NewIndex;
  }
  Result.ValueCount = ValueCount;
  return Result;
}

mesh* CreateMesh( c8* MeshKey, c8* MeshName, c8* MeshPath,
                  const u32 IndexCount,
                  const u32* VerticeIndeces, const u32* NormalIndeces, const u32* TextureIndeces,
                  const v3* VerticeData,     const v3* NormalData,     const v2* TextureData)
{

  indexed_array VerticeArray = CreateNewIndexedArray(GlobalTransientArena, IndexCount, VerticeIndeces, sizeof(v3), (bptr) VerticeData, V3CompareFunction);
  indexed_array NormalArray = {};
  if(NormalIndeces)
  {
     NormalArray = CreateNewIndexedArray(GlobalTransientArena, IndexCount, NormalIndeces, sizeof(v3), (bptr) NormalData, V3CompareFunction);
  }
  indexed_array TextureArray = {};
  if(TextureIndeces)
  {
     TextureArray = CreateNewIndexedArray(GlobalTransientArena, IndexCount, TextureIndeces, sizeof(v2), (bptr) TextureData, V2CompareFunction);
  }

  midx TotalMeshSize = GetMeshSize(IndexCount, VerticeArray.ValueCount, NormalArray.ValueCount, TextureArray.ValueCount);
  header* Header     = CreateHeader(type::MESH, MeshKey, MeshName, MeshPath, TotalMeshSize);
  mesh* Result       = InitializeMesh(IndexCount, VerticeArray.ValueCount, NormalArray.ValueCount, TextureArray.ValueCount, Header->Data);
  

  utils::Copy(sizeof(u32)*VerticeArray.IndexCount,             VerticeArray.IndexArray, Result->vi);
  utils::Copy(VerticeArray.ValueSize*VerticeArray.ValueCount,  VerticeArray.ValueArray, Result->v);
  if(NormalIndeces)
  {
    utils::Copy(sizeof(u32)*NormalArray.IndexCount,             NormalArray.IndexArray, Result->vni);
    utils::Copy(NormalArray.ValueSize*NormalArray.ValueCount,   NormalArray.ValueArray, Result->vn);
  }
  if(TextureIndeces)
  {
    utils::Copy(sizeof(u32)*TextureArray.IndexCount,              TextureArray.IndexArray, Result->vti);
    utils::Copy(TextureArray.ValueSize*TextureArray.ValueCount,   TextureArray.ValueArray, Result->vt);
  }

  return Result;
}

string CreateString(const c8* Str, u32 MaxLength) {
  string Result = {};
  Result.Length = jstr::StringLength(Str);
  Assert(Result.Length < MaxLength);
  Result.String = (c8*) Allocate(&GlobalAssetManager->Memory, (Result.Length+1) * sizeof(c8));
  jstr::CopyStrings(Result.Length, Str, Result.Length, Result.String);
  return Result;
}

void DeleteString(string String) {
  if(String.String)
  {
    FreeMemory(&GlobalAssetManager->Memory, String.String);
  }
}

void* TransientAllocator(u32 MemorySize) {
  void* Result = PushSize(GlobalTransientArena, MemorySize);
  return Result;
}

header* FindHeader(u32 Key) {
  header* Result = (header*) Find(&GlobalAssetManager->Headers, Key);
  return Result;
}

void FreeAsset(header* Header)
{
  switch(Header->Type)
  {
    case type::MESH: {
      // LoadGLVertexBuffer allocates the whole mesh as a contious block
      FreeMemory(&GlobalAssetManager->Memory, Header);
    }
    case type::MATERIAL: {
      // LoadGLVertexBuffer allocates the whole mesh as a contious block
      FreeMemory(&GlobalAssetManager->Memory, Header);
    }
    case type::RENDER_GROUP: {
      render_group* RenderGroup = (render_group*) Header->Data;
      for (int i = 0; i < RenderGroup->ElementCount; ++i)
      {
        render_group_element* Element = RenderGroup->Elements + i;
        header* MeshHeader = (header*) RetreatByType(Element->Mesh, header);
        Assert(MeshHeader->Type == type::MESH);
        FreeAsset(MeshHeader);

        header* MaterialHeader = (header*) RetreatByType(Element->Material, header);
        if(MaterialHeader->Type == type::MATERIAL)
        {
          // If several elements point to the same material, this should ensure we only free a material once.
          FreeAsset(MaterialHeader);
        }

        FreeMemory(&GlobalAssetManager->Memory, Header);

      }
    }
    default: {
      INVALID_CODE_PATH
    };
  }
}

void* Find(type Type, u32 Key) {
  header* Header = FindHeader(Key);
  void* Result = 0;
  if(Header)
  {
    Result = Header->Data;
  }

  return Result;
}

void* Find(type Type, c8* Name) {
  u32 Key = ToKey(Type, Name);
  void* Result = Find(Type, Key);
  return Result;
}

void Free(type Type, u32 Key)
{
  header* Header = FindHeader(Key);
  if(Header)
  {
    FreeAsset(Header);
  }
}

void Free(type Type, c8* Name) {
  u32 Key = ToKey(Type, Name);
  Free(Type, Key);
}

texture* CopyObjBitmapToTexture(c8* Key, texture_type Type, obj_bitmap* ObjBitmap)
{
  if(!ObjBitmap){return 0;};

  u32 TextureSizeBytes = sizeof(texture) + ObjBitmap->Width * ObjBitmap->Height * ObjBitmap->BPP / 8.f;

  header* Header = CreateHeader(type::TEXTURE, Key, ObjBitmap->Name, ObjBitmap->Path, TextureSizeBytes);
  texture* Result = (texture*) Header->Data;
  Result->Type    = Type;
  Result->BPP     = ObjBitmap->BPP;
  Result->Width   = ObjBitmap->Width;
  Result->Height  = ObjBitmap->Height;
  Result->Pixels  = AdvanceBytePointer(Result, sizeof(texture));
  utils::Copy(TextureSizeBytes, ObjBitmap->Pixels, Result->Pixels);
  return Result;
}

midx MaterialSize(
    v4* Kd,
    v4* Ka,
    v4* Tf,
    v4* Ks,
    v4* Ke,
    r32* d,
    r32* Ni,
    r32* Ns,
    u32* IlluminationMode
  )
{
  u32 KdSize = (u32) BranchlessArithmatic(Kd == 0, 0, sizeof(v4));
  u32 KaSize = (u32) BranchlessArithmatic(Ka == 0, 0, sizeof(v4));
  u32 TfSize = (u32) BranchlessArithmatic(Tf == 0, 0, sizeof(v4));
  u32 KsSize = (u32) BranchlessArithmatic(Ks == 0, 0, sizeof(v4));
  u32 KeSize = (u32) BranchlessArithmatic(Ke == 0, 0, sizeof(v4));
  u32 dSize  = (u32) BranchlessArithmatic(d  == 0, 0, sizeof(r32));
  u32 NiSize = (u32) BranchlessArithmatic(Ni == 0, 0, sizeof(r32));
  u32 NsSize = (u32) BranchlessArithmatic(Ns == 0, 0, sizeof(r32));
  u32 MaterialSizeBytes = sizeof(material) + KdSize + KaSize + TfSize + KsSize + KeSize + dSize + NiSize + NsSize;
  return MaterialSizeBytes;
}

void InitiateMaterial (
    v4* Kd,
    v4* Ka,
    v4* Tf,
    v4* Ks,
    v4* Ke,
    r32* d,
    r32* Ni,
    r32* Ns,
    r32 BumpMapBM,
    texture* BumpMap,
    texture* MapKd,
    texture* MapKs,
    u32* IlluminationMode, 
    material* Material
  )
{
  u32 KdSize = BranchlessArithmatic(Kd == 0, 0, sizeof(v4));
  u32 KaSize = BranchlessArithmatic(Ka == 0, 0, sizeof(v4));
  u32 TfSize = BranchlessArithmatic(Tf == 0, 0, sizeof(v4));
  u32 KsSize = BranchlessArithmatic(Ks == 0, 0, sizeof(v4));
  u32 KeSize = BranchlessArithmatic(Ke == 0, 0, sizeof(v4));
  u32 dSize  = BranchlessArithmatic(d  == 0, 0, sizeof(r32));
  u32 NiSize = BranchlessArithmatic(Ni == 0, 0, sizeof(r32));
  u32 NsSize = BranchlessArithmatic(Ns == 0, 0, sizeof(r32));
  u32 MaterialSizeBytes = sizeof(material) + KdSize + KaSize + TfSize + KsSize + KeSize + dSize + NiSize + NsSize;

  if(Kd) {
    Material->Kd = (v4*)  AdvanceBytePointer(Material, sizeof(material));
    *Material->Kd = *Kd;
  }
  if(Ka) {
    Material->Ka = (v4*)  AdvanceBytePointer(Material, sizeof(material) + KdSize);
    *Material->Ka = *Ka;
  }
  if(Tf) {
    Material->Tf = (v4*)  AdvanceBytePointer(Material, sizeof(material) + KdSize + KaSize);
    *Material->Tf = *Tf;
  }
  if(Ks) {
    Material->Ks = (v4*)  AdvanceBytePointer(Material, sizeof(material) + KdSize + KaSize + TfSize);
    *Material->Ks = *Ks;
  }
  if(Ke) {
    Material->Ke = (v4*)  AdvanceBytePointer(Material, sizeof(material) + KdSize + KaSize + TfSize + KsSize);
    *Material->Ke = *Ke;
  }
  if(d) {
    Material->d  = (r32*) AdvanceBytePointer(Material, sizeof(material) + KdSize + KaSize + TfSize + KsSize + KeSize);
    *Material->d = *d;
  }
  if(Ni) {
    Material->Ni = (r32*) AdvanceBytePointer(Material, sizeof(material) + KdSize + KaSize + TfSize + KsSize + KeSize + dSize);
    *Material->Ni = *Ni;
  }
  if(Ns) {
    Material->Ns = (r32*) AdvanceBytePointer(Material, sizeof(material) + KdSize + KaSize + TfSize + KsSize + KeSize + dSize + NiSize);
    *Material->Ns = *Ns;
  }
  
  Material->BumpMapBM = BumpMapBM;
  Material->BumpMap   = BumpMap;
  Material->MapKd     = MapKd;
  Material->MapKs     = MapKs;
}

material* CopyObjMtlToMaterial(c8* Path, mtl_material* ObjMtl, c8* Key)
{
  midx MaterialSizeBytes = MaterialSize(ObjMtl->Kd, ObjMtl->Ka, ObjMtl->Tf, ObjMtl->Ks, ObjMtl->Ke, ObjMtl->d, ObjMtl->Ni, ObjMtl->Ns, ObjMtl->IlluminationMode);
  header* Header   = CreateHeader(type::MATERIAL, Key, ObjMtl->Name, Path, MaterialSizeBytes);
  texture* BumpMap = CopyObjBitmapToTexture(Key, texture_type::BUMP_MAP, ObjMtl->BumpMap);
  texture* MapKd   = CopyObjBitmapToTexture(Key, texture_type::DIFFUSE_COLOR, ObjMtl->MapKd);
  texture* MapKs   = CopyObjBitmapToTexture(Key, texture_type::SPECULAR_COLOR, ObjMtl->MapKs);

  material* Result = (material*) Header->Data;
  InitiateMaterial(
    ObjMtl->Kd, ObjMtl->Ka, ObjMtl->Tf, ObjMtl->Ks, ObjMtl->Ke, ObjMtl->d, ObjMtl->Ni, ObjMtl->Ns,
    ObjMtl->BumpMapBM, BumpMap, MapKd, MapKs, ObjMtl->IlluminationMode,
    Result);

  return Result;
}

struct material_map {
  u32 MaterialCount;
  material** Materials;
  mtl_material** Mtl_Materials;
};

material_map CreateMaterialMap(u32 MaterialCount)
{
  material_map Result = {};
  Result.MaterialCount = MaterialCount;
  Result.Materials     = PushArray(GlobalTransientArena, MaterialCount, material*);
  Result.Mtl_Materials = PushArray(GlobalTransientArena, MaterialCount, mtl_material*);
  return Result;
}

material* GetMaterial(material_map* MaterialMap, mtl_material* Mtl){
  for (int i = 0; i < MaterialMap->MaterialCount; ++i)
  {
    if(Mtl == MaterialMap->Mtl_Materials[i])
    {
      return MaterialMap->Materials[i];
    }
  }
  return 0;
}

render_group_element CreateRenderGroupElement(c8* Key, c8* Name, c8* Path, obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
{  
  render_group_element Result = {};

  obj_mesh_indeces* ObjIndeces = ObjGrp->Indeces;
  Result.SmoothingGroup = -1;
  if(ObjGrp->SmoothingGroup) {
    Result.SmoothingGroup =  *ObjGrp->SmoothingGroup;
  }

  Result.Mesh = CreateMesh(
      Key,
      ObjGrp->GroupName,
      Path,
      ObjIndeces->Count,
      ObjIndeces->vi,
      ObjIndeces->ni,
      ObjIndeces->ti,
      MeshData->v,
      MeshData->vn,
      MeshData->vt);

  Result.Material = GetMaterial(MaterialMap, ObjGrp->Material);
  return Result;
}


c8* CreateUniqueKey(c8* Name, u32 Index, u32 MaxCount)
{
  c8* Result = Name;
  if(MaxCount > 1)
  {
    u32 Length = ASSET_MAX_NAME_LENGTH;
    Result = (c8*) PushArray(GlobalTransientArena, Length, c8);
    FormatString(Result, Length-1, "%s_%d/%d", Name, Index+1, MaxCount);
  }
  return Result;
}

u32 LoadObj(c8* Path, c8* KeyString)
{
  Assert(Path && *Path != '\0');
  if(!KeyString || *KeyString == '\0')
  {
    KeyString = Path;
  }

  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  

  // MATERIAL
  obj_mtl_data* ObjMtlGroup = Obj->MaterialData;
  material_map MaterialMap = CreateMaterialMap(ObjMtlGroup->MaterialCount);
  for (int i = 0; i < ObjMtlGroup->MaterialCount; ++i)
  {
    mtl_material* Mtl = ObjMtlGroup->Materials + i;
    c8* MtlKey = CreateUniqueKey(KeyString, i, Obj->ObjectCount);
    material* Material = CopyObjMtlToMaterial(ObjMtlGroup->Path, Mtl, MtlKey);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = Material;
  }

  // RENDER_GROUP
  midx RenderGroupMemSize = sizeof(render_group) + Obj->ObjectCount * sizeof(render_group_element);
  header* Header = CreateHeader(type::RENDER_GROUP, KeyString, Obj->ObjectName, Path, RenderGroupMemSize);
  render_group* RenderGroup = (render_group*) Header->Data;
  RenderGroup->ElementCount = Obj->ObjectCount;
  RenderGroup->Elements = (render_group_element*) AdvanceBytePointer(RenderGroup, sizeof(render_group));

  // RENDER_GROUP_ELEMENT
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    obj_group* ObjGrp = Obj->ObjectGroups + i;
    c8 MeshNameBuff[ASSET_MAX_NAME_LENGTH] = {};
    c8* MeshName = CreateUniqueKey(KeyString, i, Obj->ObjectCount);
    RenderGroup->Elements[i] = CreateRenderGroupElement(KeyString, MeshName, Path, ObjGrp, Obj->MeshData, &MaterialMap);
  }

  return Header->Key;
}

void CopyMesh(const mesh* SrcMesh, mesh* DstMesh)
{
  Assert(SrcMesh->v && SrcMesh->vi && DstMesh->v && DstMesh->vi);
  DstMesh->IndexCount  = SrcMesh->IndexCount;
  DstMesh->vCount = SrcMesh->vCount;
  utils::Copy(SrcMesh->IndexCount  * sizeof(u32), SrcMesh->vi, DstMesh->vi);
  utils::Copy(SrcMesh->vCount * sizeof(v3),       SrcMesh->v, DstMesh->v);


  if(SrcMesh->vn){
    DstMesh->vnCount = SrcMesh->vnCount;
    utils::Copy(SrcMesh->IndexCount * sizeof(u32), SrcMesh->vni, DstMesh->vni);
    utils::Copy(SrcMesh->vnCount * sizeof(v3),     SrcMesh->vn, DstMesh->vn);
  }
  if(SrcMesh->vt){ 
    DstMesh->vtCount = SrcMesh->vtCount;
    utils::Copy(SrcMesh->IndexCount * sizeof(u32), SrcMesh->vti, DstMesh->vti);
    utils::Copy(SrcMesh->vtCount * sizeof(v2), SrcMesh->vt, DstMesh->vt);
  }
}


mesh* LoadMesh(c8* KeyString, const mesh* Mesh, u32* ResultKey)
{
  midx MeshSize = GetMeshSize(Mesh);
  header* Header = CreateHeader(type::MESH, KeyString, KeyString, "N/A", MeshSize);
  mesh* LoadedMesh = InitializeMesh(Mesh->IndexCount, Mesh->vCount, Mesh->vnCount, Mesh->vtCount, Header->Data);
  CopyMesh(Mesh, LoadedMesh);

  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return LoadedMesh;
}

gl_vertex_buffer* LoadGLVertexBuffer(c8* Name, const gl_vertex_buffer Data, u32* ResultKey){ return 0; }

u32 LoadTga(c8* Path, texture_type Type, c8* UniqueName)
{
  obj_bitmap* ObjBitmap = LoadTGA(TransientAllocator, Path);
  if(UniqueName == 0 || *UniqueName =='\0')
  {
    UniqueName = Path;
  }
  texture* Texture = CopyObjBitmapToTexture(UniqueName, Type, ObjBitmap);
  header* Header = (header*) RetreatByType(Texture, header);
  return Header->Key; 
}

}