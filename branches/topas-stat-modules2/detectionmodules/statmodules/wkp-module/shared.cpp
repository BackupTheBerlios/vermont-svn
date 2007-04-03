/**************************************************************************/
/*    Copyright (C) 2006 Romain Michalec                                  */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software   */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,          */
/*    MA  02110-1301, USA                                                 */
/*                                                                        */
/**************************************************************************/

#include "shared.h"
#include <ostream>
#include <sstream>

// ============== SOME METHODS FOR CLASS EndPoint ================

std::string EndPoint::toString() const
{
        std::stringstream sstream;
        sstream << ipAddr[0] << "." << ipAddr[1] << "." << ipAddr[2] << "." << ipAddr[3] << ":" << portNr << " | " << (uint16_t) protocolID;
        return sstream.str();
}

// ======================== Output Operators ========================

std::ostream& operator << (std::ostream& ost, const EndPoint& e)
{
        ost << e.ipAddr[0] << "." << e.ipAddr[1] << "." << e.ipAddr[2] << "." << e.ipAddr[3] << ":" << e.portNr << " | " << (uint16_t) e.protocolID;
        return ost;
}

std::ostream & operator << (std::ostream & os, const std::map<EndPoint,Info> & m) {
  std::map<EndPoint,Info>::const_iterator it = m.begin();
  while (it != m.end()){
    os << it->first << " --- packets_in: " << it->second.packets_in << ", packets_out: " << it->second.packets_out << ", bytes_in: " << it->second.bytes_in << ", bytes_out: " << it->second.bytes_out << ", records_in: " << it->second.records_in << ", records_out: " << it->second.records_out << std::endl;
    it++;
  }
  return os;
}

std::ostream & operator << (std::ostream & os, const std::list<int64_t> & L) {
  std::list<int64_t>::const_iterator it = L.begin();
  while (it != L.end()) {
    os << *it << ',';
    it++;
  }
  return os;
}

std::ostream & operator << (std::ostream & os, const std::vector<unsigned> & V) {
  std::vector<unsigned>::const_iterator it = V.begin();
  while (it != V.end()) {
    os << *it << ',';
    it++;
  }
  return os;
}

std::ostream & operator << (std::ostream & os, const std::vector<int64_t> & V) {
  std::vector<int64_t>::const_iterator it = V.begin();
  os << "(";
  // there is at least one element in V, so there will be no problem
  // dereferencing "it" without check
  os << *it; it++;
  while (it != V.end()) {
    os << ", " << *it;
    it++;
  }
  os << ")";
  return os;
}

std::ostream & operator << (std::ostream & os, const std::list<std::vector<int64_t> > & L) {
  // here we need a ckeck as L for sample_new is initially empty
  if (L.size() > 0) {
    std::list<std::vector<int64_t> >::const_iterator it = L.begin();
    os << *it; it++;
    while (it != L.end()) {
      os << ", " << *it;
      it++;
    }
  }
  return os;
}
