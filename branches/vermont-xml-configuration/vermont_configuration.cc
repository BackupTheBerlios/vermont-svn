#include "vermont_configuration.h"
#include "observer_configuration.h"
#include "metering_configuration.h"
#include "collector_configuration.h"
#include "exporter_configuration.h"

/*********************************** Configuration *****************************/

std::string Configuration::getContent(xmlNodePtr p) const
{
	xmlChar* v = xmlNodeListGetString(doc, p->xmlChildrenNode, 1);
	std::string ret = (const char*) v;
	xmlFree(v);
	return ret;
}


void Configuration::fillNextVector(xmlNodePtr p)
{
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

unsigned Configuration::getTimeInMsecs(xmlNodePtr i) const
{
	unsigned ret = 0;
	xmlChar* unit = xmlGetProp(i, (const xmlChar*)"unit");
	if (!xmlStrcmp(unit, (const xmlChar*)"sec")) {
		ret = (unsigned)atoi(getContent(i).c_str()) * 1000;
	} else if (!xmlStrcmp(unit, (const xmlChar*)"msec")) {
		ret = (unsigned)atoi(getContent(i).c_str());
	} else if (!xmlStrcmp(unit, (const xmlChar*)"usec")) {
		ret = (unsigned)atoi(getContent(i).c_str()) / 1000;
	}
	xmlFree(unit);
	return ret;
}

/****************************** VermontConfiguration ***************************/


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
			subsystems[oConf->getId()] = oConf;
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"meteringProcess")) {
			MeteringConfiguration* mConf = new MeteringConfiguration(document, current);
			subsystems[mConf->getId()] = mConf;
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"exportingProcess")) {
			ExporterConfiguration* eConf = new ExporterConfiguration(document, current);
			subsystems[eConf->getId()] = eConf;
		} else if (!xmlStrcmp(current->name, (const xmlChar*)"collectingProcess")) {
			CollectorConfiguration* cConf = new CollectorConfiguration(document, current);
			subsystems[cConf->getId()] = cConf;
		}

		current = current->next;
	}
}

VermontConfiguration::~VermontConfiguration()
{
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		delete i->second;
	}
	xmlFreeDoc(document);
}

void VermontConfiguration::readMainConfiguration()
{
	
}

void VermontConfiguration::readSubsystemConfiguration()
{
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		i->second->configure();
	}
}

void VermontConfiguration::connectSubsystems()
{

	// sequence is important!!!
	// 1.) connect observers
	// 2.) connect exporters
	// 3.) connect collectors
	// 4.) connect metering processes
	// TODO: this is ugly!
	std::string TYPES[] = {
		configTypes::observer,
		configTypes::exporter,
		configTypes::collector,
		configTypes::metering,
	};
	// TODO: this is inefficient
	for (unsigned t = 0; t != 4; ++t) {
		for (SubsystemConfiguration::iterator i = subsystems.begin();
	    	 i != subsystems.end(); ++i) {	
			std::string id = i->first;
			if (id.find(TYPES[t])) {
				continue;
			}
			Configuration* c = i->second;
			const std::vector<std::string>& nextVector = c->getNextVector();
			for (unsigned j = 0; j != nextVector.size(); ++j) {
				if (subsystems.find(nextVector[j]) == subsystems.end()) {
					throw std::runtime_error("Could not find " + nextVector[j] + " in subsystem list");
				}
				if (!c) msg(MSG_ERROR, "c is null");
				if (!subsystems[nextVector[j]]) msg(MSG_ERROR, "subsystems[nextVector[j]] is null!");
				msg(MSG_DEBUG, "VermontConfiguration: connecting %s to %s", c->getId().c_str(), subsystems[nextVector[j]]->getId().c_str()); 
				c->connect(subsystems[nextVector[j]]);
				msg(MSG_DEBUG, "VermontConfiguration: successfully connected %s to %s", c->getId().c_str(), subsystems[nextVector[j]]->getId().c_str());
			}
		}
	}
}

void VermontConfiguration::startSubsystems()
{
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		i->second->startSystem();
	}
}

void VermontConfiguration::pollAggregators()
{
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		MeteringConfiguration* m = dynamic_cast<MeteringConfiguration*>(i->second);
		if (m) {
			m->pollAggregator();
		}
	}
}

