#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <vector>


class Filter;
class PacketProcessor;
class Template;


class MeteringConfiguration : public Configuration {
public:
	MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MeteringConfiguration();
	
	virtual void configure();
	virtual void setUp();
	virtual void connect(Configuration*);
	virtual void startSystem();
	
	void setObservationId(uint16_t id);

	Filter* getFilters() const { return filter; }
	
	bool isSampling() const { return sampling; }
	bool isAggregating() const { return aggregating; }
protected:
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

	bool sampling;
	bool aggregating;
	
	int observationId;
	bool observationIdSet;
};

#endif
