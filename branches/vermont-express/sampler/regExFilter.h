/** @file
 * Filter a packet by checking if it is containing a predefined regEx string
 */

#ifndef REGEXFILTER_H
#define REGEXFILTER_H

#include <list>
#include <string>
#include <string.h>
#include "msg.h"
#include "PacketProcessor.h"
#include <sys/types.h>
#include <boost/regex.hpp>
//#include <regexp9.h>
//#include <utf.h>
//#include <fmt.h>
//#include <regex.h>




class regExFilter:public PacketProcessor
{

public:

  regExFilter ()
  {

  };

  virtual ~ regExFilter ()
  {

  };


  virtual bool processPacket (const Packet * p);

  int filtertype;
  std::string match;


protected:


  bool compare (char *data);




};

#endif
