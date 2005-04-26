/** @file
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib" 
 *
 */
 
#include <string.h>
#include "sndIpfix.h"
#include "common.h"
#include "../ipfixlolib/ipfixlolib.h"

/***** Definitions ***********************************************************/

#define SND_SOURCE_ID 70538

/***** Global Variables ******************************************************/

ipfix_exporter* exporter;
uint16_t lastTemplateId;

static uint8_t ringbufferPos = 0; /**< Pointer to next free slot in @c conversionRingbuffer. */
static uint8_t conversionRingbuffer[1 << (8 * sizeof(ringbufferPos))]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeSndIpfix() {
	lastTemplateId = 10000;
	if (ipfix_init_exporter(SND_SOURCE_ID, &exporter) != 0) {
		fatal("ipfix_init_exporter failed");
		return -1;
		}
	return 0;
	}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeSndIpfix() {
	ipfix_deinit_exporter(exporter);
	return 0;
	}

/**
 * Adds a new collector to send Records to
 * @return handle to use when calling @c sndIpfixClose()
 */
IpfixSender* sndIpfixUdpIpv4(char* ip, uint16_t port) {
	IpfixSender* ipfixSender = (IpfixSender*)malloc(sizeof(IpfixSender));
	strcpy(ipfixSender->ip, ip);
	ipfixSender->port = port;
	
	if (ipfix_add_collector(exporter, ipfixSender->ip, ipfixSender->port, UDP) != 0) {
		fatal("ipfix_add_collector failed");
		return NULL;
		}
	
	return ipfixSender;
	}

/**
 * Removes a collector from the list of Collectors to send Records to
 * @param ipfixSender handle obtained by calling @c sndIpfixUdpIpv4()
 */
void sndIpfixClose(IpfixSender* ipfixSender) {
	if (ipfix_remove_collector(exporter, ipfixSender->ip, ipfixSender->port) != 0) {
		fatal("ipfix_remove_collector failed");
		}
	free(ipfixSender);
	}

void startSndIpfix(IpfixSender* ipfixSender) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	}

void stopSndIpfix(IpfixSender* ipfixSender) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	}

/**
 * Announces a new Template
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int sndNewDataTemplate(DataTemplateInfo* dataTemplateInfo) {
	uint16_t my_template_id = ++lastTemplateId;
	if (lastTemplateId > 60000) {
		/* FIXME: Does not always work, e.g. if more than 50000 new Templates per minute are created */
		lastTemplateId = 10000;
		}

	/* put Template ID in Template's userData */
	int* p = (int*)malloc(sizeof(int));
	*p = htons(my_template_id);
	dataTemplateInfo->userData = p;
	
	int i;

	/* Count number of IPv4 fields with length 5 */	
	int splitFields = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];
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
		FieldInfo* fi = &dataTemplateInfo->dataInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
			}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
			}
		}
						
	ipfix_start_datatemplate_set(exporter, my_template_id, dataTemplateInfo->fieldCount + splitFields, dataTemplateInfo->dataCount + splitFixedfields);

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];
		
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

	debugf("%d data fields", dataTemplateInfo->dataCount);
	
	int dataLength = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->dataInfo[i];
		
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
	
	debugf("%d data length", dataLength);

	char* data = (char*)malloc(dataLength);
	memcpy(data, dataTemplateInfo->data, dataLength);
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

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
	ipfix_put_template_data(exporter, my_template_id, data, dataLength);
	free(data);

	ipfix_end_template_set(exporter, my_template_id);

	return 0;
	}

/**
 * Invalidates a template; Does NOT free dataTemplateInfo
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int sndDestroyDataTemplate(DataTemplateInfo* dataTemplateInfo) {
	free(dataTemplateInfo->userData);
	return 0;
	}
	
/**
 * Put new Data Record in outbound exporter queue
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int sndDataDataRecord(DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	int i;
	
	/* print Data Record */
	printf("\n-+--- exporting Record\n");
	printf(" `- fixed data\n"); 
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
		}
	printf(" `- variable data\n"); 
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
		printf("\n");
		}
	printf(" `---\n\n");
	

	/* get Template ID from Template's userData */
	uint16_t my_n_template_id = *(uint16_t*)dataTemplateInfo->userData;
	
	if (ipfix_start_data_set(exporter, &my_n_template_id) != 0 ) {
		fatal("ipfix_start_data_set failed!");
		return -1;
		}
		
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

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

	if (ipfix_end_data_set(exporter) != 0) {
		fatal("ipfix_end_data_set failed");
		return -1;
		}

	if (ipfix_send(exporter) != 0) {
		fatal("ipfix_send failed");
		return -1;
		}
	
	return 0;
	}
