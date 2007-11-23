#include "ConnectionFilter.h"

ConnectionFilter::ConnectionFilter(unsigned timeout, unsigned bytes, unsigned filterSize, unsigned hashFunctions)
	: synFilter(filterSize, hashFunctions), exportFilter(filterSize, hashFunctions),
	  connectionFilter(filterSize, hashFunctions)
{
	this->timeout = timeout;
	this->exportBytes = bytes;
}

bool ConnectionFilter::processPacket(const Packet* p)
{
	unsigned flagsOffset = p->transportHeaderOffset + 13;
	static const uint8_t SYN = 0x02;
	//static const uint8_t ACK = 0x10;
	static const uint8_t FIN = 0x01;
	static const uint8_t RST = 0x04;
	static QuintupleKey key;
	static unsigned tmp;

	if (p->ipProtocolType != Packet::TCP) 
		return false;

	// use Packet::getPacketData()
	key.getQuintuple()->srcIp   = *((uint32_t*)(p->netHeader + 12));
	key.getQuintuple()->dstIp   = *((uint32_t*)(p->netHeader + 16));
	key.getQuintuple()->proto   = p->ipProtocolType;
	key.getQuintuple()->srcPort = *((uint32_t*)(p->transportHeader));
	key.getQuintuple()->dstPort = *((uint32_t*)(p->transportHeader + 2));

	if (*((uint8_t*)p->data + flagsOffset) & SYN) {
		msg(MSG_DEBUG, "ConnectionFilter: Got SYN packet");
		msg(MSG_DEBUG, "ConnectionFilter: timestamp: %u", p->timestamp.tv_sec);
		synFilter.set(key.data, key.len, (agetime_t)p->timestamp.tv_sec);
		// do not export SYN packet, or should we?
		return false;
	} else if (*((uint8_t*)p->data + flagsOffset) & RST || *((uint8_t*)p->data + flagsOffset) & FIN) {
		int tmp = 0 - exportFilter.get(key.data, key.len);
		
		msg(MSG_DEBUG, "ConnectionFilter: Got %s packet", *((uint8_t*)p->data + flagsOffset) & RST?"RST":"FIN");
	
		exportFilter.set(key.data, key.len, tmp);
		connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
		// do not export FIN/RST packets, or should we?
		return false;
	} else {
		msg(MSG_DEBUG, "ConnectionFilter: Got a normal packet");
		if ((tmp = exportFilter.get(key.data, key.len)) > 0) {
			msg(MSG_DEBUG, "ConnectionFilter: Connection known, exporting packet");
			static unsigned diffVal;
			if (tmp > p->data_length)
				diffVal = -p->data_length;
			else
				diffVal = -tmp;
			exportFilter.set(key.data, key.len, diffVal);
			if (tmp <= 0) {
				connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
			}
			return true;
		} else {
			msg(MSG_DEBUG, "%u, %u", p->timestamp.tv_sec, synFilter.get(key.data, key.len));
			msg(MSG_DEBUG, "%u, %i", (unsigned)(p->timestamp.tv_sec - synFilter.get(key.data, key.len)),
			synFilter.get(key.data, key.len) - connectionFilter.get(key.data, key.len));
			if ((unsigned)(p->timestamp.tv_sec - synFilter.get(key.data, key.len)) < timeout &&
			    synFilter.get(key.data, key.len) - connectionFilter.get(key.data, key.len) > 0) {
			    	msg(MSG_DEBUG, "ConnectionFilter: Found new connection, exporting packet");
				if (p->data_length < exportBytes) {
					exportFilter.set(key.data, key.len, exportBytes - p->data_length);
				}
				return true;
			}
			msg(MSG_DEBUG, "ConnectionFilter: Connection expired, not exporting packet");
			return false;
		}
	}

	msg(MSG_FATAL, "ConnectionFilter: SOMTHING IS SCRWED UP, YOU SHOULD NEVER SEE THIS MESSAGE!");
	return false; // make compiler happy
}
