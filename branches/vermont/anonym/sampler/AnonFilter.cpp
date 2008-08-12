#include "AnonFilter.h"

#include <anon/AnonHashSha1.h>

AnonFilter::AnonFilter()
{
}

void AnonFilter::addAnonymization(AnonFields f, AnonMethods m, const std::string& parameter)
{
}

bool AnonFilter::processPacket(const Packet* p)
{
	// anonymize only the ip adresses with sha1
	static AnonHashSha1 sha1; 

	sha1.anonimizeBuffer(p->netHeader + 12, 4);
	sha1.anonimizeBuffer(p->netHeader + 16, 4);

	return true;
}
