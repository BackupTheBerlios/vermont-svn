
#include "Connection.h"
#include "crc16.hpp"

#include <sstream>

/**
 * creates new connection element
 * @param connTimeout time in seconds when connection element times out
 */
Connection::Connection(uint32_t connTimeout)
	: srcIP(0), dstIP(0), srcPort(0), dstPort(0), 
	  srcTimeStart(0), srcTimeEnd(0),
	  dstTimeStart(0), dstTimeEnd(0),
	  srcOctets(0), dstOctets(0),
	  srcPackets(0), dstPackets(0),
	  srcTcpControlBits(0), dstTcpControlBits(0),
	  protocol(0)
{
	timeExpire = time(0) + connTimeout;
}


string Connection::printIP(uint32_t ip)
{
	ostringstream oss;
	uint8_t* pip = (uint8_t*)(&ip);
	oss << static_cast<int>(pip[0]) << "." << static_cast<int>(pip[1]) << "."
		<< static_cast<int>(pip[2]) << "." << static_cast<int>(pip[3]);
	return oss.str();
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
	if (srcIP) oss << "srcIP: " << printIP(srcIP) << endl;
	if (dstIP) oss << "dstIP: " << printIP(dstIP) << endl;
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
 * swaps aggregatable fields inside the connection
 */
void Connection::swapFields()
{
	uint32_t tmp;
	tmp = srcIP;
	srcIP = dstIP;
	dstIP = tmp;

	uint16_t tmp2;
	tmp2 = srcPort;
	srcPort = dstPort;
	dstPort = tmp2;
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
