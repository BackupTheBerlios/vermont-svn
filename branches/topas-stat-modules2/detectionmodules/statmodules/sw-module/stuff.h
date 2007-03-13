#ifndef _STUFF_H_
#define _STUFF_H_

#include "datastore.h"
#include <iostream>

// this structure is used to store traffic information
// about a node

struct Info {
  uint64_t packets_in;
  uint64_t packets_out;
  uint64_t bytes_in;
  uint64_t bytes_out;
	uint16_t records;
};


// ==================== ENDPOINT CLASS EndPoint ====================
class EndPoint {

	private:

		IpAddress ipAddr;
		uint16_t	port;
		byte	proto;

		friend std::ostream& operator << (std::ostream&, const EndPoint&);

	public:

		// Constructors
		EndPoint() : ipAddr (0,0,0,0) {

			port = 0;
			proto = 0;

		}

		EndPoint(const IpAddress & ip, uint16_t port, byte protocol) : ipAddr (ip[0], ip[1], ip[2], ip[3]) {

			port = port;
			proto = protocol;

		}

		// Destructor
		~EndPoint() {};


		// Operators (needed for use in maps)
		bool operator==(const EndPoint& e) const {
			return (ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
								&& ipAddr[2] == e.ipAddr[2] && ipAddr[3] == e.ipAddr[3])
							&& (port == e.port)
							&& (proto == e.proto);
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
		&& ipAddr[2] == e.ipAddr[2] && ipAddr[3] == e.ipAddr[3] && port < e.port)
				return true;
			if (ipAddr[0] == e.ipAddr[0] && ipAddr[1] == e.ipAddr[1]
		&& ipAddr[2] == e.ipAddr[2] && ipAddr[3] == e.ipAddr[3] && port == e.port && proto == e.proto)
				return true;

			return false;
		}

		std::string toString() const;

};

//std::ostream & operator << (std::ostream &, const std::map<EndPoint,Info> &);

#endif
