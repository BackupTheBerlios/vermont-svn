#include "ConnectionFilter.h"

ConnectionFilter::ConnectionFilter()
{
	
}

bool ConnectionFilter::processPacket(const Packet* p)
{
	unsigned flagsOffset = p->transportHeaderOffset + 13;
	static const uint8_t SYN = 0x02;
	static const uint8_t ACK = 0x10;
	static const uint8_t FIN = 0x01;
	static const uint8_t RST = 0x04;
	static uint32_t srcIp;
	static uint32_t dstIp;
	static QuintupleKey key;
	static unsigned tmp;

	// configure
	static unsigned maxTime = 10;
	static unsigned exportBytes = 1000;


	if (p->ipProtocolType != Packet::TCP) 
		return false;

	// use Packet::getPacketData()
	key.getQuintuple()->srcIp   = *((uint32_t*)(p->netHeader + 12));
	key.getQuintuple()->dstIp   = *((uint32_t*)(p->netHeader + 16));
	key.getQuintuple()->proto   = p->ipProtocolType;
	key.getQuintuple()->srcPort = *((uint32_t*)(p->transportHeader));
	key.getQuintuple()->dstPort = *((uint32_t*)(p->transportHeader + 2));

	// handle syn packets. do not export them (or should we?)
	if (*((uint8_t*)p->data + flagsOffset) & SYN) {
		synFilter.getAndSetLastTime(key.data, key.len, p->timestamp.tv_sec);
		return false;
	}

	if (*((uint8_t*)p->data + flagsOffset) & ACK) {
		if ((tmp = exportFilter.getValue(key.data, key.len)) > 0) {
			static unsigned diffVal;
			if (tmp > p->data_length)
				diffVal = -p->data_length;
			else
				diffVal = -tmp;
			exportFilter.getAndSetValue(key.data, key.len, diffVal);
			if (tmp <= 0) {
				connectionFilter.getAndSetLastTime(key.data, key.len, p->timestamp.tv_sec);
			}
			return true;
		} else {
			if (p->timestamp.tv_sec - synFilter.getLastTime(key.data, key.len) < maxTime &&
			    synFilter.getLastTime(key.data, key.len) - connectionFilter.getLastTime(key.data, key.len) > 0) {
				if (p->data_length < exportBytes) {
					exportFilter.getAndSetValue(key.data, key.len, exportBytes - p->data_length);
				}
				return true;
			}
			return false;
		}
	}


	if (*((uint8_t*)p->data + flagsOffset) & RST || *((uint8_t*)p->data + flagsOffset) & FIN) {
		int tmp = 0 - exportFilter.getValue(key.data, key.len);
		exportFilter.getAndSetValue(key.data, key.len, tmp);
		connectionFilter.getAndSetLastTime(key.data, key.len, p->timestamp.tv_sec);
		// do not export FIN/RST packets
		return false;
	}

	return false; // make compiler happy
}
