#ifndef _STUFF_H_
#define _STUFF_H_

#include <ipaddress.h>

#include <map>
#include <iostream>

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
		uint16_t	portNr;
		byte	protocolID;

		friend std::ostream& operator << (std::ostream&, const EndPoint&);

	public:

		// Constructors
		EndPoint() : ipAddr (0,0,0,0) {

			portNr = 0;
			protocolID = 0;

		}

    EndPoint(const IpAddress & ip, uint16_t p, byte pid) : ipAddr (ip[0], ip[1], ip[2], ip[3]) {

      portNr = p;
      protocolID = pid;

    }

    // copy constructor
    EndPoint(const EndPoint & e) : ipAddr(e.ipAddr[0], e.ipAddr[1], e.ipAddr[2], e.ipAddr[3]){

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

		bool operator<(const EndPoint& e) const	{
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

std::ostream & operator << (std::ostream &, const std::map<EndPoint,Info> &);

#endif
