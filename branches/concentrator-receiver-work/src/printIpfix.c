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
			if (i > 0) {
				strncat(s, ",", len - strlen(s));
			}
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
		snprintf(s, len, "IC~");
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

static int timeToString(FieldType type, FieldData* data, char* s, int len) {
	time_t ttnow;
	struct tm tmnow;
	time_t tt;
	struct tm tm;
	uint32_t tstamp = 0;
	size_t r;
	
	if (type.length > 4) {
		errorf("Timestamp with length %d unparseable", type.length);
		return -1;
	}
	if (type.length < 4) {
		debugf("Padding timestamp of length %d", type.length);
	}
	
	ttnow = time(0);
	memcpy(&tstamp, data, type.length); tstamp = ntohl(tstamp); tt = tstamp;
	
	
	if (!localtime_r(&ttnow, &tmnow)) {
		error("CurrentTime-to-Localtime conversion failed");
		return -1;
	}

	if (!localtime_r(&tt, &tm)) {
		error("Timestamp-to-Localtime conversion failed");
		return -1;
	}
	
	if (tmnow.tm_year != tm.tm_year) {
		r = strftime(s, len, "%Y", &tm);
	} 
	else if ((tmnow.tm_mon != tm.tm_mon) || (tmnow.tm_mday != tm.tm_mday)) {
		char* fmt = "%y-%m-%d";
		r = strftime(s, len, fmt, &tm);
	}
	else {
		r = strftime(s, len, "%H:%M:%S", &tm);
	}
	
	if (r == 0) {
		error("Localtime-to-String conversion failed");
		return -1;
	}
	
	return 0;
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
	case IPFIX_TYPEID_flowCreationTime:
	case IPFIX_TYPEID_flowEndTime:
		return 8;
		break;
	case IPFIX_TYPEID_sourceIPv4Mask:
	case IPFIX_TYPEID_destinationIPv4Mask:
		return 2;
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
	case IPFIX_TYPEID_flowCreationTime:
		strncpy(s, "cTime", len);
		break;
	case IPFIX_TYPEID_flowEndTime:
		strncpy(s, "eTime", len);
		break;
	case IPFIX_TYPEID_sourceIPv4Mask:
		strncpy(s, "sM", len);
		break;
	case IPFIX_TYPEID_destinationIPv4Mask:
		strncpy(s, "dM", len);
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
	case IPFIX_TYPEID_flowCreationTime:
	case IPFIX_TYPEID_flowEndTime:
		return timeToString(type, data, s, len);
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

	ipfixPrinter->stream = stdout;

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
 * Assignes stream to ipfixPrinter
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param stream will be assigned to ipfixprinter
 */
void setIpfixPrinterStream(IpfixPrinter* ipfixPrinter, FILE* stream) {
	ipfixPrinter->stream = stream;
}

/**
 * Prints a Template
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param templateInfo Pointer to a structure defining the Template used
 */
int printTemplate(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo) {
	int i;
	int j;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter;
	myIpfixPrinter->lastTemplate = templateInfo;
	
	fprintf(myIpfixPrinter->stream, "\n\n");

	int lineLen = 0;
	for (i = 0; i < templateInfo->fieldCount; i++) {
		if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
			lineLen++;
		}
		FieldType ftype = templateInfo->fieldInfo[i].type;
		int flen = fieldTypeToStringLength(ftype); lineLen+=flen;

		if (fieldTypeToString(ftype, buf, 40) == -1) {
			error("Error converting fieldType");
			continue;
		}
		
		if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++){
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
		fprintf(myIpfixPrinter->stream, "%s", buf);
	}
	fprintf(myIpfixPrinter->stream, "\n");
	
	for (j = 0; j < lineLen; j++) fprintf(myIpfixPrinter->stream, "=");
	fprintf(myIpfixPrinter->stream, "\n");
	
	return 0;
}

/**
 * Prints a Template that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param templateInfo Pointer to a structure defining the Template used
 */
int printTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, TemplateInfo* templateInfo) {
	IpfixPrinter* ipfixPrinter = ipfixPrinter_;
	fprintf(ipfixPrinter->stream, "\n\nDestroyed a Template\n");
	
	return 0;
}
	
/**
 * Prints a DataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param templateInfo Pointer to a structure defining the Template used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printDataRecord(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data) {
	int i;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter;

	if (myIpfixPrinter->lastTemplate != templateInfo) {
		printTemplate(ipfixPrinter, sourceID, templateInfo);
	}

	for (i = 0; i < templateInfo->fieldCount; i++) {
		if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
		}
		FieldType ftype = templateInfo->fieldInfo[i].type;
		FieldData* fdata = data + templateInfo->fieldInfo[i].offset;
		int flen = fieldTypeToStringLength(ftype);
		
		if (fieldToString(ftype, fdata, buf, 40) == -1) {
			error("Error converting field");
			continue;
		}

		int j;
		if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
		fprintf(myIpfixPrinter->stream, "%s", buf);
	}
	fprintf(myIpfixPrinter->stream, "\n");
	
	return 0;
}

/**
 * Prints a OptionsTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 */
int printOptionsTemplate(void* ipfixPrinter_, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo) {
        int i;
        int j;
        char buf[40];
        IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter_;
        myIpfixPrinter->lastTemplate = optionsTemplateInfo;
	
        fprintf(myIpfixPrinter->stream, "\n\n");
        fprintf(myIpfixPrinter->stream, "Options Template:\n");
	
        int lineLen = 0;
        for (i = 0; i < optionsTemplateInfo->scopeCount; i++) {
                if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
			lineLen++;
		}
                FieldType ftype = optionsTemplateInfo->scopeInfo[i].type;
                int flen = fieldTypeToStringLength(ftype); lineLen+=flen;

                if (fieldTypeToString(ftype, buf, 40) == -1) {
                        fprintf(myIpfixPrinter->stream, "Error converting fieldType");
                        continue;
		}

                if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
                fprintf(myIpfixPrinter->stream, "%s", buf);
	}
        fprintf(myIpfixPrinter->stream, "||"); lineLen+=2;
        for (i = 0; i < optionsTemplateInfo->fieldCount; i++) {
                if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
			lineLen++;
		}
                FieldType ftype = optionsTemplateInfo->fieldInfo[i].type;
                int flen = fieldTypeToStringLength(ftype); lineLen+=flen;
		
                if (fieldTypeToString(ftype, buf, 40) == -1) {
                        fprintf(myIpfixPrinter->stream, "Error converting fieldType");
                        continue;
		}

                if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
                fprintf(myIpfixPrinter->stream, "%s", buf);
	}
        fprintf(myIpfixPrinter->stream, "\n");
	
        for (j = 0; j < lineLen; j++) {
		fprintf(myIpfixPrinter->stream, "=");
	}
        fprintf(myIpfixPrinter->stream, "\n");

        return 0;

}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 */
int printOptionsTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo) {
	IpfixPrinter* ipfixPrinter = (IpfixPrinter*)ipfixPrinter_;
	fprintf(ipfixPrinter->stream, "\n\nDestroyed an OptionsTemplate\n");

	return 0;
}

	
/**
 * Prints an OptionsRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printOptionsRecord(void* ipfixPrinter_, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data) {
        int i;
        char buf[40];
        IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter_;
	
        if (myIpfixPrinter->lastTemplate != optionsTemplateInfo) {
                printOptionsTemplate(myIpfixPrinter, sourceID, optionsTemplateInfo);
	}
	
        for (i = 0; i < optionsTemplateInfo->scopeCount; i++) {
                if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
		}
                FieldType ftype = optionsTemplateInfo->scopeInfo[i].type;
                FieldData* fdata = data + optionsTemplateInfo->scopeInfo[i].offset;
                int flen = fieldTypeToStringLength(ftype);

                if (fieldToString(ftype, fdata, buf, 40) == -1) {
                        fprintf(myIpfixPrinter->stream, "Error converting field");
                        continue;
		}

                int j;
                if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
                fprintf(myIpfixPrinter->stream, "%s", buf);
	}
        fprintf(myIpfixPrinter->stream, "||");
        for (i = 0; i < optionsTemplateInfo->fieldCount; i++) {
                if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
		}
                FieldType ftype = optionsTemplateInfo->fieldInfo[i].type;
                FieldData* fdata = data + optionsTemplateInfo->fieldInfo[i].offset;
                int flen = fieldTypeToStringLength(ftype);
		
                if (fieldToString(ftype, fdata, buf, 40) == -1) {
                        fprintf(myIpfixPrinter->stream, "Error converting field");
                        continue;
		}
		
                int j;
                if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
                fprintf(myIpfixPrinter->stream, "%s", buf);
	}
        fprintf(myIpfixPrinter->stream, "\n");
	
        return 0;
}

/**
 * Prints a DataTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printDataTemplate(void* ipfixPrinter_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	int i;
	int j;
	int lineLen;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter_;
	myIpfixPrinter->lastTemplate = dataTemplateInfo;
	
	fprintf(myIpfixPrinter->stream, "\n\n");
	fprintf(myIpfixPrinter->stream, "Data Template\n");
	
	lineLen = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
			lineLen++; 
		}
		FieldType ftype = dataTemplateInfo->dataInfo[i].type;
		int flen = fieldTypeToStringLength(ftype); lineLen+=flen;

		if (fieldTypeToString(ftype, buf, 40) == -1) {
			error("Error converting fieldType");
			continue;
		}
		
		if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
		fprintf(myIpfixPrinter->stream, "%s", buf);
	}
	fprintf(myIpfixPrinter->stream, "\n");
	
	for (j = 0; j < lineLen; j++) {
		fprintf(myIpfixPrinter->stream, "-");
	}
	fprintf(myIpfixPrinter->stream, "\n");

	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
		}
		FieldType ftype = dataTemplateInfo->dataInfo[i].type;
		FieldData* fdata = dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset;
		int flen = fieldTypeToStringLength(ftype);

		if (fieldToString(ftype, fdata, buf, 40) == -1) {
			error("Error converting field");
			continue;
		}
		
		int j;
		if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
		fprintf(myIpfixPrinter->stream, "%s", buf);
	}
	fprintf(myIpfixPrinter->stream, "\n");

	fprintf(myIpfixPrinter->stream, "\n");

	lineLen = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
			lineLen++;
		}
		FieldType ftype = dataTemplateInfo->fieldInfo[i].type;
		int flen = fieldTypeToStringLength(ftype); lineLen+=flen;
		
		if (fieldTypeToString(ftype, buf, 40) == -1) {
			error("Error converting fieldType");
			continue;
		}
		
		if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
		fprintf(myIpfixPrinter->stream, "%s", buf);
	}
	fprintf(myIpfixPrinter->stream, "\n");
	
	for (j = 0; j < lineLen; j++) {
		fprintf(myIpfixPrinter->stream, "=");
	}
	fprintf(myIpfixPrinter->stream, "\n");
	
	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printDataTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	IpfixPrinter* ipfixPrinter = (IpfixPrinter*)ipfixPrinter_;
	fprintf(ipfixPrinter->stream, "\n\nDestroyed a DataTemplate\n");
	
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
int printDataDataRecord(void* ipfixPrinter_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	int i;
	char buf[40];
	IpfixPrinter* myIpfixPrinter = (IpfixPrinter*)ipfixPrinter_;
	
	if (myIpfixPrinter->lastTemplate != dataTemplateInfo) {
		fprintf(myIpfixPrinter->stream, "\n");
		printDataTemplate(myIpfixPrinter, sourceID, dataTemplateInfo);
	}
	
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		if (i > 0) {
			fprintf(myIpfixPrinter->stream, "|");
		}
		FieldType ftype = dataTemplateInfo->fieldInfo[i].type;
		FieldData* fdata = data + dataTemplateInfo->fieldInfo[i].offset;
		int flen = fieldTypeToStringLength(ftype);

		if (fieldToString(ftype, fdata, buf, 40) == -1) {
			error("Error converting field");
			continue;
		}
		
		int j;
		if (strlen(buf) < flen) {
			for (j = 0; j < flen-strlen(buf); j++) {
				fprintf(myIpfixPrinter->stream, " ");
			}
		}
		fprintf(myIpfixPrinter->stream, "%s", buf);
	}
	fprintf(myIpfixPrinter->stream, "\n");
	
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
