#include "exporter_configuration.h"
#include "metering_configuration.h"

#include <sampler/ExporterSink.h>

ExporterConfiguration::ExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), hasCollector(false), exporterSink(0),
	ipfixSender(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got exporter without unique id!");
	}
	id = configTypes::exporter + (const char*)idString;
	xmlFree(idString);
}

ExporterConfiguration::~ExporterConfiguration()
{
	delete exporterSink;
	if (ipfixSender) {
		stopIpfixSender(ipfixSender);
		destroyIpfixSender(ipfixSender);
		deinitializeIpfixSenders();
	}
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
			if (hasCollector) {
				throw std::runtime_error("VERMONT currently supports one collector per Exporter! If you need to send to more than one collector, you'll have to create a new exporter section in your config file");
			}
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
			protocolType = getContent(i);
			if (protocolType == "17") {
				protocolType = "UDP";
			}
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"port")) {
			port = (uint16_t)atoi(getContent(i).c_str());
		}		
		i = i->next;
	}
	hasCollector = true;
}

void ExporterConfiguration::setUp()
{

}

void ExporterConfiguration::createExporterSink(Template* t, uint16_t sourceId)
{
	exporterSink = new ExporterSink(t, sourceId);
	exporterSink->addCollector(ipAddress.c_str(),
				   port,
				   protocolType.c_str());
}

void ExporterConfiguration::createIpfixSender(uint16_t sourceId)
{
	initializeIpfixSenders();
	ipfixSender = ::createIpfixSender(sourceId,
					ipAddress.c_str(),
					port);
	if (!ipfixSender) {
		throw std::runtime_error("Could not create IpfixSender!");
	}
	// we need to start IpfixSender right here, because ipfixAggregator
	// needs a running IpfixSender before it can be created
	// TODO: FIX THIS!
	startIpfixSender(ipfixSender);
}

void ExporterConfiguration::connect(Configuration*)
{
	throw std::runtime_error("Exporter is an end target and cannot be connected to something!");
}

void ExporterConfiguration::startSystem()
{
	if (exporterSink) {
		msg(MSG_DEBUG, "ExporterConfiguration: Starting ExporterSink for Sampler");
		exporterSink->runSink();
	} else if (ipfixSender) {
		msg(MSG_DEBUG, "ExporterConfiguration: Running IpfixSender");
		// ipfixSender already runs (see createIpfixSender())
	} else {
		throw std::runtime_error("Can neither start an ExporterSink, nor an IpfixSender -> something is broken!");
	}
}
