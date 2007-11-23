#ifndef _CONNECTION_FILTER_H_
#define _CONNECTION_FILTER_H_

#include "PacketProcessor.h"

#include <common/CountBloomFilter.h>
#include <common/AgeBloomFilter.h>

class ConnectionFilter : public PacketProcessor {
public:
	ConnectionFilter();

	virtual bool processPacket(const Packet* p);
protected:
	AgeBloomFilter synFilter;
	AgeBloomFilter connectionFilter;
	CountBloomFilter exportFilter;
};

#endif
