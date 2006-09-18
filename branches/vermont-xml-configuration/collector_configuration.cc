#include "collector_configuration.h"
#include "metering_configuration.h"


#include <msg.h>
#include <concentrator/aggregator.h>


CollectorConfiguration::CollectorConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), ipfixCollector(NULL), hasCollector(false),
	  observationDomainId(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got collector without unique id!");
	}
	id = configTypes::collector + (const char*)idString;
	xmlFree(idString);
}

CollectorConfiguration::~CollectorConfiguration()
{
	stopIpfixCollector(ipfixCollector);
	destroyIpfixCollector(ipfixCollector);
	deinitializeIpfixReceivers();
}

void CollectorConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"listener")) {
			if (hasCollector) {
				throw std::runtime_error("Only one listener within a collector allowed! If you need more listeners, you'll have to create more collectors");
			}
			readListener(i);
			hasCollector = true;
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"udpTemplateLifetime")) {
			msg(MSG_ERROR, "Oooops ... Don't know how to handle udpTemplateLifetime!");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"observationDomainId")) {
			observationDomainId = atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"next")) {
			fillNextVector(i);
		}
		i = i->next;
	}
	setUp();
}

void CollectorConfiguration::readListener(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"ipAddressType")) {
			// we only have ipv4 at the moment
			// so nothing is implemented yet for ipv6
			if (getContent(i) != "4") {
				msg(MSG_INFO, "Only ipv4 is supported at the moment. \"ipAddressType\" will be ignored at the moment");
			}
		} else  if (!xmlStrcmp(i->name, (const xmlChar*)"ipAddress")) {
			ipAddress = getContent(i);
			msg(MSG_INFO, "Listening on a specific interface isn't supported right now. Vermont will listen on all interfaces. \"ipAddress\" will be ignored at the moment");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"transportProtocol")) {
			protocolType = atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"port")) {
			port = (uint16_t)atoi(getContent(i).c_str());
		}
		i = i->next;
	}
}

void CollectorConfiguration::setUp()
{
	ipfixCollector = createIpfixCollector();
	if (!ipfixCollector) {
		throw std::runtime_error("Could not create collector");
	}

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(UDP_IPV4, port);
	if (!ipfixReceiver) {
		throw std::runtime_error("Could not create receiver");
	}
	addIpfixReceiver(ipfixCollector, ipfixReceiver);

	ipfixPacketProcessor = createIpfixPacketProcessor();
	if (!ipfixPacketProcessor) {
		throw std::runtime_error("Could not create IPFIX packet processor");
	}

	ipfixParser = createIpfixParser();
	if (!ipfixParser) {
		throw std::runtime_error("Could not create IPFIX parser");
	}
}

void CollectorConfiguration::connect(Configuration* c)
{
	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	
	if (metering) {
		if (metering->isAggregating()) {
			metering->setObservationId(observationDomainId);
			msg(MSG_DEBUG, "CollectorConfiguration: Got metering process which is aggreagting");
			IpfixAggregator* aggregator = metering->getAggregator();
			if (!aggregator) {
				throw std::runtime_error("CollectorConfiguration: ipfixAggregator is null -> This is a bug! Please report it");
			}
			msg(MSG_DEBUG, "Adding aggregator to ipfixParser");
			addIpfixParserCallbacks(ipfixParser, getAggregatorCallbackInfo(aggregator));
			msg(MSG_DEBUG, "Adding ipfixParser to ipfixPacketProcessor");
			setIpfixParser(ipfixPacketProcessor, ipfixParser);
			msg(MSG_DEBUG, "Adding ipfixPacketProcessor to ipfixCollector");
			addIpfixPacketProcessor(ipfixCollector, ipfixPacketProcessor);
			msg(MSG_DEBUG, "Sucessfully set up connection between collector and aggregator");
		} else {
			throw std::runtime_error("Metering process isn't aggregating -> cannot connect it to an collector!");
		}
		return;
	}
	
	throw std::runtime_error("Cannot connect " + c->getId() + " to a collector!");
}

void CollectorConfiguration::startSystem()
{
	startIpfixCollector(ipfixCollector);
}
