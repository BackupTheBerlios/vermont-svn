#include "collector_configuration.h"


CollectorConfiguration::CollectorConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got collector without unique id!");
	}
	id = configTypes::collector + (const char*)idString;
	xmlFree(idString);
}

void CollectorConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"ipAddressType")) {
			// we only have ipv4 at the moment
			// so nothing is implemented yet for ipv6
		} else  if (!xmlStrcmp(i->name, (const xmlChar*)"ipAddress")) {
			ipAddress = getContent(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"transportProtocol")) {
			protocolType = atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"port")) {
			port = (uint16_t)atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"next")) {
			fillNextVector(i);
		}
		i = i->next;
	}

		    
}

void CollectorConfiguration::setUp()
{

}

void CollectorConfiguration::connect(Configuration*)
{
	throw std::runtime_error("A collector cannot be a target to Configuration::connect()!");
}

void CollectorConfiguration::startSystem()
{
	throw std::runtime_error("CollectorConfiguration::start() isn't implemented yet!");
}
