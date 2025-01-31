#pragma once
#include "commons/types.h"
#include "containers/chunk_list.h"

namespace ecs{

struct component_head;
struct component_list;
struct entity;

  

// TODO: Assemble entities into a balanced binary search tree for "easy" search and access

//  Each entity_component_mapping only point to one component_head* stored in a chunk_list of entity_component_mapping_entry in entity manager
//    Pro: Components allocated togeather would be next to each other.
//         Easy to implement
//    Con: entity_component_mapping* Next; would be the only "wasted cache" memory
// Note: Can be improved later by useing the memory management strategy we use in the interface class/
//              Pro: Don't take the 50% cache hit from having a entity_component_mapping_entry* for each component_head*
//                   Very flexible
//                   Easy to implement
//                   Have to extract management strategy to its seaparate file which can be use elsewhere
//              Con: Have to extract management strategy to its seaparate file which can be alot of work

struct entity_id
{
  u32 EntityID;
  u32 ChunkListIndex;
};

struct entity_manager
{
  memory_arena Arena;
  temporary_memory TemporaryMemory;

  u32 EntityIdCounter; // Guarantees a nuique id for new entities
  // List filled with type entity
  chunk_list EntityList;
  // List filled with entity_component_link
  chunk_list EntityComponentLinks;

  u32 ComponentTypeCount;
  component_list* ComponentTypeVector;
};

struct entity_manager_definition
{
  bitmask32 ComponentFlag;
  bitmask32 RequirementsFlag;
  u32 ComponentChunkCount;
  u32 ComponentByteSize;
};
entity_manager* CreateEntityManager(u32 EntityChunkCount, u32 EntityMapChunkCount, u32 ComponentCount, entity_manager_definition* DefinitionVector);


// Create Entities and Components
entity_id NewEntity( entity_manager* EM );
entity_id NewEntity( entity_manager* EM, bitmask32 ComponentFlags);
void NewComponents(entity_manager* EM, entity_id* EntityID, bitmask32 ComponentFlags);

// TODO: Add Unit tests
inline b32 IsValid(entity_id* EntityID)
{
  return EntityID && EntityID->EntityID;
}

// TODO: Add Unit tests
inline b32 Compare(entity_id* A, entity_id* B)
{
  // Two invalid entities are not considered to be the same
  return IsValid(A) && IsValid(B) && (A->EntityID == B->EntityID);
}

// Access Entities and components
entity_id GetEntityIDFromComponent( bptr Component );

u32 GetEntityCountHoldingTypes(entity_manager* EM, bitmask32 ComponentFlags);
void GetEntitiesHoldingTypes(entity_manager* EM, bitmask32 ComponentFlags, entity_id* ResultVector);
bptr GetComponent(entity_manager* EM, entity_id* EntityID, bitmask32 ComponentFlag);

// TODO: Add Unit tests
b32 HasComponents(entity_manager* EM, entity_id* EntityID, u32 ComponentFlags);
b32 HasOneOfComponents(entity_manager* EM, entity_id* EntityID, u32 ComponentFlags);

struct filtered_entity_iterator
{
  entity_manager* EM;
  bitmask32 ComponentFilter;
  entity* CurrentEntity;
  chunk_list_iterator ComponentIterator;
};
entity_id GetEntityID( filtered_entity_iterator* Iterator);
b32 Next(filtered_entity_iterator* EntityIterator);
filtered_entity_iterator GetComponentsOfType(entity_manager* EM, bitmask32 ComponentFlagsToFilterOn);
bptr GetComponent(entity_manager* EM, filtered_entity_iterator* ComponentList, bitmask32 ComponentFlag);

// Delete entities and components
void DeleteComponent(entity_manager* EM, entity_id* EntityID, bitmask32 ComponentFlag);
void DeleteEntity(entity_manager* EM, entity_id* EntityID);
void DeleteEntities(entity_manager* EM, u32 Count, entity_id* EntityID);

}