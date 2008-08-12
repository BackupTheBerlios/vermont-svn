#include "AnonFilter.h"
#include "Template.h"

#include <anon/AnonHashSha1.h>
#include <ipfixlolib/ipfix_names.h>

AnonFilter::AnonFilter()
{
}

AnonFilter::~AnonFilter()
{
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		delete i->second;
	}
}

AnonPrimitive* AnonFilter::createPrimitive(AnonMethods m, const std::string& paramter)
{
	AnonPrimitive* ret = 0;
	switch (m) {
	HashSha1:
		ret = new AnonHashSha1();
		break;
	ByteWiseHashHmacSha1:
	ByteWiseHashSha1:
	ConstOverwrite:
	ContinousChar:
	HashHmacSha1:
	Randomize:
	Shuffle:
	Whitenoise:
	default:
		msg(MSG_ERROR, "AnonPrimitve number %i is unknown", m);
	}

	return ret;
}

void AnonFilter::addAnonymization(uint16_t f, AnonMethods m, const std::string& parameter)
{
	AnonPrimitive* a = createPrimitive(m, parameter);
	methods[f] = a;
}

bool AnonFilter::processPacket(const Packet* p)
{
	static uint16_t offset;
	static unsigned short header;
	static unsigned long packetClass;
	static const struct ipfix_identifier* ident;

	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		if (!Template::getFieldOffsetAndHeader(i->first, &offset, &header, &packetClass)) {
			msg(MSG_ERROR, "Unkown or unsupported type id %i detected.", i->first);
			continue;
		}
		if (!(ident = ipfix_id_lookup(i->first))) {
			msg(MSG_ERROR, "Unknown or unsupported id %i detected.", i->first);
			continue;
		}

		switch (header) {
		case HEAD_NETWORK:
			i->second->anonimizeBuffer(p->netHeader + offset, ident->length);
			break;
		case HEAD_TRANSPORT:
			i->second->anonimizeBuffer(p->transportHeader + offset, ident->length);
			break;
		default:
			msg(MSG_ERROR, "Cannot deal with header type %i", header);
		}
	}
	return true;
}
