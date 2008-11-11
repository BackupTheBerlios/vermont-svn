#ifndef BUCKET_H_
#define BUCKET_H_

#include "IpfixRecord.hpp"
#include "Element.h"

/**
 * Single Bucket containing one buffered flow's variable data.
 *Is either a direct entry in @c Hashtable::bucket or a member of another Hashtable::Bucket's spillchain
 */
class Bucket{
public:
	uint32_t expireTime; /**<timestamp when this bucket will expire if no new flows are added*/
	uint32_t forceExpireTime;/**<timestamp when this bucket is forced to expire */
	boost::shared_array<IpfixRecord::Data> data; /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo*/
	Bucket* prev; /**< previous bucket in spillchain*/
	Bucket* next; /**< next bucket in spillchain*/
	uint32_t observationDomainID;
	Element<Bucket*>* listNode;
	uint32_t hash;
	void initBucket(uint32_t hashnumber, Bucket* prevBuck, Bucket* nextBuck){hash = hashnumber; prev = prevBuck; next = nextBuck;};
};
#endif
