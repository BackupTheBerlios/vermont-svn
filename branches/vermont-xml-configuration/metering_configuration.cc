#include "metering_configuration.h"
#include "exporter_configuration.h"
#include "packetselection_configuration.h"
#include "packetreporting_configuration.h"
#include "flowmetering_configuration.h"

#include <sampler/Filter.h>
#include <sampler/ExporterSink.h>
#include <sampler/HookingFilter.h>
#include <concentrator/sampler_hook_entry.h>
#include <concentrator/ipfix.h>

#include <cctype>


/*************************** MeteringConfiguration ***************************/

MeteringConfiguration::MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), packetSelection(0), packetReporting(0),
		 flowMetering(0), observationDomainId(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got metering process without unique id!");
	}
	id = configTypes::metering + (const char*)idString;
	xmlFree(idString);
}

MeteringConfiguration::~MeteringConfiguration()
{
	delete packetReporting;
	delete packetSelection;
	delete flowMetering;
}

void MeteringConfiguration::setObservationDomainId(uint16_t id)
{
	observationDomainId = id;
}

void MeteringConfiguration::configure()
{
	msg(MSG_INFO, "MeteringConfiguration: Start reading meteringProcess");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "packetSelection")) {
			packetSelection = new PacketSelectionConfiguration(doc, i);
			packetSelection->configure();
		} else if (tagMatches(i, "packetReporting")) {
			packetReporting = new PacketReportingConfiguration(doc, i);
			packetReporting->configure();
		} else if (tagMatches(i, "flowMetering")) {
			flowMetering = new FlowMeteringConfiguration(doc, i);
			flowMetering->configure();
		} else if (tagMatches(i, "next")) {
			fillNextVector(i);
		}
		i = i->next;
	}

	msg(MSG_INFO, "MeteringConfiguration: Successfully parsed meteringProcess section");
}

PacketSelectionConfiguration* MeteringConfiguration::getPacketSelectionConfiguration()
{
	// an observervationProcess _needs_ a packetSelection, even if
	// it wasn't part of the configuration file
	if (!packetSelection) {
		packetSelection = new PacketSelectionConfiguration();
	}
	return packetSelection;
}


void MeteringConfiguration::connect(Configuration* c)
{	
	// a MeteringConfiguration can put it's data into
	// - an exporting process (if it does FlowMetering or PacketReporting)
	// - an metering process (if the source does PacketSelection
	//   and the destination does FlowMetering or PacketReporting

	ExporterConfiguration* exporter = dynamic_cast<ExporterConfiguration*>(c);
	if (exporter) {
		if (packetReporting) {
			msg(MSG_DEBUG, "Connecting packetReporting to exporter");
			exporter->createExporterSink(packetReporting->t, observationDomainId);
			packetSelection->filter->setReceiver(exporter->getExporterSink());
		}
		if (flowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "Setting up HookingFilter");
				HookingFilter* h = new HookingFilter(sampler_hook_entry);
				h->setContext(flowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
 			msg(MSG_DEBUG, "Setting up IpfixSender");
 			exporter->createIpfixSender(observationDomainId);
 			addAggregatorCallbacks(flowMetering->ipfixAggregator, 
 					       getIpfixSenderCallbackInfo(exporter->getIpfixSender()));
		}
		return;
	}

	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		metering->setObservationDomainId(observationDomainId);
		
		if (metering->flowMetering) {
			HookingFilter* h = new HookingFilter(sampler_hook_entry);
			h->setContext(metering->flowMetering->ipfixAggregator);
			packetSelection->filter->addProcessor(h);
		}
		if (metering->packetReporting) {
			std::vector<PacketProcessor *> filters = packetSelection->filter->getProcessors();
			for (unsigned i = 0; i != filters.size(); ++i) {
				metering->packetSelection->filter->addProcessor(filters[i]);
			}
		}
		
		return;
	}

	throw std::runtime_error("Cannot connect " + c->getId() + " to metering process!");
}

void MeteringConfiguration::startSystem()
{
	packetSelection->startSystem();
	if (flowMetering) {
		flowMetering->startSystem();
	}
}
