#ifndef _VERMONT_CONFIGURATION_H_
#define _VERMONT_CONFIGURATION_H_


#include <string>
#include <stdexcept>


class ConcentratorConfiguration;
class SamplerConfiguration;


/**
 * holding the configuration data for vermont and it's subsystems.
 */
class VermontConfiguration {
public:
	VermontConfiguration(const std::string& configFile) { throw std::runtime_error("not yet implemented");}
	~VermontConfiguration() { }
	
	void configureSampler() {}
	void configureConcentrator() {}
	void configureLogging() {}
	void configureHooking() {}
		
private:
	SamplerConfiguration* samplerConfiguration;
	ConcentratorConfiguration* concentratorConfiguration;
};


#endif
