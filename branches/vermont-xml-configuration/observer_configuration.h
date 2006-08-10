#ifndef _OBSERVER_CONFIGURATION_H_
#define _OBSERVER_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <string>


class ObserverConfiguration : public Configuration {
public:
	ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	
	void configure();

private:
	unsigned int observationDomain;
	std::string type;
	std::string interface;
	std::string filter;
	int captureLength;

	void parseParameters(xmlNodePtr p);
};

#endif
