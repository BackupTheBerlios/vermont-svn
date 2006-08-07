#include "vermont_configuration.h"
#include "observer_configuration.h"
#include "metering_configuration.h"
#include "collector_configuration.h"
#include "exporter_configuration.h"


VermontConfiguration::VermontConfiguration(const std::string& configFile)
{
	document = xmlParseFile(configFile.c_str());
	if (NULL == document) {
		throw std::runtime_error("Could not parse " + configFile + "!");
	}
	current = xmlDocGetRootElement(document);
	if (NULL == document) {
		throw std::runtime_error(configFile + " is an empty XML-Document!");
	}

	if (xmlStrcmp(current->name, (const xmlChar *) "ipfixConfig")) {
		xmlFreeDoc(document);
		throw std::runtime_error("Root element does not match \"ipfixConfig\"."
					 " This is not a valid configuration file!");
	}

	current = current->xmlChildrenNode;
	while (current != NULL) {
		if (!xmlStrcmp(current->name, (const xmlChar*)"vermont")) {
			vermontNode = current;
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"observationPoint")) {
			ObserverConfiguration* oConf = new ObserverConfiguration(document, current);
			observerConfigurations.push_back(oConf);
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"meteringProcess")) {
			MeteringConfiguration* mConf = new MeteringConfiguration(current);
			meteringConfigurations.push_back(mConf);
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"exportingProcess")) {
			ExporterConfiguration* eConf = new ExporterConfiguration(current);
			exporterConfigurations.push_back(eConf);
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"collectingProcess")) {
			CollectorConfiguration* cConf = new CollectorConfiguration(current);
			collectorConfigurations.push_back(cConf);
		}

		current = current->next;
	}
}

VermontConfiguration::~VermontConfiguration()
{
	for (unsigned i = 0; i != observerConfigurations.size(); ++i) {
		delete observerConfigurations[i];
	}
	for (unsigned i = 0; i != meteringConfigurations.size(); ++i) {
		delete meteringConfigurations[i];
	}
	for (unsigned i = 0; i != exporterConfigurations.size(); ++i) {
		delete exporterConfigurations[i];
	}
	for (unsigned i = 0; i != collectorConfigurations.size(); ++i) {
		delete collectorConfigurations[i];
	}
	xmlFreeDoc(document);
}

void VermontConfiguration::configureMainSystem()
{
	
}

void VermontConfiguration::configureObservers()
{
	throw std::runtime_error("not yet implemented");
}

void VermontConfiguration::configureCollectors()
{
	throw std::runtime_error("not yet implemented");
}

void VermontConfiguration::configureMeteringProcesses()
{
	throw std::runtime_error("not yet implemented");
}


void VermontConfiguration::configureExporters()
{
	throw std::runtime_error("not yet implemented");
}

void VermontConfiguration::connectSubsystems()
{
	throw std::runtime_error("not yet implemented");
}

void VermontConfiguration::startSubsystems()
{
	throw std::runtime_error("not yet implemented");
}

