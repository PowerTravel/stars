#pragma once

#include "platform/text_input.h"

struct container_node;

#define MENU_INPUT_NODE_TEXT_LENGTH 128
struct text_input_node {
  utf8_byte TextMemory[MENU_INPUT_NODE_TEXT_LENGTH];
  utf8_string_buffer Buffer;
};

container_node* CreateTextInputNode(menu_interface* Interface);
menu_functions GetTextInputfunctions();
inline text_input_node* GetTextInputNode(container_node* Container);