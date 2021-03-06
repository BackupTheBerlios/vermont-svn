/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/

#ifndef   	FLOWMETERING_CONFIGURATION_H_
#define   	FLOWMETERING_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <concentrator/aggregator.h>
#include <concentrator/rules.h>


class MeteringConfiguration;


class FlowMeteringConfiguration : public Configuration {
public:
	FlowMeteringConfiguration(xmlDocPtr doc, xmlNodePtr start);
	~FlowMeteringConfiguration();

	virtual void configure();
	virtual void setUp();
	virtual void connect(Configuration*);
	virtual void startSystem();
	IpfixAggregator* getIpfixAggregator() { return ipfixAggregator; }

protected:
	Rule* readRule(xmlNodePtr i);
	IpfixAggregator* ipfixAggregator;

	friend class MeteringConfiguration;
};

#endif 	    /* !FLOWMETERING_CONFIGURATION_H_ */
