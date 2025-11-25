#pragma once

#include "commons/string.h"
#include "platform/jwin_platform_memory.h"

#define ATTRIBUTE_MAX_LENGTH 128

struct xml_string {
  u32 Length;
  c8* String;
};

struct xml_attribute
{
  xml_string Attribute;
  xml_string Value;
};

struct xml_element {
  xml_string Name;
  u32 AttributeCount;
  xml_attribute* Attributes;
};

#define XML_PROLOG_S "<?"
#define XML_PROLOG_E "?>"
#define XML_ATTRIBTUE_S "<"
#define XML_ATTRIBTUE_E "/>"

xml_string GetAttributeContent(c8* XmlString){
  Assert(*XmlString == '<');

  c8* StartScan = XmlString+1;
  c8* EndScan   = jstr::FindFirstOf(" />", XmlString);
  if(*EndScan == ' ')
  {
    
  }
  xml_string Result = {};
  Result.String = StartScan;
  Result.Length = EndScan - StartScan;
}

xml_element* parse(memory_arena* Arena, c8* XmlFile) {

  c8* Scan = XmlFile;

  xml_element* Root = PushStruct(Arena, xml_element);

  while(Scan)
  {

    while(*Scan != '<')
    {
      Scan
    }

  }

}