#ifndef _VERMONT_CONFIGURATION_H_
#define _VERMONT_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


#include <string>
#include <stdexcept>
#include <vector>


class MeteringConfiguration;
class ObserverConfiguration;
class CollectorConfiguration;
class ExporterConfiguration;


/**
 * holding the configuration data for vermont and it's subsystems.
 */
class VermontConfiguration {
public:
	VermontConfiguration(const std::string& configFile);
	~VermontConfiguration();
	
	void configureObservers();
	void configureCollectors();
	void configureConcentrators();
	void configureExporters();

	void configureMainSystem();

	void connectSubsystems();
	void startSubsystems();
		
private:
	std::vector<ObserverConfiguration*> observerConfigurations;
	std::vector<CollectorConfiguration*> collectorConfigurations;
	std::vector<MeteringConfiguration*> meteringConfigurations;
	std::vector<ExporterConfiguration*> exporterConfigurations;

	xmlDocPtr document;
	xmlNodePtr current;
	
	// points to vermont specific configuration data
	xmlNodePtr vermontNode;
};


#endif
