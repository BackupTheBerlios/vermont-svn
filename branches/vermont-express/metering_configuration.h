/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "ipfix_configuration.h"

#include <concentrator/aggregator.h>
#include <flowcon/aggregator.h>

#include <vector>
#include <ctime>


class FlowMeteringConfiguration;
class ExpressFlowMeteringConfiguration;
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
	ExpressFlowMeteringConfiguration* getExpressFlowMeteringConfiguration() { return expressflowMetering; }
	PacketReportingConfiguration* getPacketReportingConfiguration() { return packetReporting; }
	PacketSelectionConfiguration* getPacketSelectionConfiguration();
private:
	PacketSelectionConfiguration* packetSelection;
	PacketReportingConfiguration* packetReporting;
	FlowMeteringConfiguration* flowMetering;
	ExpressFlowMeteringConfiguration* expressflowMetering;

	uint16_t observationDomainId;
};

#endif
