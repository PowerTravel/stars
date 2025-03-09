#pragma once

#include "conainer_node.h"
#include "menu_interface.h"

// b32 name( menu_interface* Interface, container_node* CallerNode, void* Data )
MENU_UPDATE_FUNCTION(V3InputUpdateFunction)
{
  text_input_node* TextInputNode = GetTextInputNode(CallerNode);
  if(jwin::Pushed(Interface->Keyboard->Key_BACK) && TextInputNode->Buffer.Position > 0)
  {
    EraseFromBuffer(&TextInputNode->Buffer, 1);
  }else if(jwin::Pushed(Interface->Keyboard->Key_ESCAPE) || jwin::Pushed(Interface->Keyboard->Key_ENTER)){
    *CallerNode->UpdateFunctionRunning = false;
  }else{
    PushInputToBuffer(Interface->Keyboard, &TextInputNode->Buffer, ENGLISH);
  }

  return true;
}

// void name( menu_interface* Interface, container_node* CallerNode, void* Data)
MENU_EVENT_CALLBACK(TextInputMouseDown)
{
  Platform.DEBUGPrint("Mouse Down\n");
  if(!CallerNode->UpdateFunctionRunning)
  {
    text_input_node* TextInputNode = GetTextInputNode(CallerNode);
    TextInputNode->Time = Interface->Time;
    PushToUpdateQueue(Interface, CallerNode, V3InputUpdateFunction, 0, false);
  }
}

MENU_EVENT_CALLBACK(TextInputGainingFocus)
{
  if(!CallerNode->UpdateFunctionRunning)
  {
    text_input_node* TextInputNode = GetTextInputNode(CallerNode);
    TextInputNode->Time = Interface->Time;
    PushToUpdateQueue(Interface, CallerNode, V3InputUpdateFunction, 0, false);
  }
  Platform.DEBUGPrint("Gaining Focus\n");
}

MENU_EVENT_CALLBACK(TextInputLosingFocus)
{
  Platform.DEBUGPrint("Losing Focus\n");
  if(CallerNode->UpdateFunctionRunning)
  {
    *CallerNode->UpdateFunctionRunning = false;
  }
}

container_node* CreateV3InputModule(menu_interface* Interface){

  container_node* V3InputModule = NewContainer(Interface, container_type::Grid);
  grid_node* GridNode = GetGridNode(PositionGrid);
  GridNode->Col = 3;
  GridNode->Row = 1;
  GridNode->TotalMarginX = 0.0;
  GridNode->TotalMarginY = 0.0;
  GridNode->Stack = true;
  GridNode->StackXAlignment = menu_region_alignment::CENTER;
  GridNode->StackYAlignment = menu_region_alignment::CENTER;

  container_node* V3InputModule = NewContainer(Interface, container_type::TextInput);
  text_input_node* TextInputNode = GetTextInputNode(V3InputModule);
  TextInputNode->Buffer = Utf8StringBuffer(sizeof(TextInputNode->TextMemory), TextInputNode->TextMemory);
  TextInputNode->TextPixelSize = Interface->BodyFontSize;
  r32 LineSpacing = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), TextInputNode->TextPixelSize);

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, V3InputModule, 0, TextInputMouseDown, 0);
  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, V3InputModule, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, 1);
  SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, LineSpacing*1.1f);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
  alignment_attribute* AlignmentAttr = (alignment_attribute*) PushAttribute(Interface, V3InputModule, ATTRIBUTE_ALIGNMENT);
  AlignmentAttr->XAlignment = menu_region_alignment::CENTER;
  AlignmentAttr->YAlignment = menu_region_alignment::CENTER;

  SetColor(Interface, V3InputModule, menu::GetColor(GetColorTable(), "charcoal")*1.1);

  return V3InputModule;
}