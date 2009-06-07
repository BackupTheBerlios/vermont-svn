/*
 * HostStatistics.h
 *
 *
 * Author: Matthias Segschneider <matthias.segschneider@gmx.de>
 *
 */

#ifndef HOSTSTATISTICS_H_
#define HOSTSTATISTICS_H_

#include <time.h>
#include <map>

#include "IpfixRecordDestination.h"

class HostStatistics : public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
public:
	HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath, uint16_t logInt);
	void onDataDataRecord(IpfixDataDataRecord* record);

	virtual void onReconfiguration1();

private:
	std::string ipSubnet;
	std::string addrFilter;
	std::string logPath;
	uint16_t logInt;
	uint32_t netAddr;
	uint8_t netSize;
	time_t logTimer;
	std::map<uint32_t, uint64_t> trafficMap; // key: IP Address, value: Bytes (sum of all packets with this IP)
};

#endif /* HOSTSTATISTICS_H_ */