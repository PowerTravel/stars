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


rect2f GetSizedParentRegion(size_attribute* SizeAttr, rect2f BaseRegion)
{
  rect2f Result = {};
  if(SizeAttr->Width.Type == menu_size_type::RELATIVE_)
  {
    Result.W = SizeAttr->Width.Value * BaseRegion.W;
  }else if(SizeAttr->Width.Type == menu_size_type::ABSOLUTE_){
    Result.W = SizeAttr->Width.Value;  
  }

  if(SizeAttr->Height.Type == menu_size_type::RELATIVE_)
  {
    Result.H = SizeAttr->Height.Value * BaseRegion.H;
  }else if(SizeAttr->Height.Type == menu_size_type::ABSOLUTE_){
    Result.H = SizeAttr->Height.Value;
  }
  
  switch(SizeAttr->XAlignment)
  {
    case menu_region_alignment::LEFT:
    {
      Result.X = BaseRegion.X;
    }break;
    case menu_region_alignment::RIGHT:
    {
      Result.X = BaseRegion.X + BaseRegion.W - Result.W;
    }break;
    case menu_region_alignment::CENTER:
    {
      Result.X = BaseRegion.X + (BaseRegion.W - Result.W)*0.5f;
    }break;
  }
  switch(SizeAttr->YAlignment)
  {
    case menu_region_alignment::TOP:
    {
      //Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
      Result.Y = BaseRegion.Y;
    }break;
    case menu_region_alignment::BOT:
    {
      Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
    }break;
    case menu_region_alignment::CENTER:
    {
      Result.Y = BaseRegion.Y + (BaseRegion.H - Result.H)*0.5f;
    }break;
  }

  if(SizeAttr->LeftOffset.Type == menu_size_type::RELATIVE_)
  {
    Result.X += SizeAttr->LeftOffset.Value * BaseRegion.W;
  }else if(SizeAttr->LeftOffset.Type == menu_size_type::ABSOLUTE_){
    Result.X += SizeAttr->LeftOffset.Value;
  }

  if(SizeAttr->TopOffset.Type == menu_size_type::RELATIVE_)
  {
    Result.Y -= SizeAttr->TopOffset.Value * BaseRegion.H;
  }else if(SizeAttr->TopOffset.Type == menu_size_type::ABSOLUTE_){
    Result.Y -= SizeAttr->TopOffset.Value;
  }
  
  return Result;
}

b32 CallMouseExitFunctions(menu_interface* Interface, u32 NodeCount, container_node** Nodes)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < NodeCount; ++i)
  {
    container_node* Node = Nodes[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event =   GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseExit)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

b32 CallMouseEnterFunctions(menu_interface* Interface, u32 NodeCount, container_node** Nodes)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < NodeCount; ++i)
  {
    container_node* Node = Nodes[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseEnter)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

b32 CallMouseDownFunctions(menu_interface* Interface, u32 NodeCount, container_node** Nodes)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < NodeCount; ++i)
  {
    container_node* Node = Nodes[i];
    while(Node)
    {
      if(HasAttribute(Node, ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseDown)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}

b32 CallMouseUpFunctions(menu_interface* Interface, u32 NodeCount, container_node** Nodes)
{
  b32 FunctionCalled = false;
  for (u32 i = 0; i < NodeCount; ++i)
  {
    container_node* Node = Nodes[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseUp)
          {
            FunctionCalled = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return FunctionCalled;
}