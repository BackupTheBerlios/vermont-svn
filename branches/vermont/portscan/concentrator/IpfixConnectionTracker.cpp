
#include "IpfixConnectionTracker.h"
#include "crc16.hpp"

#include <sstream>
#include <iostream>


IpfixConnectionTracker::IpfixConnectionTracker(uint32_t connTimeout)
	: threadExited(0), hashtable(connTimeout), connTimeout(connTimeout), expireThread(threadExpireConns)
{
}

IpfixConnectionTracker::~IpfixConnectionTracker()
{
}

int IpfixConnectionTracker::onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo)
{
	return 0;
}

int IpfixConnectionTracker::onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo)
{
	return 0;
}

int IpfixConnectionTracker::onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data) 
{
	Connection* conn = connectionManager.getNewInstance();
	conn->init(connTimeout);

	IpfixRecord::FieldInfo* fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4Address, 0);
	if (fi != 0) conn->srcIP = *(uint32_t*)(data+fi->offset);
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4Address, 0);
	if (fi != 0) conn->dstIP = *(uint32_t*)(data+fi->offset);
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_sourceTransportPort, 0);
	if (fi != 0) conn->srcPort = ntohs(*(uint16_t*)(data+fi->offset));
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_destinationTransportPort, 0);
	if (fi != 0) conn->dstPort = ntohs(*(uint16_t*)(data+fi->offset));
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSeconds, 0);
	if (fi != 0) conn->srcTimeStart = ntohl(*(uint32_t*)(data+fi->offset))*1000;
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowEndSeconds, 0);
	if (fi != 0) conn->srcTimeEnd = ntohl(*(uint32_t*)(data+fi->offset))*1000;
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowStartMilliSeconds, 0);
	if (fi != 0) conn->srcTimeStart = ntohll(*(uint64_t*)(data+fi->offset));
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowEndMilliSeconds, 0);
	if (fi != 0) conn->srcTimeEnd = ntohll(*(uint64_t*)(data+fi->offset));
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_octetDeltaCount, 0);
	if (fi != 0) conn->srcOctets = ntohll(*(uint64_t*)(data+fi->offset));
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_packetDeltaCount, 0);
	if (fi != 0) conn->srcPackets = ntohll(*(uint64_t*)(data+fi->offset));
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_tcpControlBits, 0);
	if (fi != 0) conn->srcTcpControlBits = *(uint8_t*)(data+fi->offset);
	fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_protocolIdentifier, 0);
	if (fi != 0) conn->protocol = *(uint8_t*)(data+fi->offset);

	hashtable.addFlow(conn);

	return 0;
}

/**
 * helper function to start thread
 */
void* IpfixConnectionTracker::threadExpireConns(void* connTracker)
{
	DPRINTF("starting thread");
	reinterpret_cast<IpfixConnectionTracker*>(connTracker)->expireConnectionsLoop();
	DPRINTF("thread stopped");
	return 0;
}

/**
 * expires connections in hashtable at regular intervals
 */
void IpfixConnectionTracker::expireConnectionsLoop()
{
	threadExited = false;

	queue<Connection*> expiredConns;
	while (!exitFlag) {
		sleep(1); // TODO: make it a variable!
		hashtable.expireConnections(&expiredConns);
		if (expiredConns.empty()) continue;
		//cout << "-------------------------" << endl;
		//cout << "expired connections: " << endl;
		while (!expiredConns.empty()) {
			Connection* c = expiredConns.front();
			expiredConns.pop();
			//cout << c->toString();
			
			c->addReference(receivers.size()-1);
			//cout << "receiver size: " << receivers.size() << endl;
			list<ConnectionReceiver*>::const_iterator iter = receivers.begin();
			while (iter != receivers.end()) {
				(*iter)->push(c);
				iter++;
			}
		}
	}
	threadExited = true;
}

void IpfixConnectionTracker::startThread()
{
	expireThread.run(this);
}

void IpfixConnectionTracker::stopThread()
{
	exitFlag = true;
	expireThread.join();
}

void IpfixConnectionTracker::addConnectionReceiver(ConnectionReceiver* cr)
{
	receivers.push_back(cr);
}



IpfixConnectionTracker::Hashtable::Hashtable(uint32_t expireTime)
	: expireTime(expireTime),
	  statTotalEntries(0), statExportedEntries(0)
{
	StatisticsManager::getInstance().addModule(this);
}

IpfixConnectionTracker::Hashtable::~Hashtable()
{
	// TODO: tidy up!
}

/**
 * tries to find connection inside list, if found, aggregate it
 * if connection was aggregated, instance is automatically dereferenced
 * ATTENTION: if flow was not found, it is _NOT_ added!
 * @param to flow has to be added in direction src->dst if true, else false
 * @returns true if flow was found, false if not
 */
bool IpfixConnectionTracker::Hashtable::aggregateFlow(Connection* c, list<Connection*>* clist, bool to)
{
	if (clist->empty()) return false;
	list<Connection*>::iterator iter = clist->begin();
	while (iter != clist->end()) {
		if (c->compareTo(*iter, to)) {
			(*iter)->aggregate(c, expireTime, to);
			c->removeReference();
			return true;
		}
		iter++;
	}
	return false;
}

/**
 * inserts flow to hashtable and does not check for duplicates
 */
void IpfixConnectionTracker::Hashtable::insertFlow(Connection* c)
{
	uint16_t hash = c->getHash(true);
	htable[hash].push_back(c);
}

/**
 * adds given flow to hashtable
 */
void IpfixConnectionTracker::Hashtable::addFlow(Connection* c)
{
	tableMutex.lock();

	// try to find connection entry in hashtable
	uint16_t hash = c->getHash(true);
	list<Connection*>* clist = &htable[hash];
	if (!aggregateFlow(c, clist, true)) {
		// not aggregated yet, try other direction
		hash = c->getHash(false);
		clist = &htable[hash];
		if (!aggregateFlow(c, clist, false)) {
			// we need to insert it into the hashtable
			insertFlow(c);
		}
	}

	tableMutex.unlock();
}

/**
 * look through hashtable and move connections which are expired to given parameter
 */
void IpfixConnectionTracker::Hashtable::expireConnections(queue<Connection*>* expiredFlows)
{
	uint32_t curtime = time(0);
	uint32_t numexported = 0;
	uint32_t numentries = 0;
	tableMutex.lock();

	// go through whole hashtable and try to find flows which are expired
	for (uint32_t i=0; i<TABLE_SIZE; i++) {
		list<Connection*>::iterator iter = htable[i].begin();
		while (iter != htable[i].end()) {
			numentries++;
			if ((*iter)->timeExpire<=curtime) {
				// expire flow and remove it from hashtable
				DPRINTF("expiring connection");
				numexported++;
				expiredFlows->push(*iter);
				list<Connection*>::iterator todelete = iter;
				iter++;
				htable[i].erase(todelete);
			} else {
				iter++;
			}
		}
	}

	tableMutex.unlock();
	statExportedEntries = numexported;
	statTotalEntries = numentries;
}

std::string IpfixConnectionTracker::Hashtable::getStatistics()
{
	ostringstream oss;
	oss << "Conntrack hashtable: number of entries: " << statTotalEntries << endl;
	oss << "Conntrack hashtable: exported entries : " << statExportedEntries << endl;
	return oss.str();
}
