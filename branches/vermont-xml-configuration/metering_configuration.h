#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <vector>


class MeteringConfiguration : public Configuration {
public:
	MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MeteringConfiguration();
	
	void configure();

private:
	class InfoElementId;
	class ReportedIE;


	void readPacketSelection(xmlNodePtr i);
	void readPacketReporting(xmlNodePtr i);
	void readFlowMetering(xmlNodePtr i);

	int interval;
	int spacing;

	std::vector<InfoElementId*> filters;
	std::vector<ReportedIE*> exportedFields;
};

#endif
