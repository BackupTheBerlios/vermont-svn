#include "observer_configuration.h"
#include "metering_configuration.h"
#include "msg.h"

#include <sampler/Template.h>
#include <sampler/PacketProcessor.h>
#include <sampler/Filter.h>
#include <sampler/ExporterSink.h>
#include <sampler/HookingFilter.h>
#include <concentrator/sampler_hook_entry.h>


#include <stdexcept>
#include <cstdlib>


ObserverConfiguration::ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), observer(NULL), captureLength(0), pcapChar(NULL)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got observer without unique id!");
	}
	id = configTypes::observer + (const char*)idString;
	xmlFree(idString);
}

ObserverConfiguration::~ObserverConfiguration()
{
	delete observer;
	delete pcapChar;
}


void ObserverConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "observationDomainId")) {
			observationDomain = std::atoi(getContent(i).c_str());
		} else if (tagMatches(i, "type")) {
			type = getContent(i);
			if (type != "pcap") {
				msg(MSG_FATAL, "Vermont does not provide any observer type but pcap");
				throw std::runtime_error("Could not read observer configuration");
			}
		} else if (tagMatches(i, "parameters")) {
			parseParameters(i);
		} else if (tagMatches(i, "next")) {
			fillNextVector(i);
		}
		i = i->next;
	}

	setUp();
}

void ObserverConfiguration::parseParameters(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "interface")) {
			interface = getContent(i);
		} else if (tagMatches(i, "pcap_filter")) {
			pcapFilter = getContent(i);
		} else if (tagMatches(i, "capture_len")) {
			captureLength = atoi(getContent(i).c_str());
		}
		i = i->next;
	}
}


void ObserverConfiguration::setUp()
{
	observer = new Observer(interface.c_str());
	if (captureLength) {
		if (!observer->setCaptureLen(captureLength)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified - using %d", observer->getCaptureLen());
		}
	}
	
	pcapChar = new char[pcapFilter.size() + 1];
	strncpy(pcapChar, pcapFilter.c_str(), pcapFilter.size() + 1);	
	if (!observer->prepare(pcapChar)) {
		msg(MSG_FATAL, "Observer: preparing failed");
		throw std::runtime_error("Observer setup failed!");
	}
}


void ObserverConfiguration::connect(Configuration* c)
{
	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		metering->setObservationId(observationDomain);
		if (metering->isAggregating()) {
			msg(MSG_INFO, "Adding HookingFilter");
			HookingFilter* h = new HookingFilter(sampler_hook_entry);
			h->setContext(metering->getAggregator());
			metering->getFilters()->addProcessor(h);
		}
		observer->addReceiver(metering->getFilters());
		return;
	}
	
	throw std::runtime_error("Cannot connect Observer to " + c->getId() + "!");
}

void ObserverConfiguration::startSystem()
{
	observer->startCapture();
}
