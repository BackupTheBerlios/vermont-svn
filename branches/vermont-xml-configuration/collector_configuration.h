#ifndef _COLLECTOR_CONFIGURATION_H_
#define _COLLECTOR_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <concentrator/rcvIpfix.h>



class CollectorConfiguration : public Configuration{
public:
	CollectorConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~CollectorConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	
protected:
	void setUp();
	void readListener(xmlNodePtr i);
	
private:
	std::string ipAddress;
	unsigned protocolType;
	uint16_t port;
	
	IpfixCollector* ipfixCollector;
	IpfixPacketProcessor* ipfixPacketProcessor;
	IpfixParser* ipfixParser;
	bool hasCollector;
};

#endif
