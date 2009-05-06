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
	HostFilter(std::string addrfilter, std::set<uint32_t> iplist);
	
	bool processPacket(Packet *p);
	
private:
	HostFilterCfg* conf;
	std::string addrFilter;
	std::set<unit32_t> ipList;
}

#endif /*HOSTFILTER_H_*/
