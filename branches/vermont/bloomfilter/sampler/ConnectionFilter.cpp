#include "ConnectionFilter.h"

ConnectionFilter::ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned filterSize)
	: synFilter(hashFunctions, filterSize), exportFilter(hashFunctions, filterSize),
	  connectionFilter(hashFunctions, filterSize)
{
	this->timeout = timeout;
	this->exportBytes = bytes;
}

bool ConnectionFilter::processPacket(const Packet* p)
{
	unsigned flagsOffset = p->transportHeaderOffset + 13;
	static const uint8_t SYN = 0x02;
	static const uint8_t FIN = 0x01;
	static const uint8_t RST = 0x04;
	static QuintupleKey key;
	static unsigned long tmp;

	if (p->ipProtocolType != Packet::TCP) {
		DPRINTF("Got a non-TCP packet. Protocol-type is %i", p->ipProtocolType);
		return false;
	}

	// use Packet::getPacketData()
	uint32_t ip1 = *((uint32_t*)(p->netHeader + 12));
	uint32_t ip2 = *((uint32_t*)(p->netHeader + 16));
	uint16_t port1 = *((uint16_t*)(p->transportHeader));
	uint16_t port2 = *((uint16_t*)(p->transportHeader + 2));

	key.getQuintuple()->srcIp   = ip1>ip2?ip1:ip2;
	key.getQuintuple()->dstIp   = ip1>ip2?ip2:ip1;
	key.getQuintuple()->srcPort = port1>port2?port1:port2;
	key.getQuintuple()->dstPort = port1>port2?port2:port1;

	if (*((uint8_t*)p->data + flagsOffset) & SYN) {
		DPRINTF("ConnectionFilter: Got SYN packet");
		synFilter.set(key.data, key.len, (agetime_t)p->timestamp.tv_sec);
		// do not export SYN packet, or should we?
		return false;
	} else if (*((uint8_t*)p->data + flagsOffset) & RST || *((uint8_t*)p->data + flagsOffset) & FIN) {
		int tmp = - exportFilter.get(key.data, key.len);
		
		DPRINTF("ConnectionFilter: Got %s packet", *((uint8_t*)p->data + flagsOffset) & RST?"RST":"FIN");
	
		exportFilter.set(key.data, key.len, tmp);
		connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
		// do not export FIN/RST packets, or should we?
		return false;
	} else {
		DPRINTF("ConnectionFilter: Got a normal packet");
		if ((tmp = exportFilter.get(key.data, key.len)) > 0) {
			DPRINTF("ConnectionFilter: Connection known, exporting packet");
			static unsigned diffVal;
			if (tmp > p->data_length)
				diffVal = -p->data_length;
			else
				diffVal = -tmp;
			exportFilter.set(key.data, key.len, diffVal);
			if (exportFilter.get(key.data, key.len) <= 0) {
				connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
			}
			DPRINTF("ConnectionFilter: We have to export %i bytes after exporting this packet", exportFilter.get(key.data, key.len));
			return true;
		} else {
			if ((unsigned)(p->timestamp.tv_sec - synFilter.get(key.data, key.len)) < timeout &&
			    synFilter.get(key.data, key.len) - connectionFilter.get(key.data, key.len) > 0) {
			    	DPRINTF("ConnectionFilter: Found new connection, exporting packet");
				if (p->data_length < exportBytes) {
					exportFilter.set(key.data, key.len, exportBytes - p->data_length);
				}
				DPRINTF("ConnectionFilter: We have to export %i bytes after exporting this packet", exportFilter.get(key.data, key.len));
				return true;
			}
			DPRINTF("ConnectionFilter: Paket will not be exported");
			return false;
		}
	}

	msg(MSG_FATAL, "ConnectionFilter: SOMTHING IS SCRWED UP, YOU SHOULD NEVER SEE THIS MESSAGE!");
	return false; // make compiler happy
}
