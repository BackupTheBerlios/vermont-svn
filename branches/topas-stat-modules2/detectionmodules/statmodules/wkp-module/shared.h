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

#ifndef _SHARED_H_
#define _SHARED_H_

#include <ipaddress.h>
#include <list>
#include <map>
#include <vector>
#include <iostream>

// ======================== STRUCT Info ========================

// this structure is used to store traffic information
// about a node

struct Info {
  uint64_t packets_in;
  uint64_t packets_out;
  uint64_t bytes_in;
  uint64_t bytes_out;
  uint64_t records_in;
  uint64_t records_out;
};


// ==================== ENDPOINT CLASS EndPoint ====================
class EndPoint {

  private:

    IpAddress ipAddr;
    short netmask;
    int portNr;
    int protocolID;

    friend std::ostream& operator << (std::ostream&, const EndPoint&);

  public:

    // Constructors
    EndPoint() : ipAddr (0,0,0,0) {

      netmask = 32;
      portNr = 0;
      protocolID = 0;

    }

    EndPoint(const IpAddress & ip, short nmask, int port, int protocol) : ipAddr (ip[0], ip[1], ip[2], ip[3]) {

      if (nmask >= 0 && nmask <= 32) {
        netmask = nmask;
        applyNetMask();
      }
      else {
        std::cerr << "Invalid Netmask occured! Netmask may only be a value "
        << "between 0 and 32! Thus, 32 will be assumed for now!" << std::endl;
        netmask = 32;
      }
      portNr = port;
      protocolID = protocol;

    }

    // copy constructor
    EndPoint(const EndPoint & e) : ipAddr(e.ipAddr[0], e.ipAddr[1], e.ipAddr[2], e.ipAddr[3]){

      netmask = e.netmask;
      // no need to apply it, because it was already applied
      //to the original endpoint

      portNr = e.portNr;
      protocolID = e.protocolID;

    }

    // Destructor
    ~EndPoint() {};

    // for the following two operators, netmask doesnt need to be compared,
    // because it was already applied to the ip address

    // Operators (needed for use in maps)
    bool operator==(const EndPoint& e) const {
      return ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
                && ipAddr[2] == e.ipAddr[2] && ipAddr[3] == e.ipAddr[3]
              && portNr == e.portNr && protocolID == e.protocolID;
    }

    bool operator<(const EndPoint& e) const {
      if (ipAddr[0] < e.ipAddr[0])
        return true;
      if (ipAddr[0] == e.ipAddr[0] && ipAddr[1] < e.ipAddr[1])
        return true;
      if (ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
    && ipAddr[2] < e.ipAddr[2])
        return true;
      if (ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
    && ipAddr[2] == e.ipAddr[2] && ipAddr[3] < e.ipAddr[3])
        return true;
      if (ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
    && ipAddr[2] == e.ipAddr[2] && ipAddr[3] == e.ipAddr[3] && portNr < e.portNr)
        return true;
      if (ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
    && ipAddr[2] == e.ipAddr[2] && ipAddr[3] == e.ipAddr[3] && portNr == e.portNr && protocolID < e.protocolID)
        return true;

      return false;
    }

    std::string toString() const;
    void fromString(const std::string &, bool);

    // Setters & Getters
    void setIpAddress(const IpAddress & ip) {
      ipAddr.setAddress(ip[0], ip[1], ip[2], ip[3]);
    }

    void setPortNr(const int & p) {
      portNr = p;
    }

    void setProtocolID(const int & pid) {
      protocolID = pid;
    }

    void setNetMask(const short & n) {
      if (n >= 0 && n <= 32) {
        netmask = n;
        applyNetMask();
      }
      else {
        std::cerr << "Invalid Netmask occured! Netmask may only be a value "
        << "between 0 and 32! Thus, 32 will be assumed for now!" << std::endl;
        netmask = 32;
      }
    }

    IpAddress getIpAddress() const { return ipAddr; }
    int getPortNr() const { return portNr; }
    int getProtocolID() const { return protocolID; }
    short getNetMask() const { return netmask; }

    void applyNetMask();

};


// ======================== Output Operators ========================

std::ostream & operator << (std::ostream &, const std::list<int64_t> &);
std::ostream & operator << (std::ostream &, const std::vector<unsigned> &);
std::ostream & operator << (std::ostream &, const std::vector<int64_t> &);
std::ostream & operator << (std::ostream &, const std::vector<double> &);
std::ostream & operator << (std::ostream &, const std::list<std::vector<int64_t> > &);
std::ostream & operator << (std::ostream &, const std::map<EndPoint,Info> &);

#endif
