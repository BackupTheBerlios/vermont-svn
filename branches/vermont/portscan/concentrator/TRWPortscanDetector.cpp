#include "TRWPortscanDetector.h"
#include "crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>


TRWPortscanDetector::TRWPortscanDetector()
{
	// make some initialization calculations
	float theta_0 = 0.8; // probability that benign host makes successful connection
	float theta_1 = 0.2; // probability that malicious host makes successful connection
	float P_F = 0.00001; // probability of false alarm
	float P_D = 0.99999; // probability of scanner detection

	float eta_1 = 1/P_F;
	float eta_0 = 1-P_D;
	logeta_0 = logf(eta_0);
	logeta_1 = logf(eta_1);
	X_0 = logf(theta_1/theta_0);
	X_1 = logf((1-theta_1)/(1-theta_0));
	msg(MSG_INFO, "TRW variables: logeta_0: %f, logeta_1: %f, X_0: %f, X_1: %f", logeta_0, logeta_1, X_0, X_1);

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
	trw->timeExpire = 0;
	trw->decision = PENDING;
	trw->S_N = 0;

	statEntriesAdded++;

	return trw;
}

TRWPortscanDetector::TRWEntry* TRWPortscanDetector::getEntry(Connection* conn)
{
	time_t curtime = time(0);
	uint32_t hash = crc32(0, 2, &reinterpret_cast<char*>(&conn->srcIP)[2]) & (HASH_SIZE-1);

	list<TRWEntry*>::iterator iter = trwEntries[hash].begin();
	while (iter != trwEntries[hash].end()) {
		// detect expired entries
		while (iter != trwEntries[hash].end() && (*iter)->timeExpire!=0 && curtime>(*iter)->timeExpire) {
			// yes, we need to remove this one
			TRWEntry* te = *iter;
			iter++;
			delete te;
			trwEntries[hash].remove(te);
			statEntriesRemoved++;
		}
		if (iter != trwEntries[hash].end() && (*iter)->srcIP == conn->srcIP) {
			// found the entry
			return *iter;
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

	//if (conn->srcIP==0x8AAAAB53) {
		//msg(MSG_INFO, "***********homeip  data:");
		//msg(MSG_INFO, "srcIP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(te->srcIP).c_str(), 
				//IPToString(te->dstSubnet).c_str(), IPToString(te->dstSubnetMask).c_str());
		//msg(MSG_INFO, "numFailedConns: %d, numSuccConns: %d", te->numFailedConns, te->numSuccConns);
		//cout << conn->toString();
	//}

	// this host was already decided on, don't do anything any more
	if (te->decision != PENDING) return;

	// determine if connection was a failed or successful connection attempt
	// by looking if answering host sets the syn+ack bits for the threeway handshake
	bool connsuccess;
	if ((conn->dstTcpControlBits&(Connection::SYN|Connection::ACK))!=(Connection::SYN|Connection::ACK)) {
		// no, this is not a successful connection attempt!
		te->numFailedConns++;
		connsuccess = false;

	} else {
		te->numSuccConns++;
		connsuccess = true;
	}

	// only work with this connection, if it wasn't accessed before by this host
	if (find(te->accessedHosts.begin(), te->accessedHosts.end(), conn->dstIP) != te->accessedHosts.end()) return;

	te->accessedHosts.push_back(conn->dstIP);

	te->S_N += (connsuccess ? X_0 : X_1);

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

	DPRINTF("IP: %s, S_N: %f", IPToString(te->srcIP).c_str(), te->S_N);
	
	// look if information is adequate for deciding on host
	if (te->S_N<logeta_0) {
		// no portscanner, just let entry stay here until it expires
		te->timeExpire = time(0)+TIME_EXPIRE;
		te->decision = BENIGN;
	} else if (te->S_N>logeta_1) {
		//this is a portscanner!
		te->decision = SCANNER;
		statNumScanners++;
		te->timeExpire = time(0)+TIME_EXPIRE;
		msg(MSG_INFO, "PORTSCANNER:");
		msg(MSG_INFO, "srcIP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(te->srcIP).c_str(), 
				IPToString(te->dstSubnet).c_str(), IPToString(te->dstSubnetMask).c_str());
		msg(MSG_INFO, "numFailedConns: %d, numSuccConns: %d", te->numFailedConns, te->numSuccConns);
		char cmdline[500];
		sprintf(cmdline, "perl -I../ims_idmefsender/includes ../ims_idmefsender/ims_idmefsender.pl %s %s %s %d %d",
				IPToString(te->srcIP).c_str(), IPToString(te->dstSubnet).c_str(), 
				IPToString(te->dstSubnetMask).c_str(), te->numSuccConns, te->numFailedConns);

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

