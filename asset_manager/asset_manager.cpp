#include "asset_manager.h"
#include "platform/obj_loader.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

extern asset::manager* GlobalAssetManager;
extern memory_arena* GlobalTransientArena;

namespace asset {

struct header {
  type Type;
  u32 Key;
  string Name;
  string FilePath;
  string KeyString;
  midx DataSize;
  void* Data;
};

u32 ToKey(type Type, c8* Name)
{
  c8 TempKeyString[ASSET_MAX_KEY_LENGTH] = {};

  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  u32 NameLength = jstr::StringLength(Name);
  
  string KeyString = {};
  u32 KeyStringLength = TypeLength + NameLength + 2;
  u32 FormattedLength = FormatString(TempKeyString, KeyStringLength+1, "%s::%s", TypeString, Name);
  Assert(FormattedLength <= KeyStringLength);
  u32 Result = utils::djb2_hash(TempKeyString);
  return Result;
}

header* CreateHeader(type Type, c8* Name, c8* Path, u32 DataSize)
{
  // Layout of any memory allocated in the asset_manager is -> | HEADER | DATA | NAME | PATH | KEY |
  // This way, anyone who has a pointer to DATA can get the header by just rewinding the pointer sizeof(header) bytes.

  u32 NameLength = jstr::StringLength(Name);
  u32 NameSize = (NameLength+1)* sizeof(c8);
  u32 PathLength = jstr::StringLength(Path);
  u32 PathSize = (PathLength+1)* sizeof(c8);
  
  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  u32 KeyStringLength = TypeLength + PathLength + NameLength + 4;
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
  FormatString(Result->KeyString.String, KeyStringSize, "%s::%s::%s", TypeString, Path, Name);

  Result->Key  = utils::djb2_hash(Result->KeyString.String);
  return Result;
}

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

midx GetMeshSize(u32 IndexCount, u32 VertexCount, b32 HasNormals, b32 HasTextures) {
  midx IndexMemSize   = IndexCount * sizeof(u32);
  midx VerticeMemSize = VertexCount * sizeof(v3);
  midx NormalMemSize  = BranchlessArithmatic(HasNormals,  VertexCount * sizeof(v3), 0);
  midx TextureMemSize = BranchlessArithmatic(HasTextures, VertexCount * sizeof(v2), 0);
  midx TotalMeshSize = sizeof(mesh) + IndexMemSize + VerticeMemSize + NormalMemSize + TextureMemSize;
  return TotalMeshSize;
}

mesh* InitializeMesh(u32 IndexCount, u32 VertexCount, b32 HasNormals, b32 HasTextures, void* Memory) {
  midx IndexMemSize   = IndexCount * sizeof(u32);
  midx VerticeMemSize = VertexCount * sizeof(v3);
  midx NormalMemSize  = BranchlessArithmatic(HasNormals,  VertexCount * sizeof(v3), 0);
  
  mesh* Result = (mesh*) Memory;
  Result->IndexCount = IndexCount;
  Result->VertexCount = VertexCount;
  Result->Indeces = (u32*) AdvanceBytePointer(Result, sizeof(mesh));
  Result->v       = (v3*)  AdvanceBytePointer(Result, sizeof(mesh) + IndexMemSize);
  if(HasNormals) {
    Result->vn = (v3*) AdvanceBytePointer(Result, sizeof(mesh) + IndexMemSize + VerticeMemSize);
  }
  if(HasTextures) {
    Result->vt = (v2*) AdvanceBytePointer(Result, sizeof(mesh) + IndexMemSize + VerticeMemSize + NormalMemSize);
  }
  return Result;
}

mesh* CreateMesh( c8* MeshName, c8* MeshPath,
                  const u32 IndexCount,
                  const u32* VerticeIndeces, const u32* TextureIndeces, const u32* NormalIndeces,
                  const v3* VerticeData,     const v2* TextureData,     const v3* NormalData)
{
  u32* GLVerticeIndexArray  = PushArray(GlobalTransientArena, 3*IndexCount, u32);
  u32* GLIndexArray         = PushArray(GlobalTransientArena, IndexCount, u32);

  u32 VerticeArrayCount = 0;
  for( u32 i = 0; i < IndexCount; ++i )
  {
    const u32 vidx = VerticeIndeces[i];
    const u32 tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const u32 nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    u32 NewElement[3] = {vidx, tidx, nidx};
    u32 Index = PushUnique((u8*)GLVerticeIndexArray, VerticeArrayCount, sizeof(NewElement), (u8*) NewElement,
                           [](const u8* DataA, const u8* DataB) {
                             u32* U32A = (u32*) DataA;
                             const u32 A1 = *(U32A+0);
                             const u32 A2 = *(U32A+1);
                             const u32 A3 = *(U32A+2);
                             u32* U32B = (u32*) DataB;
                             const u32 B1 = *(U32B+0);
                             const u32 B2 = *(U32B+1);
                             const u32 B3 = *(U32B+2);
                             b32 result = (A1 == B1) && (A2 == B2) && (A3 == B3);
                             return result;
                           });
    if(Index == VerticeArrayCount)
    {
      VerticeArrayCount++;
    }
    
    GLIndexArray[i] = Index;
  }
  
  midx TotalMeshSize  = GetMeshSize(IndexCount, VerticeArrayCount, NormalIndeces != 0, TextureIndeces != 0);
  header* Header      = CreateHeader(type::MESH, MeshName, MeshPath, TotalMeshSize);
  mesh* Result        = InitializeMesh(IndexCount, VerticeArrayCount, NormalIndeces != 0, TextureIndeces != 0, Header->Data);
  
  v3* Vertice = Result->v;
  v3* Normal  = Result->vn;
  v2* Texture = Result->vt;
  for( u32 i = 0; i < VerticeArrayCount; ++i )
  {
    u32 Index = 3*i;
    u32 VerticeIndex = GLVerticeIndexArray[Index];
    *Vertice++ = VerticeData[Index];
    if(NormalData)
    {
      u32 NormalIndex = GLVerticeIndexArray[Index + 2];
      *Normal++ = NormalData[NormalIndex];
    }
    if(TextureData)
    {
      u32 TextureIndex = GLVerticeIndexArray[Index + 1];
      *Texture++ = TextureData[TextureIndex];
    }
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

texture* CopyObjBitmapToTexture(texture_type Type, obj_bitmap* ObjBitmap)
{
  if(!ObjBitmap){return 0;};

  u32 TextureSizeBytes = sizeof(texture) + ObjBitmap->Width * ObjBitmap->Height * ObjBitmap->BPP / 8.f;

  header* Header = CreateHeader(type::TEXTURE, ObjBitmap->Name, ObjBitmap->Path, TextureSizeBytes);
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

material* CopyObjMtlToMaterial(c8* Path, mtl_material* ObjMtl)
{
  midx MaterialSizeBytes = MaterialSize(ObjMtl->Kd, ObjMtl->Ka, ObjMtl->Tf, ObjMtl->Ks, ObjMtl->Ke, ObjMtl->d, ObjMtl->Ni, ObjMtl->Ns, ObjMtl->IlluminationMode);
  header* Header   = CreateHeader(type::MATERIAL, ObjMtl->Name, Path, MaterialSizeBytes);
  texture* BumpMap = CopyObjBitmapToTexture(texture_type::BUMP_MAP, ObjMtl->BumpMap);
  texture* MapKd   = CopyObjBitmapToTexture(texture_type::DIFFUSE_COLOR, ObjMtl->MapKd);
  texture* MapKs   = CopyObjBitmapToTexture(texture_type::SPECULAR_COLOR, ObjMtl->MapKs);

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

render_group_element CreateRenderGroupElement(c8* Name, c8* Path, obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
{  
  render_group_element Result = {};

  obj_mesh_indeces* ObjIndeces = ObjGrp->Indeces;
  Result.SmoothingGroup = -1;
  if(ObjGrp->SmoothingGroup) {
    Result.SmoothingGroup =  *ObjGrp->SmoothingGroup;
  }

  Result.Mesh = CreateMesh(
      ObjGrp->GroupName,
      Path,
      ObjIndeces->Count,
      ObjIndeces->vi,
      ObjIndeces->ti,
      ObjIndeces->ni,
      MeshData->v,
      MeshData->vt,
      MeshData->vn);

  Result.Material = GetMaterial(MaterialMap, ObjGrp->Material);
  return Result;
}


c8* CreateDefaultName(c8* Name, u32 Index, u32 MaxCount)
{
  c8* Result = Name;
  if(!Name || *Name == '\0')
  {
    u32 Length = ASSET_MAX_NAME_LENGTH;
    Result = (c8*) PushArray(GlobalTransientArena, Length, c8);
    FormatString(Result, Length-1, "%d/%d", Index, MaxCount);
  }
  return Result;
}

u32 LoadObj(c8* Path)
{
  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  // MATERIAL
  obj_mtl_data* ObjMtlGroup = Obj->MaterialData;
  material_map MaterialMap = CreateMaterialMap(ObjMtlGroup->MaterialCount);
  for (int i = 0; i < ObjMtlGroup->MaterialCount; ++i)
  {
    mtl_material* Mtl = ObjMtlGroup->Materials + i;
    material* Material = CopyObjMtlToMaterial(ObjMtlGroup->Path, Mtl);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = Material;
  }

  // RENDER_GROUP
  midx RenderGroupMemSize = sizeof(render_group) + Obj->ObjectCount * sizeof(render_group_element);
  header* Header = CreateHeader(type::RENDER_GROUP, Obj->ObjectName, Path, RenderGroupMemSize);
  render_group* RenderGroup = (render_group*) Header->Data;
  RenderGroup->ElementCount = Obj->ObjectCount;
  RenderGroup->Elements = (render_group_element*) AdvanceBytePointer(RenderGroup, sizeof(render_group));

  // RENDER_GROUP_ELEMENT
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    obj_group* ObjGrp = Obj->ObjectGroups + i;
    c8 MeshNameBuff[ASSET_MAX_NAME_LENGTH] = {};
    c8* MeshName = CreateDefaultName(ObjGrp->GroupName, i, Obj->ObjectCount);
    RenderGroup->Elements[i] = CreateRenderGroupElement(MeshName, Path, ObjGrp, Obj->MeshData, &MaterialMap);
  }

  return Header->Key;
}

/*

render_group_element* LoadRenderObject(c8* Name, u32 RenderObjectSize, render_group_element* RenderObject, mesh* Mesh, u32* ResultKey)
{
  CreateHeader(type::render_group_element, Name, "N/A", RenderObjectSize);

  utils::Copy(RenderObjeceSize, RenderObject, Mesh);
}

gl_vertex_buffer* LoadGLVertexBuffer(c8* Name, const gl_vertex_buffer Data, u32* ResultKey)
{
  u32 VertexBufferSize = sizeof(gl_vertex_buffer);
  u32 IndexSize = Data.IndexCount * sizeof(u32);
  u32 VertexSize = Data.VertexCount * sizeof(opengl_vertex);
  u32 TotalSize = VertexBufferSize + IndexSize + VertexSize;

  gl_vertex_buffer* VertexBuffer = (gl_vertex_buffer*) Allocate(&GlobalAssetManager->Memory, TotalSize);
  VertexBuffer->IndexCount = Data.IndexCount;
  VertexBuffer->Indeces = (u32*) AdvanceBytePointer(VertexBuffer, VertexBufferSize);
  utils::Copy(IndexSize, Data.Indeces, VertexBuffer->Indeces);
  VertexBuffer->VertexCount = Data.VertexCount;
  VertexBuffer->VertexData = (opengl_vertex*) AdvanceBytePointer(VertexBuffer, VertexBufferSize + IndexSize);
  utils::Copy(VertexSize, Data.VertexData, VertexBuffer->VertexData);

  u32 Key = Load(type::gl_vertex_buffer, Name, "N/A", (void*) VertexBuffer);
  if(ResultKey)
  {
    *ResultKey = Key;
  }
  return VertexBuffer;
}

obj_bitmap* LoadTga(c8* Name, c8* Path, u32* ResultKey)
{
  obj_bitmap* Result = LoadTGA(TransientAllocator, Path);
  u32 Key = Load(type::TGA, Name, Path, (void*) Result);
  if(ResultKey)
  {
    *ResultKey = Key;
  }
  return Result; 
}
*/


void CopyMesh(const mesh* SrcMesh, mesh* DstMesh)
{
  Assert(SrcMesh->v && SrcMesh->Indeces && DstMesh->v && DstMesh->Indeces);
  DstMesh->IndexCount  = SrcMesh->IndexCount;
  DstMesh->VertexCount = SrcMesh->VertexCount;
  utils::Copy(SrcMesh->IndexCount * sizeof(u32), SrcMesh->Indeces, DstMesh->Indeces);
  utils::Copy(SrcMesh->VertexCount * sizeof(v3), SrcMesh->v, DstMesh->v);
  if(SrcMesh->vn) utils::Copy(SrcMesh->VertexCount * sizeof(v3), SrcMesh->vn, DstMesh->vn);
  if(SrcMesh->vt) utils::Copy(SrcMesh->VertexCount * sizeof(v2), SrcMesh->vt, DstMesh->vt);
}

mesh* LoadMesh(c8* Name, const mesh* Mesh, u32* ResultKey)
{
  midx MeshSize = GetMeshSize(Mesh->IndexCount, Mesh->VertexCount, Mesh->vn!=0, Mesh->vt!=0);
  header* Header = CreateHeader(type::MESH, Name, "N/A", MeshSize); // Todo: Fix Key
  mesh* LoadedMesh = InitializeMesh(Mesh->IndexCount, Mesh->VertexCount, Mesh->vn!=0, Mesh->vt!=0, Header->Data);
  CopyMesh(Mesh, LoadedMesh);

  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return LoadedMesh;
}

gl_vertex_buffer* LoadGLVertexBuffer(c8* Name, const gl_vertex_buffer Data, u32* ResultKey){ return 0; }
texture* LoadTga(c8* Name, c8* Path, u32* ResultKey){ return 0; }

}