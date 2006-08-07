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

	std::string getId() {
		return id;
	}

protected:
	xmlNodePtr start;
	xmlDocPtr doc;
	std::string id;

	std::vector<std::string> nextVector;

	std::string getContent(xmlNodePtr p) {
		xmlChar* v = xmlNodeListGetString(doc, p->xmlChildrenNode, 1);
		std::string ret = (const char*) v;
		xmlFree(v);
		return ret;
	}

	void fillNextVector(xmlNodePtr p) {
		xmlNodePtr j = p->xmlChildrenNode;
		while (NULL != j) {
			if (!xmlStrcmp(j->name, (const xmlChar*)"meteringProcessId")) {
				nextVector.push_back(configTypes::metering + 
						     getContent(j));
			} else if (!xmlStrcmp(j->name, (const xmlChar*)"exportingProcessId")) {
				nextVector.push_back(configTypes::exporter +
						     getContent(j));
			}
			j = j->next;
		}

	}
};


/**
 * holding the configuration data for vermont and it's subsystems.
 */
class VermontConfiguration {
public:
	VermontConfiguration(const std::string& configFile);
	~VermontConfiguration();
	
	void configureObservers();
	void configureCollectors();
	void configureMeteringProcesses();
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
