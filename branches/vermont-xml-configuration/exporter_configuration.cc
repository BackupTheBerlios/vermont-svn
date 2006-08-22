#include "exporter_configuration.h"

ExporterConfiguration::ExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got exporter without unique id!");
	}
	id = configTypes::exporter + (const char*)idString;
	xmlFree(idString);
}

void ExporterConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"ipfixPacketRestrictions")) {
			readPacketRestrictions(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"udpTemplateManagement")) {
			readUdpTemplateManagement(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"collector")) {
			readCollector(i);
		}
		i = i->next;
	}
}

void ExporterConfiguration::readPacketRestrictions(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"maxPacketSize")) {
			maxPacketSize = (uint16_t)atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"maxExportDelay")) {
			exportDelay = getTimeInMsecs(i);
		}
		i = i->next;
	}
}

void ExporterConfiguration::readUdpTemplateManagement(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"templateRefreshTimeout")) {
			templateRefreshTime = getTimeInMsecs(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"templateRefreshRate")) {
			templateRefreshRate = (unsigned)atoi(getContent(i).c_str());
		}
		i = i->next;
	}
}

void ExporterConfiguration::readCollector(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
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
		}		
		i = i->next;
	}
}

void ExporterConfiguration::setUp()
{

}

void ExporterConfiguration::connect(Configuration*)
{

}
