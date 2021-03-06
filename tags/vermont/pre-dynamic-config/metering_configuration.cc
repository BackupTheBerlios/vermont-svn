/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "metering_configuration.h"
#include "exporter_configuration.h"
#include "pcapexporter_configuration.h"
#include "packetselection_configuration.h"
#include "packetreporting_configuration.h"
#include "flowmetering_configuration.h"
#include "dbwriter_configuration.h"

#include <sampler/Filter.h>
#include <sampler/PcapExporterSink.h>
#include <sampler/ExporterSink.h>
#include <sampler/HookingFilter.h>
#include <sampler/ExpressHookingFilter.h>
#include <concentrator/ipfix.hpp>

#include <cctype>


/*************************** MeteringConfiguration ***************************/

MeteringConfiguration::MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), packetSelection(0), packetReporting(0),
		 flowMetering(0), expressflowMetering(0), observationDomainId(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		THROWEXCEPTION("Got metering process without unique id!");
	}
	id = configTypes::metering + (const char*)idString;
	xmlFree(idString);
}

MeteringConfiguration::~MeteringConfiguration()
{
	// FIXME: delete packet processors
	delete packetReporting;
	delete packetSelection;
	delete flowMetering;
}

void MeteringConfiguration::setObservationDomainId(uint16_t id)
{
	observationDomainId = id;
}

void MeteringConfiguration::setCaptureLength(int len)
{
	captureLength = len;
}

void MeteringConfiguration::setDataLinkType(int type)
{
	dataLinkType = type;
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
		} else if (tagMatches(i, "expressflowMetering")) {
			expressflowMetering = new FlowMeteringConfiguration(doc, i);
			expressflowMetering->configure();
	//		expressflowMetering = new ExpressFlowMeteringConfiguration(doc, i);
	//		expressflowMetering->configure();
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
	// - an dbWriter (if it does FlowMetering)
	// - a pcapexporter process

	ExporterConfiguration* exporter = dynamic_cast<ExporterConfiguration*>(c);
	if (exporter) {
		if (packetReporting) {
			if (!packetSelection) {
				THROWEXCEPTION("MeteringConfiguration: packetReporting can only be connected to observationPoint!");
			}
			msg(MSG_DEBUG, "MeteringConfiguration: Connecting packetReporting to exporter");
			// rough estimation of the maximum record length including variable length fields
			uint16_t recordsPerPacket = packetReporting->recordLength + packetReporting->recordVLFields*captureLength;
			msg(MSG_INFO, "MeteringConfiguration: Estimated record length is %u", recordsPerPacket);	
			exporter->createExporterSink(packetReporting->t, observationDomainId, recordsPerPacket);
			packetSelection->filter->setReceiver(exporter->getExporterSink());
		}
		if (flowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "MeteringConfiguration: Setting up HookingFilter for Standard Aggregator");
				HookingFilter* h = new HookingFilter(flowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
 			msg(MSG_DEBUG, "MeteringConfiguration: Setting up IpfixSender");
 			exporter->createIpfixSender(observationDomainId);
 			flowMetering->ipfixAggregator->addFlowSink(exporter->getIpfixSender());
		}
		if (expressflowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "MeteringConfiguration: Setting up HookingFilter for Express Aggregator.");
				ExpressHookingFilter* h = new ExpressHookingFilter(expressflowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
 			msg(MSG_DEBUG, "MeteringConfiguration: Setting up IpfixSender for Express Aggregator");
 			exporter->createIpfixSender(observationDomainId);
 			expressflowMetering->ipfixAggregator->addFlowSink(exporter->getIpfixSender());
		}
		return;
	}

	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		metering->setObservationDomainId(observationDomainId);
		
		if (metering->flowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "MeteringConfiguration: Setting up HookingFilter for Standard Aggregator");
				HookingFilter* h = new HookingFilter(metering->flowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
		}
		if (metering->expressflowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "MeteringConfiguration: Setting up HookingFilter for Express Aggregator.");
				ExpressHookingFilter* h = new ExpressHookingFilter(metering->expressflowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
		}
		if (metering->packetReporting) {
			// install our packetSelection into the other meteringprocess
			// and forget our packetReporting in this chain
			if (packetSelection) {
				delete metering->packetSelection;
				metering->packetSelection = packetSelection;
				// the other metering process will now handle the packet selection
				// avoid double freeing, set pointer to NULL (unfortunately, we cannot use
				// the packet selection in other contexts any more)
				packetSelection = NULL;
			}
			else
				THROWEXCEPTION("MeteringConfiguration: packetReporting can only be connected to observationPoint!");
		}
		
		return;
	}

#ifdef DB_SUPPORT_ENABLED
	DbWriterConfiguration* dbWriterConfiguration = dynamic_cast<DbWriterConfiguration*>(c);
	if (dbWriterConfiguration) {
		if (!(flowMetering || expressflowMetering)) {
			THROWEXCEPTION("MeteringProcess: Only flowMetering and expressflowMetering can be connected to dbWriter!");
		}

                dbWriterConfiguration->setObservationDomainId(observationDomainId);
		if (flowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "MeteringConfiguration: Setting up HookingFilter for Standard Aggregator");
				HookingFilter* h = new HookingFilter(flowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
 			msg(MSG_DEBUG, "MeteringConfiguration: Setting up IpfixSender");
			flowMetering->ipfixAggregator->addFlowSink(dbWriterConfiguration->getDbWriter());
		}
		if (expressflowMetering) {
			if (packetSelection) {
				msg(MSG_DEBUG, "MeteringConfiguration: Setting up HookingFilter for Express Aggregator.");
				ExpressHookingFilter* h = new ExpressHookingFilter(expressflowMetering->ipfixAggregator);
				packetSelection->filter->addProcessor(h);
			}
 			msg(MSG_DEBUG, "MeteringConfiguration: Setting up IpfixDbWriter for Express Aggregator");
 			expressflowMetering->ipfixAggregator->addFlowSink(dbWriterConfiguration->getDbWriter());
		}

		return;
	}
#endif
	
	PcapExporterConfiguration* pcapExporter = dynamic_cast<PcapExporterConfiguration*>(c);
	if (pcapExporter) {
		msg(MSG_DEBUG, "MeteringConfiguration: Adding pcapExporter to filter");
		pcapExporter->getExporterSink()->setDataLinkType(dataLinkType);
		getPacketSelectionConfiguration()->filter->setReceiver(pcapExporter->getExporterSink());
		return;
	}

	THROWEXCEPTION("Cannot connect %s to metering process!", c->getId().c_str());
}

void MeteringConfiguration::startSystem()
{
	msg(MSG_INFO, "MeteringConfiguration: Running metering process.");
	if (packetSelection) {
		packetSelection->startSystem();
	}
	if (flowMetering) {
		flowMetering->startSystem();
	}
	if (expressflowMetering) {
		expressflowMetering->startSystem();
	}
}

void MeteringConfiguration::stopSystem()
{
	msg(MSG_INFO, "MeteringConfiguration: Stopping metering process.");
	if (packetSelection) {
		packetSelection->stopSystem();
	}
	if (flowMetering) {
		flowMetering->stopSystem();
	}
}

