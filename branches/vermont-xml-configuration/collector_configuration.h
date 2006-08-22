#ifndef _COLLECTOR_CONFIGURATION_H_
#define _COLLECTOR_CONFIGURATION_H_


#include "vermont_configuration.h"


class CollectorConfiguration : public Configuration{
public:
	CollectorConfiguration(xmlDocPtr document, xmlNodePtr startPoint);

	virtual void configure();
	virtual void connect(Configuration*);

protected:
	void setUp();
	
private:
	std::string ipAddress;
	unsigned protocolType;
	uint16_t port;
};

#endif
