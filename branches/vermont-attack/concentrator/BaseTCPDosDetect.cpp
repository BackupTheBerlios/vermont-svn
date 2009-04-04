#include "BaseTCPDosDetect.h"

uint8_t BaseTCPDosDetect::idToMask(uint16_t field)
{
		switch(field)
		{
		case IPFIX_TYPEID_sourceIPv4Address: return 1;
		case IPFIX_TYPEID_destinationIPv4Address: return 2;
		case IPFIX_TYPEID_sourceTransportPort: return 4;
		case IPFIX_TYPEID_destinationTransportPort: return 8;
		default: return 0;
		}

}

BaseTCPDosDetect::BaseTCPDosDetect()
	{ 
		HashAttack = NULL;
		HashDefend = NULL;
	}
	
BaseTCPDosDetect::~BaseTCPDosDetect()
	{

	if (HashAttack)
	delete HashAttack;
	
	if (HashDefend)
	delete HashDefend;

	}
