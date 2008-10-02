#ifndef BUCKET_H_
#define BUCKET_H_

#include "IpfixRecord.hpp"
#include "Element.h"


class Bucket{
public:
	uint32_t expireTime;
	uint32_t forceExpireTime;
	boost::shared_array<IpfixRecord::Data> data;
	Bucket* prev;
	Bucket* next;
	uint32_t observationDomainID;
	Element<Bucket*>* listNode;
	uint32_t hash;
};
#endif
