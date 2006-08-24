#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <concentrator/aggregator.h>


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
	IpfixAggregator* getAggregator() const;
	
	bool isSampling() const { return sampling; }
	bool isAggregating() const { return aggregating; }
	
	void pollAggregator();
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
	IpfixAggregator* ipfixAggregator;

	bool sampling;
	bool aggregating;
	
	int observationId;
	bool observationIdSet;
};

#endif
