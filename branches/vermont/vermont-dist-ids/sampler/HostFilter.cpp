/*
 * PSAMP Reference Implementation
 *
 * HostFilter.cpp
 *
 * Filter by IP addresses from IP header given by (offset, size, comparison, targetvalue)
 *
 * Author: Matthias Segschneider <matthias.segschneider@gmx.de>
 *
 */

#include "HostFilter.h"

#define IPV4_ADDRESS_BYTE_LENGTH	4		// Bytes
#define IPV4_SRC_IP_OFFSET			12
#define IPV4_DST_IP_OFFSET			16

bool HostFilter::processPacket(Packet *p)
{
	// if no IPv4 Packet
	if (p->classification != PCLASS_NET_IP4)
	{
		return false;
	}
	// srcIP
	uint32_t srcIp = *reinterpret_cast<uint32_t*>(p->netHeader+IPV4_SRC_IP_OFFSET);
	// dstIP
	uint32_t dstIp = *reinterpret_cast<uint32_t*>(p->netHeader+IPV4_DST_IP_OFFSET);

	if (addrFilter == "src") {
		return (ipList.find(srcIp) != ipList.end());
	} else if (addrFilter == "dst") {
		return (ipList.find(dstIp) != ipList.end());
	} else if (addrFilter == "both") {
		return ((ipList.find(srcIp) != ipList.end()) || (ipList.find(dstIp) != ipList.end()));
	} else {
		msg(MSG_FATAL, "Unknown observer config statement %s\n", conf.getAddrFilter());
		return false;
	}
}
