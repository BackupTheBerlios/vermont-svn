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

#include <datastore.h>
#include <list>
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
    uint16_t  portNr;
    byte  protocolID;

    friend std::ostream& operator << (std::ostream&, const EndPoint&);

  public:

    // Constructors
    EndPoint() : ipAddr (0,0,0,0) {

      portNr = 0;
      protocolID = 0;

    }

    EndPoint(const IpAddress & ip, uint16_t port, byte protocol) : ipAddr (ip[0], ip[1], ip[2], ip[3]) {

      portNr = port;
      protocolID = protocol;

    }

    // copy constructor
    EndPoint(const EndPoint & e) : ipAddr(e.ipAddr[0], e.ipAddr[1], e.ipAddr[2],        e.ipAddr[3]){

      portNr = e.portNr;
      protocolID = e.protocolID;

    }

    // Destructor
    ~EndPoint() {};


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

    // Setters & Getters
    void setIpAddress(const IpAddress & ip) {
      ipAddr.setAddress(ip[0], ip[1], ip[2], ip[3]);
    }

    void setPortNr(const uint16_t & p) {
      portNr = p;
    }

    void setProtocolID(const byte & pid) {
      protocolID = pid;
    }

    IpAddress getIpAddress() const { return ipAddr; }
    uint16_t getPortNr() const { return portNr; }
    byte getProtocolID() const { return protocolID; }

};

// ==================== CLASS DirectedIpAddress ====================

// we use this class as key field in std::map< key, value >

#define OUT -1
#define IN 1
#define DEFAULT 0

class DirectedIpAddress : public IpAddress {

private:

  int Direction; // = OUT or IN, more rarely DEFAULT

public:

  DirectedIpAddress (byte, byte, byte, byte, int dir);
  DirectedIpAddress (const byte tab[4], int dir);
  DirectedIpAddress (const IpAddress &, int dir);
  ~DirectedIpAddress () {}

  void setDirectedIpAddress (byte, byte, byte, byte, int dir);
  void setDirectedIpAddress (const byte tab[4], int dir);
  void setDirectedIpAddress (const IpAddress &, int dir);

  int getDirection () const {return Direction;}

  // mask functions:
  // - remanent_mask changes DirectedIpAddress object
  // - mask is only temporary
  // warning: netmask is not checked before being applied
  // use only 0 <= m1,m2,m3,m4 <= 255 (or 0x00 and 0xFF)
  DirectedIpAddress mask (byte, byte, byte, byte);
  DirectedIpAddress mask (const byte m[4]);
  void remanent_mask (byte, byte, byte, byte);
  void remanent_mask (const byte m[4]);

  // we need an order relation to use DirectedIpAddress
  // as key field in std::map< key, value >, so:
  bool operator == (const DirectedIpAddress &) const;
  bool operator < (const DirectedIpAddress &) const;

};

// ======================== STRUCT Metrics ========================

// this structure is used to extract metric-values to

struct Metrics {
  int64_t packets_in;
  int64_t packets_out;
  int64_t bytes_in;
  int64_t bytes_out;
  int64_t records_in;
  int64_t records_out;
  int64_t bytes_per_packet_in;   // bytes_in/packets_in
  int64_t bytes_per_packet_out;  // bytes_out/packets_out
  int64_t p_out_minus_p_in;
  int64_t b_out_minus_b_in;
  int64_t pt_minus_pt1_in;       // packets_in(t)-packets_in(t-1)
  int64_t pt_minus_pt1_out;      // packets_out(t)-packets_out(t-1)
  int64_t bt_minus_bt1_in;       // bytes_in(t)-bytes_in(t-1)
  int64_t bt_minus_bt1_out;      // bytes_out(t)-bytes_out(t-1)
};

// ======================== STRUCT Samples ========================

// we use this structure as value field in std::map< key, value >
// it contains a list of the old and the new metrics.
// Letting Old and New be lists of struct Metrics enables us
// to store values for multiple monitored values

struct Samples {
  std::list<Metrics> Old;
  std::list<Metrics> New;
};


// ======================== Output Operators ========================

std::ostream & operator << (std::ostream &, const std::list<int64_t> &);
std::ostream & operator << (std::ostream &, const std::vector<unsigned> &);
std::ostream & operator << (std::ostream &, const DirectedIpAddress &);
std::ostream & operator << (std::ostream &, const std::map<EndPoint,Info> &);
std::ostream & operator << (std::ostream &, const Metrics &);
//std::ostream & operator << (std::ostream &, const std::list<Metrics> &);

#endif
