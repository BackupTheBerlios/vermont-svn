#ifdef HAVE_GSL

#ifndef _CONNECTION_FILTER_H_
#define _CONNECTION_FILTER_H_

#include "PacketProcessor.h"

#include <common/CountBloomFilter.h>
#include <common/AgeBloomFilter.h>

class ConnectionFilter : public PacketProcessor {
public:
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize);

	virtual bool processPacket(const Packet* p);
protected:
	AgeBloomFilter synFilter;
	CountBloomFilter exportFilter;
	AgeBloomFilter connectionFilter;
	unsigned timeout;
	unsigned exportBytes;
};

#endif

#endif
