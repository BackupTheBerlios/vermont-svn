#ifndef _MAIN_CONFIGURATION_H_
#define _MAIN_CONFIGURATION_H_


#include "ipfix_configuration.h"

#include <string>


class MainConfiguration : public Configuration {
public:
	MainConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MainConfiguration();
		
	virtual void configure();
	virtual void startSystem();
	virtual void connect(Configuration*);

	unsigned getPollInterval() { return poll_interval; }
	
protected:
	virtual void setUp() {}

	unsigned poll_interval;
	std::string logfile;
	unsigned log_interval;
};

#endif
