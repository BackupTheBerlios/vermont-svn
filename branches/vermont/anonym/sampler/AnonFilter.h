#ifndef _ANON_FILTER_H_
#define _ANON_FILTER_H_

#include "PacketProcessor.h"

#include <common/CountBloomFilter.h>
#include <common/AgeBloomFilter.h>

#include <anon/AnonPrimitive.h>

#include <map>

class AnonFilter : public PacketProcessor {
public:
	typedef enum {
		ByteWiseHashHmacSha1,
		ByteWiseHashSha1,
		ConstOverwrite,
		ContinousChar,
		HashHmacSha1,
		HashSha1,
		Randomize,
		Shuffle,
		Whitenoise
	} AnonMethods;

	typedef enum {
		SrcIpv4 = 16,
		DstIpv4 = 20
	} AnonFields;

	AnonFilter();

	void addAnonymization(AnonFields f, AnonMethods m, const std::string& parameter = "");

	virtual bool processPacket(const Packet* p);
protected:
	std::map<AnonFields, AnonPrimitive*> methods;
};

#endif
