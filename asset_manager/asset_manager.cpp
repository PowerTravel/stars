#include "asset_manager.h"
#include "platform/obj_loader.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

extern asset::manager* GlobalAssetManager;
extern memory_arena* GlobalTransientArena;

namespace asset {

u32 ToKey(type Type, const c8* UniqueName)
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

header* CreateHeader(type Type, const c8* UniqueName, const c8* Name, const c8* Path, midx DataSize) {
  Assert(UniqueName && *UniqueName != '\0');
  // Layout of any memory allocated in the asset_manager is -> | HEADER | DATA | NAME | PATH | KEY |
  // This way, anyone who has a pointer to DATA can get the header by just rewinding the pointer sizeof(header) bytes.

  midx NameLength = jstr::StringLength(Name);
  midx NameSize = (NameLength+1)* sizeof(c8);
  midx PathLength = jstr::StringLength(Path);
  midx PathSize = (PathLength+1)* sizeof(c8);
  midx UniqueNameLength = jstr::StringLength(UniqueName);
  midx UniqueNameSize = (UniqueNameLength+1)* sizeof(c8);

  const c8* TypeString = TypeToString(Type);
  midx TypeLength = jstr::StringLength(TypeString);
  midx KeyStringLength = TypeLength + UniqueNameLength + 4;
  midx KeyStringSize   = (KeyStringLength+1) * sizeof(c8);

  midx HeaderSize = sizeof(header) + NameSize + PathSize + KeyStringLength;
  midx MemorySize = HeaderSize + DataSize;
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

file_local inline midx GetMeshSize(const mesh* Mesh) {
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

/*
IndexArray = {4 5 6 4 6 7};
ValueArray = {
  {-1,-1, 1},
  {-1, 1, 1},
  { 1, 1, 1},
  { 1,-1, 1},
  {-1,-1,-1},
  {-1, 1,-1},
  { 1, 1,-1},
  { 1,-1,-1},
}

IndexTracker = {0 0 0 0 1 2 3 0}
UniqueIndeces = {0, 1, 2, 0, 2}
UniqueVertices = {
    {-1,-1,-1},
    {-1, 1,-1},
    { 1, 1,-1},
    {},
    {},
    {},
    {},
    {},
}

*/

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

struct index_tracker {
  u32 OldIndex;
  u32 NewIndex;
};

index_tracker* Find(u32 TrackerSize, index_tracker* IndexTracker, u32 HashedIndex, u32 OldIndex, u32* Collisions)
{
  u32 ArrayIndex = HashedIndex;
  index_tracker* Element = IndexTracker + ArrayIndex;
  u32 Coll = 0;
  while(Element->OldIndex != 0 &&  Element->OldIndex != OldIndex)
  {
    ArrayIndex++;
    if(ArrayIndex >= TrackerSize)
    {
      ArrayIndex = 0;
    }
    Element = IndexTracker + ArrayIndex;
    Coll++;
  }
  
  if(Element->OldIndex == 0)
  {
    *Collisions = Coll;
  }
  // Element is either empty or points to OldIndex;
  return Element;
}


indexed_array CreateNewIndexedArraySmallValueCount(memory_arena* Arena, u32 IndexCount, const u32* IndexArray, midx ValueSize, u32 MaxValueCount, bptr ValueArray,
  b32 (*CompareFunction)(const bptr DataA, const bptr DataB))
{
  indexed_array Result = IndexedArray(Arena, IndexCount, ValueSize, Minimum(MaxValueCount, IndexCount));
  u32* IndexTracker = PushArray(Arena, MaxValueCount, u32);

  u32 ValueCount = 0;
  for (int i = 0; i < IndexCount; ++i)
  {
    u32 OldIndex = IndexArray[i];
    if(IndexTracker[OldIndex]==0)
    {
      u32 NewIndex = ValueCount;
      Result.IndexArray[i]   = NewIndex;
      IndexTracker[OldIndex] = ++ValueCount;

      utils::Copy(ValueSize, ValueArray + ValueSize*OldIndex, Result.ValueArray + ValueSize*NewIndex);
    }else{
      Result.IndexArray[i] = IndexTracker[OldIndex] - 1;
    }
  }
  Result.ValueCount = ValueCount;
  return Result;
}

indexed_array CreateNewIndexedArrayHashedList(memory_arena* Arena, u32 IndexCount, const u32* IndexArray, midx ValueSize, u32 MaxValueCount, bptr ValueArray,
  b32 (*CompareFunction)(const bptr DataA, const bptr DataB))
{
  indexed_array Result = IndexedArray(Arena, IndexCount, ValueSize, Minimum(MaxValueCount, IndexCount));

  u32 TrackerSize = utils::GetHashListSize(IndexCount,3);
  index_tracker* IndexTracker = PushArray(Arena, TrackerSize, index_tracker);
  u32 ValueCount = 0;
  u32 TotalCollisions = 0;
  for (int i = 0; i < IndexCount; ++i)
  {
    u32 OldIndex = IndexArray[i];

    u32 HashedIndex = utils::Hash(OldIndex) % TrackerSize;
    u32 Collision = 0;
    index_tracker* TrackElement = Find(TrackerSize, IndexTracker, HashedIndex, OldIndex, &Collision);
    TotalCollisions += Collision;
    if(TrackElement->NewIndex == 0)
    {
      u32 NewIndex = ValueCount;
      Result.IndexArray[i] = NewIndex;

      TrackElement->OldIndex = OldIndex;
      TrackElement->NewIndex = ++ValueCount;

      utils::Copy(ValueSize, ValueArray + ValueSize*OldIndex, Result.ValueArray + ValueSize*NewIndex);
    }else{
      Result.IndexArray[i] = TrackElement->NewIndex - 1;
    }

  }
  if(TotalCollisions)
  {  
  Platform.DEBUGPrint("--==Collisions==--\n\t%d Collisions\n\t%d Elements\n\t%d ListSize,\n\t%f Collisions/ElementCount\n\t%f Collisions / ElementCount \n", 
    TotalCollisions, IndexCount, TrackerSize,  (r32)TotalCollisions / (r32) IndexCount,(r32)IndexCount / (r32) TrackerSize);
  }
  Result.ValueCount = ValueCount;
  return Result;
}

indexed_array CreateNewIndexedArray(memory_arena* Arena, u32 IndexCount, const u32* IndexArray, midx ValueSize, u32 MaxValueCount, bptr ValueArray,
  b32 (*CompareFunction)(const bptr DataA, const bptr DataB))
{
  indexed_array Result = {};
  #if 0
  if(MaxValueCount < 2048)
  {
    // TODO: Use this one if there are few groups instead
    Result = CreateNewIndexedArraySmallValueCount(Arena, IndexCount, IndexArray, ValueSize, MaxValueCount, ValueArray, CompareFunction);
  }else{
    // TODO: Use this one if there are alot of groups groups
    Result = CreateNewIndexedArrayHashedList(Arena, IndexCount, IndexArray, ValueSize, MaxValueCount, ValueArray, CompareFunction);
  }
  #endif
  Result = CreateNewIndexedArraySmallValueCount(Arena, IndexCount, IndexArray, ValueSize, MaxValueCount, ValueArray, CompareFunction);
  return Result;
}

aabb3f GetAABB(u32 VertexCount, const v3* VerticeArray)
{
  v3 MinimumVal = V3(R32Max, R32Max, R32Max);
  v3 MaximumVal = V3(R32Min, R32Min, R32Min);
  for (int i = 0; i < VertexCount; ++i)
  {
    const v3* Vertex = VerticeArray+i;
    MinimumVal.X = Minimum(MinimumVal.X, Vertex->X);
    MinimumVal.Y = Minimum(MinimumVal.Y, Vertex->Y);
    MinimumVal.Z = Minimum(MinimumVal.Z, Vertex->Z);
    MaximumVal.X = Maximum(MaximumVal.X, Vertex->X);
    MaximumVal.Y = Maximum(MaximumVal.Y, Vertex->Y);
    MaximumVal.Z = Maximum(MaximumVal.Z, Vertex->Z);
  }
  aabb3f Result = AABB3f(MinimumVal, MaximumVal);
  return Result;
}

mesh* CreateMesh( const c8* MeshKey, const c8* MeshName, const c8* MeshPath,
                  const u32 IndexCount,
                  const u32* VerticeIndeces, const u32* NormalIndeces, const u32* TextureIndeces,
                  const u32 VerticeCount,    const u32 NormalCount,    const u32 TextureCount,
                  const v3* VerticeData,     const v3* NormalData,     const v2* TextureData)
{

  indexed_array VerticeArray = {};
  if(VerticeIndeces)
  {
    VerticeArray = CreateNewIndexedArray(GlobalTransientArena, IndexCount, VerticeIndeces, sizeof(v3), VerticeCount, (bptr) VerticeData, V3CompareFunction);
  }

  indexed_array NormalArray = {};
  if(NormalIndeces)
  {
    NormalArray = CreateNewIndexedArray(GlobalTransientArena, IndexCount, NormalIndeces, sizeof(v3), NormalCount, (bptr) NormalData, V3CompareFunction);
  }

  indexed_array TextureArray = {};
  if(TextureIndeces)
  {
    TextureArray = CreateNewIndexedArray(GlobalTransientArena, IndexCount, TextureIndeces, sizeof(v2), TextureCount, (bptr) TextureData, V2CompareFunction);
  }

  midx TotalMeshSize = GetMeshSize(IndexCount, VerticeArray.ValueCount, NormalArray.ValueCount, TextureArray.ValueCount);
  header* Header     = CreateHeader(type::MESH, MeshKey, MeshName, MeshPath, TotalMeshSize);
  mesh* Result       = InitializeMesh(IndexCount, VerticeArray.ValueCount, NormalArray.ValueCount, TextureArray.ValueCount, Header->Data);

  if(VerticeIndeces)
  {
    utils::Copy(sizeof(u32)*VerticeArray.IndexCount,             VerticeArray.IndexArray, Result->vi);
    utils::Copy(VerticeArray.ValueSize*VerticeArray.ValueCount,  VerticeArray.ValueArray, Result->v);
  }
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

  Result->AABB = GetAABB(VerticeArray.ValueCount, (v3*) VerticeArray.ValueArray);

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
    case type::PHONG_MATERIAL: {
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
        if(MaterialHeader->Type == type::PHONG_MATERIAL)
        {
          // If several elements point to the same phong_material, this should ensure we only free a phong_material once.
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

void* Find(type Type, const c8* Name) {
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

file_local u32 CopyObjBitmapToTexture(const c8* Key, const obj_bitmap* ObjBitmap)
{
  if(!ObjBitmap){return 0;};

  Assert(ObjBitmap->BPP == 32);
  u32 ImageSizeBytes = sizeof(image) + ObjBitmap->Width * ObjBitmap->Height * ObjBitmap->BPP / 8.f;

  header* Header   = CreateHeader(type::IMAGE, Key, ObjBitmap->Name, ObjBitmap->Path, ImageSizeBytes);
  image* Image = (image*) Header->Data;
  Image->Channels = 4;
  Image->Width    = ObjBitmap->Width;
  Image->Height   = ObjBitmap->Height;
  Image->Pixels   = AdvanceBytePointer(Image, sizeof(image));
  utils::Copy(ImageSizeBytes, ObjBitmap->Pixels, Image->Pixels);
  return Header->Key;
}

file_local midx GetMaterialSize(
    v4* Ka,
    v4* Kd,
    v4* Tf,
    v4* Ks,
    v4* Ke,
    r32* d,
    r32* Ni,
    r32* Ns
  )
{
  u32 KaSize = (u32) BranchlessArithmatic(Ka == 0, 0, sizeof(v4));
  u32 KdSize = (u32) BranchlessArithmatic(Kd == 0, 0, sizeof(v4));
  u32 TfSize = (u32) BranchlessArithmatic(Tf == 0, 0, sizeof(v4));
  u32 KsSize = (u32) BranchlessArithmatic(Ks == 0, 0, sizeof(v4));
  u32 KeSize = (u32) BranchlessArithmatic(Ke == 0, 0, sizeof(v4));
  u32 dSize  = (u32) BranchlessArithmatic(d  == 0, 0, sizeof(r32));
  u32 NiSize = (u32) BranchlessArithmatic(Ni == 0, 0, sizeof(r32));
  u32 NsSize = (u32) BranchlessArithmatic(Ns == 0, 0, sizeof(r32));
  u32 MaterialSizeBytes = sizeof(phong_material) + KdSize + KaSize + TfSize + KsSize + KeSize + dSize + NiSize + NsSize;
  return MaterialSizeBytes;
}

file_local midx GetMaterialSize(const phong_material* Material){
  midx Result = GetMaterialSize(
    Material->Ka,
    Material->Kd,
    Material->Tf,
    Material->Ks,
    Material->Ke,
    Material->d,
    Material->Ni,
    Material->Ns);
  return Result;
}

void InitiateMaterial (
    v4* Ka,
    v4* Kd,
    v4* Tf,
    v4* Ks,
    v4* Ke,
    r32* d,
    r32* Ni,
    r32* Ns,
    r32 BumpMapBM,
    u32 BumpMapHandle,
    u32 MapKdHandle,
    u32 MapKsHandle,
    phong_material* Material
  )
{
  u32 KaSize = BranchlessArithmatic(Ka == 0, 0, sizeof(v4));
  u32 KdSize = BranchlessArithmatic(Kd == 0, 0, sizeof(v4));
  u32 TfSize = BranchlessArithmatic(Tf == 0, 0, sizeof(v4));
  u32 KsSize = BranchlessArithmatic(Ks == 0, 0, sizeof(v4));
  u32 KeSize = BranchlessArithmatic(Ke == 0, 0, sizeof(v4));
  u32 dSize  = BranchlessArithmatic(d  == 0, 0, sizeof(r32));
  u32 NiSize = BranchlessArithmatic(Ni == 0, 0, sizeof(r32));
  u32 NsSize = BranchlessArithmatic(Ns == 0, 0, sizeof(r32));

  
  if(Ka) {
    Material->Ka = (v4*)  AdvanceBytePointer(Material, sizeof(phong_material));
    *Material->Ka = *Ka;
  }
  if(Kd) {
    Material->Kd = (v4*)  AdvanceBytePointer(Material, sizeof(phong_material) + KaSize);
    *Material->Kd = *Kd;
  }
  if(Tf) {
    Material->Tf = (v4*)  AdvanceBytePointer(Material, sizeof(phong_material) + KaSize + KdSize);
    *Material->Tf = *Tf;
  }
  if(Ks) {
    Material->Ks = (v4*)  AdvanceBytePointer(Material, sizeof(phong_material) + KaSize + KdSize + TfSize);
    *Material->Ks = *Ks;
  }
  if(Ke) {
    Material->Ke = (v4*)  AdvanceBytePointer(Material, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize);
    *Material->Ke = *Ke;
  }
  if(d) {
    Material->d  = (r32*) AdvanceBytePointer(Material, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize + KeSize);
    *Material->d = *d;
  }
  if(Ni) {
    Material->Ni = (r32*) AdvanceBytePointer(Material, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize + KeSize + dSize);
    *Material->Ni = *Ni;
  }
  if(Ns) {
    Material->Ns = (r32*) AdvanceBytePointer(Material, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize + KeSize + dSize + NiSize);
    *Material->Ns = *Ns;
  }
  
  Material->BumpMapBM = BumpMapBM;
  if(BumpMapHandle)
  {
    image* Image = (image*) Find(type::IMAGE, BumpMapHandle);
    Assert(Image);
    Material->HasBumpMap = true;
    Material->BumpMap = DefaultTexture(Image);
  }
  if(MapKdHandle)
  {
    image* Image = (image*) Find(type::IMAGE, MapKdHandle);
    Assert(Image);
    Material->HasDiffuseTexture = true;
    Material->DiffuseTexture = DefaultTexture(Image);
  }
  if(MapKsHandle)
  {
    image* Image = (image*) Find(type::IMAGE, MapKsHandle);
    Assert(Image);
    Material->HasSpecularTexture = true;
    Material->SpecularTexture = DefaultTexture(Image);
  }
}


file_local void CopyMaterial( const phong_material* Src, phong_material* Dst)
{
  u32 KaSize = BranchlessArithmatic(Src->Ka == 0, 0, sizeof(v4));
  u32 KdSize = BranchlessArithmatic(Src->Kd == 0, 0, sizeof(v4));
  u32 TfSize = BranchlessArithmatic(Src->Tf == 0, 0, sizeof(v4));
  u32 KsSize = BranchlessArithmatic(Src->Ks == 0, 0, sizeof(v4));
  u32 KeSize = BranchlessArithmatic(Src->Ke == 0, 0, sizeof(v4));
  u32 dSize  = BranchlessArithmatic(Src->d  == 0, 0, sizeof(r32));
  u32 NiSize = BranchlessArithmatic(Src->Ni == 0, 0, sizeof(r32));
  u32 NsSize = BranchlessArithmatic(Src->Ns == 0, 0, sizeof(r32));
  
  if(Src->Ka) {
    Dst->Ka = (v4*)  AdvanceBytePointer(Dst, sizeof(phong_material));
    *Dst->Ka = *Src->Ka;
  }
  if(Src->Kd) {
    Dst->Kd = (v4*)  AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize);
    *Dst->Kd = *Src->Kd;
  }
  if(Src->Tf) {
    Dst->Tf = (v4*)  AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize + KdSize);
    *Dst->Tf = *Src->Tf;
  }
  if(Src->Ks) {
    Dst->Ks = (v4*)  AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize + KdSize + TfSize);
    *Dst->Ks = *Src->Ks;
  }
  if(Src->Ke) {
    Dst->Ke = (v4*)  AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize);
    *Dst->Ke = *Src->Ke;
  }
  if(Src->d) {
    Dst->d = (r32*) AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize + KeSize);
    *Dst->d = *Src->d;
  }
  if(Src->Ni) {
    Dst->Ni = (r32*) AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize + KeSize + dSize);
    *Dst->Ni = *Src->Ni;
  }
  if(Src->Ns) {
    Dst->Ns = (r32*) AdvanceBytePointer(Dst, sizeof(phong_material) + KaSize + KdSize + TfSize + KsSize + KeSize + dSize + NiSize);
    *Dst->Ns = *Src->Ns;
  }
  
  Dst->BumpMapBM  = Src->BumpMapBM;
  Dst->HasBumpMap = Src->HasBumpMap;
  Dst->BumpMap    = Src->BumpMap;
  Dst->HasDiffuseTexture = Src->HasDiffuseTexture;
  Dst->DiffuseTexture    = Src->DiffuseTexture;
  Dst->HasSpecularTexture = Src->HasSpecularTexture;
  Dst->SpecularTexture = Src->SpecularTexture;
}

phong_material* CopyObjMtlToMaterial(c8* Path, mtl_material* ObjMtl, c8* Key)
{
  midx MaterialSizeBytes = GetMaterialSize(ObjMtl->Ka, ObjMtl->Kd, ObjMtl->Tf, ObjMtl->Ks, ObjMtl->Ke, ObjMtl->d, ObjMtl->Ni, ObjMtl->Ns);
  header* Header   = CreateHeader(type::PHONG_MATERIAL, Key, ObjMtl->Name, Path, MaterialSizeBytes);
  u32 BumpMapHandle = CopyObjBitmapToTexture(Key, ObjMtl->BumpMap);
  u32 MapKdHandle   = CopyObjBitmapToTexture(Key, ObjMtl->MapKd);
  u32 MapKsHandle   = CopyObjBitmapToTexture(Key, ObjMtl->MapKs);

  phong_material* Result = (phong_material*) Header->Data;
  InitiateMaterial(
    ObjMtl->Ka, ObjMtl->Kd, ObjMtl->Tf, ObjMtl->Ks, ObjMtl->Ke, ObjMtl->d, ObjMtl->Ni, ObjMtl->Ns,
    ObjMtl->BumpMapBM, BumpMapHandle, MapKdHandle, MapKsHandle,
    Result);

  return Result;
}

struct material_map {
  u32 MaterialCount;
  phong_material** Materials;
  mtl_material** Mtl_Materials;
};

material_map CreateMaterialMap(u32 MaterialCount)
{
  material_map Result = {};
  Result.MaterialCount = MaterialCount;
  Result.Materials     = PushArray(GlobalTransientArena, MaterialCount, phong_material*);
  Result.Mtl_Materials = PushArray(GlobalTransientArena, MaterialCount, mtl_material*);
  return Result;
}

phong_material* GetMaterial(material_map* MaterialMap, mtl_material* Mtl){
  for (int i = 0; i < MaterialMap->MaterialCount; ++i)
  {
    if(Mtl == MaterialMap->Mtl_Materials[i])
    {
      return MaterialMap->Materials[i];
    }
  }
  return 0;
}

render_group_element CreateRenderGroupElement(const c8* Key, const c8* Name, const c8* Path, obj_group* ObjGrp, obj_mesh_data* MeshData, material_map* MaterialMap)
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
      MeshData->nv,
      MeshData->nvn,
      MeshData->nvt,
      MeshData->v,
      MeshData->vn,
      MeshData->vt);

  Result.Material = GetMaterial(MaterialMap, ObjGrp->Material);
  return Result;
}


c8* CreateUniqueKey(const c8* Name, u32 Index, u32 MaxCount)
{
  c8* Result = (c8*) Name;
  if(MaxCount > 1)
  {
    u32 Length = ASSET_MAX_NAME_LENGTH;
    Result = (c8*) PushArray(GlobalTransientArena, Length, c8);
    FormatString(Result, Length-1, "%s_%d/%d", Name, Index+1, MaxCount);
  }
  return Result;
}

u32 LoadObj(const c8* Path, const c8* UniqueName)
{
  Assert(Path && *Path != '\0');
  if(!UniqueName || *UniqueName == '\0')
  {
    UniqueName = Path;
  }

  obj_loaded_file* Obj = ReadOBJFile(TransientAllocator, GlobalTransientArena, Path);

  // MATERIAL
  obj_mtl_data* ObjMtlGroup = Obj->MaterialData;
  material_map MaterialMap = CreateMaterialMap(ObjMtlGroup->MaterialCount);
  for (int i = 0; i < ObjMtlGroup->MaterialCount; ++i)
  {
    mtl_material* Mtl = ObjMtlGroup->Materials + i;
    c8* MtlKey = CreateUniqueKey(UniqueName, i, Obj->ObjectCount);
    phong_material* Material = CopyObjMtlToMaterial(ObjMtlGroup->Path, Mtl, MtlKey);
    MaterialMap.Mtl_Materials[i] = Mtl;
    MaterialMap.Materials[i] = Material;
  }

  // RENDER_GROUP
  midx RenderGroupMemSize = sizeof(render_group) + Obj->ObjectCount * sizeof(render_group_element);
  header* Header = CreateHeader(type::RENDER_GROUP, UniqueName, Obj->ObjectName, Path, RenderGroupMemSize);
  render_group* RenderGroup = (render_group*) Header->Data;
  
  u32 ActualElementCount = {};
  RenderGroup->Elements = (render_group_element*) AdvanceBytePointer(RenderGroup, sizeof(render_group));

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
      c8* MeshName = CreateUniqueKey(UniqueName, i, Obj->ObjectCount);
      RenderGroup->Elements[i] = CreateRenderGroupElement(UniqueName, MeshName, Path, ObjGrp, Obj->MeshData, &MaterialMap);
      ElementCount++;
    }
  }
  RenderGroup->ElementCount = ElementCount;

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

u32 LoadTga(const c8* Path, const c8* UniqueName)
{
  obj_bitmap* ObjBitmap = LoadTGA(TransientAllocator, Path);
  if(UniqueName == 0 || *UniqueName =='\0')
  {
    UniqueName = Path;
  }
  u32 TextureHandle = CopyObjBitmapToTexture(UniqueName, ObjBitmap);
  return TextureHandle; 
}


mesh* LoadMesh(const c8* UniqueName, const mesh* Mesh, u32* ResultKey)
{
  midx MeshSize = GetMeshSize(Mesh);
  header* Header = CreateHeader(type::MESH, UniqueName, UniqueName, "N/A", MeshSize);
  mesh* Result = InitializeMesh(Mesh->IndexCount, Mesh->vCount, Mesh->vnCount, Mesh->vtCount, Header->Data);
  CopyMesh(Mesh, Result);

  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}

phong_material* LoadMaterial(const c8* UniqueName, const phong_material* Material, u32* ResultKey)
{
  midx MaterialSize = GetMaterialSize(Material);
  header* Header = CreateHeader(type::PHONG_MATERIAL, UniqueName, UniqueName, "N/A", MaterialSize);
  phong_material* Result = (phong_material*) Header->Data;
  CopyMaterial(Material, Result);
  return Result;
}


/// Gltf Loaders
image* LoadImage(const c8* UniqueName, const c8* Name, const c8* Path, const image* Image, u32* ResultKey){

  midx ImageSize = (Image->Channels) * (Image->Width) * (Image->Height);
  header* Header = CreateHeader(type::IMAGE, UniqueName, Name, Path, ImageSize + sizeof(image));
  image* Result = (image*) Header->Data;
  Result->Channels = Image->Channels;
  Result->Width    = Image->Width;
  Result->Height   = Image->Height;
  Result->Pixels   = AdvanceByType(Result, image);
  utils::Copy(ImageSize, (void*) Image->Pixels, (void*) Result->Pixels);
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}

pbr_material* LoadPbrMaterial(const c8* UniqueName, const pbr_material* PbrMaterial, u32* ResultKey)
{
  header* Header = CreateHeader(type::PBR_MATERIAL, UniqueName, UniqueName, "N/A", sizeof(pbr_material));
  pbr_material* Result = (pbr_material*) Header->Data;
  *Result = *PbrMaterial;
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}

midx GetMeshSize2( const gltf_tmp::mesh* Mesh ) {
  midx StructSize     = sizeof(gltf_tmp::mesh);
  midx IndexMemSize   = Mesh->IndexCount * sizeof(int);
  midx VerticeMemSize = Mesh->VertexCount  * sizeof(v3);
  midx NormalMemSize  = Mesh->VertexNormal ? Mesh->VertexCount  * sizeof(v3) : 0;

  midx TextureSetMemSizeD1   = Mesh->TextureVertexSetCount*sizeof(v2*);
  midx TextureVerticeMemSize = TextureSetMemSizeD1 ? Mesh->TextureVertexSetCount * Mesh->VertexCount * sizeof(v2) : 0;

  midx TotalMeshSize = StructSize + IndexMemSize + VerticeMemSize + NormalMemSize + TextureSetMemSizeD1 + TextureVerticeMemSize;
  return TotalMeshSize;
}

void Copy(const gltf_tmp::mesh* Src, gltf_tmp::mesh* Dst, size_t TotalSize)
{
  bptr MemScan = AdvanceBytePointer(Dst, sizeof(gltf_tmp::mesh));

  if(Src->Indeces)
  {
    Dst->IndexCount = Src->IndexCount;
    Dst->Indeces = (int*) MemScan;
    size_t IndexSize = Src->IndexCount  * sizeof(int);
    utils::Copy(IndexSize, (void*) Src->Indeces, (void*) Dst->Indeces);
    MemScan = AdvanceBytePointer(MemScan, IndexSize);
  }else{
    Assert(0); // Indeces are not required, but were not handling it atm. This is to catch that
  }

  Assert(Src->Vertex); // Vertex data must exist
  const size_t VertexCount = Src->VertexCount;
  Dst->VertexCount = VertexCount;
  Dst->Vertex = (v3*) MemScan;
  size_t VertexSize = VertexCount  * sizeof(v3);
  utils::Copy(VertexSize, (void*) Src->Vertex, (void*) Dst->Vertex);
  MemScan = AdvanceBytePointer(MemScan, VertexSize);

  if(Src->VertexNormal)
  {
    Dst->VertexNormal = (v3*) MemScan;
    size_t VertexNormalSize = VertexCount  * sizeof(v3);
    utils::Copy(VertexNormalSize, (void*) Src->VertexNormal, (void*) Dst->VertexNormal);
    MemScan = AdvanceBytePointer(MemScan, VertexNormalSize);
  }

  if(Src->TextureVertexSetCount)
  {
    Dst->TextureVertexSetCount = Src->TextureVertexSetCount;
    Dst->TextureVertices = (v2**) MemScan;
    size_t TextureVerticeD1Size = Src->TextureVertexSetCount  * sizeof(v2*);
    MemScan = AdvanceBytePointer(MemScan, TextureVerticeD1Size);
    for (int i = 0; i < Dst->TextureVertexSetCount; ++i)
    {
      Dst->TextureVertices[i] = (v2*) MemScan;
      size_t ArraySize = sizeof(v2) * VertexCount;
      utils::Copy(ArraySize, (void*) Src->TextureVertices[i], (void*) Dst->TextureVertices[i]);
      MemScan = AdvanceBytePointer(MemScan, ArraySize);
    }
  }

  Assert(MemScan - ((uint8_t*)Dst) == TotalSize);

  Dst->Topology = Src->Topology;
  Dst->AABB = Src->AABB;
}

gltf_tmp::mesh* LoadMesh2(const c8* UniqueName, const gltf_tmp::mesh* Mesh, u32* ResultKey)
{
  midx MeshSize = GetMeshSize2(Mesh);
  header* Header = CreateHeader(type::MESH, UniqueName, UniqueName, "N/A", MeshSize);
  gltf_tmp::mesh* Result = (gltf_tmp::mesh*)Header->Data;
  Copy(Mesh, Result, MeshSize);
  
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}
}