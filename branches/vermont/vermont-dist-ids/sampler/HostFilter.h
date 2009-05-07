/*
 * PSAMP Reference Implementation
 *
 * HostFilter.h
 *
 * Filter by IP addresses from IP header given by (offset, size, comparison, targetvalue)
 *
 * Author: Matthias Segschneider <matthias.segschneider@gmx.de>
 *
 */

#ifndef HOSTFILTER_H_
#define HOSTFILTER_H_

#include "PacketProcessor.h"

class HostFilter : public PacketProcessor
{
public:
	HostFilter(std::string addrFilter, std::set<uint32_t> ipList);

	bool processPacket(Packet *p);

private:
	std::string addrFilter;
	std::set<uint32_t> ipList;
};

#endif /*HOSTFILTER_H_*/
