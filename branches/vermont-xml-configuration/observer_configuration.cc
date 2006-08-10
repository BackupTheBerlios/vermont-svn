#include "observer_configuration.h"


#include <stdexcept>
#include <cstdlib>


ObserverConfiguration::ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got observer without unique id!");
	}
	id = configTypes::observer + (const char*)idString;
	xmlFree(idString);
}


void ObserverConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"observationDomainId")) {
			observationDomain = std::atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"type")) {
			type = getContent(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"parameters")) {
			parseParameters(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"next")) {
			fillNextVector(i);
		}
		i = i->next;
	}
}

void ObserverConfiguration::parseParameters(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"interface")) {
			interface = getContent(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"pcap_filter")) {
			filter = getContent(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"capture_len")) {
			captureLength = atoi(getContent(i).c_str());
		}
		i = i->next;
	}
}
