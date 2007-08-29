
#include "Connection.h"
#include "crc16.hpp"
#include "common/Misc.h"

#include <sstream>
#include <algorithm>
#include <iostream>

/**
 * creates new connection element
 * @param connTimeout time in seconds when connection element times out
 */
Connection::Connection(InstanceManager<Connection>* im)
	: ManagedInstance<Connection>(im)
{
}

void Connection::init(uint32_t connTimeout)
{
	timeExpire = time(0) + connTimeout;
	srcIP = 0;
	dstIP = 0;
	srcPort = 0;
	dstPort = 0;
	srcTimeStart = 0;
	srcTimeEnd = 0;
	dstTimeStart = 0;
	dstTimeEnd = 0;
	srcOctets = 0;
	dstOctets = 0;
	srcPackets = 0;
	dstPackets = 0;
	srcTcpControlBits = 0;
	dstTcpControlBits = 0;
	protocol = 0;
}

/**
 * swaps all data fields inside the connection
 */
void Connection::swapDataFields()
{
	swap(srcIP, dstIP);
	swap(srcPort, dstPort);
	swap(srcTimeStart, dstTimeStart);
	swap(srcTimeEnd, dstTimeEnd);
	swap(srcOctets, dstOctets);
	swap(srcPackets, dstPackets);
	swap(srcTcpControlBits, dstTcpControlBits);
}

/**
 * a nice little function here: it tries to determine the host which initiated the
 * connection and, if needed, swaps all data so that the initiating host is
 * specified as source host
 */
void Connection::swapIfNeeded()
{
	// try to find initiating host by analyzing the SYN and ACK bits
	if ((srcTcpControlBits&SYN) && !(srcTcpControlBits&ACK)) return;
	if ((dstTcpControlBits&SYN) && !(dstTcpControlBits&ACK)) {
		swapDataFields();
		return;
	}

	// now try the starting time
	if (srcTimeStart>dstTimeStart) swapDataFields();
}

string Connection::printTcpControlBits(uint8_t bits)
{
	ostringstream oss;
	const string strbits[] = { "", "", "URG", "ACK", "PSH", "RST", "SYN", "FIN" };
	for (int i=0; i<8; i++) {
		if ((bits&0x80)>0) oss << strbits[i];
		bits <<= 1;
	}
	return oss.str();
}


string Connection::toString()
{
	ostringstream oss;

	oss << "connection: " << endl;
	if (srcIP) oss << "srcIP: " << IPToString(srcIP) << endl;
	if (dstIP) oss << "dstIP: " << IPToString(dstIP) << endl;
	if (srcPort) oss << "srcPort: " << srcPort << endl;
	if (dstPort) oss << "dstPort: " << dstPort << endl;
	if (srcTimeStart) oss << "srcTimeStart: " << srcTimeStart << endl;
	if (srcTimeEnd) oss << "srcTimeEnd: " << srcTimeEnd << endl;
	if (dstTimeStart) oss << "dstTimeStart: " << dstTimeStart << endl;
	if (dstTimeEnd) oss << "dstTimeEnd: " << dstTimeEnd << endl;
	oss << "srcOctets: " << srcOctets << ", dstOctets: " << dstOctets << endl;
	oss << "srcPackets: " << srcPackets << ", dstPackets: " << dstPackets << endl;
	if (srcTcpControlBits || dstTcpControlBits) oss << "srcTcpControlBits: " << printTcpControlBits(srcTcpControlBits) 
													<< ", dstTcpControlBits: " << printTcpControlBits(dstTcpControlBits) << endl;
	if (protocol) oss << "protocol: " << static_cast<uint32_t>(protocol) << endl;

	return oss.str();
}

/**
 * compares aggregatable fields to another connection
 * @param to connection direction to be regarded, true if src->dst
 */
bool Connection::compareTo(Connection* c, bool to)
{
	if (to) {
		return memcmp(&srcIP, &c->srcIP, 12)==0;
	} else {
		return (srcIP==c->dstIP && dstIP==c->srcIP &&
				srcPort==c->dstPort && dstPort==c->srcPort);
	}
}

/**
 * calculates hash from flow
 * @param which direction should be used? true if src->dst, else false
 */
uint16_t Connection::getHash(bool to)
{
	if (to) {
		return crc16(0, 12, reinterpret_cast<char*>(&srcIP));
	} else {
		uint16_t hash = crc16(0, 4, reinterpret_cast<char*>(&dstIP));
		hash = crc16(hash, 4, reinterpret_cast<char*>(&srcIP));
		hash = crc16(hash, 2, reinterpret_cast<char*>(&dstPort));
		hash = crc16(hash, 2, reinterpret_cast<char*>(&srcPort));
		return hash;
	}
}

/**
 * aggregates fields from given connection into this connection
 * @param expireTime seconds until this connection expires
 * @param to true if this connection has to be aggregated in direction src->dst or false if reverse
 */
void Connection::aggregate(Connection* c, uint32_t expireTime, bool to)
{
	timeExpire = time(0) + expireTime;

	if (to) {
		srcOctets += c->srcOctets;
		srcPackets += c->srcPackets;
		srcTcpControlBits |= c->srcTcpControlBits;
		if (c->srcTimeStart < srcTimeStart) srcTimeStart = c->srcTimeStart;
		if (c->srcTimeEnd > srcTimeEnd) srcTimeEnd = c->srcTimeEnd;
	} else {
		dstOctets += c->srcOctets;
		dstPackets += c->srcPackets;
		dstTcpControlBits |= c->srcTcpControlBits;
		if (c->dstTimeStart < srcTimeStart) dstTimeStart = c->srcTimeStart;
		if (c->dstTimeEnd > srcTimeEnd) dstTimeEnd = c->srcTimeEnd;
	}
}
