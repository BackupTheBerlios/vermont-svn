#include "vermont_configuration.h"
#include "observer_configuration.h"
#include "metering_configuration.h"
#include "collector_configuration.h"
#include "exporter_configuration.h"
#include "main_configuration.h"

std::string getContent(xmlDocPtr doc, xmlNodePtr p)
{
	xmlChar* v = xmlNodeListGetString(doc, p->xmlChildrenNode, 1);
	std::string ret = (const char*) v;
	xmlFree(v);
	return ret;
}

bool xmlCompare(const xmlNodePtr node, const std::string& tagName)
{
	return !xmlStrcmp(node->name, (const xmlChar*)tagName.c_str());
}

/*********************************** Configuration *****************************/

std::string Configuration::getContent(xmlNodePtr p) const
{
	return ::getContent(doc, p);
}

bool Configuration::tagMatches(const xmlNodePtr node, const std::string& tagName) const
{
	return xmlCompare(node, tagName);
}


void Configuration::fillNextVector(xmlNodePtr p)
{
	xmlNodePtr j = p->xmlChildrenNode;
	while (NULL != j) {
		if (tagMatches(j, "meteringProcessId")) {
			nextVector.push_back(configTypes::metering + 
					     getContent(j));
		} else if (tagMatches(j, "exportingProcessId")) {
			nextVector.push_back(configTypes::exporter +
					     getContent(j));
		}
		j = j->next;
	}

}

unsigned Configuration::getTimeInUsecs(xmlNodePtr i) const
{
	unsigned ret = 0;
	xmlChar* unit = xmlGetProp(i, (const xmlChar*)"unit");
	if (!xmlStrcmp(unit, (const xmlChar*)"sec")) {
		ret = (unsigned)atoi(getContent(i).c_str()) * 1000000;
	} else if (!xmlStrcmp(unit, (const xmlChar*)"msec")) {
		ret = (unsigned)atoi(getContent(i).c_str()) * 1000;
	} else if (!xmlStrcmp(unit, (const xmlChar*)"usec")) {
		ret = (unsigned)atoi(getContent(i).c_str());
	}
	xmlFree(unit);
	return ret;	
}

unsigned Configuration::getTimeInMsecs(xmlNodePtr i) const
{
	return getTimeInUsecs(i) / 1000;
}

unsigned Configuration::getTimeInSecs(xmlNodePtr i) const
{
	return getTimeInUsecs(i) / 10000000;
}

/****************************** VermontConfiguration ***************************/


VermontConfiguration::VermontConfiguration(const std::string& configFile)
	: stop(false)
{
	document = xmlParseFile(configFile.c_str());
	if (!document) {
		throw std::runtime_error("Could not parse " + configFile + "!");
	}
	current = xmlDocGetRootElement(document);
	if (!current) {
		throw std::runtime_error(configFile + " is an empty XML-Document!");
	}

	if (!xmlCompare(current, "ipfixConfig")) {
		xmlFreeDoc(document);
		throw std::runtime_error("Root element does not match \"ipfixConfig\"."
					 " This is not a valid configuration file!");
	}

	current = current->xmlChildrenNode;
	while (current != NULL) {
		Configuration* conf = 0;

		if (xmlCompare(current, "vermont")) {
			conf = new MainConfiguration(document, current);
		} else if (xmlCompare(current, "observationPoint")) {
			conf = new ObserverConfiguration(document, current);
		} else if (xmlCompare(current, "meteringProcess")) {
			conf = new MeteringConfiguration(document, current);
		} else if (xmlCompare(current, "exportingProcess")) {
			conf = new ExporterConfiguration(document, current);
		} else if (xmlCompare(current, "collectingProcess")) {
			conf = new CollectorConfiguration(document, current);
		}
		if (conf) {
			subsystems[conf->getId()] = conf;
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

void VermontConfiguration::pollAggregatorLoop()
{
	while (!stop) {
		for (SubsystemConfiguration::iterator i = subsystems.begin();
		     i != subsystems.end(); ++i) {
			MeteringConfiguration* m = dynamic_cast<MeteringConfiguration*>(i->second);
			if (m) {
				m->pollAggregator();
			}
		}
	}
}

