#include "asset_manager.h"
#include "platform/obj_loader.h"
#include "renderer/render_push_buffer/render_push_buffer.h"

extern asset::manager* GlobalAssetManager;
extern memory_arena* GlobalTransientArena;

namespace asset {

struct string {
  u32 Length; // Length of string + 1;
  c8* String;
};

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

struct header {
  type Type;
  u32 Key;
  string Name;
  string FilePath;
  string KeyString;
  void* Data;
};

b32 Equals(header* HeaderA, header* HeaderB){
  b32 Result = jstr::ExactlyEquals( HeaderA->KeyString.String, HeaderB->KeyString.String );
  return Result;
}

c8* TypeToString(type Type)
{
  switch(Type)
  {
    case type::NONE: return "NONE";
    case type::OBJ: return "OBJ";
    case type::TGA: return "TGA";
    case type::GL_VERTEX_BUFFER: return "GL_VERTEX_BUFFER";
    default: {
      INVALID_CODE_PATH
    }
  }

  return '\0';
}

string CreateKeyString(type Type, const string& Name)
{
  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  
  string KeyString = {};
  KeyString.Length = TypeLength + Name.Length + 2;
  KeyString.String = (c8*) Allocate(&GlobalAssetManager->Memory, (KeyString.Length+1) * sizeof(c8));
  u32 FormattedLength = FormatString(KeyString.String, KeyString.Length+1, "%s::%s", TypeString, Name.String);
  Assert(FormattedLength == KeyString.Length);
  return KeyString;
}

u32 ToKey(type Type, c8* Name)
{
  c8 TempKeyString[512] = {};

  const c8* TypeString = TypeToString(Type);
  u32 TypeLength = jstr::StringLength(TypeString);
  u32 NameLength = jstr::StringLength(Name);
  
  string KeyString = {};
  KeyString.Length = TypeLength + NameLength + 2;
  KeyString.String = TempKeyString;
  u32 FormattedLength = FormatString(KeyString.String, KeyString.Length+1, "%s::%s", TypeString, Name);
  Assert(FormattedLength <= KeyString.Length);
  u32 Result = utils::djb2_hash(KeyString.String);
  return Result;
}

void* StorageAllocator(u32 MemorySize) {
  void* Result = Allocate(&GlobalAssetManager->Memory, MemorySize);
  return Result;
}

header* FindHeader(u32 Key) {
  header* Result = (header*) Find(&GlobalAssetManager->Headers, Key);
  return Result;
}

internal void
DeleteHeader(header* Header)
{
  DeleteString(Header->Name);
  DeleteString(Header->FilePath);
  DeleteString(Header->KeyString);
  FreeMemory(&GlobalAssetManager->Memory, Header);
  Delete(&GlobalAssetManager->Headers, Header->Key);
}

void FreeAsset(header* Header)
{
  switch(Header->Type)
  {
    case type::OBJ: {
      FreeObj([](void* Data){
        FreeMemory(&GlobalAssetManager->Memory, Data);
      }, (obj_loaded_file*) Header->Data);
    } break;
    case type::TGA: {
      FreeBitmap([](void* Data){
        FreeMemory(&GlobalAssetManager->Memory, Data);
      }, (obj_bitmap*) Header->Data);
    };
    case type::GL_VERTEX_BUFFER: {
      // LoadGLVertexBuffer allocates the whole mesh as a contious block
      FreeMemory(&GlobalAssetManager->Memory, Header->Data);
    }
    default: {
      INVALID_CODE_PATH
    };
  }
 
  DeleteHeader(Header);
}

header* CreateHeader(type Type, c8* Name, c8* Path, void* Data)
{
  header* Result = (header*) Allocate(&GlobalAssetManager->Memory, sizeof(header));
  Result->Type = Type;
  Result->Name = CreateString(Name, 255);
  Result->FilePath = CreateString(Path, 255);
  Result->KeyString = CreateKeyString(Type, Result->Name);
  Result->Key = utils::djb2_hash(Result->KeyString.String);
  Result->Data = Data;
  return Result;
}

u32 Load(type Type, c8* Name, c8* Path, void* Asset)
{
  header* Header = CreateHeader(Type, Name, Path, Asset);
  Insert(&GlobalAssetManager->Headers, Header->Key, (void*) Header);
  return Header->Key;
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


obj_loaded_file* LoadObj(c8* Name, c8* Path, u32* ResultKey)
{
  obj_loaded_file* Result =  ReadOBJFile(StorageAllocator, GlobalTransientArena, Path);
  u32 Key = Load(type::OBJ, Name, Path, (void*) Result);
  if(ResultKey)
  {
    *ResultKey = Key;
  }
  return Result;
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

  u32 Key = Load(type::GL_VERTEX_BUFFER, Name, "N/A", (void*) VertexBuffer);
  if(ResultKey)
  {
    *ResultKey = Key;
  }
  return VertexBuffer;
}

obj_bitmap* LoadTga(c8* Name, c8* Path, u32* ResultKey)
{
  obj_bitmap* Result = LoadTGA(StorageAllocator, Path);
  u32 Key = Load(type::TGA, Name, Path, (void*) Result);
  if(ResultKey)
  {
    *ResultKey = Key;
  }
  return Result;
}

}