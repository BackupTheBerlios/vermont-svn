#ifndef _MAIN_CONFIGURATION_H_
#define _MAIN_CONFIGURATION_H_


#include "vermont_configuration.h"


class MainConfiguration : public Configuration {
public:
	MainConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MainConfiguration();
		
	virtual void configure();
	virtual void startSystem() {}
	virtual void connect(Configuration*) {}
	
protected:
	virtual void setUp() {}		
};

#endif
