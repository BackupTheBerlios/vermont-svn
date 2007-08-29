
#include "Connection.h"
#include "crc16.hpp"

#include <sstream>

/**
 * creates new connection element
 * @param connTimeout time in seconds when connection element times out
 */
Connection::Connection(uint32_t connTimeout)
	: srcIP(0), dstIP(0), srcPort(0), dstPort(0), timeStart(0), timeEnd(0),
	  octets(0), tcpControlBits(0), protocol(0)
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


string Connection::toString()
{
	ostringstream oss;

	oss << "connection: " << endl;
	if (srcIP) oss << "srcIP: " << printIP(srcIP) << endl;
	if (dstIP) oss << "dstIP: " << printIP(dstIP) << endl;
	if (srcPort) oss << "srcPort: " << srcPort << endl;
	if (dstPort) oss << "dstPort: " << dstPort << endl;
	if (timeStart) oss << "timeStart: " << timeStart << endl;
	if (timeEnd) oss << "timeEnd: " << timeEnd << endl;
	oss << "octets: " << octets << endl;
	if (tcpControlBits) oss << "tcpControlBits: " << tcpControlBits << endl;
	if (protocol) oss << "protocol: " << static_cast<uint32_t>(protocol) << endl;

	return oss.str();
}

/**
 * compares aggregatable fields to another connection
 */
bool Connection::compareTo(Connection* c)
{
	return memcmp(&srcIP, &c->srcIP, 12)==0;
}

/**
 * calculates hash from flow
 */
uint16_t Connection::getHash()
{
	return crc16(0, 12, reinterpret_cast<char*>(&srcIP));
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
 */
void Connection::aggregate(Connection* c, uint32_t expireTime)
{
	if (c->timeStart < timeStart) timeStart = c->timeStart;
	if (c->timeEnd > timeEnd) timeEnd = c->timeEnd;
	octets += c->octets;
	tcpControlBits |= c->tcpControlBits;
	timeExpire = time(0) + expireTime;
}
