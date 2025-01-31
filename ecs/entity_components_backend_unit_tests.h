#include "entity_components_backend.h"
namespace entity_components_backend_tests
{
namespace ecs{

enum component_type_test
{
  TEST_COMPONENT_FLAG_NONE  = 0,
  TEST_COMPONENT_FLAG_A     = 1<<0,
  TEST_COMPONENT_FLAG_B     = 1<<1,
  TEST_COMPONENT_FLAG_C     = 1<<2,
  TEST_COMPONENT_FLAG_D     = 1<<3,
  TEST_COMPONENT_FLAG_E     = 1<<4,
  TEST_COMPONENT_FLAG_FINAL = 1<<5,
};

struct test_component_a
{
  u32 a;
};

struct test_component_b
{
  u32 a;
  u32 b;
};

struct test_component_c
{
  u32 a;
  u32 b;
  u32 c;
};

struct test_component_d
{
  u32 a;
  u32 b;
  u32 c;
  u32 d;
};

struct test_component_e
{
  u32 a;
  u32 b;
  u32 c;
  u32 d;
  u32 e;
};


entity_manager* CreateEntityManager(u32 EntityChunkCount, u32 ChunkSizeA, u32 ChunkSizeB, u32 ChunkSizeC, u32 ChunkSizeD, u32 ChunkSizeE)
{
  entity_manager_definition Definitions[] =
  {
    {TEST_COMPONENT_FLAG_A, COMPONENT_FLAG_NONE,   ChunkSizeA, sizeof(test_component_a)},
    {TEST_COMPONENT_FLAG_B, COMPONENT_FLAG_NONE,   ChunkSizeB, sizeof(test_component_b)},
    {TEST_COMPONENT_FLAG_C, TEST_COMPONENT_FLAG_A, ChunkSizeC, sizeof(test_component_c)},
    {TEST_COMPONENT_FLAG_D, COMPONENT_FLAG_NONE,   ChunkSizeD, sizeof(test_component_d)},
    {TEST_COMPONENT_FLAG_E, TEST_COMPONENT_FLAG_C, ChunkSizeE, sizeof(test_component_e)}
  }; 
  
  entity_manager* Result = CreateEntityManager(EntityChunkCount, EntityChunkCount, ArrayCount(Definitions), Definitions);

  Assert(Result->ComponentTypeCount == ArrayCount(Definitions));
  Assert(GetCapacity(&(Result->EntityList)) == EntityChunkCount);
  Assert(GetCapacity(&(Result->EntityComponentLinks)) == EntityChunkCount);
  Assert(GetCapacity(&Result->ComponentTypeVector[0].Components) == ChunkSizeA);
  Assert(GetCapacity(&Result->ComponentTypeVector[1].Components) == ChunkSizeB);
  Assert(GetCapacity(&Result->ComponentTypeVector[2].Components) == ChunkSizeC);
  Assert(GetCapacity(&Result->ComponentTypeVector[3].Components) == ChunkSizeD);
  Assert(GetCapacity(&Result->ComponentTypeVector[4].Components) == ChunkSizeE);
  Assert(GetBlockCount(&Result->ComponentTypeVector[0].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[1].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[2].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[3].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[4].Components) == 0);

  return Result;
}

void AssertComponentCounts(entity_manager* EntityManager, u32 EntityCount, u32 ACount, u32 BCount, u32 CCount, u32 DCount, u32 ECount)
{
  Assert(GetBlockCount(&EntityManager->EntityList) == EntityCount);
  Assert(GetBlockCount(&EntityManager->ComponentTypeVector[0].Components) == ACount);
  Assert(GetBlockCount(&EntityManager->ComponentTypeVector[1].Components) == BCount);
  Assert(GetBlockCount(&EntityManager->ComponentTypeVector[2].Components) == CCount);
  Assert(GetBlockCount(&EntityManager->ComponentTypeVector[3].Components) == DCount);
  Assert(GetBlockCount(&EntityManager->ComponentTypeVector[4].Components) == ECount);
}

void RunUnitTestsA(memory_arena* Arena)
{
  ScopedMemory ScopedMem = ScopedMemory(Arena);
  u32 EntityChunkCount = 10;
  u32 ChunkSizeA = 2;
  u32 ChunkSizeB = 4;
  u32 ChunkSizeC = 8;
  u32 ChunkSizeD = 16;
  u32 ChunkSizeE = 16;
  entity_manager* EntityManager = CreateEntityManager(EntityChunkCount, ChunkSizeA, ChunkSizeB, ChunkSizeC, ChunkSizeD, ChunkSizeE);
  AssertComponentCounts(EntityManager, 0,0,0,0,0,0);
  u32 ComponentCountA = ChunkSizeA * 5-1; // 9
  entity_id* EntityIDs = PushArray(Arena, ComponentCountA, entity_id);
  {
    // Test creating alot of TEST_COMPONENT_FLAG_A and loop
    // Entity      1 2 3 4 5 6 7 8 9
    // Components  a a a a a a a a a
    
    for(u32 i = 0; i<ComponentCountA; i++)
    {
      entity_id EntityID = NewEntity( EntityManager );
      NewComponents(EntityManager, &EntityID, TEST_COMPONENT_FLAG_A);
      test_component_a* A = (test_component_a*) GetComponent(EntityManager, &EntityID, TEST_COMPONENT_FLAG_A);
      EntityIDs[i] = EntityID;
      A->a = EntityID.EntityID;
    }
    AssertComponentCounts(EntityManager, 9,9,0,0,0,0);
    // Capacity is the number of chunks needed to hold all allocated components. In our case ChunkSizeA x 5 = 10;
    Assert(GetCapacity(&EntityManager->ComponentTypeVector[0].Components) == ComponentCountA+1);
    Assert(GetBlockCount(&EntityManager->ComponentTypeVector[0].Components) == ComponentCountA);
  }

  {
    // Normal loop of all entitiy indexes
    // Entity      1 2 3 4 5 6 7 8 9
    // Components  a a a a a a a a a
    for(u32 i = 0; i<ComponentCountA; i++)
    {
      entity_id* EntityID = EntityIDs + i;
      test_component_a* A =  (test_component_a*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_A);
      test_component_b* B =  (test_component_b*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_B);
      test_component_c* C =  (test_component_c*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_C);
      test_component_d* D =  (test_component_d*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_D);
      test_component_e* E =  (test_component_e*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_E);
      Assert(A);
      Assert(A->a == EntityID->EntityID);
      Assert(!B);
      Assert(!C);
      Assert(!D);
      Assert(!E);
    }
    AssertComponentCounts(EntityManager,9,9,0,0,0,0);
  }

  {
    // Testing iterator functionality
    // Entity      1 2 3 4 5 6 7 8 9
    // Components  a a a a a a a a a
    filtered_entity_iterator Iterator = GetComponentsOfType(EntityManager, TEST_COMPONENT_FLAG_A);
    u32 index = 0;
    AssertComponentCounts(EntityManager,9,9,0,0,0,0);
    while(Next(&Iterator))
    {
      test_component_a* A =  (test_component_a*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_A);
      test_component_b* B =  (test_component_b*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_B);
      test_component_c* C =  (test_component_c*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_C);
      test_component_d* D =  (test_component_d*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_D);
      test_component_e* E =  (test_component_e*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_E);

      Assert(A);
      Assert(!B);
      Assert(!C);
      Assert(!D);
      Assert(!E);

      entity_id* EntityID = EntityIDs + index++;
      Assert(A->a == EntityID->EntityID);
    }
    Assert(index == ComponentCountA);
  }

  {
    // See that we get 0 components of type B
    // Entity      1 2 3 4 5 6 7 8 9
    // Components  a a a a a a a a a
    filtered_entity_iterator Iterator = GetComponentsOfType(EntityManager, TEST_COMPONENT_FLAG_B);
    u32 index = 0;
    while(Next(&Iterator))
    {
      index++;
    }
    Assert(index == 0);
  }

  entity_id Entity10 = {};
  {
    // Testing creating several components using recursive requirements gathering E requires C which requires A
    // Entity      1 2 3 4 5 6 7 8 9 10
    // Components  a a a a a a a a a a
    //
    //                               c
    //                               
    //                               e
    AssertComponentCounts(EntityManager,9,9,0,0,0,0);
    Entity10 = NewEntity( EntityManager );
    NewComponents(EntityManager, &Entity10, TEST_COMPONENT_FLAG_E);
    AssertComponentCounts(EntityManager,10,10,0,1,0,1);
    test_component_a* A =  (test_component_a*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_A);
    test_component_b* B =  (test_component_b*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_B);
    test_component_c* C =  (test_component_c*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_C);
    test_component_d* D =  (test_component_d*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_D);
    test_component_e* E =  (test_component_e*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_E);
    Assert(A);
    Assert(!B);
    Assert(C);
    Assert(!D);
    Assert(E);

    A->a = 1;
    C->a = 4;
    C->b = 5;
    C->c = 6;
    E->a = 11;
    E->b = 12;
    E->c = 13;
    E->d = 14;
    E->e = 15;
  }

  {
    // Testing adding additional B and D entity 7
    // Checks that we can insert the mapping in the correct order (b between a and c, d between c and e)
    // Entity      1 2 3 4 5 6 7 8 9 10
    // Components  a a a a a a a a a a
    //                               b
    //                               c
    //                               d
    //                               e
    NewComponents(EntityManager, &Entity10, TEST_COMPONENT_FLAG_B | TEST_COMPONENT_FLAG_D);
    AssertComponentCounts(EntityManager,10,10,1,1,1,1);
    test_component_a* A =  (test_component_a*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_A);
    test_component_b* B =  (test_component_b*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_B);
    test_component_c* C =  (test_component_c*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_C);
    test_component_d* D =  (test_component_d*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_D);
    test_component_e* E =  (test_component_e*) GetComponent(EntityManager, &Entity10, TEST_COMPONENT_FLAG_E);

    Assert(A);
    Assert(B);
    Assert(C);
    Assert(D);
    Assert(E);

    Assert(A->a == 1);
    B->a = 2;
    B->b = 3;
    Assert(C->a == 4);
    Assert(C->b == 5);
    Assert(C->c == 6);
    D->a = 7;
    D->b = 8;
    D->c = 9;
    D->d = 10;
    Assert(E->a == 11);
    Assert(E->b == 12);
    Assert(E->c == 13);
    Assert(E->d == 14);
    Assert(E->e == 15);
  }

  {
    // Testing adding new entity without a and adding additional B to entity 2 and iterate through all entities with a and b
    // Entity      1 2 3 4 5 6 7 8 9 10 11
    // Components  a a a a a a a a a a  
    //               b               b  b
    //                               c
    //                               d
    //                               e

    {
      // Adding new entity with component b
      entity_id Entity11 = NewEntity( EntityManager );
      Assert(Entity11.EntityID == 11);
      NewComponents(EntityManager, &Entity11, TEST_COMPONENT_FLAG_B);
      AssertComponentCounts(EntityManager,11,10,2,1,1,1);
      test_component_a* A =  (test_component_a*) GetComponent(EntityManager, &Entity11, TEST_COMPONENT_FLAG_A);
      test_component_b* B =  (test_component_b*) GetComponent(EntityManager, &Entity11, TEST_COMPONENT_FLAG_B);
      test_component_c* C =  (test_component_c*) GetComponent(EntityManager, &Entity11, TEST_COMPONENT_FLAG_C);
      test_component_d* D =  (test_component_d*) GetComponent(EntityManager, &Entity11, TEST_COMPONENT_FLAG_D);
      test_component_e* E =  (test_component_e*) GetComponent(EntityManager, &Entity11, TEST_COMPONENT_FLAG_E);

      Assert(!A);
      Assert(B);
      Assert(!C);
      Assert(!D);
      Assert(!E);
      B->a = 11;
      B->b = 12;
    }
      

    {
      // Adding component b to entity 2
      entity_id* Entity2 = EntityIDs + 1;
      Assert(Entity2->EntityID == 2);
      NewComponents(EntityManager, Entity2, TEST_COMPONENT_FLAG_B);
      AssertComponentCounts(EntityManager,11,10,3,1,1,1);
      test_component_a* A =  (test_component_a*) GetComponent(EntityManager, Entity2, TEST_COMPONENT_FLAG_A);
      test_component_b* B =  (test_component_b*) GetComponent(EntityManager, Entity2, TEST_COMPONENT_FLAG_B);
      test_component_c* C =  (test_component_c*) GetComponent(EntityManager, Entity2, TEST_COMPONENT_FLAG_C);
      test_component_d* D =  (test_component_d*) GetComponent(EntityManager, Entity2, TEST_COMPONENT_FLAG_D);
      test_component_e* E =  (test_component_e*) GetComponent(EntityManager, Entity2, TEST_COMPONENT_FLAG_E);

      Assert(A);
      Assert(B);
      Assert(!C);
      Assert(!D);
      Assert(!E);

      Assert(A->a == 2);
      B->a = 22;
      B->b = 23;
    }

    {
      // Looping through all entities with component b and a
      u32 index = 0;
      filtered_entity_iterator Iterator = GetComponentsOfType(EntityManager, TEST_COMPONENT_FLAG_A | TEST_COMPONENT_FLAG_B);
      while(Next(&Iterator))
      {
        test_component_a* A =  (test_component_a*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_A);
        test_component_b* B =  (test_component_b*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_B);
        if(index == 0)
        {
          test_component_c* C =  (test_component_c*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_C);
          test_component_d* D =  (test_component_d*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_D);
          test_component_e* E =  (test_component_e*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_E);

          Assert(A);
          Assert(B);
          Assert(C);
          Assert(D);
          Assert(E);

          Assert(A->a == 1);
          Assert(B->a == 2);
          Assert(B->b == 3);
          Assert(C->a == 4);
          Assert(C->b == 5);
          Assert(C->c == 6);
          Assert(D->a = 7);
          Assert(D->b = 8);
          Assert(D->c = 9);
          Assert(D->d = 10);
          Assert(E->a == 11);
          Assert(E->b == 12);
          Assert(E->c == 13);
          Assert(E->d == 14);
          Assert(E->e == 15);

        }else if(index == 1)
        {
          // Newly added components
          test_component_c* C =  (test_component_c*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_C);
          test_component_d* D =  (test_component_d*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_D);
          test_component_e* E =  (test_component_e*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_E);

          Assert(A);
          Assert(B);
          Assert(!C);
          Assert(!D);
          Assert(!E);

          Assert(A->a == 2);
          Assert(B->a == 22);
          Assert(B->b == 23);
        }else
        {
          INVALID_CODE_PATH
        }
        index++;
      }
    }
  }

  // Testing adding new entity without a and adding additional B to entity 2 and iterate through all entities with a and b
  // Entity      1 2 3 4 5 6 7 8 9 10 11
  // Components  a a a a a a a a a a  
  //               b               b  b
  //                               c
  //                               d
  //                               e
  // ----> Becomes
  // Entity      1 2 3 4 5 6 7 8 9 10 11
  // Components  a   a a a a a a a    
  //                                  b
  //                                
  //                               d
  //    
  {
    entity_id Entities[2];
    entity_id Entity2={};
    {
      // Test removing components
      u32 EntityCount = GetEntityCountHoldingTypes(EntityManager, TEST_COMPONENT_FLAG_B | TEST_COMPONENT_FLAG_A);
      Assert(EntityCount == 2);
      GetEntitiesHoldingTypes(EntityManager, TEST_COMPONENT_FLAG_B | TEST_COMPONENT_FLAG_A, Entities);
      // Entities are gathererd in the order of when the components were added. 
      // Component B was the smalles common component list and Entity 10 got its component before component 2.
      Assert(Entities[0].EntityID == 10);
      Assert(Entities[0].ChunkListIndex == 9);
      Entity10 = Entities[0];
      Assert(Entities[1].EntityID == 2);
      Assert(Entities[1].ChunkListIndex == 1);
      Entity2 = Entities[1];
      AssertComponentCounts(EntityManager,11,10,3,1,1,1);
      while(EntityCount--)
      {
        DeleteComponents(EntityManager, &Entities[EntityCount], TEST_COMPONENT_FLAG_B | TEST_COMPONENT_FLAG_A);
      }
      AssertComponentCounts(EntityManager,11,8,1,0,1,0);
    }
    {
      // Delete entity 2 and add a new one.
      // Entity      1   3 4 5 6 7 8 9 10 11
      // Components  a   a a a a a a a    
      //                                  b
      //                                
      //                               d
      // 
      DeleteEntity(EntityManager, &Entity2);
      AssertComponentCounts(EntityManager,10,8,1,0,1,0);
      Assert(GetEntityCountHoldingTypes(EntityManager, TEST_COMPONENT_FLAG_D) == 1);
      Assert(GetEntityCountHoldingTypes(EntityManager, TEST_COMPONENT_FLAG_B) == 1);

      // Add new entity make sure it fills into 2nd position
      // Entity      1 12 3 4 5 6 7 8 9 10 11
      // Components  a  a a a a a a a a    
      //                                   b
      //                c                 
      //                                d
      //                e
      entity_id Entity = NewEntity(EntityManager, TEST_COMPONENT_FLAG_E);
      Assert(Entity.EntityID == 12);
      Assert(Entity.ChunkListIndex == 1);
      Assert(GetComponent(EntityManager, &Entity, TEST_COMPONENT_FLAG_A) != 0);
      Assert(GetComponent(EntityManager, &Entity, TEST_COMPONENT_FLAG_B) == 0);
      Assert(GetComponent(EntityManager, &Entity, TEST_COMPONENT_FLAG_C) != 0);
      Assert(GetComponent(EntityManager, &Entity, TEST_COMPONENT_FLAG_D) == 0);
      Assert(GetComponent(EntityManager, &Entity, TEST_COMPONENT_FLAG_E) != 0);
      AssertComponentCounts(EntityManager,11,9,1,1,1,1);
    }
  }

  {
    // Test removing entities with component A
    // Entity                         10 11
    // Components       
    //                                   b
    //                                  
    //                                d
    //                 
    u32 EntityCount = GetEntityCountHoldingTypes(EntityManager, TEST_COMPONENT_FLAG_A);
    Assert(EntityCount == 9);
    entity_id Entities[9] = {};
    GetEntitiesHoldingTypes(EntityManager, TEST_COMPONENT_FLAG_A, Entities);
    DeleteEntities(EntityManager, 9, Entities);
    AssertComponentCounts(EntityManager,2,0,1,0,1,0);
    Assert(GetBlockCount(&EntityManager->EntityList) == 2);
    Assert(GetBlockCount(&EntityManager->EntityComponentLinks) == 2);
  }

}


void RunUnitTests(memory_arena* Arena)
{
  RunUnitTestsA(Arena);
}

}}