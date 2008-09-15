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
#include <anon/AnonCryptoPan.h>

#include <ipfixlolib/ipfix_names.h>

AnonFilter::AnonFilter()
{
}

AnonFilter::~AnonFilter()
{
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		delete i->method;
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
		break;
	case AnonMethod::CryptoPan:
		ret = new AnonCryptoPan(parameter);
		break;
	default:
		msg(MSG_FATAL, "AnonPrimitive number %i is unknown", m);
		THROWEXCEPTION("AnonPrimitive number %i is unknown", m);
	}

	return ret;
}

void AnonFilter::addAnonymization(uint16_t id, int len,  AnonMethod::Method  methodName, const std::string& parameter)
{
	AnonPrimitive* a = createPrimitive(methodName, parameter);
	AnonIE ie;
	ie.id = id;
	ie.len = len;
	ie.method = a;
	methods.push_back(ie);
}

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
