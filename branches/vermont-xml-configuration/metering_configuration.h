#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <vector>


class ExporterSink;
class Filter;
class PacketProcessor;
class Template;


class MeteringConfiguration : public Configuration {
public:
	MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MeteringConfiguration();
	
	virtual void configure();
	virtual void setUp();
	virtual void connect(const Configuration*);

	bool isSampling() const { return sampling; }
	bool isAggregating() const { return aggregating; }

	ExporterSink* getExporterSink() const;
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
	ExporterSink* exporterSink;

	bool sampling;
	bool aggregating;
};

#endif
