#include "text_input_window.h"
#include "menu_interface.h"


// b32 name( menu_interface* Interface, container_node* CallerNode, void* Data )
MENU_UPDATE_FUNCTION(TextInputUpdateFunction)
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
    PushToUpdateQueue(Interface, CallerNode, TextInputUpdateFunction, 0, false);
  }
}

MENU_EVENT_CALLBACK(TextInputGainingFocus)
{
  if(!CallerNode->UpdateFunctionRunning)
  {
    text_input_node* TextInputNode = GetTextInputNode(CallerNode);
    TextInputNode->Time = Interface->Time;
    PushToUpdateQueue(Interface, CallerNode, TextInputUpdateFunction, 0, false);
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

// void name( menu_interface* Interface, container_node* Node)
MENU_DRAW(TextMenuDraw)
{
  utf8_byte OutputString[MENU_INPUT_NODE_TEXT_LENGTH+4] = {};
  utf8_string_buffer OutputBuffer = Utf8StringBuffer(sizeof(OutputString), OutputString);

  text_input_node* TextInputNode = GetTextInputNode(Node);
  AppendStringToBuffer(TextInputNode->TextMemory, &OutputBuffer);
  
  if(Node->UpdateFunctionRunning)
  {
    if(Interface->Keyboard->LastPushedKeyHoldTime < 0.1f)
    {
      TextInputNode->Time = Interface->Time;
    }

    if(!(((s32) (Interface->Time - TextInputNode->Time)) % 2) || 
      Interface->Keyboard->LastPushedKey != jwin::KeyboardButton_COUNT != 0)
    {
      AppendStringToBuffer((const utf8_byte*) "_", &OutputBuffer);
    }
  }
  r32 XOffset = Node->Region.W * 0.01;
  DrawTextInRect(Node->Region, XOffset, TextInputNode->TextPixelSize, OutputBuffer.Buffer);
}


container_node* CreateTextInputNode(menu_interface* Interface) {

  container_node* TextInputContainer = NewContainer(Interface, container_type::TextInput);
  text_input_node* TextInputNode = GetTextInputNode(TextInputContainer);
  TextInputNode->Buffer = Utf8StringBuffer(sizeof(TextInputNode->TextMemory), TextInputNode->TextMemory);
  TextInputNode->TextPixelSize = 14;
  r32 LineSpacing = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), TextInputNode->TextPixelSize);
  v2 TextSize = GetTextSizeCanonicalSpace(GetRenderSystem(), TextInputNode->TextPixelSize, (utf8_byte*) "Sample Text For Input length");

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, TextInputContainer, 0, TextInputMouseDown, 0);
  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, TextInputContainer, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, TextSize.X);
  SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, LineSpacing*1.1f);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.01);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.01);
  SizeAttr->XAlignment = menu_region_alignment::LEFT;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;

  SetColor(Interface, TextInputContainer, menu::GetColor(GetColorTable(), "charcoal")*1.1);

  return TextInputContainer;
}

menu_functions GetTextInputfunctions(){
  menu_functions Result = GetDefaultFunctions();
  Result.Draw = DeclareFunction(menu_draw, TextMenuDraw);
  Result.GainingFocus = DeclareFunction(node_gaining_focus, TextInputGainingFocus);
  Result.LosingFocus = DeclareFunction(node_gaining_focus, TextInputLosingFocus);
  return Result;  
}

inline text_input_node* GetTextInputNode(container_node* Container)
{
  Assert(Container->Type == container_type::TextInput);
  text_input_node* Result = (text_input_node*) GetContainerPayload(Container);
  return Result;
}
