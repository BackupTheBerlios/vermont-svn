#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "vermont_configuration.h"

#include <concentrator/aggregator.h>

#include <vector>
#include <ctime>


class FlowMeteringConfiguration;
class PacketSelectionConfiguration;
class PacketReportingConfiguration;

class MeteringConfiguration : public Configuration {
public:
	MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MeteringConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();

	void setObservationDomainId(uint16_t id);

	FlowMeteringConfiguration* getFlowMeteringConfiguration() { return flowMetering; }
	PacketReportingConfiguration* getPacketReportingConfiguration() { return packetReporting; }
	PacketSelectionConfiguration* getPacketSelectionConfiguration();
private:
	PacketSelectionConfiguration* packetSelection;
	PacketReportingConfiguration* packetReporting;
	FlowMeteringConfiguration* flowMetering;
	bool gotSection;

	uint16_t observationDomainId;
};

#endif
