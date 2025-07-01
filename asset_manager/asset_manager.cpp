#include "asset_manager.h"

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

void* AllocateFunction(u32 MemorySize) {
  void* Result = Allocate(&GlobalAssetManager->Memory, MemorySize);
  return Result;
}
  
void* LoadAsset(type Type, c8* Path)
{
  void* Result = 0;
  switch(Type)
  {
    case type::OBJ: {
      Result = (void*) ReadOBJFile(AllocateFunction, GlobalTransientArena, Path);
    } break;
    default: {
      INVALID_CODE_PATH
    };
  }

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

void* Load(type Type, c8* Name, c8* Path, u32* ResultKey)
{
  u32 Key = ToKey(Type, Name);
  if(FindHeader(Key))
  {
    return 0;
  }

  void* Asset = LoadAsset(Type, Path);
  header* Header = CreateHeader(type::OBJ, Name, Path, Asset);
  Insert(&GlobalAssetManager->Headers, Key, (void*) Header);
  if(ResultKey)
  {
    *ResultKey = Key;
  }

  return Asset;
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

}