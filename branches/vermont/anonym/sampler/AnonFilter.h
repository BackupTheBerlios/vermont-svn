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

	AnonFilter();
	~AnonFilter();

	void addAnonymization(uint16_t id, AnonMethods m, const std::string& parameter = "");

	virtual bool processPacket(const Packet* p);
protected:
	typedef std::map<uint16_t, AnonPrimitive*> MethodMap;
	MethodMap  methods;

private:
	AnonPrimitive* createPrimitive(AnonMethods m, const std::string& paramter);
};

#endif
