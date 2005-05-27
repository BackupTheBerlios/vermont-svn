/** @file
 * IPFIX Printer module.
 *
 * Prints received flows to stdout 
 *
 */
 
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "printIpfix.h"
#include "common.h"

/***** Global Variables ******************************************************/

/***** Internal Functions ****************************************************/

static int ipv4ToString(FieldType type, FieldData* data, char* s, int len) {
	int octet1 = 0;
	int octet2 = 0;
	int octet3 = 0;
	int octet4 = 0;
	int imask = 0;
	if (type.length >= 1) octet1 = data[0];
	if (type.length >= 2) octet2 = data[1];
	if (type.length >= 3) octet3 = data[2];
	if (type.length >= 4) octet4 = data[3];
	if (type.length >= 5) imask = data[4];
	if (type.length > 5) {
		errorf("IPv4 Address with length %d unparseable", type.length);
		return -1;
		}
	
	if ((type.length == 5) /*&& (imask != 0)*/) {
		snprintf(s, len, "%d.%d.%d.%d/%d", octet1, octet2, octet3, octet4, 32-imask);
		} else {
		snprintf(s, len, "%d.%d.%d.%d", octet1, octet2, octet3, octet4);
		}

	return 0;
	}

static int portToString(FieldType type, FieldData* data, char* s, int len) {
	if (type.length == 0) {
		strncpy(s, "-", len);
		return 0;
		}
	if (type.length == 2) {
		int port = ((uint16_t)data[0] << 8)+data[1];
		snprintf(s, len, "%d", port);
		return 0;
		}
	if ((type.length >= 4) && ((type.length % 4) == 0)) {
		int i;
		char buf[40];

		strncpy(s, "", len);
		for (i = 0; i < type.length; i+=4) {
			int starti = ((uint16_t)data[i+0] << 8)+data[i+1];
			int endi = ((uint16_t)data[i+2] << 8)+data[i+3];
			if (i > 0) strncat(s, ",", len - strlen(s));
			if (starti != endi) {
				snprintf(buf, 40, "%d:%d", starti, endi);
				strncat(s, buf, len-strlen(s));
				} else {
				snprintf(buf, 40, "%d", starti);
				strncat(s, buf, len-strlen(s));
				}
			} 
		return 0;
		}
	
	errorf("Port with length %d unparseable", type.length);
	return -1;
	}

static int protocolToString(FieldType type, FieldData* data, char* s, int len) {
	if (type.length != 1) {
		errorf("Protocol with length %d unparseable", type.length);
		return -1;
		}
	switch (data[0]) {
		case IPFIX_protocolIdentifier_ICMP:
			snprintf(s, len, "ICMP");
			return 0;
		case IPFIX_protocolIdentifier_TCP:
			snprintf(s, len, "TCP");
			return 0;
		case IPFIX_protocolIdentifier_UDP: 
			snprintf(s, len, "UDP");
			return 0;
		case IPFIX_protocolIdentifier_RAW: 
			snprintf(s, len, "RAW");
			return 0;
		default:
			snprintf(s, len, "%u", data[0]);
			return 0;
		}
	}

static int uintToString(FieldType type, FieldData* data, char* s, int len) {
	switch (type.length) {
		case 1:
			snprintf(s, len, "%hhu", *(uint8_t*)data);
			return 0;
		case 2:
			snprintf(s, len, "%hu", ntohs(*(uint16_t*)data));
			return 0;
		case 4:
			snprintf(s, len, "%u", ntohl(*(uint32_t*)data));
			return 0;
		case 8:
			snprintf(s, len, "%Lu", ntohll(*(uint64_t*)data));
			return 0;
		default:
			errorf("Uint with length %d unparseable", type.length);
			return -1;
		}
	}

/**
 * Get estimated string length of fields of type @c type
 */
static int fieldTypeToStringLength(FieldType type) {
	switch (type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
			return 3;
			break;
		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
			return 18;
			break;				
		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationtransportPort:
			return 11;
			break;
		default:
			return 5;
			break;
		}
	}

/**
 * Store string representation of field type in @c type in @c s.
 * Will store no more than @c len bytes
 * @return 0 on success, -1 otherwise
 */
static int fieldTypeToString(FieldType type, char* s, int len) {
	switch (type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
			strncpy(s, "pto", len);
			break;
		case IPFIX_TYPEID_sourceIPv4Address:
			strncpy(s, "srcIP", len);
			break;
		case IPFIX_TYPEID_destinationIPv4Address:
			strncpy(s, "dstIP", len);
			break;				
		case IPFIX_TYPEID_sourceTransportPort:
			strncpy(s, "sPort", len);
			break;
		case IPFIX_TYPEID_destinationtransportPort:
			strncpy(s, "dPort", len);
			break;
		case IPFIX_TYPEID_inPacketDeltaCount:
			strncpy(s, "pckts", len);
			break;
		case IPFIX_TYPEID_inOctetDeltaCount:
			strncpy(s, "bytes", len);
			break;
		default:
			snprintf(s, len, "%u", type.id);
			break;
		}

	return 0;
	}

/**
 * Store string representation of field data in @c data in @c s.
 * Will store no more than @c len bytes
 * @return 0 on success, -1 otherwise
 */
static int fieldToString(FieldType type, FieldData* data, char* s, int len) {
	switch (type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
			return protocolToString(type, data, s, len);
			break;
		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
			return ipv4ToString(type, data, s, len);
			break;				
		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationtransportPort:
			return portToString(type, data, s, len);
			break;
		default:
			return uintToString(type, data, s, len);
			break;
		}
	}

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixPrinters() {
	return 0;
	}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixPrinters() {
	return 0;
	}

/**
 * Creates a new IpfixPrinter. Do not forget to call @c startIpfixPrinter() to begin printing
 * @return handle to use when calling @c destroyIpfixPrinter()
 */
IpfixPrinter* createIpfixPrinter() {
	IpfixPrinter* ipfixPrinter = (IpfixPrinter*)malloc(sizeof(IpfixPrinter));
	ipfixPrinter->lastTemplate = 0;	

	return ipfixPrinter;
	}

/**
 * Frees memory used by an IpfixPrinter
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void destroyIpfixPrinter(IpfixPrinter* ipfixPrinter) {
	free(ipfixPrinter);
	}

/**
 * Starts or resumes printing messages
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void startIpfixPrinter(IpfixPrinter* ipfixPrinter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	}

/**
 * Temporarily pauses printing messages
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void stopIpfixPrinter(IpfixPrinter* ipfixPrinter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	}

/**
 * Prints a Template
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printTemplate(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo) {
	int i;
	int j;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter;
	myIpfixPrinter->lastTemplate = templateInfo;

	int lineLen = 0;
	for (i = 0; i < templateInfo->fieldCount; i++) {
		if (i > 0) { printf("|"); lineLen++; }
		FieldType ftype = templateInfo->fieldInfo[i].type;
		int flen = fieldTypeToStringLength(ftype); lineLen+=flen;

		if (fieldTypeToString(ftype, buf, 40) == -1) {
			error("Error converting fieldType");
			continue;
			}

		for (j = 0; j < flen-strlen(buf); j++) printf(" ");
		printf("%s", buf);
		}
	printf("\n");

	for (j = 0; j < lineLen; j++) printf("=");
	printf("\n");

	return 0;
	}

/**
 * Prints a Template that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printTemplateDestruction(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo) {
	printf("Destroyed a Template\n");

	return 0;
	}
	
/**
 * Prints a DataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printDataRecord(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data) {
	int i;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter;

	if (myIpfixPrinter->lastTemplate != templateInfo) {
		printf("\n");
		printTemplate(ipfixPrinter, sourceID, templateInfo);
		}

	for (i = 0; i < templateInfo->fieldCount; i++) {
		if (i > 0) printf("|");
		FieldType ftype = templateInfo->fieldInfo[i].type;
		FieldData* fdata = data + templateInfo->fieldInfo[i].offset;
		int flen = fieldTypeToStringLength(ftype);

		if (fieldToString(ftype, fdata, buf, 40) == -1) {
			error("Error converting field");
			continue;
			}

		int j;
		for (j = 0; j < flen-strlen(buf); j++) printf(" ");
		printf("%s", buf);
		}
	printf("\n");

	return 0;
	}

/**
 * Prints a OptionsTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printOptionsTemplate(void* ipfixPrinter, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo) {
	printf("Got an OptionsTemplate\n");

	return 0;
	}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printOptionsTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo) {
	printf("Destroyed an OptionsTemplate\n");

	return 0;
	}
	
/**
 * Prints an OptionsRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printOptionsRecord(void* ipfixPrinter, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data) {
	printf("Got an OptionsDataRecord\n");

	return 0;
	}

/**
 * Prints a DataTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printDataTemplate(void* ipfixPrinter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	int i;
	int j;
	int lineLen;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter;
	myIpfixPrinter->lastTemplate = dataTemplateInfo;

	lineLen = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		if (i > 0) { printf("|"); lineLen++; }
		FieldType ftype = dataTemplateInfo->dataInfo[i].type;
		int flen = fieldTypeToStringLength(ftype); lineLen+=flen;

		if (fieldTypeToString(ftype, buf, 40) == -1) {
			error("Error converting fieldType");
			continue;
			}

		for (j = 0; j < flen-strlen(buf); j++) printf(" ");
		printf("%s", buf);
		}
	printf("\n");

	for (j = 0; j < lineLen; j++) printf("-");
	printf("\n");

	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		if (i > 0) printf("|");
		FieldType ftype = dataTemplateInfo->dataInfo[i].type;
		FieldData* fdata = dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset;
		int flen = fieldTypeToStringLength(ftype);

		if (fieldToString(ftype, fdata, buf, 40) == -1) {
			error("Error converting field");
			continue;
			}

		int j;
		for (j = 0; j < flen-strlen(buf); j++) printf(" ");
		printf("%s", buf);
		}
	printf("\n");

	lineLen = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		if (i > 0) { printf("|"); lineLen++; }
		FieldType ftype = dataTemplateInfo->fieldInfo[i].type;
		int flen = fieldTypeToStringLength(ftype); lineLen+=flen;

		if (fieldTypeToString(ftype, buf, 40) == -1) {
			error("Error converting fieldType");
			continue;
			}

		for (j = 0; j < flen-strlen(buf); j++) printf(" ");
		printf("%s", buf);
		}
	printf("\n");

	for (j = 0; j < lineLen; j++) printf("=");
	printf("\n");

	return 0;
	}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printDataTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	printf("Destroyed a DataTemplate\n");

	return 0;
	}
	
/**
 * Prints a DataDataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printDataDataRecord(void* ipfixPrinter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	int i;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter;

	if (myIpfixPrinter->lastTemplate != dataTemplateInfo) {
		printf("\n");
		printDataTemplate(ipfixPrinter, sourceID, dataTemplateInfo);
		}

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		if (i > 0) printf("|");
		FieldType ftype = dataTemplateInfo->fieldInfo[i].type;
		FieldData* fdata = data + dataTemplateInfo->fieldInfo[i].offset;
		int flen = fieldTypeToStringLength(ftype);

		if (fieldToString(ftype, fdata, buf, 40) == -1) {
			error("Error converting field");
			continue;
			}

		int j;
		for (j = 0; j < flen-strlen(buf); j++) printf(" ");
		printf("%s", buf);
		}
	printf("\n");

	return 0;
	}

CallbackInfo getIpfixPrinterCallbackInfo(IpfixPrinter* ipfixPrinter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixPrinter;
	ci.templateCallbackFunction = printTemplate;
	ci.dataRecordCallbackFunction = printDataRecord;
	ci.templateDestructionCallbackFunction = printTemplateDestruction;
	ci.optionsTemplateCallbackFunction = printOptionsTemplate;
	ci.optionsRecordCallbackFunction = printOptionsRecord;
	ci.optionsTemplateDestructionCallbackFunction = printOptionsTemplateDestruction;
	ci.dataTemplateCallbackFunction = printDataTemplate;
	ci.dataDataRecordCallbackFunction = printDataDataRecord;
	ci.dataTemplateDestructionCallbackFunction = printDataTemplateDestruction;
	return ci;
	}
