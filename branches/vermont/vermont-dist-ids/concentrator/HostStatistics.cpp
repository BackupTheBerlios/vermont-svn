/*
 * PSAMP Reference Implementation
 *
 * HostStatistics.cpp
 *
 * Author: Matthias Segschneider <matthias.segschneider@gmx.de>
 *
 */
#include "HostStatistics.h"
#include "Connection.h"
#include <netdb.h>
#include "common/Misc.h"


HostStatistics::HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath, uint16_t logInt)
	: ipSubnet(ipSubnet), addrFilter(addrFilter), logPath(logPath), logInt(logInt)
{
	// check if srcIP or dstIP in the subnet (1.1.1.1/16)
	// split string at the '/'
	size_t found = ipSubnet.find_first_of("/");
	std::string ip_str = ipSubnet.substr(0, found);
	netSize = atoi(ipSubnet.substr(found + 1).c_str());
	netAddr = *(uint32_t *)gethostbyname(ip_str.c_str())->h_addr;
	logTimer = time(NULL);
	
	// DEBUG
	FILE* logFile = fopen("host_stats.log", "a");
	fprintf(logFile, "netAddr: %u\t", netAddr);
	fprintf(logFile, " - %s\n", IPToString(netAddr).c_str());
	fclose(logFile);
}

void HostStatistics::onDataDataRecord(IpfixDataDataRecord* record)
{
	Connection conn(record);
	std::map<uint32_t, uint64_t>::iterator it;

	FILE* logFile = fopen("host_stats.log", "a");

	if ((addrFilter == "src") && ((conn.srcIP&netAddr) == netAddr)) {
		it = trafficMap.find(conn.srcIP);
		if (it == trafficMap.end())	{
			trafficMap.insert(pair<uint32_t, uint64_t>(conn.srcIP, ntohl(conn.srcOctets)));
		} else {
			it->second += ntohl(conn.srcOctets);
		}
	} else if ((addrFilter == "dst") && ((conn.dstIP&netAddr) == netAddr)) {
		it = trafficMap.find(conn.dstIP);
		if (it == trafficMap.end()) {
			trafficMap.insert(pair<uint32_t, uint64_t>(conn.dstIP, ntohl(conn.dstOctets)));
		} else {
			it->second += ntohl(conn.dstOctets);
		}
	} else {
		if ((conn.srcIP&netAddr) == netAddr) {
			fprintf(logFile, "Treffer - src\t");
			it = trafficMap.find(conn.srcIP);
			if (it == trafficMap.end())	{
				fprintf(logFile, "- %s\n", IPToString(conn.srcIP).c_str());
				trafficMap.insert(pair<uint32_t, uint64_t>(conn.srcIP, (ntohl(conn.srcOctets) + ntohl(conn.dstOctets))));
			} else {
				fprintf(logFile, "\n");
				it->second += (ntohl(conn.srcOctets) + ntohl(conn.dstOctets));
			}
		} else if ((conn.dstIP&netAddr) == netAddr) {
			fprintf(logFile, "Treffer - dst\t");
			it = trafficMap.find(conn.dstIP);
			if (it == trafficMap.end())	{
				fprintf(logFile, "- %s\n", IPToString(conn.dstIP).c_str());
				trafficMap.insert(pair<uint32_t, uint64_t>(conn.dstIP, (ntohl(conn.srcOctets) + ntohl(conn.dstOctets))));
			} else {
				fprintf(logFile, "\n");
				it->second += (ntohl(conn.srcOctets) + ntohl(conn.dstOctets));
			}
		}
	}
	fclose(logFile);
}

void HostStatistics::onReconfiguration1()
{
	std::map<uint32_t, uint64_t>::iterator it;

	FILE* logFile = fopen(logPath.c_str(), "w");
	// insert current timestamp
	fprintf(logFile, "%d", (int)time(NULL));
	// for each element in ipList, write an entry like: IP:Bytesum
	for (it = trafficMap.begin(); it != trafficMap.end(); it++) {
		fprintf(logFile, " %s:%u", IPToString(it->first).c_str(), (uint32_t)it->second);
	}
	fclose(logFile);
}
