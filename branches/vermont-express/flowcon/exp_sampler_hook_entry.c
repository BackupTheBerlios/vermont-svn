/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert
 */

#define HOOK_SOURCE_ID 4913
/* just for abs */
#include <stdlib.h>
/* be sure we get the htonl et al inlined */
#include <netinet/in.h>

#include "exp_aggregator.h"
#include "exp_rcvIpfix.h"

#include "sampler/packet_hook.h"
#include "exp_sampler_hook_entry.h"

#include "exp_ipfix.h"
#include "exp_printIpfix.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 this is the function called by sampler::HookingFilter
 the entrypoint into the concentrator
 this is a mess and not thread safe
 */

void Express_sampler_hook_entry(void *ctx, void *data)
{
	int transport_offset;
	struct packet_hook *ph=(struct packet_hook *)data;
        void *aggregator=ctx;
	FieldData *fdata=(FieldData *)ph->ip_header;
	uint32_t pad1;
	uint8_t pad2;
	
	//DPRINTF("hook_entry: length is %d\n", ph->length);

	/* save IP header */
	pad1=((uint32_t *)ph->ip_header)[1];
	pad2=((uint16_t *)ph->ip_header)[5];
	((uint32_t *)ph->ip_header)[1]=htonl((uint32_t)ph->timestamp->tv_sec);
	((uint8_t *)ph->ip_header)[10]=(uint8_t)1;

	// Check if transport header is available
	if(ph->transport_header == NULL) {
	    ExpressaggregateDataRecord(aggregator, NULL, ph->length, fdata, 0);
	}
	else
	{
	    transport_offset=abs(ph->transport_header - ph->ip_header);
	    ExpressaggregateDataRecord(aggregator, NULL, ph->length, fdata, transport_offset);
	}

	/* restore IP header */
	((uint32_t *)ph->ip_header)[1]=pad1;
	((uint8_t *)ph->ip_header)[10]=pad2;

}

#ifdef __cplusplus
}
#endif
