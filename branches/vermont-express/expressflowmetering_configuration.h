/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/

#ifndef   	EXPRESSFLOWMETERING_CONFIGURATION_H_
#define   	EXPRESSFLOWMETERING_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <flowcon/exp_aggregator.h>
#include <flowcon/exp_rules.h>


class MeteringConfiguration;


class ExpressFlowMeteringConfiguration : public Configuration {
public:
	ExpressFlowMeteringConfiguration(xmlDocPtr doc, xmlNodePtr start);
	~ExpressFlowMeteringConfiguration();

	virtual void configure();
	virtual void setUp();
	virtual void connect(Configuration*);
	virtual void startSystem();
	IpfixExpressAggregator* getIpfixExpressAggregator() { return ipfixExpressAggregator; }

protected:
	ExpressRule* readExpressRule(xmlNodePtr i);
	IpfixExpressAggregator* ipfixExpressAggregator;

	friend class MeteringConfiguration;
};

#endif 	    /* !EXPRESSFLOWMETERING_CONFIGURATION_H_ */
