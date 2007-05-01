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
        sstream << ipAddr << ":" << portNr << "|" << (uint16_t) protocolID;
        return sstream.str();
}

void EndPoint::fromString(const std::string & epstr)
{
  std::string::size_type i = epstr.find(':', 0);
  std::string ipstr(epstr, 0, i);
  std::string::size_type j = epstr.find('|', i);
  std::string portstr(epstr, i+1, j-i-1);
  std::string::size_type k = epstr.find('_', j);
  std::string protostr(epstr, j+1, k-j-1);

  ipAddr.fromString(ipstr);
  portNr = atoi(portstr.c_str());
  protocolID = atoi(protostr.c_str());

  return;
}


// ======================== Output Operators ========================

std::ostream& operator << (std::ostream& ost, const EndPoint& e)
{
        ost << e.ipAddr << ":" << e.portNr << "|" << (uint16_t) e.protocolID;
        return ost;
}

std::ostream & operator << (std::ostream & os, const std::map<EndPoint,Info> & m) {
  std::map<EndPoint,Info>::const_iterator it = m.begin();
  while (it != m.end()){
    os << it->first << "_" << it->second.packets_in << " " << it->second.packets_out << " " << it->second.bytes_in << " " << it->second.bytes_out << " " << it->second.records_in << " " << it->second.records_out << "\n";
    it++;
  }
  os << "---" << "\n";
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
  os << "( ";
  while (it != V.end()) {
    os << *it << " ";
    it++;
  }
  os << ")";
  return os;
}

std::ostream & operator << (std::ostream & os, const std::vector<double> & V) {
std::vector<double>::const_iterator it = V.begin();
  os << "( ";
  while (it != V.end()) {
    os << *it << " ";
    it++;
  }
  os << ")";
  return os;
}

std::ostream & operator << (std::ostream & os, const std::list<std::vector<int64_t> > & L) {
  // We need a ckeck as L for sample_new is initially empty
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
