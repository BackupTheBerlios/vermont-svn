#if !defined(IPFIXCONNECTIONTRACKER_H)
#define IPFIXCONNECTIONTRACKER_H


#include "FlowSink.hpp"
#include "IpfixRecord.hpp"
#include "Connection.h"

#include <string>

using namespace std;

/**
 * tries to assign two flows to one connection for later analysis, e.g. portscan detection
 * uses internally a hashtable
 */
class IpfixConnectionTracker : public FlowSink
{
	protected:
		class Hashtable
		{
			public:
				static const uint32_t TABLE_SIZE = 65536;

				Hashtable(uint32_t expireTime);
				virtual ~Hashtable();

				void addFlow(Connection* c);
				bool aggregateFlow(Connection* c);
				void insertFlow(Connection* c);
				void expireConnections(queue<Connection*>* expiredFlows);

			private:
				list<Connection*> htable[TABLE_SIZE];
				Mutex tableMutex;
				uint32_t expireTime;
		};

	public:
		bool threadExited;

		IpfixConnectionTracker(uint32_t connTimeout);
		virtual ~IpfixConnectionTracker();

		int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		int onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);

		int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data);
		static void* threadExpireConns(void* connTracker);
		void expireConnectionsLoop();
		void startThread();
		void stopThread();

	private:
		Hashtable hashtable;
		uint32_t connTimeout;
		Thread expireThread;
};


#endif
