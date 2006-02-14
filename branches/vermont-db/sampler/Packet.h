/*
 * PSAMP Reference Implementation
 *
 * Packet.h
 *
 * Encapsulates a captured packet with simple, thread-aware
 * reference-(usage-) counting.
 *
 * Author: Michael Drueing <michael@drueing.de>
 *         Gerhard Muenz <gerhard.muenz@gmx.de>
 *
 */

/*
 changed by: Ronny T. Lampert, 2005, for VERMONT
 */

#ifndef PACKET_H
#define PACKET_H

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "msg.h"

#include "Lock.h"

// the various header types (actually, HEAD_PAYLOAD is not neccessarily a header but it works like one for
// our purposes)
#define HEAD_RAW                  0
#define HEAD_NETWORK              1  // for fields that lie inside the network header
#define HEAD_NETWORK_AND_BEYOND   2  // for fields that might go beyond the network header border
#define HEAD_TRANSPORT            3  // for fields that lie inside the transport header
#define HEAD_TRANSPORT_AND_BEYOND 4  // for fields that might go beyond the transport header border
#define HEAD_PAYLOAD              5


// Packet classifications
//
// This bitmask defines various classifications for the packet. Which protocol headers it contains for example
// Bits:
//   3                   2                   1  
// 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// |               |~~~~~~~~~~~~~~~~~~~~~| |~~~~~~~~~~~~~~~~~~~~~|
// |               |                     | +---------------------+----- Network headers
// |               +---------------------+----------------------------- Transport headers
// +------------------------------------------------------------------- Payload available

// Network header classifications
#define PCLASS_NET_IP4             (1UL <<  0)
#define PCLASS_NET_IP6             (1UL <<  1)

#define PCLASS_NETMASK             0x00000fff

// Transport header classifications
#define PCLASS_TRN_TCP             (1UL << 12)
#define PCLASS_TRN_UDP             (1UL << 13)
#define PCLASS_TRN_ICMP            (1UL << 14)
#define PCLASS_TRN_IGMP            (1UL << 15)

#define PCLASS_TRNMASK             0x00fff000
// .... etc. etc. etc. 

// Payload classification (here, payload refers to data beyond transport header)
#define PCLASS_PAYLOAD             (1UL << 31)

class Packet
{
public:
	// The number of total packets received, will be incremented by each constructor call
	// implemented as public-variable for speed reasons (or lazyness reasons? ;-)
	static unsigned long totalPacketsReceived;

	/*
	 data: the raw packet data from the wire, including physical header
	 ipHeader: start of the IP header: data + (physical dependent) IP header offset
	 transportHeader: start of the transport layer header (TCP/UDP): ip_header + variable IP header length
	 */
	unsigned char *data;
	unsigned char *netHeader;
	unsigned char *transportHeader;
	unsigned char *payload;

	// the packet classification, i.e. what headers are present?
	unsigned long classification;

	// The number of captured bytes
	unsigned int length;

	// The length of the data buffer (can be larger than length)
	unsigned int data_length;

	// when was the packet received?
	struct timeval timestamp;

	// construct a new Packet for a specified number of 'users'
	Packet(void *packetData, unsigned int len, unsigned int data_len, int numUsers = 1) : users(numUsers), refCountLock()
	{
		data = (unsigned char *)packetData;
		netHeader = data + IPHeaderOffset;
		//transportHeader = (unsigned char *)netHeader + netTransportHeaderOffset(netHeader);
		length = len;
		data_length = data_len;

		totalPacketsReceived++;

		classify();
		/*
		 DO NOT SET TIMESTAMP HERE
		 IS SET IN OBSERVER!
		 */
	};

	// Delete the packet and free all data associated with it. Should only be called
	// if users==0 !
	~Packet()
	{
		if(users > 0) {
			DPRINTF("Packet: WARNING: freeing in-use packet!\n");
		}

		free(data);
	}

	// call this function after processing the packet, NOT delete()!
	void release()
	{
		int newUsers;

		refCountLock.lock();
		--users;
		newUsers = users;
		refCountLock.unlock();

		if(newUsers == 0) {
			delete this;
		} else if(newUsers < 0) {
			DPRINTF("Packet: WARNING: trying to free already freed packet!\n");
		}
	};

	// the supplied classification is _either_ PCLASS_NET_xxx or PCLASS_TRN_xxx (or PCLASS_PAYLOAD)
	// we check whether at least one of the Packet's classification-bits is also set in the supplied
	// parameter. If yes, then out packet matches the supplied class and can be safely processed.
	// otherwise, the Exporter should send a NULL value
	inline bool matches(unsigned long checkClassification) const
	{
		return (classification & checkClassification) != 0;
	}

	// classify the packet headers
	void classify()
	{
		unsigned char protocol = 0;
		unsigned char tcpDataOffset;
		
		classification = 0;
		transportHeader = NULL;
		payload = NULL;
		
		// first check for IPv4 header which needs to be at least 20 bytes long
		if ( (netHeader + 20 <= data + length) && ((*netHeader >> 4) == 4) )
		{
			protocol = *(netHeader + 9);
			classification |= PCLASS_NET_IP4;
			transportHeader = netHeader + ( ( *netHeader & 0x0f ) << 2);
		    
			// check if there is data for the transport header
			if (transportHeader >= data + length)
			    transportHeader = NULL;
		}
		// TODO: Add checks for IPv6 or similar here

		// if we found a transport header, continue classifying
		if (transportHeader && protocol)
		{
			switch (protocol)
			{
			case 1:		// ICMP
				// ICMP header is 4 bytes fixed-length
				payload = transportHeader + 4;
				
				// check if the packet is big enough to actually be ICMP
				if (transportHeader + 4 <= data + length)
					classification |= PCLASS_TRN_ICMP;
				
				break;
			case 2:		// IGMP
				// header is 8-bytes fixed size
				payload = transportHeader + 8;
				
				if (transportHeader + 8 <= data + length)
					classification |= PCLASS_TRN_IGMP;

				break;
			case 6:         // TCP
				// we need at least 12 more bytes in the packet to extract the "Data Offset"
				if (transportHeader + 12 <= data + length)
				{
					// extract "Data Offset" field at TCP header offset 12 (upper 4 bits)
					tcpDataOffset = *(transportHeader + 12) >> 4;
				
					// calculate payload offset
					payload = transportHeader + (tcpDataOffset << 2);
				
					// check if the complete TCP header is inside the received packet data
					if (transportHeader + (tcpDataOffset << 2) <= data + length)
						classification |= PCLASS_TRN_TCP;
				}
				
				break;
			case 17:        // UDP
				// UDP has a fixed header size of 8 bytes

				if (transportHeader + 8 <= data + length)
				{
					classification |= PCLASS_TRN_UDP;

					payload = transportHeader + 8;
				}
				
				break;
			default:
				break;
			}

			// check if we actually _have_ payload
			if (payload && (payload < data + length))
				classification |= PCLASS_PAYLOAD;

			//fprintf(stderr, "class %08lx, proto %d, data %p, net %p, trn %p, payload %p\n", classification, protocol, data, netHeader, transportHeader, payload);
		}
	}

	// read data from the IP header
	void copyPacketData(void *dest, int offset, int size) const
	{
		memcpy(dest, (char *)netHeader + offset, size);
	}


	// return a pointer into the packet to IP header offset given
	//
	// ATTENTION: If the returned pointer actually points to data of size fieldLength is only
	// checked for HEAD_RAW, HEAD_NETWORK_AND_BEYOND, HEAD_TRANSPORT_AND_BEYOND, and HEAD_PAYLOAD.
	// For fields within the network or transport header (HEAD_NETWORK, HEAD_TRANSPORT), we assume 
	// that it was verified before that the packet is of the correct packet class and that its buffer 
	// holds enough data.
	// You can check the packet class for a single field using match(). If you want to check
	// against all fields in a template, you should use checkPacketConformity() of the Template class.
	// If enough data for the network/transport header has been captured, is checked by classify().
	void * getPacketData(int offset, int header, int fieldLength) const
	{
	    // DPRINTF("offset: %d header: %d fieldlen: %d available: %d", offset, header, fieldLength, data_length);
	    switch(header)
	    {
		
		// for the following types, we omit the length check
		case HEAD_NETWORK:
		    return netHeader + offset;
		case HEAD_TRANSPORT:
		    return transportHeader + offset;

		// for the following types, we check the length
		case HEAD_RAW:
		    return (data + offset + fieldLength < data + data_length) ? data + offset : NULL;
		case HEAD_NETWORK_AND_BEYOND:
		    return (netHeader + offset + fieldLength < data + data_length) ? netHeader + offset : NULL;
		case HEAD_TRANSPORT_AND_BEYOND:
		    return (transportHeader + offset + fieldLength < data + data_length) ? transportHeader + offset : NULL;
		case HEAD_PAYLOAD:
		    return (payload + offset + fieldLength < data + data_length) ? payload + offset : NULL;
		default:
		    return (data + offset + fieldLength < data + data_length) ? data + offset : NULL;
	    }
	}

private:
	/*
	 the raw offset at which the IP header starts in the packet
	 for Ethernet, this is 14 bytes (MAC header size)
	 */
	static const int IPHeaderOffset=14;

	/*
	 Number of concurrent users of this packet. Decremented each time
	 release() is called. After it reaches zero, the packet is deleted.
         */
	int users;

	Lock refCountLock;

	/*
	 return the offset the transport header lies
	 IP knows about variable length options field
         */
	static inline unsigned int netTransportHeaderOffset(void *ipPacket)
	{
		/*
		 the header length (incl. options field) is:
		 last 4 bits in the first byte * 4bytes
		 */
		unsigned char len = *((unsigned char *)ipPacket) & 0x0f;

		return len << 2;
	}

};

#endif