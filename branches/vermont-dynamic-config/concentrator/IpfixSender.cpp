/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "IpfixSender.hpp"
#include "ipfix.hpp"

#include "common/msg.h"
#include "common/Time.h"

#include <sstream>
#include <stdexcept>
#include <string.h>

using namespace std;

/*
 we start OUR template IDs at <this>
 we need our own template IDs and they should be unique
 */
#define SENDER_TEMPLATE_ID_LOW 10000
/* go back to SENDER_TEMPLATE_ID_LOW if _HI is reached */
#define SENDER_TEMPLATE_ID_HI 60000

/**
 * Creates a new IPFIX Exporter. Do not forget to call @c startIpfixSender() to begin sending
 * @param sourceID Source ID this exporter will report
 * @param ip destination collector's address
 * @param port destination collector's port
 * @return handle to use when calling @c destroyIpfixSender()
 */
IpfixSender::IpfixSender(uint16_t observationDomainId, const char* ip, uint16_t port) 
	: maxFlowLatency(100), // FIXME: this has to be set in the configuration!
	  noCachedRecords(0),
	  thread(threadWrapper)
{
	ipfix_exporter** exporterP = &this->ipfixExporter;
	statSentRecords = 0;
	currentTemplateId = 0;
	lastTemplateId = SENDER_TEMPLATE_ID_LOW;

	if(ipfix_init_exporter(observationDomainId, exporterP) != 0) {
		msg(MSG_FATAL, "sndIpfix: ipfix_init_exporter failed");
		goto out;
	}

	if (ip && port) {
		Collector newCollector;
		strcpy(newCollector.ip, ip);
		newCollector.port = port;

		addCollector(ip, port);

		collectors.push_back(newCollector);
	}
	
	StatisticsManager::getInstance().addModule(this);
	
	msg(MSG_DEBUG, "IpfixSender: running");
	return;
	
out:
	THROWEXCEPTION("IpfixSender creation failed");
	return;	
}

/**
 * Removes a collector from the list of Collectors to send Records to
 */
IpfixSender::~IpfixSender() {
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	ipfix_deinit_exporter(exporter);
}

/**
 * Starts or resumes sending messages
 */
void IpfixSender::performStart() 
{
	thread.run(this);
}

/**
 * Temporarily pauses sending messages
 */
void IpfixSender::performShutdown() 
{
	thread.join();
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 * @param ips handle to the Exporter
 * @param ip string of the IP
 * @param port port number
 * FIXME: support for other than UDP
 */
void IpfixSender::addCollector(const char *ip, uint16_t port)
{
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;

	if(ipfix_add_collector(ex, ip, port, UDP) != 0) {
		THROWEXCEPTION("IpfixSender: ipfix_add_collector of %s:%d failed", ip, port);
	}
	
	msg(MSG_INFO, "IpfixSender: adding %s:%d to exporter", ip, port);

	Collector newCollector;
	strcpy(newCollector.ip, ip);
	newCollector.port = port;
	collectors.push_back(newCollector);
}

/**
 * Announces a new Template
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixSender::onDataTemplate(IpfixDataTemplateRecord* record)
{
	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo = record->dataTemplateInfo;
	uint16_t my_template_id;
	uint16_t my_preceding;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	if (!exporter) {
		THROWEXCEPTION("sndIpfix: Exporter not set");
	}

	/* get or assign template ID */
	if(dataTemplateInfo->templateId)
	    my_template_id = dataTemplateInfo->templateId;
	else
	    my_template_id = dataTemplateInfo->templateId = ++lastTemplateId;

	my_preceding = dataTemplateInfo->preceding;
	if (lastTemplateId >= SENDER_TEMPLATE_ID_HI) {
		/* FIXME: Does not always work, e.g. if more than 50000 new Templates per minute are created */
		lastTemplateId = SENDER_TEMPLATE_ID_LOW;
	}
	
	int i;

	/* Count number of IPv4 fields with length 5 */
	int splitFields = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFields++;
		}
	}

	/* Count number of IPv4 fields with length 5 */
	int splitFixedfields = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
	}

	if (0 != ipfix_start_datatemplate_set(exporter, my_template_id, my_preceding, dataTemplateInfo->fieldCount + splitFields, dataTemplateInfo->dataCount + splitFixedfields)) {
		THROWEXCEPTION("sndIpfix: ipfix_start_datatemplate_set failed");
	}

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_field(exporter, my_template_id, fi->type.id, fi->type.length, fi->type.eid);
		}
	}

	DPRINTF("%d data fields", dataTemplateInfo->dataCount);

	int dataLength = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		dataLength += fi->type.length;

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_fixedfield(exporter, my_template_id, fi->type.id, fi->type.length, fi->type.eid);
		}
	}

	DPRINTF("%d data length", dataLength);

	char* data = (char*)dataLength?(char*)malloc(dataLength):0; // electric fence does not like 0-byte mallocs
	memcpy(data, dataTemplateInfo->data, dataLength);

	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		/* Invert imask of IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = (uint8_t*)(data + fi->offset + 4);
			*mask = 32 - *mask;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = (uint8_t*)(data + fi->offset + 4);
			*mask = 32 - *mask;
		}
		else {
		}

	}

	if (0 != ipfix_put_template_data(exporter, my_template_id, data, dataLength)) {
		free(data);
		THROWEXCEPTION("sndIpfix: ipfix_put_template_data failed");
	}
	free(data);

	if (0 != ipfix_end_template_set(exporter, my_template_id)) {
		THROWEXCEPTION("sndIpfix: ipfix_end_template_set failed");
	}

	msg(MSG_INFO, "sndIpfix created template with ID %u", my_template_id);
}

/**
 * Invalidates a template; Does NOT free dataTemplateInfo
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixSender::onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record)
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	if (!exporter) {
		THROWEXCEPTION("exporter not set");
	}

	uint16_t my_template_id = record->dataTemplateInfo->templateId;


	/* Remove template from ipfixlolib */
	if (0 != ipfix_remove_template_set(exporter, my_template_id)) {
		msg(MSG_FATAL, "sndIpfix: ipfix_remove_template_set failed");
	}
	else
	{
		msg(MSG_INFO, "sndIpfix removed template with ID %u", my_template_id);
	}

	free(record->dataTemplateInfo->userData);
}


/**
 * Start generating a new Data Set unless the template ID is the same as the current Data Set.
 * Unfinished Data Set are terminated and sent if necessary.
 * @param templateId of the new Data Set
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::startDataSet(uint16_t templateId) 
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	uint16_t my_n_template_id = htons(templateId);
	
	/* check if we can use the current Data Set */
	//TODO: make maximum number of records per Data Set variable
	if((noCachedRecords < 10) && (templateId == currentTemplateId))
		return;

	if(noCachedRecords > 0) endAndSendDataSet();
	
	if (ipfix_start_data_set(exporter, my_n_template_id) != 0 ) {
		THROWEXCEPTION("sndIpfix: ipfix_start_data_set failed!");
	}

	currentTemplateId = templateId;
}
	

/**
 * Terminates and sends current Data Set if available.
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::endAndSendDataSet() 
{
	if(noCachedRecords > 0) {
		ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	
		if (ipfix_end_data_set(exporter) != 0) {
			THROWEXCEPTION("sndIpfix: ipfix_end_data_set failed");
		}

		if (ipfix_send(exporter) != 0) {
			THROWEXCEPTION("sndIpfix: ipfix_send failed");
		}

		removeRecordReferences();

		currentTemplateId = 0;
	}
}


/**
 * removes references to flows inside buffer recordsToRelease
 */
void IpfixSender::removeRecordReferences()
{
	while (!recordsToRelease.empty()) {
		recordsToRelease.front()->removeReference();
		recordsToRelease.pop();
	}
	noCachedRecords = 0;
}
	

/**
 * Put new Data Record in outbound exporter queue
 * @param rec Data Data Record
 */
void IpfixSender::onDataDataRecord(IpfixDataDataRecord* record)
{
	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo = record->dataTemplateInfo;
	IpfixRecord::Data* data = record->data;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	if (!exporter) {
		THROWEXCEPTION("exporter not set");
	}

	startDataSet(dataTemplateInfo->templateId);

	int i;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(exporter, data + fi->offset, 4);
			ipfix_put_data_field(exporter, mask, 1);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(exporter, data + fi->offset, 4);
			ipfix_put_data_field(exporter, mask, 1);
		}
		else {
			ipfix_put_data_field(exporter, data + fi->offset, fi->type.length);
		}

	}

	statSentRecords++;
	
	recordsToRelease.push(record);
	noCachedRecords++;
}


/**
 * wrapper function for thread creation
 */
void* IpfixSender::threadWrapper(void* instance)
{
	IpfixSender* is = reinterpret_cast<IpfixSender*>(instance);
	is->processLoop();
	return NULL;
}


/**
 * loop which processes incoming flows and sends a new IPFIX packet
 * when necessary
 * it ensures, that a new packet is sent when no more flows can be stored in it
 * (maximum size is reached), or when the specified timeout maxFlowLatency is
 * reached after the first flow was received
 * 
 * NOTICE: this algorithm does *NOT* take into account that functions called by
 * IpfixRecordDestination::receive initiate the packet send themselves (as is done
 * when the template id changes)
 */
void IpfixSender::processLoop()
{
	timespec timeout;
	addToCurTime(&timeout, maxFlowLatency);
	
	while (!exitFlag) {
		IpfixRecord* record;
		if (!incomingRecords.popAbs(timeout, &record)) {
			// either timeout or exitFlag was set
			if (exitFlag) break;
			if (noCachedRecords > 0) {
				// send new packet to network
				endAndSendDataSet();
			}
			addToCurTime(&timeout, maxFlowLatency);
		} else {
			// record was received, queue it in ipfixlolib
			IpfixRecordDestination::receive(record);
			if (noCachedRecords >= 10) { // FIXME: we need to put as many flows in a packet as possible, not only 10!
				// send packet
				endAndSendDataSet();
				addToCurTime(&timeout, maxFlowLatency);
			}
		}		
	}
}

void IpfixSender::connectTo(BaseDestination*)
{
	THROWEXCEPTION("don't call me!");
}

void IpfixSender::disconnect()
{
	THROWEXCEPTION("don't call me!");
}


bool IpfixSender::isConnected() const
{
	THROWEXCEPTION("don't call me!");
	return false;
}


/**
 * if flows are cached at the moment, this function sends them to the network
 * immediately
 */
void IpfixSender::flushPacket()
{
	endAndSendDataSet();
}




/**
 * statistics function called by StatisticsManager
 */
std::string IpfixSender::getStatistics()
{
	ostringstream oss;
	
	uint32_t sent = statSentRecords;
	statSentRecords -= sent;
	
	oss << "IpfixReceiverUdpIpV4: received packets: " << sent << endl;	

	return oss.str();
}
