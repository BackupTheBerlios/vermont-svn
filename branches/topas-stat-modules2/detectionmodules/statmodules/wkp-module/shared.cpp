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

// Needed to create filenames etc.
std::string EndPoint::toString() const
{
        std::stringstream sstream;
        sstream << ipAddr << "_" << portNr << "|" << (uint16_t) protocolID;
        return sstream.str();
}



// Creates an EndPoint from its string representation
// withNetMask = true --> string contains netmask
// withNetMask = false --> string doesnt contain netmask
void EndPoint::fromString(const std::string & epstr, bool withNetMask)
{
  if (withNetMask == true) {
    std::string::size_type h = epstr.find('/', 0);
    std::string ipstr(epstr, 0, h);
    std::string::size_type i = epstr.find(':', h);
    std::string netmaskstr(epstr, h+1, i-h-1);
    std::string::size_type j = epstr.find('|', i);
    std::string portstr(epstr, i+1, j-i-1);
    std::string::size_type k = epstr.find('_', j);
    std::string protostr(epstr, j+1, k-j-1);

    ipAddr.fromString(ipstr);
    netmask = atoi(netmaskstr.c_str());
    if (netmask >= 0 && netmask < 32)
      applyNetMask();
    else {
      std::cerr << "Invalid Netmask occured! Netmask may only be a value "
      << "between 0 and 32! Thus, 32 will be assumed for now!" << std::endl;
      netmask = 32;
    }
    portNr = atoi(portstr.c_str());
    protocolID = atoi(protostr.c_str());
  }
  else {
    std::string::size_type i = epstr.find(':', 0);
    std::string ipstr(epstr, 0, i);
    std::string::size_type j = epstr.find('|', i);
    std::string portstr(epstr, i+1, j-i-1);
    std::string::size_type k = epstr.find('_', j);
    std::string protostr(epstr, j+1, k-j-1);

    ipAddr.fromString(ipstr);
    netmask = 32;
    portNr = atoi(portstr.c_str());
    protocolID = atoi(protostr.c_str());
  }

  return;
}

void EndPoint::applyNetMask() {

  unsigned int mask[4];
  std::string Mask;
  switch( atoi(netmask) ) {
    case  0: Mask="0x00000000"; break;
    case  1: Mask="0x80000000"; break;
    case  2: Mask="0xC0000000"; break;
    case  3: Mask="0xE0000000"; break;
    case  4: Mask="0xF0000000"; break;
    case  5: Mask="0xF8000000"; break;
    case  6: Mask="0xFC000000"; break;
    case  7: Mask="0xFE000000"; break;
    case  8: Mask="0xFF000000"; break;
    case  9: Mask="0xFF800000"; break;
    case 10: Mask="0xFFC00000"; break;
    case 11: Mask="0xFFE00000"; break;
    case 12: Mask="0xFFF00000"; break;
    case 13: Mask="0xFFF80000"; break;
    case 14: Mask="0xFFFC0000"; break;
    case 15: Mask="0xFFFE0000"; break;
    case 16: Mask="0xFFFF0000"; break;
    case 17: Mask="0xFFFF8000"; break;
    case 18: Mask="0xFFFFC000"; break;
    case 19: Mask="0xFFFFE000"; break;
    case 20: Mask="0xFFFFF000"; break;
    case 21: Mask="0xFFFFF800"; break;
    case 22: Mask="0xFFFFFC00"; break;
    case 23: Mask="0xFFFFFE00"; break;
    case 24: Mask="0xFFFFFF00"; break;
    case 25: Mask="0xFFFFFF80"; break;
    case 26: Mask="0xFFFFFFC0"; break;
    case 27: Mask="0xFFFFFFE0"; break;
    case 28: Mask="0xFFFFFFF0"; break;
    case 29: Mask="0xFFFFFFF8"; break;
    case 30: Mask="0xFFFFFFFC"; break;
    case 31: Mask="0xFFFFFFFE"; break;
    case 32: Mask="0xFFFFFFFF"; break;
  }
  char mask1[3] = {Mask[2],Mask[3],'\0'};
  char mask2[3] = {Mask[4],Mask[5],'\0'};
  char mask3[3] = {Mask[6],Mask[7],'\0'};
  char mask4[3] = {Mask[8],Mask[9],'\0'};
  mask[0] = strtol (mask1, NULL, 16);
  mask[1] = strtol (mask2, NULL, 16);
  mask[2] = strtol (mask3, NULL, 16);
  mask[3] = strtol (mask4, NULL, 16);

  ipAddr.remanent_mask(mask);

  return;
}


// ======================== Output Operators ========================

std::ostream& operator << (std::ostream& ost, const EndPoint& e)
{
        ost << e.ipAddr <<  ":" << e.portNr << "|" << (uint16_t) e.protocolID;
        return ost;
}

std::ostream & operator << (std::ostream & os, const std::map<EndPoint,Info> & m) {
  std::map<EndPoint,Info>::const_iterator it = m.begin();
  while (it != m.end()){
    os << it->first << "_" << it->second.packets_in << " " << it->second.packets_out << " " << it->second.bytes_in << " " << it->second.bytes_out << " " << it->second.records_in << " " << it->second.records_out << "\n";
    it++;
  }
  os << "---" << "\n" << std::flush;
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
