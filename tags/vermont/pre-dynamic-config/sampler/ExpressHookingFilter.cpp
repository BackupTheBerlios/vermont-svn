#define HOOK_SOURCE_ID 4913
/* just for abs */
#include <stdlib.h>
/* be sure we get the htonl et al inlined */
#include <netinet/in.h>

#include "ExpressHookingFilter.h"
#include "Packet.h"
#include "concentrator/ipfix.hpp"
#include "concentrator/IpfixAggregator.hpp"
#include "concentrator/IpfixParser.hpp"
#include "concentrator/IpfixPrinter.hpp"

#include "common/msg.h"


namespace {
	void setFieldInfo(IpfixRecord::FieldInfo* fi, IpfixRecord::FieldInfo::Type::Id id, IpfixRecord::FieldInfo::Type::Length length, uint16_t offset) {
		fi->type.id = id;
		fi->type.length = length;
		fi->type.isVariableLength = 0;
		fi->type.eid = 0;
		fi->offset = offset; 
	}
}

ExpressHookingFilter::ExpressHookingFilter(FlowSink *flowSink) : flowSink(flowSink) 
{
}

ExpressHookingFilter::~ExpressHookingFilter() 
{
}

/*
 this is the function called by sampler::HookingFilter
 the entrypoint into the concentrator
 this is a mess and not thread safe
 */
bool ExpressHookingFilter::processPacket(const Packet *p)
{

	/* we want only IPv4 packets  */
	if((p->classification & PCLASS_NET_IP4) == 0)
		return true;

	uint32_t pad1;
	uint8_t pad2;


	/* save IP header */
	pad1=((uint32_t *)p->netHeader)[1];
	pad2=((uint16_t *)p->netHeader)[5];
	((uint32_t *)p->netHeader)[1]=htonl((uint32_t)p->timestamp.tv_sec);
	((uint8_t *)p->netHeader)[10]=(uint8_t)1;

	flowSink->onPacket(p);


	/* restore IP header */
	((uint32_t *)p->netHeader)[1]=pad1;
	((uint8_t *)p->netHeader)[10]=pad2;




	return true;
}


