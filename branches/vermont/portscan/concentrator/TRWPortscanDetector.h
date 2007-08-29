#if !defined(TRWPORTSCANDETECTOR_H)
#define TRWPORTSCANDETECTOR_H

#include "ConnectionReceiver.h"
#include "common/StatisticsManager.h"

#include <list>

using namespace std;

class TRWPortscanDetector : public ConnectionReceiver, StatisticsModule
{
	public:
		TRWPortscanDetector();
		virtual ~TRWPortscanDetector();
		virtual void push(Connection* conn);

	private:
		struct TRWEntry {
			uint32_t srcIP;
			uint32_t dstSubnet;
			uint32_t dstSubnetMask;
			uint32_t numFailedConns;
			uint32_t numSuccConns;
			int32_t timeExpire;
			bool reported;
		};

		const static int HASH_SIZE = 65536;
		const static int TIME_EXPIRE = 10;
		list<TRWEntry*> trwEntries[HASH_SIZE];
		uint32_t statEntriesAdded;
		uint32_t statEntriesRemoved;
		uint32_t statNumScanners;

		TRWEntry* createEntry(Connection* conn);
		TRWEntry* getEntry(Connection* conn);
		void addConnection(Connection* conn);
		virtual string getStatistics();
};

#endif
