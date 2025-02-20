#include "text_input_window.h"
#include "menu_interface.h"


// b32 name( menu_interface* Interface, container_node* CallerNode, void* Data )
MENU_UPDATE_FUNCTION(TextInputUpdateFunction)
{

  text_input_node* TextInputNode = GetTextInputNode(CallerNode);
  if(jwin::Pushed(Interface->Keyboard->Key_BACK) && TextInputNode->Buffer.Position > 0)
  {
    EraseFromBuffer(&TextInputNode->Buffer, 1);
  }else{
    PushInputToBuffer(Interface->Keyboard, &TextInputNode->Buffer, ENGLISH);
  }

  return true;
}

// void name( menu_interface* Interface, container_node* CallerNode, void* Data)
MENU_EVENT_CALLBACK(TextInputMouseDown)
{
  if(!CallerNode->UpdateFunctionRunning)
  {
    PushToUpdateQueue(Interface, CallerNode, TextInputUpdateFunction, 0, false);
  }
}

// void name( menu_interface* Interface, container_node* Node)
MENU_DRAW(TextMenuDraw)
{
  utf8_byte OutputString[MENU_INPUT_NODE_TEXT_LENGTH+4] = {};
  utf8_string_buffer OutputBuffer = Utf8StringBuffer(sizeof(OutputString), OutputString);

  text_input_node* TextInputNode = GetTextInputNode(Node);
  AppendStringToBuffer(TextInputNode->TextMemory, &OutputBuffer);
  
  if(!(((s32) Interface->Time) % 2) || 
    Interface->Keyboard->LastPushedKey != jwin::KeyboardButton_COUNT != 0 || 
    Interface->Keyboard->LastPushedKeyHoldTime < 1.f)
  {
    AppendStringToBuffer((const utf8_byte*) "_", &OutputBuffer);
  }
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), V2(Node->Region.X,Node->Region.Y), 12, OutputBuffer.Buffer);
}

container_node* CreateTextInputNode(menu_interface* Interface) {
  container_node* TextInputContainer = NewContainer(Interface, container_type::TextInput);

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, TextInputContainer, 0, TextInputMouseDown, 0);

  size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, TextInputContainer, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, 0.4);
  SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, Interface->HeaderSize);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0);
  SizeAttr->XAlignment = menu_region_alignment::CENTER;
  SizeAttr->YAlignment = menu_region_alignment::CENTER;

  text_input_node* TextInputNode = GetTextInputNode(TextInputContainer);
  TextInputNode->Buffer = Utf8StringBuffer(sizeof(TextInputNode->TextMemory), TextInputNode->TextMemory);
  return TextInputContainer;
}

menu_functions GetTextInputfunctions(){
  menu_functions Result = GetDefaultFunctions();
  Result.Draw = DeclareFunction(menu_draw, TextMenuDraw);
  return Result;  
}

inline text_input_node* GetTextInputNode(container_node* Container)
{
  Assert(Container->Type == container_type::TextInput);
  text_input_node* Result = (text_input_node*) GetContainerPayload(Container);
  return Result;
}
