#pragma once

#include "commons/jstring.h"
#include "asset_types.h"
#include "asset_manager.h"
#include "gltf_mapper.h"
#include "obj_mapper.h"
#include "io/obj.h"

namespace asset {

enum class asset_file_type {
  UNKNOWN,
  TGA,
  PNG,
  OBJ,
  MTL,
  GLTF
};

static inline asset_file_type GetFiletypeFromEnding(const char* Path) {

  static const char* PathEnding = Path - 4;
  static const char TGAEnding[] = ".tga";
  static const char PNGEnding[] = ".png";
  static const char OBJEnding[] = ".obj";
  static const char MTLEnding[] = ".mtl";
  static const char GLTFEnding[] = ".gltf";

  size_t Length = cmn::Length(Path);

  asset_file_type Result = asset_file_type::UNKNOWN;
  if(cmn::Equals(Path + Length - 4, TGAEnding)) {
    Result = asset_file_type::TGA;
  }else if(cmn::Equals(Path + Length - 4, PNGEnding)){
    Result = asset_file_type::PNG;
  }else if(cmn::Equals(Path + Length - 4, OBJEnding)){
    Result = asset_file_type::OBJ;
  }else if(cmn::Equals(Path + Length - 4, MTLEnding)){
    Result = asset_file_type::MTL;
  }else if(cmn::Equals(Path + Length - 5, GLTFEnding)){
    Result = asset_file_type::GLTF;
  }

  return Result;
}


file_local u32 CopyObjBitmapToTexture(const c8* Key, const obj_bitmap* ObjBitmap)
{
  if(!ObjBitmap){return 0;};

  Assert(ObjBitmap->BPP == 32);
  u32 ImageSizeBytes = sizeof(image) + ObjBitmap->Width * ObjBitmap->Height * ObjBitmap->BPP / 8.f;

  header* Header   = asset::CreateHeader(type::IMAGE, Key, ObjBitmap->Name, ObjBitmap->Path, ImageSizeBytes);
  image* Image = (image*) Header->Data;
  Image->Channels = 4;
  Image->Width    = ObjBitmap->Width;
  Image->Height   = ObjBitmap->Height;
  Image->Pixels   = AdvanceBytePointer(Image, sizeof(image));
  utils::Copy(ImageSizeBytes, ObjBitmap->Pixels, Image->Pixels);
  return Header->Key;
}


static int LoadTga2(const char* Path, const char* UniqueName) {
  const obj_bitmap* Tga = LoadTGA([](u32 ByteSize){
    return PushSize(GlobalTransientArena, ByteSize);
  }, Path);
  const asset::image Image = ToImage(Tga);
  u32 Result = 0;
  image* LoadedImage = LoadImage(UniqueName, UniqueName, Path, &Image, &Result);
  return Result;
}

static int LoadPng2(const char* Path, const char* UniqueName) {
  Assert(0);
  return 0;
}

static int LoadGltf(const char* Path, const char* UniqueName) {

  
  size_t FullLength = jstr::StringLength( Path );
  Assert(FullLength < 255);


  char* OneBeforeLastSlash = jstr::FindLastOf( "\\", Path)-1;
  char* OnePastLastSlash = OneBeforeLastSlash+2;
  OneBeforeLastSlash[1] = '\0';

  char FolderBuf[256] = {};
  size_t FolderLength = jstr::StringLength( Path );
  jstr::CopyStrings( FolderLength, Path, FolderLength+1, FolderBuf );

  char FileNameBuf[256] = {};
  size_t FileNameLength = jstr::StringLength( OnePastLastSlash );
  jstr::CopyStrings( FileNameLength, OnePastLastSlash, FileNameLength+1, FileNameBuf );
  

  gltf::raw_gltf_data Gltf = gltf::Load(FolderBuf, FileNameBuf,
  [](const char* Path, size_t* Size){
    debug_read_file_result ReadResult = Platform.DEBUGPlatformReadEntireFile(Path);
    *Size = ReadResult.ContentSize;
    return ReadResult.Contents;
  },
  [](void* FileDataToFree){
    Platform.DEBUGPlatformFreeFileMemory(FileDataToFree);
  });

  size_t RenderTreeCount = 0;
  gltf_tmp::render_tree* RenderTrees = asset::gltf_tmp::ToRenderTree(&Gltf, &RenderTreeCount);

  u32 ResultKey = 0;
  for (int i = 0; i < RenderTreeCount; ++i)
  {
    asset::LoadRenderTree(UniqueName, Path, &RenderTrees[i], &ResultKey);
  }
  

  gltf::Free(&Gltf);
  return ResultKey;
}

int Load(const char* Path, const char* UniqueName = 0)
{
  asset_file_type FileType = GetFiletypeFromEnding(Path);
  int AssetHandle = 0;
  switch(FileType)
  {
    case asset_file_type::TGA: {
      AssetHandle = LoadTga2(Path, UniqueName);
    } break;
    case asset_file_type::PNG: {
      AssetHandle = LoadPng2(Path, UniqueName);
    } break;
    case asset_file_type::OBJ: {
      asset::gltf_tmp::render_tree* Asset = LoadObj(Path, UniqueName);
      AssetHandle = asset::ToHeader(Asset)->Key;
    } break;
    case asset_file_type::GLTF: {
      AssetHandle = LoadGltf(Path, UniqueName);
    } break;
  }
  Assert(AssetHandle);
  return AssetHandle;
}
}