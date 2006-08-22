#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <sampler/Template.h>
#include <sampler/PacketProcessor.h>
#include <sampler/Filter.h>


#include <vector>


class MeteringConfiguration : public Configuration {
public:
	MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MeteringConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);

protected:
	void setUp();
	void buildFilter();
	void buildTemplate();

private:
	class InfoElementId;
	class ReportedIE;


	void readPacketSelection(xmlNodePtr i);
	void readPacketReporting(xmlNodePtr i);
	void readFlowMetering(xmlNodePtr i);

	int interval;
	int spacing;
	int templateId;

	//std::vector<InfoElementId*> filters;
	std::vector<PacketProcessor*> filters;
	std::vector<ReportedIE*> exportedFields;

	Template* t;
	Filter* filter;
};

#endif
