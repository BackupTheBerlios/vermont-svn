#ifndef _VERMONT_CONFIGURATION_H_
#define _VERMONT_CONFIGURATION_H_


#include <string>
#include <stdexcept>
#include <vector>


class ConcentratorConfiguration;
class ObserverConfiguration;
class CollectorConfiguration;
class ExporterConfiguration;


/**
 * holding the configuration data for vermont and it's subsystems.
 */
class VermontConfiguration {
public:
	VermontConfiguration(const std::string& configFile) { throw std::runtime_error("not yet implemented");}
	~VermontConfiguration() { }
	
	void configureObservers() {}
	void configureCollectors() {}
	void configureConcentrators() {}
	void configureExporters() {}
	void configureLogging() {}
	void configureHooking() {}

	void connectSubsystems() {}
	void startSubsystems() {}
		
private:
	std::vector<ObserverConfiguration*> samplerConfigurations;
	std::vector<CollectorConfiguration*> collectorConfigurations;
	std::vector<ConcentratorConfiguration*> concentratorConfigurations;
	std::vector<ExporterConfiguration*> exporterConfigurations;
};


#endif
