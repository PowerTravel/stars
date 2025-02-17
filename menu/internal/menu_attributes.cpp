#include "menu_attributes.h"

inline internal u32
GetAttributeSize(u32 Attributes)
{
  return GetAttributeSize((container_attribute)Attributes);
}

b32 HasAttribute(container_node* Node, container_attribute Attri)
{
  return Node->Attributes & ((u32) Attri);
}

void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  Assert(!(Node->Attributes & AttributeType));

  Node->Attributes = Node->Attributes | AttributeType;
  menu_attribute_header** HeaderPtr = &Node->FirstAttribute;
  while(*HeaderPtr)
  {
    menu_attribute_header* Header = *HeaderPtr;
    HeaderPtr = &(Header->Next);
  }

  u32 TotalSize = sizeof(menu_attribute_header) + GetAttributeSize(AttributeType);
  menu_attribute_header* Attr =  (menu_attribute_header*) Allocate(&Interface->LinkedMemory, TotalSize);
  Attr->Type = AttributeType;
  *HeaderPtr = Attr;
  void* Result = (void*)(((u8*)Attr) + sizeof(menu_attribute_header));
  return Result;
}

internal u32
GetAttributeBatchSize(container_attribute Attri)
{
  u32 Result = 0;
  u32 Attributes = (u32) Attri;
  while(Attributes)
  {
    bit_scan_result ScanResult = FindLeastSignificantSetBit(Attributes);
    Assert(ScanResult.Found);

    u32 Attribute = (1 << ScanResult.Index);
    Result += GetAttributeSize(Attribute);
    Attributes -= Attribute;
  }

  return Result;
}


menu_event* GetMenuEvent(menu_interface* Interface, u32 Handle)
{
  Assert(Handle < ArrayCount(Interface->MenuEventCallbacks));
  menu_event* Event = &Interface->MenuEventCallbacks[Handle];
  return Event;
}


void DeregisterEvent(menu_interface* Interface, u32 Handle)
{
  menu_event* Event = GetMenuEvent(Interface, Handle);
  Interface->MenuEventCallbackCount--;
  ListRemove(Event);
  if(Event->OnDelete)
  {
    CallFunctionPointer(Event->OnDelete, Interface, Event->CallerNode, Event->Data);
  }
  *Event = {};
}

u8* GetAttributePointer(container_node* Node, container_attribute Attri)
{
  Assert(Attri & Node->Attributes);
  menu_attribute_header * Attribute = Node->FirstAttribute;
  u8* Result = 0;
  while(Attribute)
  {
    if(Attribute->Type == Attri)
    {
      Result = ( (u8*) Attribute  ) + sizeof(menu_attribute_header);
      break;
    }
    Attribute = Attribute->Next;
  }
  return Result;
}


void ClearMenuEvents(menu_interface* Interface, container_node* Node)
{
  if(HasAttribute(Node, ATTRIBUTE_MENU_EVENT_HANDLE))
  {
    menu_event_handle_attribtue* EventHandles = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
    for(u32 Idx = 0; Idx < EventHandles->HandleCount; ++Idx)
    {
      u32 Handle = EventHandles->Handles[Idx];
      DeregisterEvent(Interface, Handle);
      EventHandles->Handles[Idx] = 0;
    }
    EventHandles->HandleCount = 0;  
  }
}



void DeleteAllAttributes(menu_interface* Interface, container_node* Node)
{
  // TODO: Free attribute data here too?
  while(Node->FirstAttribute)
  {
    menu_attribute_header* Attribute = Node->FirstAttribute;
    Node->FirstAttribute = Node->FirstAttribute->Next;
    FreeMemory(&Interface->LinkedMemory, (void*) Attribute);
  }
  Node->Attributes = ATTRIBUTE_NONE;
}

void DeleteAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  Assert(Node->Attributes & (u32)AttributeType);

  menu_attribute_header** AttributePtr = &Node->FirstAttribute;
  while((*AttributePtr)->Type != AttributeType)
  {
    AttributePtr = &(*AttributePtr)->Next;
  }  

  menu_attribute_header* AttributeToRemove = *AttributePtr;
  *AttributePtr = AttributeToRemove->Next;

  FreeMemory(&Interface->LinkedMemory, (void*) AttributeToRemove);
  Node->Attributes =Node->Attributes - (u32)AttributeType;
}

