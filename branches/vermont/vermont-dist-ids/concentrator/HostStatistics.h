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

class HostStatistics : public IpfixRecordDestination
{
public:
	HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath, uint16_t logInt);
	void onDataDataRecord(IpfixDataDataRecord* record);

private:
	bool onReconfiguration();

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
