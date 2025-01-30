#pragma once

//#include "math/affine_transformations.h"
//struct component_camera;
//struct component_controller;
//struct component_hitbox;
//struct component_position;
//struct component_electrical;
//struct component_connector_pin;
//struct keyboard_input;
//struct mouse_input;
//struct game_controller_input;

struct entity_manager;

//struct component_controller
//{
//  keyboard_input* Keyboard;
//  mouse_input* Mouse;
//  game_controller_input* Controller;
//};

enum component_type
{
  COMPONENT_FLAG_NONE               = 0,
  COMPONENT_FLAG_POSITION           = 1<<0,
  COMPONENT_FLAG_END                = 1<<1
  //COMPONENT_FLAG_CONTROLLER         = 1<<0,
  //COMPONENT_FLAG_COLLIDER           = 1<<2,
  //COMPONENT_FLAG_DYNAMICS           = 1<<3,
  //COMPONENT_FLAG_RENDER             = 1<<4,
};

// Initiates the backend entity manager with specific components
entity_manager* CreateEntityManager();

//#define GetCameraComponent(EntityID) ((component_camera*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_CAMERA))
//#define GetControllerComponent(EntityID) ((component_controller*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_CONTROLLER))
//#define GetHitboxComponent(EntityID) ((component_hitbox*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_HITBOX))
#define GetPositionComponent(EntityID) ((component_position*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_POSITION))
//#define GetElectricalComponent(EntityID) ((component_electrical*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_ELECTRICAL))
//#define GetConnectorPinComponent(EntityID) ((component_connector_pin*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_CONNECTOR_PIN))
//#define GetWireComponent(EntityID) ((component_wire*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_WIRE))
//#define GetWireJunction(EntityID) ((component_wire_junction*) GetComponent(GlobalState->World.EntityManager, EntityID, COMPONENT_FLAG_WIRE_JUNCTION))