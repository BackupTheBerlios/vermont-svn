#ifndef _VERMONT_CONFIGURATION_H_
#define _VERMONT_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


#include <string>
#include <stdexcept>
#include <vector>
#include <map>


class MeteringConfiguration;
class ObserverConfiguration;
class CollectorConfiguration;
class ExporterConfiguration;


namespace configTypes
{
	const std::string observer  = "observer";
	const std::string exporter  = "exporter";
	const std::string collector = "collector";
	const std::string metering  = "metering";
};


/**
 * base class for all subsystem configuration classes
 */
class Configuration {
public:
	Configuration(xmlDocPtr document, xmlNodePtr startNode) {
		start = startNode;
		doc = document;
	}

	virtual void configure() = 0;
	virtual void startSystem() = 0;

	/**
	 * connects c to this
	 */
	virtual void connect(Configuration* c) = 0;

	std::string getId() const{
		return id;
	}

	const std::vector<std::string>& getNextVector() { return nextVector; }

protected:
	xmlNodePtr start;
	xmlDocPtr doc;
	std::string id;

	std::vector<std::string> nextVector;
	std::string getContent(xmlNodePtr p) const; 
	void fillNextVector(xmlNodePtr p);
	unsigned getTimeInMsecs(xmlNodePtr i) const;

	virtual void setUp() = 0;
};


/**
 * holding the configuration data for vermont and it's subsystems.
 */
class VermontConfiguration {
public:
	VermontConfiguration(const std::string& configFile);
	~VermontConfiguration();
	
	void readSubsystemConfiguration();
	void readMainConfiguration();

	void connectSubsystems();
	void startSubsystems();
		
private:
	typedef std::map<std::string, Configuration*> SubsystemConfiguration;
	
	SubsystemConfiguration subsystems;

	xmlDocPtr document;
	xmlNodePtr current;
	
	// points to vermont specific configuration data
	xmlNodePtr vermontNode;
};


#endif
