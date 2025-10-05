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
  static const char PNGEnding[] = ".tga";
  static const char OBJEnding[] = ".obj";
  static const char MTLEnding[] = ".mtl";
  static const char GLTFEnding[] = ".gltf";

  asset_file_type Result = asset_file_type::UNKNOWN;
  if(cmn::Equals(Path - 4, TGAEnding)) {
    Result = asset_file_type::TGA;
  }else if(cmn::Equals(Path - 4, PNGEnding)){
    Result = asset_file_type::PNG;
  }else if(cmn::Equals(Path - 4, OBJEnding)){
    Result = asset_file_type::OBJ;
  }else if(cmn::Equals(Path - 4, MTLEnding)){
    Result = asset_file_type::MTL;
  }else if(cmn::Equals(Path - 5, GLTFEnding)){
    Result = asset_file_type::GLTF;
  }

  return Result;
}

static void LoadTga2(const char* Path) {

}
static void LoadPng2(const char* Path) {

}

static void LoadObj2(const c8* Path, const c8* UniqueName)
{

}

static void LoadGltf2(const char* Path) {

}

void Load(const char* Path, const char* UniqueName = 0)
{
  asset_file_type FileType = GetFiletypeFromEnding(Path);
  switch(FileType)
  {
    case asset_file_type::TGA: {
      LoadTga2(Path);
    } break;
    case asset_file_type::PNG: {
      LoadPng2(Path);
    } break;
    case asset_file_type::OBJ: {
      LoadObj2(Path, UniqueName);
    } break;
    case asset_file_type::GLTF: {
      LoadGltf2(Path);
    } break;
  }
}
}