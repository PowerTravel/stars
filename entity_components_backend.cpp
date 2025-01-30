#include "entity_components_backend.h"
#include "commons/macros.h"


// entity_component_link: A struct holding references between entities and components
// Types is a bitmask where each set references a specific component
// NOTE: Extract the variable sized bitfield in chunk_list to its own type and use here
//       because at the moment we can only support 32 different types of components.
struct entity_component_link
{
  component_head* Component;
  entity_component_link* Next; // Next of different type

  // TODO: Enable to have many of the same component type
  //       How to associate with required components?
//  entity_component_link* NextOfSameType;
};

struct entity
{
  entity_id ID; // ID starts at 1. Index is ID-1
  u32 ChunkListIndex;
  bitmask32 ComponentFlags;
  entity_component_link* FirstComponentLink; // Points us to the associated components in the component list.
};

struct component_head
{
  entity* Entity;
  bitmask32 Type;
};

struct component_list
{
  bitmask32 Type;
  u32 Requirements;
  chunk_list Components;
};


internal inline b32
IndexOfLeastSignificantSetBit( bitmask32 EntityFlags, u32* Index )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  *Index = BitScan.Index;
  return BitScan.Found;
}

internal inline u32 IndexOfLeastSignificantSetBit( bitmask32 EntityFlags )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  Assert(BitScan.Found);
  return BitScan.Index;
}

internal inline entity* GetEntityFromID(entity_manager* EM, entity_id* EntityID) 
{ 
  entity* Entity = (entity*)  GetBlockIfItExists(&EM->EntityList, EntityID->ChunkListIndex);
  Assert(Entity->ID.EntityID == EntityID->EntityID);
  return Entity;
}

internal entity_component_link*
AllocateNewComponents(entity_manager* EM, entity* Entity, bitmask32 NewComponentFlags)
{
  u32 SetBitCount = GetSetBitCount(NewComponentFlags);
  Assert(SetBitCount > 0); // Don't try and allocate 0 links
  entity_component_link* LinkHead = 0;
  entity_component_link* Tail = 0;
  u32 ComponentIndex = 0;
  while(IndexOfLeastSignificantSetBit(NewComponentFlags, &ComponentIndex))
  {
    Assert(ComponentIndex < EM->ComponentTypeCount); // Make sure the component bit exist
    // Allocate new components
    component_list* ComponentList = EM->ComponentTypeVector + ComponentIndex;
    Assert(ComponentList->Type & (1<<ComponentIndex)); // Make sure we have the right component list

    component_head* ComponentHead = (component_head*) GetNewBlock(&EM->Arena, &ComponentList->Components);
    ComponentHead->Entity = Entity;
    ComponentHead->Type = ComponentList->Type;

    // Link the component to the entity.
    if(!Tail)
    {
      LinkHead = (entity_component_link*) GetNewBlock(&EM->Arena, &(EM->EntityComponentLinks));
      Tail = LinkHead;
    }else{
      Tail->Next = (entity_component_link*) GetNewBlock(&EM->Arena, &EM->EntityComponentLinks);
      Tail = Tail->Next;
    }

    Tail->Component = ComponentHead;

    NewComponentFlags -= ComponentList->Type;
  }
  return LinkHead;
}

internal entity_component_link*
MergeMaps(entity_component_link* OldComponentMapBase, entity_component_link* NewComponentMapBase)
{
  Assert(NewComponentMapBase);
  entity_component_link* ResultMapBase = NewComponentMapBase;
  if(OldComponentMapBase)
  {
    entity_component_link* OldComponentMap = OldComponentMapBase;
    entity_component_link* NewComponentMap = NewComponentMapBase;

    u32 OldComponentIndex = 0;
    u32 NewComponentIndex = 0;
    b32 FoundOld = IndexOfLeastSignificantSetBit(OldComponentMap->Component->Type, &OldComponentIndex);
    Assert(FoundOld);
    b32 FoundNew = IndexOfLeastSignificantSetBit(NewComponentMap->Component->Type, &NewComponentIndex);
    
    if(OldComponentIndex < NewComponentIndex)
    {
      ResultMapBase = OldComponentMap;
      OldComponentMap = OldComponentMap->Next;
    }else{
      Assert(OldComponentIndex != NewComponentIndex);
      ResultMapBase = NewComponentMap;
      NewComponentMap = NewComponentMap->Next;
    }

    entity_component_link* ResultMap = ResultMapBase;
    // Insert it into the Existing entity.
    while(NewComponentMap && OldComponentMap)
    {
      IndexOfLeastSignificantSetBit(OldComponentMap->Component->Type, &OldComponentIndex);
      IndexOfLeastSignificantSetBit(NewComponentMap->Component->Type, &NewComponentIndex);
      if(OldComponentIndex < NewComponentIndex)
      {
        ResultMap->Next = OldComponentMap;
        ResultMap = ResultMap->Next;
        OldComponentMap = OldComponentMap->Next;
      }else{
        Assert(OldComponentIndex != NewComponentIndex);
        ResultMap->Next = NewComponentMap;
        ResultMap = ResultMap->Next;
        NewComponentMap = NewComponentMap->Next;
      }
    }

    if(NewComponentMap)
    {
      Assert(!OldComponentMap);
      ResultMap->Next = NewComponentMap;
    }else if(OldComponentMap)
    {
      ResultMap->Next = OldComponentMap;
    }
  }
  
  return ResultMapBase;
}

internal void CreateAndInsertNewComponents(entity_manager* EM, entity* Entity, bitmask32 NewComponentFlags)
{
  // Make sure NewComponentFlags is _not_ in the entity already
  Assert((Entity->ComponentFlags & NewComponentFlags) == 0 );
  // Create new list of entity_component_mapping_entry;
  entity_component_link* NewComponentMapBase = AllocateNewComponents(EM, Entity, NewComponentFlags);
  entity_component_link* OldComponentMapBase = Entity->FirstComponentLink;
  entity_component_link* MergedMap = MergeMaps(OldComponentMapBase, NewComponentMapBase);
  Entity->FirstComponentLink = MergedMap;
  Entity->ComponentFlags = Entity->ComponentFlags | NewComponentFlags;
}

internal bitmask32 GetTotalRequirements(entity_manager* EM, bitmask32 ComponentFlags)
{
  bitmask32 SummedFlags = ComponentFlags;
  u32 ComponentIndex = 0;
  while(IndexOfLeastSignificantSetBit(ComponentFlags, &ComponentIndex))
  {
    Assert(ComponentIndex < EM->ComponentTypeCount );
    component_list* ComponentList = EM->ComponentTypeVector + ComponentIndex;
    // Sum all required components
    u32 Requirements = ComponentList->Requirements;
    Requirements = Requirements | GetTotalRequirements(EM, Requirements);
    SummedFlags = SummedFlags | Requirements;
    ComponentFlags -= ComponentList->Type;
  }
  return SummedFlags;
}

// Enumerates each set bit in Bit and finds what index BitSet holds
// Example:
// BitMaskOfMap:   | 0 1 1 0 1 1 0 |
// BitToEnumerate: | 0 0 0 0 1 0 0 |
// Enumeration:    |   0 1   2 3   | < Order of components in entity_component_link::Components
// Result: (2)               ^     
u32 GetIndexOfBitInComponentMap(bitmask32 BitToEnumerate, bitmask32 BitMaskOfMap)
{
  Assert(BitMaskOfMap & BitToEnumerate);
  u32 Index=0;
  u32 IndexOfBitToEnumerate = 0;
  IndexOfLeastSignificantSetBit(BitToEnumerate, &IndexOfBitToEnumerate);

  u32 IndexOfBitInMap = 0;
  while(IndexOfLeastSignificantSetBit(BitMaskOfMap, &IndexOfBitInMap))
  {
    if(IndexOfBitToEnumerate == IndexOfBitInMap)
    {
      return Index;
    }
    BitMaskOfMap -= (1<<IndexOfBitInMap);
    ++Index;
  }
  INVALID_CODE_PATH;
  return 0;
}

internal inline entity_component_link*
GetEntityComponentLink( entity* Entity, bitmask32 ComponentFlag )
{
  u32 ComponentIndex = GetIndexOfBitInComponentMap(ComponentFlag, Entity->ComponentFlags);
  Assert(GetSetBitCount(ComponentFlag) == 1);
  entity_component_link* EntityComponentMap = Entity->FirstComponentLink;
  while(ComponentIndex > 0)
  {
    Assert(EntityComponentMap->Next);
    EntityComponentMap = EntityComponentMap->Next;
    ComponentIndex--;
  }
  return EntityComponentMap;
}

internal bptr GetComponent(entity_manager* EM, entity* Entity, u32 ComponentFlag)
{
  if( !(Entity->ComponentFlags & ComponentFlag) )
  {
    return 0;
  }

  entity_component_link* ComponentMap = GetEntityComponentLink(Entity, ComponentFlag);
  component_head* Head = ComponentMap->Component;
  Assert(Head->Type == ComponentFlag);

  bptr Result =  AdvanceByType(Head, component_head);
  return Result;
}

internal component_list*
GetListWithLowestCount(entity_manager* EM, bitmask32 ComponentFlags)
{
  component_list* ListWithLowestCount = 0;
  u32 Index = 0;
  u32 SmallestSize = U32Max;
  bitmask32 FlagsToScan = ComponentFlags;
  while(IndexOfLeastSignificantSetBit(FlagsToScan, &Index))
  {
    component_list* List = EM->ComponentTypeVector + Index;
    u32 ListSize = GetBlockCount(&List->Components);
    if(ListSize < SmallestSize)
    {
      SmallestSize = ListSize;
      ListWithLowestCount = List;
    }
    FlagsToScan -= List->Type;
  }
  return ListWithLowestCount;
}

internal inline b32
DoesEntityHoldAllComponents(entity* Entity, bitmask32 Flags)
{
  b32 Result = (Entity->ComponentFlags & Flags) == Flags;
  return Result;
}


component_list CreateComponentList(memory_arena* Arena, bitmask32 TypeFlag, bitmask32 RequirmetFlags, u32 ComponentSize, u32 ComponentCountPerChunk)
{
  component_list Result = {};
  Result.Type = TypeFlag;
  Result.Requirements = RequirmetFlags;
  u32 TotalSizePerBlock = sizeof(component_head) + ComponentSize;
  Result.Components = NewChunkList(Arena, TotalSizePerBlock, ComponentCountPerChunk);
  return Result;
}


// Functions in header

entity_id NewEntity( entity_manager* EM )
{
  u32 ListIndex = 0;
  entity* NewEntity = (entity*) GetNewBlock(&EM->Arena, &EM->EntityList, &ListIndex);
  NewEntity->ID.EntityID = EM->EntityIdCounter++;
  NewEntity->ID.ChunkListIndex = ListIndex;

  return NewEntity->ID;
}

entity_id NewEntity( entity_manager* EM, bitmask32 ComponentFlags)
{
  entity_id Result = NewEntity(EM);
  NewComponents(EM, &Result, ComponentFlags);
  return Result;
}

void NewComponents(entity_manager* EM, entity_id* EntityID, u32 ComponentFlags)
{
  entity* Entity = GetEntityFromID(EM, EntityID);

  // Always allocate memory for requirements not yet fullfilled
  bitmask32 TotalRequirements = GetTotalRequirements(EM, ComponentFlags);
  bitmask32 NewComponentFlags = (~Entity->ComponentFlags) & TotalRequirements;

  CreateAndInsertNewComponents(EM, Entity, NewComponentFlags);
}

// Get a single component from an entity
// Returns 0 if no component exists
bptr GetComponent(entity_manager* EM, entity_id* EntityID, u32 ComponentFlag)
{
  Assert( GetSetBitCount(ComponentFlag) == 1);
  Assert( ComponentFlag != 0 );
  entity* Entity = GetEntityFromID(EM, EntityID);
  Assert(Entity); // If this is 0 it probably means that the Entity has been removed from the entity_manager at some point
  bptr Result = GetComponent(EM, Entity, ComponentFlag);
  return Result;
}

b32 HasComponents(entity_manager* EM, entity_id* EntityID, u32 ComponentFlags)
{
  Assert( ComponentFlags != 0 );
  entity* Entity = GetEntityFromID(EM, EntityID);
  Assert(Entity);
  b32 Result = (Entity->ComponentFlags & ComponentFlags) == ComponentFlags;
  return Result;
}

b32 HasOneOfComponents(entity_manager* EM, entity_id* EntityID, u32 ComponentFlags)
{
  Assert( ComponentFlags != 0 );
  entity* Entity = GetEntityFromID(EM, EntityID);
  Assert(Entity);
  b32 Result = (Entity->ComponentFlags & ComponentFlags) != 0;
  return Result;
}

filtered_entity_iterator GetComponentsOfType(entity_manager* EM, bitmask32 ComponentFlagsToFilterOn)
{
  component_list* SmallestList = GetListWithLowestCount(EM, ComponentFlagsToFilterOn);

  filtered_entity_iterator Result = {};
  if(SmallestList)
  {
    Result.EM = EM;
    Result.ComponentFilter = ComponentFlagsToFilterOn;
    Result.ComponentIterator = BeginIterator(&SmallestList->Components);
  }
  return Result;
};

bptr GetComponent(entity_manager* EM, filtered_entity_iterator* EntityIterator, bitmask32 ComponentFlag)
{
  Assert(GetSetBitCount(ComponentFlag) == 1);
  if(!EntityIterator->CurrentEntity)
  {
    return 0;
  }
  bptr Result = GetComponent(EM, &EntityIterator->CurrentEntity->ID, ComponentFlag);
  return Result;
}

b32 Next(filtered_entity_iterator* EntityIterator)
{  
  entity* Entity = 0;
  while(component_head* ComponentHead = (component_head*) Next(&EntityIterator->ComponentIterator))
  {
    if(DoesEntityHoldAllComponents(ComponentHead->Entity, EntityIterator->ComponentFilter))
    {
      Entity = ComponentHead->Entity;
      break;
    }
  }
  EntityIterator->CurrentEntity = Entity;

  return EntityIterator->CurrentEntity != 0;
}

entity_manager* CreateEntityManager(u32 EntityChunkCount, u32 EntityMapChunkCount, u32 ComponentCount, entity_manager_definition* DefinitionVector)
{
  entity_manager* Result = BootstrapPushStruct(entity_manager, Arena);

  Result->ComponentTypeCount = ComponentCount;
  Result->ComponentTypeVector = PushArray(&Result->Arena, ComponentCount, component_list);
  for(u32 idx = 0; idx < ComponentCount; idx++)
  {
    entity_manager_definition* Definition = DefinitionVector + idx;
    Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(Definition->ComponentFlag)] =
    CreateComponentList(&Result->Arena, Definition->ComponentFlag, Definition->RequirementsFlag, Definition->ComponentByteSize, Definition->ComponentChunkCount);  
  }

  Result->EntityIdCounter = 1;
  Result->EntityList = NewChunkList(&Result->Arena, sizeof(entity), EntityChunkCount);
  Result->EntityComponentLinks = NewChunkList(&Result->Arena, sizeof(entity_component_link), EntityMapChunkCount);

#if HANDMADE_SLOW
  for(s32 i = 0; i<ComponentCount; i++)
  {
    Assert(Result->ComponentTypeVector[i].Type);
  }
#endif

  return Result;
}

u32 GetEntityCountHoldingTypes(entity_manager* EM, bitmask32 ComponentFlags)
{
  u32 Result = 0;
  component_list* SmallestList = GetListWithLowestCount(EM, ComponentFlags);
  Assert(GetSetBitCount(ComponentFlags) > 0);
  b32 AllComponentsAreRequired = (ComponentFlags & SmallestList->Requirements & SmallestList->Type) == ComponentFlags;
  b32 OnlyLookingForASingleComponentType = GetSetBitCount(ComponentFlags) == 1;
  if(OnlyLookingForASingleComponentType || AllComponentsAreRequired)
  {
    Result = GetBlockCount(&SmallestList->Components);
  }else{
    chunk_list_iterator Iterator = BeginIterator(&SmallestList->Components);
    while(component_head* ComponentHead = (component_head*) Next(&Iterator))
    {
      if(DoesEntityHoldAllComponents(ComponentHead->Entity, ComponentFlags))
      {
        Result++;
      }
    }
  }
  
  return Result;
}

entity_id GetEntityIDFromComponent( bptr Component )
{
  component_head* Base = (component_head*) RetreatByType(Component, component_head);
  return Base->Entity->ID;
}

entity_id GetEntityID( filtered_entity_iterator* Iterator )
{
  Assert(Iterator->CurrentEntity);
  entity_id Result = Iterator->CurrentEntity->ID;
  return Result;
}

// Result must point to an array of pointers to entity_id long enough to hold pointers to all entities contianing type
void GetEntitiesHoldingTypes(entity_manager* EM, bitmask32 ComponentFlags, entity_id* ResultVector)
{
  Assert(GetSetBitCount(ComponentFlags) > 0);
  component_list* SmallestList = GetListWithLowestCount(EM, ComponentFlags);
  chunk_list_iterator Iterator = BeginIterator(&SmallestList->Components);
  while(component_head* ComponentHead = (component_head*) Next(&Iterator))
  {
    if(DoesEntityHoldAllComponents(ComponentHead->Entity, ComponentFlags))
    {
      *ResultVector++ = ComponentHead->Entity->ID;
    }
  }
}

internal inline component_list* GetComponentList(entity_manager* EM, bitmask32 ComponentFlag)
{
  u32 ComponentListIndex = IndexOfLeastSignificantSetBit(ComponentFlag);
  component_list* ComponentList = EM->ComponentTypeVector + ComponentListIndex;
  return ComponentList;
}

internal inline component_list* GetComponentList(entity_manager* EM, entity_component_link* ComponentLink)
{
  component_list* ComponentList = GetComponentList(EM, ComponentLink->Component->Type);
  return ComponentList;
}

bitmask32 GetCascadedRequirements(entity_manager* EM, entity* Entity, bitmask32 ComponentFlag)
{
  entity_component_link* ComponentLink = Entity->FirstComponentLink;
  u32 ComponentsToRemainCount = 0;
  bitmask32 TotalRequirements = ComponentFlag;
  while(ComponentLink)
  {
    component_list* ComponentList = GetComponentList(EM, ComponentLink);
    b32 ComponentToRemoveIsRequiredByAnother = TotalRequirements & ComponentList->Requirements;
    if(ComponentToRemoveIsRequiredByAnother)
    {
      bitmask32 NewTotalRequirements = ComponentList->Type | TotalRequirements;
      if(NewTotalRequirements > TotalRequirements)
      {
        TotalRequirements = NewTotalRequirements;
        // If we added a new flag to the summed flags, the newly added flag may itself have 
        // requirements earlier in the component list, therefore we have to look through all the 
        // links again. 
        // NOTE: Since all component types are known at start up, the reverse-requirement chain
        //       Can be calculated and chached at entity_manager initialization.
        ComponentLink = Entity->FirstComponentLink;
      }
    }
    ComponentLink = ComponentLink->Next;
  }
  Assert((TotalRequirements & Entity->ComponentFlags) == TotalRequirements);
  return TotalRequirements;
}

void DeleteComponents(entity_manager* EM, entity_id* EntityID, bitmask32 ComponentFlag)
{
  entity* Entity = GetEntityFromID(EM, EntityID);
  
  bitmask32 TotalRequirements = GetCascadedRequirements(EM, Entity, ComponentFlag);

  temporary_memory TempMem = BeginTemporaryMemory(&EM->Arena);

  u32 ComponentCount = GetSetBitCount(Entity->ComponentFlags);

  Assert(ComponentCount); // For now we cannot delete components in entities with no components. Can be handled if needed.

  // We add +1 for saftey to have some 0-space after the last link. Is necessary if we remove 0 links (should not happend I don't think)
  entity_component_link** ComponentsToRemain = PushArray(&EM->Arena, ComponentCount+1, entity_component_link*);
  entity_component_link* ComponentLink = Entity->FirstComponentLink;
  u32 ComponentsToRemainCount = 0;
  while(ComponentLink)
  {
    b32 ComponentShouldBeRemoved = (ComponentLink->Component->Type & TotalRequirements) == ComponentLink->Component->Type;
    if(ComponentShouldBeRemoved)
    {
      entity_component_link* LinkToRemove = ComponentLink;
      ComponentLink = ComponentLink->Next;

      component_list* ComponentList = GetComponentList(EM, LinkToRemove);

      FreeBlock(&ComponentList->Components, (bptr) LinkToRemove->Component);
      FreeBlock(&EM->EntityComponentLinks, (bptr) LinkToRemove);
    }else{
      ComponentsToRemain[ComponentsToRemainCount++] = ComponentLink;
      ComponentLink = ComponentLink->Next;
    }
  }
  
  // Asserting we actually removed some components.
  Assert(ComponentsToRemainCount < ComponentCount);
 
  // Reattach the remaining components to the entity
  Entity->FirstComponentLink = ComponentsToRemain[0];
  u32 LinkIndex = 1;
  ComponentLink = Entity->FirstComponentLink;
  while(ComponentLink)
  {
    // This is why it's safe to have an extra 0-space at the end of ComponentsToRemain.
    // The final "ComponentLink->Next" has to be a 0. We don't want to stick garbage memory there.
    ComponentLink->Next = ComponentsToRemain[LinkIndex++];
    ComponentLink = ComponentLink->Next;
  }
  
  Entity->ComponentFlags -= TotalRequirements;

  // Makes sure that if we have 0 components left, the FirstComponentLink is also 0
  // Or if we have components left we also have a link left
  Assert((Entity->ComponentFlags == 0 && Entity->FirstComponentLink == 0) || 
         (Entity->ComponentFlags != 0 && Entity->FirstComponentLink != 0))

  EndTemporaryMemory(TempMem);
}

void DeleteEntities(entity_manager* EM, u32 Count, entity_id* EntityID)
{
  for(u32 Index = 0; Index < Count; ++Index)
  {
    DeleteEntity(EM, EntityID+Index);
  }
}

void DeleteEntity(entity_manager* EM, entity_id* EntityID)
{
  entity* Entity = GetEntityFromID(EM, EntityID);
  bitmask32 ComponentFlags = Entity->ComponentFlags;
  entity_component_link* ComponentLink = Entity->FirstComponentLink;
  
  while(ComponentLink)
  {
    entity_component_link* ComponentLinkToRemove = ComponentLink;
    ComponentLink = ComponentLink->Next;

    u32 ComponentListIndex = IndexOfLeastSignificantSetBit(ComponentLinkToRemove->Component->Type);
    component_list* ComponentList = EM->ComponentTypeVector + ComponentListIndex;

    FreeBlock(&ComponentList->Components, (bptr) ComponentLinkToRemove->Component);
    FreeBlock(&EM->EntityComponentLinks, (bptr) ComponentLinkToRemove);
    
  }

  FreeBlock(&EM->EntityList, (bptr) Entity);
}