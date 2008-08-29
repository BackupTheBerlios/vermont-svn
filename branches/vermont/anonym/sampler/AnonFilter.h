#ifndef _ANON_FILTER_H_
#define _ANON_FILTER_H_

#include "PacketProcessor.h"

#include <common/CountBloomFilter.h>
#include <common/AgeBloomFilter.h>

#include <anon/AnonPrimitive.h>

#include <map>

class AnonMethod 
{
public:
	typedef enum {
		BytewiseHashHmacSha1,
		BytewiseHashSha1,
		ConstOverwrite,
		ContinuousChar,
		HashHmacSha1,
		HashSha1,
		Randomize,
		Shuffle,
		Whitenoise
	} Method;

	static Method stringToMethod(const std::string& m)
	{
		if (m == "BytewiseHashHmacSha1") {
			return BytewiseHashHmacSha1;
		} else if (m == "BytewiseHashSha1") {
                        return BytewiseHashSha1;
                } else if (m == "ConstOverwrite") {
                        return ConstOverwrite;
                } else if (m == "ContinuousChar") {
                        return ContinuousChar;
                }else if (m == "HashHmacSha1") {
                        return HashHmacSha1;
                }else if (m == "HashSha1") {
                        return HashSha1;
                }else if (m == "Randomize") {
                        return Randomize;
                }else if (m == "Shuffle") {
                        return Shuffle;
                }else if (m == "Whitenoise") {
                        return Whitenoise;
                }
		THROWEXCEPTION("Unknown anonymization method");

		// make compile happy
		return BytewiseHashHmacSha1;
	}
};

class AnonFilter : public PacketProcessor {
public:
	AnonFilter();
	~AnonFilter();

	void addAnonymization(uint16_t id, AnonMethod::Method m, const std::string& parameter = "");

	virtual bool processPacket(Packet* p);
protected:
	typedef std::map<uint16_t, AnonPrimitive*> MethodMap;
	MethodMap  methods;

private:
	AnonPrimitive* createPrimitive(AnonMethod::Method m, const std::string& paramter);
};

#endif
