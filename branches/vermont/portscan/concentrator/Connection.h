#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <stdint.h>
#include <string>

#include "common/ManagedInstance.h"

using namespace std;


class Connection : public ManagedInstance<Connection>
{
	public:
		static const uint8_t SYN = 0x02;
		static const uint8_t RST = 0x04;
		static const uint8_t ACK = 0x10;

		// ATTENTION: next four elements MUST be declared sequentially without another element interrupting it
		// because hash and compare is performed by accessing the memory directly from srcIP on
		// (see function calcHash and compareTo)
		// one more notice: additional fields are to be added to function swapDataFields!
		uint32_t srcIP;
		uint32_t dstIP;
		uint16_t srcPort;
		uint16_t dstPort;

		// fields to be aggregated
		uint64_t srcTimeStart; /**< milliseconds since 1970 */
		uint64_t srcTimeEnd; /**< milliseconds since 1970 */
		uint64_t dstTimeStart; /**< milliseconds since 1970 */
		uint64_t dstTimeEnd; /**< milliseconds since 1970 */
		uint64_t srcOctets;
		uint64_t dstOctets;
		uint64_t srcPackets;
		uint64_t dstPackets;
		uint8_t srcTcpControlBits;
		uint8_t dstTcpControlBits;
		uint8_t protocol;

		/** 
		 * time in seconds from 1970 on when this record will expire
		 * this value is always updated when it is aggregated
		 */
		uint32_t timeExpire; 

		Connection(InstanceManager<Connection>* im);
		void init(uint32_t connTimeout);
		void addFlow(Connection* c);
		string toString();
		string printTcpControlBits(uint8_t bits);
		bool compareTo(Connection* c, bool to);
		uint16_t getHash(bool to);
		void aggregate(Connection* c, uint32_t expireTime, bool to);
		void swapDataFields();
		void swapIfNeeded();
};

#endif
