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

uint32_t BaseTCPDosDetect::getDosTemplateId() {
	return dosTemplateId;

}
void BaseTCPDosDetect::setCycle(int clength)
{
		cycle = clength;
}

BaseTCPDosDetect::BaseTCPDosDetect(int dosTemplateId,int minimumRate,int clusterTimeout,std::map<uint32_t,uint32_t> subnets)
	{ 
		this->dosTemplateId = dosTemplateId;
		this->minimumRate = minimumRate;
		internals = subnets;
		clusterLifeTime = clusterTimeout;
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
