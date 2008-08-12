#include "AnonFilter.h"

#include <anon/AnonBytewiseHashSha1.h>

AnonFilter::AnonFilter()
{
}

void AnonFilter::addAnonymization(AnonFields f, AnonMethods m, const std::string& parameter)
{
}

bool AnonFilter::processPacket(const Packet* p)
{
	// anonymize only the ip adresses with sha1
	static AnonBytewiseHashSha1 sha1; 

	msg(MSG_INFO, "jaskfjdlas;f;adfsd");
	
	sha1.anonimizeBuffer(p->netHeader + 16, 4);
	sha1.anonimizeBuffer(p->netHeader + 20, 4);
	return true;
}
