/*
 * PSAMP Reference Implementation
 *
 * Template.cpp
 *
 * A Template definition
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#include <cstring>
#include <fstream>
#include <iostream>
#include "Template.h"
#include "Globals.h"

using namespace std;

void AddFieldFromString(Template *temp, const char *field)
{
  if (strncasecmp(field, "SRCIP4", 6) == 0)
  {
    temp->addField(FT_SRCIP4, 4, 12); // source address is as offset 12    
  }
  else if (strncasecmp(field, "DSTIP4", 6) == 0)
  {
    temp->addField(FT_DSTIP4, 4, 16); // dest address is at offset 16
  }
  else if (strncasecmp(field, "PROTO", 5) == 0)
  {
    temp->addField(FT_PROTO, 2, 9); // protocol is at offset 9
  }
  else if (strncasecmp(field, "SRCPORT", 7) == 0)
  {
    temp->addField(FT_SRCPORT, 2, 20);  // source port is as offset 20 (TCP offset 0)
  }
  else if (strncasecmp(field, "DSTPORT", 7) == 0)
  {
    temp->addField(FT_DSTPORT, 2, 22); // dest port is at offset 22 (TCP offset 2)
  }
}

Template *Template::readFromFile(const char *fileName)
{
  char buffer[256];
  Template *tmp = 0;
  
  ifstream f(fileName);
  
  // get template id
  while (!f.eof())
  {
    f.getline(buffer, 255);
    if ((buffer[0] == '#') || (buffer[0] == 0x0d) || (buffer[0] == 0x0a) || (buffer[0] == 0))
      continue;
    
    if (strncasecmp(buffer, "ID ", 3) == 0)
    {      
      // assign template id
      tmp = new Template(strtol(buffer + 3, 0, 10));
      break;
    }
    else
    {
      LOG("Expected ID\n");
      return 0;
    }    
  }  
  
  // get template fields
  while (!f.eof())
  {
    f.getline(buffer, 255);
    if ((buffer[0] == '#') || (buffer[0] == 0x0d) || (buffer[0] == 0x0a) || (buffer[0] == 0))
      continue;
      
    AddFieldFromString(tmp, buffer);
  }
  
  f.close();
  
  return tmp;
}
