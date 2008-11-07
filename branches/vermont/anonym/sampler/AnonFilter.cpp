#include "AnonFilter.h"
#include "Template.h"

#include <anon/AnonModule.h>

#include <ipfixlolib/ipfix_names.h>

bool AnonFilter::processPacket(Packet* p)
{
	static uint16_t offset;
	static unsigned short header;
	static unsigned long packetClass;
	static const struct ipfix_identifier* ident;

	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		if (!Template::getFieldOffsetAndHeader(i->id, &offset, &header, &packetClass)) {
			msg(MSG_ERROR, "Unkown or unsupported type id %i detected.", i->id);
			continue;
		}

		int len = 0;
		if (i->len == -1) {
			if (!(ident = ipfix_id_lookup(i->id))) {
				msg(MSG_ERROR, "Unknown or unsupported id %i detected.", i->id);
				continue;
			}
			len = ident->length;
		} else 
			len = i->len;

		switch (header) {
		case HEAD_NETWORK:
			i->method->anonimizeBuffer(p->netHeader + offset, len);
			break;
		case HEAD_TRANSPORT:
			i->method->anonimizeBuffer(p->transportHeader + offset, len);
			break;
		default:
			msg(MSG_ERROR, "Cannot deal with header type %i", header);
		}
	}
	return true;
}
