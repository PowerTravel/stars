#pragma once
// This file contains internal menu functions. Functiouns used to create menus, not necesarilly needed by the end menu user
#include "menu/menu_interface.h"



u32 GetAttributeSize(container_attribute Attribute);
u32 GetContainerPayloadSize(container_type Type);
void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType);