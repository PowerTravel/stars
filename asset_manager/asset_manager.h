#pragma once
#include "asset_types.h"
#include "commons/types.h"
#include "commons/string.h"
#include "commons/macros.h"
#include "ecs/components/component_collider.h"
#include "containers/linked_memory.h"
#include "platform/platform_containers.h"

#define ASSET_MAX_NAME_LENGTH 256
#define ASSET_MAX_PATH_LENGTH 256
#define ASSET_MAX_KEY_LENGTH 2048

namespace asset {
  struct manager;
}
extern asset::manager* GlobalAssetManager;

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

struct manager {
  memory_arena Arena;
  linked_memory Memory;

  // Key is Asset Handle (Hash of asset name)
  // Value is pointer to headers
  rb_tree Headers;
};


struct AssetAllocator {
  static CMN_MALLOC_FUNCTION {
    Assert(GlobalAssetManager);
    void* Result = LmAllocate(&GlobalAssetManager->Memory, sz);
    return Result;
  }

  static CMN_REALLOC_FUNCTION {
    Assert(GlobalAssetManager);
    void* Result = LmAllocate(&GlobalAssetManager->Memory, sz);
    return p;
  }

  static CMN_FREE_FUNCTION
  {
    Assert(GlobalAssetManager);  
  }
};


manager* CreateAssetManager() {
  manager* Result = BootstrapPushStruct(manager, Arena);
  Result->Memory = NewLinkedMemory(&Result->Arena, Megabytes(1));
  Result->Headers = NewRBTree(&Result->Arena, 256, 256);

  return Result;
}

header* CreateHeader(type Type, const c8* UniqueName, const c8* Name, const c8* Path, midx DataSize);

header* ToHeader(mesh* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(geometry* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(image* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(pbr_material* Asset) {
  return (header*) RetreatByType(Asset, header);
}

header* ToHeader(phong_material* Asset) {
  return (header*) RetreatByType(Asset, header);
}

c8* CreateUniqueName(const c8* Prefix, const c8* Name, const c8* Postfix, u32 Index = 0, u32 MaxCount = 0);
key ToKey(type Type, const c8* UniqueName);
void* Find(type Type, key Key);
void* Find(type Type, const c8* Name);
void Free(type Type, key Key);
void Free(type Type, const c8* Name);




typedef cmn::n_tree_<render_tree_data, AssetAllocator> render_tree_a;

inline asset::image*             FindImage(image_id ID)                  { return (asset::image*)             Find(type::IMAGE,          ID);}
inline asset::mesh*              FindMesh(mesh_id ID)                    { return (asset::mesh*)              Find(type::MESH,           ID);}
inline asset::pbr_material*      FindPbrMaterial(pbr_material_id ID)     { return (asset::pbr_material*)      Find(type::PBR_MATERIAL,   ID);}
inline asset::phong_material*    FindPhongMaterial(phong_material_id ID) { return (asset::phong_material*)    Find(type::PHONG_MATERIAL, ID);}
inline asset::camera*            FindCamera(camera_id ID)                { return (asset::camera*)            Find(type::CAMERA,         ID);}
inline asset::render_tree_a*     FindRenderTree(render_tree_id ID)       { return (asset::render_tree_a*)     Find(type::RENDER_TREE,    ID);}
inline asset::package*           FindPackage(package_id ID)              { return (asset::package*)           Find(type::PACKAGE,        ID);}
inline asset::geometry*          FindGeometry(geometry_id ID)            { return (asset::geometry*)          Find(type::GEOMETRY,       ID);}

///  New loaders for gltf
// Bah, using 'id', 'handle' and 'key' interchangeably
image*              LoadImage(const c8* UniqueName, const c8* Name, const c8* Path, const image* Image, image_id* ResultKey = 0);
pbr_material*       LoadPbrMaterial(const c8* UniqueName, const pbr_material* Image, pbr_material_id* ResultKey = 0);
phong_material*     LoadPhongMaterial(const c8* UniqueName, const phong_material* Material, phong_material_id* ResultKey = 0);
geometry*           LoadGeometry(const c8* UniqueName, const geometry* Geometry, geometry_id* ResultKey = 0);
mesh*               LoadMesh(const c8* UniqueName, const mesh* Mesh, mesh_id* ResultKey = 0);
camera*             LoadCamera(const c8* UniqueName, const camera* Camera, camera_id* ResultKey = 0);
package*            LoadPackage(const c8* UniqueName, const c8* Path, const package* RenderTree, package_id* ResultKey = 0);


struct AssetManagerAllocator;

template <typename Allocators>
render_tree_a* LoadRenderTree(const c8* UniqueName, const c8* Path, render_tree<Allocators>* RenderTree, render_tree_id* ResultKey){
  midx RenderTreeSize = sizeof(render_tree);
  header* Header = CreateHeader(type::RENDER_TREE, UniqueName, UniqueName, Path, RenderTreeSize);
  render_tree<AssetAllocator>* Result = (render_tree<AssetAllocator>*) Header->Data;
  *Result = RenderTree->Copy<AssetAllocator>();
  
  if(ResultKey)
  {
    *ResultKey = Header->Key;
  }
  return Result;
}


#define ASSET_TREE_TRAVERSAL_CALLBACK(name) void name(asset::header* Header, void* UserData)
typedef ASSET_TREE_TRAVERSAL_CALLBACK( asset_tree_traversal_function );

void InOrderTraverse(asset_tree_traversal_function* Callback, void* UserData);

} // namespace asset



