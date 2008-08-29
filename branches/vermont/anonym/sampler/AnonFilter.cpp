#include "AnonFilter.h"
#include "Template.h"

#include <anon/AnonHashSha1.h>
#include <anon/AnonBytewiseHashHmacSha1.h>
#include <anon/AnonBytewiseHashSha1.h>
#include <anon/AnonConstOverwrite.h>
#include <anon/AnonContinuousChar.h>
#include <anon/AnonWhitenoise.h>
#include <anon/AnonHashHmacSha1.h>
#include <anon/AnonRandomize.h>
#include <anon/AnonShuffle.h>

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

AnonPrimitive* AnonFilter::createPrimitive(AnonMethod::Method m, const std::string& parameter)
{
	AnonPrimitive* ret = 0;
	switch (m) {
	case AnonMethod::HashSha1:
		ret = new AnonHashSha1();
		break;
	case AnonMethod::BytewiseHashSha1:
		ret = new AnonBytewiseHashSha1();
		break;
	case AnonMethod::ContinuousChar:
		ret = new AnonContinuousChar();
		break;
	case AnonMethod::Randomize:
		ret = new AnonRandomize();
		break;
	case AnonMethod::Shuffle:
		ret = new AnonShuffle();
		break;
	case AnonMethod::Whitenoise:
		ret = new AnonWhitenoise(atoi(parameter.c_str()));
		break;
	case AnonMethod::HashHmacSha1:
		ret = new AnonHashHmacSha1(parameter);
		break;
	case AnonMethod::BytewiseHashHmacSha1:
		ret = new AnonBytewiseHashHmacSha1(parameter);
		break;
	case AnonMethod::ConstOverwrite:
		if (parameter.size() != 1) {
			THROWEXCEPTION("AnonConstOverwrite only uses one character as key");
		}
		ret = new AnonConstOverwrite(parameter.c_str()[0]);
	default:
		msg(MSG_FATAL, "AnonPrimitive number %i is unknown", m);
		THROWEXCEPTION("AnonPrimitive number %i is unknown", m);
	}

	return ret;
}

void AnonFilter::addAnonymization(uint16_t f, AnonMethod::Method  m, const std::string& parameter)
{
	AnonPrimitive* a = createPrimitive(m, parameter);
	methods[f] = a;
}

bool AnonFilter::processPacket(Packet* p)
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
