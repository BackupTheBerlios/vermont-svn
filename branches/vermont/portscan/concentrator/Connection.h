#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <stdint.h>
#include <string>

using namespace std;


class Connection
{
	public:
		// ATTENTION: next four elements MUST be declared sequentially without another element interrupting it
		// because hash value is calculated by accessing the memory directly from srcIP on
		// (see function calcHash and compareTo)
		uint32_t srcIP;
		uint32_t dstIP;
		uint16_t srcPort;
		uint16_t dstPort;

		// fields to be aggregated
		uint64_t timeStart; /**< milliseconds since 1970 */
		uint64_t timeEnd; /**< milliseconds since 1970 */
		uint64_t octets;
		uint8_t tcpControlBits;
		uint8_t protocol;

		/** 
		 * time in seconds from 1970 on when this record will expire
		 * this value is always updated when it is aggregated
		 */
		uint32_t timeExpire; 

		Connection(uint32_t connTimeout);
		void addFlow(Connection* c);
		string printIP(uint32_t ip);
		string toString();
		bool compareTo(Connection* c);
		uint16_t getHash();
		void aggregate(Connection* c, uint32_t expireTime);
		void swapFields();
};

#endif
