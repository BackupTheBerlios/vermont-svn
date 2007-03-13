#include "stuff.h"

#include <ostream>
#include <sstream>

std::string EndPoint::toString() const
{
        std::stringstream sstream;
        sstream << ipAddr[0] << "." << ipAddr[1] << "." << ipAddr[2] << "." << ipAddr[3] << ":" << port << " | " << (uint16_t) proto;
        return sstream.str();
}

std::ostream& operator << (std::ostream& ost, const EndPoint& e)
{
        ost << e.ipAddr[0] << "." << e.ipAddr[1] << "." << e.ipAddr[2] << "." << e.ipAddr[3] << ":" << e.port << " | " << (uint16_t) e.proto;
        return ost;
}

/*
std::ostream & operator << (std::ostream & os, const std::map<EndPoint,Info> & m) {
  std::map<EndPoint,Info>::const_iterator it = m.begin();
  while (it != m.end()) {
    os << it->first << " --- Records: " << it->second.records;
    it++;
  }
  return os;
}
*/
