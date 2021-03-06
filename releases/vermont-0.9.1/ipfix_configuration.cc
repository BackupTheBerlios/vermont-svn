#include "ipfix_configuration.h"
#include "observer_configuration.h"
#include "metering_configuration.h"
#include "collector_configuration.h"
#include "exporter_configuration.h"
#include "flowmetering_configuration.h"
#include "vermontmain_configuration.h"
#include "dbwriter_configuration.h"
#include "dbreader_configuration.h"


#include <ctime>

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
		} else if (tagMatches(j, "dbWriterId")) {
			nextVector.push_back(configTypes::dbwriter +
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


/****************************** IpfixConfiguration ***************************/


IpfixConfiguration::IpfixConfiguration(const std::string& configFile)
	: stop(false), isAggregating(false)
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

		if (xmlCompare(current, "vermont_main")) {
			conf = new VermontMainConfiguration(document, current);
		} else if (xmlCompare(current, "observationPoint")) {
			conf = new ObserverConfiguration(document, current);
		} else if (xmlCompare(current, "meteringProcess")) {
			conf = new MeteringConfiguration(document, current);
		} else if (xmlCompare(current, "exportingProcess")) {
			conf = new ExporterConfiguration(document, current);
		} else if (xmlCompare(current, "collectingProcess")) {
			conf = new CollectorConfiguration(document, current);
		} else if (xmlCompare(current, "dbWriter")) {
#ifdef DB_SUPPORT_ENABLED
			conf = new DbWriterConfiguration(document, current);
#else
			msg(MSG_ERROR, "IpfixConfiguration: Vermont was compiled without "
				       "support for dbWriter. Ignoring entry in config file!");
#endif
		} else if (xmlCompare(current, "dbReader")) {
#ifdef DB_SUPPORT_ENABLED
			conf = new DbReaderConfiguration(document, current);
#else
			msg(MSG_ERROR, "IpfixConfiguration: Vermont was compiled without "
				       "support for dbReader. Ignoring entry in conifg file!");
#endif
		}
		if (conf) {
			subsystems[conf->getId()] = conf;
		}
		current = current->next;
	}
}

IpfixConfiguration::~IpfixConfiguration()
{
	msg(MSG_INFO, "IpfixConfiguration: Cleaning up");
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		std::string id = i->second->getId();
		delete i->second;
	}
	xmlFreeDoc(document);
}

void IpfixConfiguration::readSubsystemConfiguration()
{
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		i->second->configure();
	}
}

void IpfixConfiguration::connectSubsystems()
{
	msg(MSG_INFO, "IpfixConfiguration: Connecting subsystems...");

	std::string TYPES[] = {
		configTypes::observer,
		configTypes::exporter,
		configTypes::dbwriter,
		configTypes::dbreader,
		configTypes::collector,
		configTypes::metering,
	};
	for (unsigned t = 0; t != 6; ++t) {
		for (SubsystemConfiguration::iterator i = subsystems.begin();
		     i != subsystems.end(); ++i) {	
			std::string id = i->first;
			if (id.find(TYPES[t])) {
				continue;
			}
			Configuration* c = i->second;

			// get aggregators from metering processes (we
			// need them for aggregator polling :/
			MeteringConfiguration* m = dynamic_cast<MeteringConfiguration*>(c);
			if (m) {
				FlowMeteringConfiguration* fm = m->getFlowMeteringConfiguration();
				if (fm) 
					aggregators.push_back(fm->getIpfixAggregator());
			}

			const std::vector<std::string>& nextVector = c->getNextVector();
			for (unsigned j = 0; j != nextVector.size(); ++j) {
				if (subsystems.find(nextVector[j]) == subsystems.end()) {
					throw std::runtime_error("Could not find " + nextVector[j] + " in subsystem list");
				}
				msg(MSG_DEBUG, "IpfixConfiguration: connecting %s to %s", c->getId().c_str(), subsystems[nextVector[j]]->getId().c_str()); 
				c->connect(subsystems[nextVector[j]]);
				msg(MSG_DEBUG, "IpfixConfiguration: successfully connected %s to %s", c->getId().c_str(), subsystems[nextVector[j]]->getId().c_str());
			}
		}
	}

	msg(MSG_INFO, "IpfixConfiguration: Successfully set up connections between subsystems");
}

void IpfixConfiguration::startSubsystems()
{
	msg(MSG_INFO, "IpfixConfiguration: Starting subsystems...");
	for (SubsystemConfiguration::iterator i = subsystems.begin();
	     i != subsystems.end(); ++i) {
		i->second->startSystem();
	}
	msg(MSG_INFO, "IpfixConfiguration: Successfully started subsystems");
}

void IpfixConfiguration::pollAggregatorLoop()
{
	unsigned poll_interval = 500;
	if (subsystems.find(configTypes::main) != subsystems.end()) {
		VermontMainConfiguration* m = dynamic_cast<VermontMainConfiguration*>(subsystems[configTypes::main]);
		poll_interval = m->getPollInterval();
	}

	timespec req, rem;
        /* break millisecond polltime into seconds and nanoseconds */
        req.tv_sec=(poll_interval * 1000000) / 1000000000;
        req.tv_nsec=(poll_interval * 1000000) % 1000000000;

	if (poll_interval == 0 || aggregators.empty()) {
		pause();
	} else {
	        msg(MSG_INFO, "Polling aggregator each %u msec", poll_interval);
		while (!stop) {
			nanosleep(&req, &rem);
			for (unsigned i = 0; i != aggregators.size(); ++i) {
				::pollAggregator(aggregators[i]);
			}
		}
	}
}

