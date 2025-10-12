#include "asset_manager.h"
#include "io/obj.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

extern asset::manager* GlobalAssetManager;
extern memory_arena* GlobalTransientArena;

namespace asset {

key ToKey(type Type, const c8* UniqueName)
{
  c8 TempKeyString[ASSET_MAX_KEY_LENGTH] = {};

  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  u32 UniqueNameLength = jstr::StringLength(UniqueName);
  
  string KeyString = {};
  u32 KeyStringLength = TypeLength + UniqueNameLength + 2;
  u32 FormattedLength = FormatString(TempKeyString, KeyStringLength+1, "%s::%s", TypeString, UniqueName);
  Assert(FormattedLength <= KeyStringLength);
  key Result = (key) utils::djb2_hash(TempKeyString);
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


  Assert(Find(Type,Result->Key) == 0);

  Insert(&GlobalAssetManager->Headers, Result->Key, (void*) Result);

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
    }break;
    case type::PHONG_MATERIAL: {
      // LoadGLVertexBuffer allocates the whole mesh as a contious block
      FreeMemory(&GlobalAssetManager->Memory, Header);
    }break;
    case type::RENDER_TREE: {
      
    }break;
    default: {
      INVALID_CODE_PATH
    };
  }
}

void* Find(type Type, key Key) {
  header* Header = FindHeader(Key);
  void* Result = 0;
  if(Header)
  {
    Result = Header->Data;
  }

  return Result;
}

void* Find(type Type, const c8* Name) {
  key Key = ToKey(Type, Name);
  void* Result = Find(Type, Key);
  return Result;
}

void Free(type Type, key Key)
{
  header* Header = FindHeader(Key);
  if(Header)
  {
    FreeAsset(Header);
  }
}

void Free(type Type, c8* Name) {
  key Key = ToKey(Type, Name);
  Free(Type, Key);
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

texture DefaultTexture(u32 ImageHandle)
{
  texture Result = {};
  Result.TexCoord = 0;
  Result.MagFilter = texture::filter::NEAREST;
  Result.MinFilter = texture::filter::NEAREST;
  Result.WrapS = texture::wrap::REPEAT;
  Result.WrapT = texture::wrap::REPEAT;
  Result.Image = (image*) Find(type::IMAGE, ImageHandle);
Assert(Result.Image);
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
    Material->BumpMap = DefaultTexture(BumpMapHandle);
  }
  if(MapKdHandle)
  {
    image* Image = (image*) Find(type::IMAGE, MapKdHandle);
    Assert(Image);
    Material->HasDiffuseTexture = true;
    Material->DiffuseTexture = DefaultTexture(MapKdHandle);
  }
  if(MapKsHandle)
  {
    image* Image = (image*) Find(type::IMAGE, MapKsHandle);
    Assert(Image);
    Material->HasSpecularTexture = true;
    Material->SpecularTexture = DefaultTexture(MapKsHandle);
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
  
  Dst->BumpMapBM          = Src->BumpMapBM;
  Dst->HasBumpMap         = Src->HasBumpMap;
  Dst->BumpMap            = Src->BumpMap;
  Dst->HasDiffuseTexture  = Src->HasDiffuseTexture;
  Dst->DiffuseTexture     = Src->DiffuseTexture;
  Dst->HasSpecularTexture = Src->HasSpecularTexture;
  Dst->SpecularTexture    = Src->SpecularTexture;
}

c8* CreateUniqueName(const c8* Prefix, const c8* Name, const c8* Postfix, u32 Index, u32 MaxCount)
{
  static size_t Counter = 0;

  c8* Result = (c8*) Name;
  u32 Length = ASSET_MAX_NAME_LENGTH;
  Result = (c8*) PushArray(GlobalTransientArena, Length, c8);
  if(MaxCount > 1)
  {  
    FormatString(Result, Length-1, "%s%s%s_%d/%d-%d", Prefix, Name, Postfix, Index+1, MaxCount, Counter++);
  }else{
    FormatString(Result, Length-1, "%s%s%s-%d", Prefix, Name, Postfix, Counter++);
  }
  
  return Result;
}

phong_material* LoadMaterial(const c8* UniqueName, const phong_material* Material, key* ResultKey)
{
  midx MaterialSize = GetMaterialSize(Material);
  header* Header = CreateHeader(type::PHONG_MATERIAL, UniqueName, UniqueName, "N/A", MaterialSize);
  phong_material* Result = (phong_material*) Header->Data;
  CopyMaterial(Material, Result);
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}


/// Gltf Loaders
image* LoadImage(const c8* UniqueName, const c8* Name, const c8* Path, const image* Image, key* ResultKey){

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

pbr_material* LoadPbrMaterial(const c8* UniqueName, const pbr_material* PbrMaterial, key* ResultKey)
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


static size_t GetPrimitiveContentSize(gltf_tmp::mesh::primitive* Primitive)
{
  size_t IndexSize = Primitive->IndexCount * sizeof(int);
  size_t VertexSize = Primitive->VertexCount * sizeof(v3);
  size_t VertexNormalSize = Primitive->VertexNormal ? Primitive->VertexCount * sizeof(v3) : 0;
  size_t TextureSetMemSizeD1 = Primitive->TextureVertexSetCount * sizeof(v2*);
  size_t TextureVerticeSize = Primitive->TextureVertexSetCount ? Primitive->TextureVertexSetCount * Primitive->VertexCount * sizeof(v2) : 0;
  size_t Result = IndexSize + VertexSize + VertexNormalSize + TextureSetMemSizeD1 + TextureVerticeSize;
  return Result;
}

static size_t GetMeshSize( const gltf_tmp::mesh* Mesh ) {

  size_t Result = sizeof(gltf_tmp::mesh);
  Result += Mesh->PrimitiveCount * sizeof(gltf_tmp::mesh::primitive);
  for (int i = 0; i < Mesh->PrimitiveCount; ++i)
  {
    Result += GetPrimitiveContentSize(&Mesh->Primitives[i]);
  }

  return Result;
}

bptr CopyMeshPrimitive(bptr PayloadPtr, const gltf_tmp::mesh::primitive* Src, gltf_tmp::mesh::primitive* Dst, size_t TotalSize)
{

  Dst->PbrMaterial = Src->PbrMaterial;
  Dst->PhongMaterial = Src->PhongMaterial;

  bptr MemScan = PayloadPtr;

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

  Assert((MemScan - PayloadPtr) == TotalSize);

  Dst->Topology = Src->Topology;
  Dst->AABB = Src->AABB;

  return MemScan;
}

void CopyMesh(const gltf_tmp::mesh* Src, gltf_tmp::mesh* Dst, size_t TotalSize)
{
  bptr MemScan = AdvanceBytePointer(Dst, sizeof(gltf_tmp::mesh));

  Dst->PrimitiveCount = Src->PrimitiveCount;
  Dst->Primitives = (gltf_tmp::mesh::primitive*) MemScan;
  MemScan = AdvanceBytePointer(MemScan, Dst->PrimitiveCount * sizeof(gltf_tmp::mesh::primitive));
  
  for (int i = 0; i < Dst->PrimitiveCount; ++i)
  {
    gltf_tmp::mesh::primitive* SrcPrimitive = &Src->Primitives[i];
    gltf_tmp::mesh::primitive* DstPrimitive = &Dst->Primitives[i];
    size_t PrimitiveSize = GetPrimitiveContentSize(SrcPrimitive);
    MemScan = CopyMeshPrimitive(MemScan, SrcPrimitive, DstPrimitive, PrimitiveSize);
  }
}

gltf_tmp::mesh* LoadMesh(const c8* UniqueName, const gltf_tmp::mesh* Mesh, key* ResultKey)
{
  midx MeshSize = GetMeshSize(Mesh);
  header* Header = CreateHeader(type::MESH, UniqueName, UniqueName, "N/A", MeshSize);
  gltf_tmp::mesh* Result = (gltf_tmp::mesh*)Header->Data;
  CopyMesh(Mesh, Result, MeshSize);
  
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}


static size_t GetRenderTreeSize(const gltf_tmp::render_tree* RenderTree)
{
  size_t StructSize = sizeof(gltf_tmp::render_tree);
  size_t NodeSize = RenderTree->NodeCount * sizeof(gltf_tmp::render_tree::node);
  size_t Result = StructSize + NodeSize;
  return Result;
}



struct node_queue {
  size_t Count;
  size_t TotCount;
  gltf_tmp::render_tree::node** Queue;
};

node_queue NodeQueue(size_t Size)
{
  node_queue Result = {}; 
  Result.Count = 0;
  Result.TotCount = Size; 
  Result.Queue = PushArray(GlobalTransientArena, Size, gltf_tmp::render_tree::node*);
  return Result;
}

bool IsEmpty(node_queue& Queue)
{
  bool Result = Queue.Count == 0;
  return Result;
}

void Push(node_queue& Queue, gltf_tmp::render_tree::node* Value)
{
  Queue.Queue[Queue.Count++] = Value;
}

gltf_tmp::render_tree::node* Pop(node_queue& Queue)
{
  Assert(Queue.Count > 0);
  if(Queue.Count == 0) return 0;
  gltf_tmp::render_tree::node* Result = Queue.Queue[--Queue.Count];
  Queue.Queue[Queue.Count+1] = 0;
  return Result;
}

void CopyTransforms(gltf_tmp::render_tree::node* Src, gltf_tmp::render_tree::node* Dst)
{
  Dst->HasTransform = Src->HasTransform;
  Dst->Transform    = Src->Transform;
}

size_t MapChildNodes(size_t NodeIndex, size_t ChildCount, gltf_tmp::render_tree::node* NodeArray, gltf_tmp::render_tree::node* DstNode) {
  u32 FirstChildIndex = NodeIndex;
  u32 LastChildIndex  = NodeIndex + ChildCount;

  for (int i = FirstChildIndex; i < LastChildIndex; ++i)
  {
    gltf_tmp::render_tree::node* Child = &NodeArray[i];
    Child->Parent = DstNode;
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

  size_t ResultNodeIndex = NodeIndex + ChildCount;
  return ResultNodeIndex;
}

void CopyRenderTree(const gltf_tmp::render_tree* Src, gltf_tmp::render_tree* Dst, size_t RenderTreeSize)
{
  bptr MemScan = AdvanceBytePointer(Dst, sizeof(gltf_tmp::render_tree));
  
  size_t NodeCount = Src->NodeCount;
  Dst->NodeCount = NodeCount;
  Dst->Nodes = (gltf_tmp::render_tree::node*) MemScan;
  size_t NodesSize = NodeCount * sizeof(gltf_tmp::render_tree::node);
  MemScan = AdvanceBytePointer(MemScan, NodesSize);

  Assert((MemScan - ((bptr) Dst)) == RenderTreeSize);

  Dst->Root = &Dst->Nodes[0];
  node_queue SrcQueue = NodeQueue(NodeCount);
  node_queue DstQueue = NodeQueue(NodeCount);
  Push(SrcQueue, Src->Root);
  Push(DstQueue, Dst->Root);
  
  size_t NodeHeadIndex = 1;
  while(!IsEmpty(SrcQueue) && !IsEmpty(DstQueue))
  {
    gltf_tmp::render_tree::node* DstNode = Pop(DstQueue);
    gltf_tmp::render_tree::node* SrcNode = Pop(SrcQueue);

    DstNode->ChildCount = SrcNode->ChildCount;
    DstNode->Mesh = SrcNode->Mesh;
    CopyTransforms(SrcNode, DstNode);

    NodeHeadIndex = MapChildNodes(NodeHeadIndex, SrcNode->ChildCount, Dst->Nodes, DstNode);
    gltf_tmp::render_tree::node* SrcChild = SrcNode->FirstChild;
    while(SrcChild)
    {
      Push(SrcQueue,SrcChild);
      SrcChild = SrcChild->NextSibling;
    }

    gltf_tmp::render_tree::node* DstChild = DstNode->FirstChild;
    while(DstChild)
    {
      Push(DstQueue, DstChild);
      DstChild = DstChild->NextSibling;
    }
  }

  Assert(IsEmpty(SrcQueue) && IsEmpty(DstQueue));
}

gltf_tmp::render_tree* LoadRenderTree(const c8* UniqueName, const c8* Path, const gltf_tmp::render_tree* RenderTree, key* ResultKey)
{
  midx RenderTreeSize = GetRenderTreeSize(RenderTree);
  header* Header = CreateHeader(type::RENDER_TREE, UniqueName, UniqueName, Path, RenderTreeSize);
  gltf_tmp::render_tree* Result = (gltf_tmp::render_tree*) Header->Data;

  CopyRenderTree(RenderTree, Result, RenderTreeSize);
  
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}

}