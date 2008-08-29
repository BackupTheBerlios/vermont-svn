#include "PayloadFilter.h"


bool PayloadFilter::processPacket(Packet* p)
{
	if (p->payload) {
		// "drop" payload
		p->data_length = p->payload - p->data;
	}
	return true;
}
