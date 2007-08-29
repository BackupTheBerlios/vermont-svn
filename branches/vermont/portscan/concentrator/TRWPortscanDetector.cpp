#include "TRWPortscanDetector.h"
#include "crc16.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>


TRWPortscanDetector::TRWPortscanDetector()
{
	StatisticsManager::getInstance().addModule(this);
}

TRWPortscanDetector::~TRWPortscanDetector()
{
}

TRWPortscanDetector::TRWEntry* TRWPortscanDetector::createEntry(Connection* conn)
{
	TRWEntry* trw = new TRWEntry();
	trw->srcIP = conn->srcIP;
	trw->dstSubnet = 0;
	trw->dstSubnetMask = 0xFFFFFFFF;
	trw->numFailedConns = 0;
	trw->numSuccConns = 0;
	trw->timeExpire = time(0) + TIME_EXPIRE;
	trw->reported = false;

	statEntriesAdded++;

	return trw;
}

TRWPortscanDetector::TRWEntry* TRWPortscanDetector::getEntry(Connection* conn)
{
	time_t curtime = time(0);
	uint16_t hash = crc16(0, 2, &reinterpret_cast<char*>(&conn->srcIP)[2]);

	list<TRWEntry*>::iterator iter = trwEntries[hash].begin();
	while (iter != trwEntries[hash].end()) {
		// detect expired entries
		while (iter != trwEntries[hash].end() && curtime>(*iter)->timeExpire) {
			// yes, we need to remove this one
			TRWEntry* te = *iter;
			iter++;
			delete te;
			trwEntries[hash].remove(te);
			statEntriesRemoved++;
		}
		if (iter != trwEntries[hash].end() && (*iter)->srcIP == conn->srcIP) {
			// found the entry, has it expired?
			if (time(0)>(*iter)->timeExpire) {
				// yes, we need to remove this one
				delete *iter;
				trwEntries[hash].remove(*iter);
				statEntriesRemoved++;
				break;
			} else {
				// no, let's return it
				return *iter;
			}
		}
		iter++;
	}

	// no entry found, create a new one
	TRWEntry* trw = createEntry(conn);
	trwEntries[hash].push_back(trw);

	return trw;
}

void TRWPortscanDetector::addConnection(Connection* conn)
{
	TRWEntry* te = getEntry(conn);

	// aggregate new connection into entry
	if (te->dstSubnet==0 && te->dstSubnetMask==0xFFFFFFFF) {
		te->dstSubnet = conn->dstIP;
	} else {
		// adapt subnet mask so that new destination ip is inside given subnet
		while ((te->dstSubnet&te->dstSubnetMask)!=(conn->dstIP&te->dstSubnetMask)) {
			te->dstSubnetMask = ntohl(te->dstSubnetMask);
			te->dstSubnetMask <<= 1;
			te->dstSubnetMask = htonl(te->dstSubnetMask);
			te->dstSubnet &= te->dstSubnetMask;
		}
	}

	// determine if connection was a failed or successful connection attempt
	// by looking if answering host sets the syn bit for the threeway handshake
	if (!(conn->dstTcpControlBits&Connection::SYN)) {
		// no, this is not a successful connection attempt!
		te->numFailedConns++;
		te->timeExpire = time(0)+TIME_EXPIRE;
	} else {
		te->numSuccConns++;
	}
	
	// announce host as portscanner, if too many unsuccessful connection attempts
	if (te->numFailedConns>100 && !te->reported) {
		te->reported = true;
		statNumScanners++;
		msg(MSG_INFO, "PORTSCANNER:");
		msg(MSG_INFO, "srcIP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(te->srcIP).c_str(), 
				IPToString(te->dstSubnet).c_str(), IPToString(te->dstSubnetMask).c_str());
		msg(MSG_INFO, "numFailedConns: %d, numSuccConns: %d", te->numFailedConns, te->numSuccConns);
		char cmdline[500];
		sprintf(cmdline, "perl -I../ims_idmefsender/includes ../ims_idmefsender/ims_idmefsender.pl %s %s %s %d",
				IPToString(te->srcIP).c_str(), IPToString(te->dstSubnet).c_str(), 
				IPToString(te->dstSubnetMask).c_str(), te->numSuccConns);

		if (system(cmdline)!=0) {
			msg(MSG_ERROR, "failed to exec '%s'", cmdline);
		}
	}
}

void TRWPortscanDetector::push(Connection* conn)
{
	conn->swapIfNeeded();

	// only use this connection if it was a connection attempt
	if (conn->srcTcpControlBits&Connection::SYN) {
		addConnection(conn);
	}

	conn->removeReference();
}

string TRWPortscanDetector::getStatistics()
{
	ostringstream oss;
	oss << "trwportscan: ips cached       : " << statEntriesAdded-statEntriesRemoved << endl;
	oss << "trwportscan: ips removed      : " << statEntriesRemoved << endl;
	oss << "trwportscan: scanners detected: " << statNumScanners << endl;
	return oss.str();
}

