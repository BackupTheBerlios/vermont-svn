/*
 * PSAMP Reference Implementation
 *
 * HostStatistics.cpp
 *
 * Author: Matthias Segschneider <matthias.segschneider@gmx.de>
 *
 */
#include "HostStatistics.h"

HostStatistics::HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath, uint16_t logInt)
	: ipSubnet(ipSubnet), addrFilter(addrFilter), logPath(logPath), logInt(logInt)
{
	// check if srcIP or dstIP in the subnet (1.1.1.1/16)
	// split string at the '/'
	size_t found = ipSubnet.find_first_of("/");
	// convert string IP to uint32_t
	std::string ip_str = ipSubnet.substr(0, found);
	size_t pos_1 = 0;
	size_t pos_2 = 0;
	for (int i = 0; i < 4; i++) {
		// shift ip_addr left 8 times (no effect in 1st round)
		// seperate ip_str at the dots, convert every number-part to integer
		// "save" the number in ip_addr
		pos_2 = ip_str.find(".", pos_1);
		netAddr = (netAddr << 8) | atoi((ip_str.substr(pos_1, pos_2)).c_str());
		pos_1 = pos_2;
	}
	netSize = (uint8_t)atoi(ipSubnet.substr(found));
	// shift the netAddr to the right; this allows '==' comparisons if the other IPs are equally shifted
	netAddr = (netAddr >> netSize);
	logTimer = time(NULL);
}

void HostStatistics::onDataDataRecord(IpfixDataDataRecord* record)
{
	Connection conn(record);
	std::map<uint32_t, uint64_t>::iterator it;

	if ((addrFilter == 'src') && ((conn->srcIP >> netSize) == netAddr)) {
		it = trafficMap.find(conn->srcIP);
		if (it == trafficMap.end())	{
			trafficMap.insert(pair<uint32_t, uint64_t>(conn->srcIP, conn->srcOctets));
		} else {
			trafficMap[it] += conn->srcOctets;
		}
	} else if ((addFilter == 'dst') && ((conn->dstIP >> netSize) == netAddr)) {
		it = trafficMap.find(conn->dstIP);
		if (it == trafficMap.end()) {
			trafficMap.insert(pair<uint32_t, uint64_t>(conn->dstIP, conn->dstOctets));
		} else {
			trafficMap[it] += conn->dstcOctets;
		}
	} else {
		if ((conn->srcIP >> netSize) == netAddr) {
			it = trafficMap.find(conn->srcIP);
			if (it == trafficMap.end())	{
				trafficMap.insert(pair<uint32_t, uint64_t>(conn->srcIP, (conn->srcOctets + conn->dstOctets)));
			} else {
				trafficMap[it] += (conn->srcOctets + conn->dstOctets);
			}
		} else if ((conn->dstIP >> netSize) == netAddr) {
			it = trafficMap.find(conn->dstIP);
			if (it == trafficMap.end())	{
				trafficMap.insert(pair<uint32_t, uint64_t>(conn->dstIP, (conn->srcOctets + conn->dstOctets)));
			} else {
				trafficMap[it] += (conn->srcOctets + conn->dstOctets);
			}
		}
	}
}

bool exportData()
{
	std::map<uint32_t, uint64_t>::iterator it;

	logFile = fopen(logPath, "w");
	// insert current timestamp
	fprintf(logFile, "%d", time(NULL));
	// for each element in ipList, write an entry like: IP:Bytesum
	for (it = trafficMap.begin(); it != trafficMap.end(); it++) {
		fprintf(logFile, " %d:%d", it->first, it->second);
	}
	logFile.close();
}
