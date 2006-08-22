#ifndef _OBSERVER_CONFIGURATION_H_
#define _OBSERVER_CONFIGURATION_H_


#include "vermont_configuration.h"


#include "sampler/Observer.h"


#include <string>


class ObserverConfiguration : public Configuration {
public:
	ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~ObserverConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);

protected:
	void setUp();

private:
	unsigned int observationDomain;
	std::string type;
	std::string interface;
	std::string pcapFilter;
	int captureLength;
	// TODO:  needed because Observer needs char*
	char* pcapChar;

	void parseParameters(xmlNodePtr p);

	Observer* observer;
};

#endif
