/******************************************************************************

IPFIX Collector module
Copyright (C) 2004 Christoph Sommer
http://www.deltadevelopment.de/users/christoph/ipfix

FIXME: Basic support for NetflowV9 packets, templates and flow records
is provided. Will break when fed options templates or field
types with type IDs >= 32768 (highest bit set).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

******************************************************************************/

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rcvIpfix.h"
#include "templateBuffer.h"
#include "rcvMessage.h"

/***** Constants ************************************************************/

#define SUPPORT_NETFLOWV9

#define NetflowV9_SetId_Template  0

/***** Data Types ************************************************************/

/**
 * IPFIX header helper.
 * Constitutes the first 16 bytes of every IPFIX Message
 */
typedef struct {
	uint16 version;			/**< Expected to be 0x000a */
	uint16 length; 
	uint32 exportTime;
	uint32 sequenceNo;
	uint32 sourceId;
	byte   data;
	} IpfixHeader;

/**
 * NetflowV9 header helper.
 * Constitutes the first bytes of every NetflowV9 Message
 */
typedef struct {
	uint16 version;                 /**< Expected to be 0x0009 */
	uint16 setCount;
	uint32 uptime;
	uint32 exportTime;
	uint32 sequenceNo;
	uint32 sourceId;
	byte   data;
	} NetflowV9Header;
                                                                  
/**
 * IPFIX "Set" helper.
 * Constitutes the first bytes of every IPFIX Template Set, Options Template Set or Data Set
 */
typedef struct {
	uint16 id;
	uint16 length;
 	byte data; 
	} IpfixSetHeader;

/**
 * IPFIX "Template Set" helper.
 * Constitutes the first bytes of every IPFIX Template
 */
typedef struct {
	uint16 templateId;
	uint16 fieldCount;
	byte data;
	} IpfixTemplateHeader;

/**
 * IPFIX "DataTemplate Set" helper.
 * Constitutes the first bytes of every IPFIX DataTemplate
 */
typedef struct {
	uint16 templateId;
	uint8 fieldCount;
	uint8 dataCount;
	byte data;
	} IpfixDataTemplateHeader;

/**
 * IPFIX "Options Template Set" helper.
 * Constitutes the first bytes of every IPFIX Options Template
 */
typedef struct {
	uint16 templateId;
	uint16 fieldCount;
	uint16 scopeCount; 
	byte data;
	} IpfixOptionsTemplateHeader;

/***** Global Variables ******************************************************/

static TemplateCallbackFunction* templateCallbackFunction;
static DataTemplateCallbackFunction* dataTemplateCallbackFunction;
static OptionsTemplateCallbackFunction* optionsTemplateCallbackFunction;

static TemplateDestructionCallbackFunction* templateDestructionCallbackFunction;
static DataTemplateDestructionCallbackFunction* dataTemplateDestructionCallbackFunction;
static OptionsTemplateDestructionCallbackFunction* optionsTemplateDestructionCallbackFunction;
  	   	 
static DataRecordCallbackFunction* dataRecordCallbackFunction;
static OptionsRecordCallbackFunction* optionsRecordCallbackFunction;
static DataDataRecordCallbackFunction* dataDataRecordCallbackFunction;

/***** Internal Functions ****************************************************/

static void processDataSet(SourceID sourceID, IpfixSetHeader* set);
static void processTemplateSet(SourceID sourceID, IpfixSetHeader* set);
static void processDataTemplateSet(SourceID sourceID, IpfixSetHeader* set);
static void processOptionsTemplateSet(SourceID sourceId, IpfixSetHeader* set);

/**
 * Processes an IPFIX template set.
 * Called by processMessage
 */
static void processTemplateSet(SourceID sourceId, IpfixSetHeader* set) {
	IpfixTemplateHeader* th = (IpfixTemplateHeader*)&set->data;
	byte* endOfSet = (byte*)set + ntoh(set->length);
	byte* record = (byte*)&th->data;
	/* TemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		BufferedTemplate* bt = (BufferedTemplate*)malloc(sizeof(BufferedTemplate));
		TemplateInfo* ti = (TemplateInfo*)malloc(sizeof(TemplateInfo));
		bt->sourceID = sourceId;
		bt->templateID = ntoh(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntoh(set->id);
		bt->templateInfo = ti;
		bt->templateDestructionCallbackFunction = templateDestructionCallbackFunction;
		ti->userData = 0;
		ti->fieldCount = ntoh(th->fieldCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		boolean lengthVarying = false;
		uint16 fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].typeX.id = ntoh(*(uint16*)((byte*)record+0));
			ti->fieldInfo[fieldNo].typeX.length = ntoh(*(uint16*)((byte*)record+2));
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].typeX.length;
			if (ti->fieldInfo[fieldNo].typeX.length == 65535) lengthVarying=true;
			if (ti->fieldInfo[fieldNo].typeX.id & 0x80) {
				ti->fieldInfo[fieldNo].typeX.eid = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[fieldNo].typeX.eid = 0;
				record = (byte*)((byte*)record+4);
				}
			}
		if (lengthVarying) {
  			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) ti->fieldInfo[fieldNo].offset = 65535;
  			}
		bufferTemplate(bt); bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;
		if (templateCallbackFunction != 0) {
			templateCallbackFunction(sourceId, ti);
			}
  		}
	}

/**
 * Processes an IPFIX Options Template Set.
 * Called by processMessage
 */
static void processOptionsTemplateSet(SourceID sourceId, IpfixSetHeader* set) {
	IpfixOptionsTemplateHeader* th = (IpfixOptionsTemplateHeader*)&set->data;
	byte* endOfSet = (byte*)set + ntoh(set->length);
	byte* record = (byte*)&th->data;
	/* OptionsTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		BufferedTemplate* bt = (BufferedTemplate*)malloc(sizeof(BufferedTemplate));
		OptionsTemplateInfo* ti = (OptionsTemplateInfo*)malloc(sizeof(OptionsTemplateInfo));
		bt->sourceID = sourceId;
		bt->templateID = ntoh(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntoh(set->id);
		bt->optionsTemplateInfo = ti;
		bt->optionsTemplateDestructionCallbackFunction = optionsTemplateDestructionCallbackFunction;
		ti->userData = 0;
		ti->scopeCount = ntoh(th->scopeCount);
		ti->scopeInfo = (FieldInfo*)malloc(ti->scopeCount * sizeof(FieldInfo));
		ti->fieldCount = ntoh(th->fieldCount)-ntoh(th->scopeCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		boolean lengthVarying = false;
		uint16 scopeNo;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
			ti->scopeInfo[scopeNo].typeX.id = ntoh(*(uint16*)((byte*)record+0));
			ti->scopeInfo[scopeNo].typeX.length = ntoh(*(uint16*)((byte*)record+2));
			ti->scopeInfo[scopeNo].offset = bt->recordLength; bt->recordLength+=ti->scopeInfo[scopeNo].typeX.length;
			if (ti->scopeInfo[scopeNo].typeX.length == 65535) lengthVarying=true;
			if (ti->scopeInfo[scopeNo].typeX.id & 0x80) {
				ti->scopeInfo[scopeNo].typeX.eid = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[scopeNo].typeX.eid = 0;
				record = (byte*)((byte*)record+4);
				}
			}
		uint16 fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].typeX.id = ntoh(*(uint16*)((byte*)record+0));
			ti->fieldInfo[fieldNo].typeX.length = ntoh(*(uint16*)((byte*)record+2));
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].typeX.length;
			if (ti->fieldInfo[fieldNo].typeX.length == 65535) lengthVarying=true;
			if (ti->fieldInfo[fieldNo].typeX.id & 0x80) {
				ti->fieldInfo[fieldNo].typeX.eid = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[fieldNo].typeX.eid = 0;
				record = (byte*)((byte*)record+4);
				}
			}
		if (lengthVarying) {
  			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->scopeCount; fieldNo++) ti->scopeInfo[fieldNo].offset = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) ti->fieldInfo[fieldNo].offset = 65535;
  			}
		bufferTemplate(bt); bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;
		if (optionsTemplateCallbackFunction != 0) {
			optionsTemplateCallbackFunction(sourceId, ti);
			}
  		}
	}

/**
 * Processes an IPFIX DataTemplate set.
 * Called by processMessage
 */
static void processDataTemplateSet(SourceID sourceId, IpfixSetHeader* set) {
	IpfixDataTemplateHeader* th = (IpfixDataTemplateHeader*)&set->data;
	byte* endOfSet = (byte*)set + ntoh(set->length);
	byte* record = (byte*)&th->data;
	/* DataTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		BufferedTemplate* bt = (BufferedTemplate*)malloc(sizeof(BufferedTemplate));
		DataTemplateInfo* ti = (DataTemplateInfo*)malloc(sizeof(DataTemplateInfo));
		bt->sourceID = sourceId;
		bt->templateID = ntoh(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntoh(set->id);
		bt->dataTemplateInfo = ti;
		bt->dataTemplateDestructionCallbackFunction = dataTemplateDestructionCallbackFunction;
		ti->userData = 0;
		ti->fieldCount = th->fieldCount;
		ti->dataCount = th->dataCount;
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		boolean lengthVarying = false;
		uint16 fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].typeX.id = ntoh(*(uint16*)((byte*)record+0));
			ti->fieldInfo[fieldNo].typeX.length = ntoh(*(uint16*)((byte*)record+2));
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].typeX.length;
			if (ti->fieldInfo[fieldNo].typeX.length == 65535) lengthVarying=true;
			if (ti->fieldInfo[fieldNo].typeX.id & 0x80) {
				ti->fieldInfo[fieldNo].typeX.eid = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[fieldNo].typeX.eid = 0;
				record = (byte*)((byte*)record+4);
				}
			}
		if (lengthVarying) {
  			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) ti->fieldInfo[fieldNo].offset = 65535;
  			}

		ti->dataInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].typeX.id = ntoh(*(uint16*)((byte*)record+0));
			ti->dataInfo[fieldNo].typeX.length = ntoh(*(uint16*)((byte*)record+2));
			if (ti->dataInfo[fieldNo].typeX.id & 0x80) {
				ti->dataInfo[fieldNo].typeX.eid = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->dataInfo[fieldNo].typeX.eid = 0;
				record = (byte*)((byte*)record+4);
				}
			ti->dataInfo[fieldNo].offset = (record - &th->data);
			if (ti->dataInfo[fieldNo].typeX.length == 65535) {
				if (*(byte*)record < 255) {
					ti->dataInfo[fieldNo].typeX.length = *(byte*)record;
					} else {
					ti->dataInfo[fieldNo].typeX.length = *(uint16*)(record+1);
					}
				}
			record = record + ti->dataInfo[fieldNo].typeX.length;
			}

		// Copy fixed data block
		int dataLength = (record - &th->data);
		ti->data = (byte*)malloc(dataLength);
		memcpy(ti->data,&th->data,dataLength);

		bufferTemplate(bt); bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;
		if (dataTemplateCallbackFunction != 0) {
			dataTemplateCallbackFunction(sourceId, ti);
			}
  		}
	}

/**
 * Processes an IPFIX data set.
 * Called by processMessage
 */
static void processDataSet(SourceID sourceId, IpfixSetHeader* set) {
	BufferedTemplate* bt = getBufferedTemplate(sourceId, ntoh(set->id));

	if (bt == 0) {
		errorf("Template %d unknown to collecting process", ntoh(set->id));
		return;
		}
	
	#ifdef SUPPORT_NETFLOWV9
	if ((bt->setID == IPFIX_SetId_Template) || (bt->setID == NetflowV9_SetId_Template)) {
	#else
	if (bt->setID == IPFIX_SetId_Template) {
	#endif
  		if (dataRecordCallbackFunction == 0) return;

		TemplateInfo* ti = bt->templateInfo;

		uint16 length = ntoh(set->length)-((byte*)(&set->data)-(byte*)set);
		byte* record = &set->data;
	
		if (bt->recordLength < 65535) {
			byte* recordX = record+length;
			/* We stop processing when no full record is left */
			while (record < recordX - (bt->recordLength - 1)) {
				dataRecordCallbackFunction(sourceId, ti, bt->recordLength, record);
				record = record + bt->recordLength;
				}
			} else {
			byte* recordX = record+length;
			/* We assume that all variable-length records are >= 4 byte, so we stop processing when only 3 bytes are left */
			while (record < recordX - 3) {
				int recordLength=0;
				int i;
				for (i = 0; i < ti->fieldCount; i++) {
					int fieldLength = 0;
					if (ti->fieldInfo[i].typeX.length < 65535) {
						fieldLength = ti->fieldInfo[i].typeX.length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->fieldInfo[i].typeX.length = fieldLength;
					ti->fieldInfo[i].offset = recordLength;
					recordLength += fieldLength;
					}
				dataRecordCallbackFunction(sourceId, ti, recordLength, record);
				record = record + recordLength;
				}
			}
		} else 
	if (bt->setID == IPFIX_SetId_OptionsTemplate) {
  		if (optionsRecordCallbackFunction == 0) return;
  	
		OptionsTemplateInfo* ti = bt->optionsTemplateInfo;

		uint16 length = ntoh(set->length)-((byte*)(&set->data)-(byte*)set);
		byte* record = &set->data;
	
		if (bt->recordLength < 65535) {
			byte* recordX = record+length;
			while (record < recordX) {
				optionsRecordCallbackFunction(sourceId, ti, bt->recordLength, record);
				record = record + bt->recordLength;
				}
			} else {
			byte* recordX = record+length;
			while (record < recordX) {
				int recordLength=0;
				int i;
				for (i = 0; i < ti->scopeCount; i++) {
					int fieldLength = 0;
					if (ti->scopeInfo[i].typeX.length < 65535) {
						fieldLength = ti->scopeInfo[i].typeX.length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->scopeInfo[i].typeX.length = fieldLength;
					ti->scopeInfo[i].offset = recordLength;
					recordLength += fieldLength;
					}
				for (i = 0; i < ti->fieldCount; i++) {
					int fieldLength = 0;
					if (ti->fieldInfo[i].typeX.length < 65535) {
						fieldLength = ti->fieldInfo[i].typeX.length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->fieldInfo[i].typeX.length = fieldLength;
					ti->fieldInfo[i].offset = recordLength;
					recordLength += fieldLength;
					}
				optionsRecordCallbackFunction(sourceId, ti, recordLength, record);
				record = record + recordLength;
				}
			}
		} else 
  	if (bt->setID == IPFIX_SetId_DataTemplate) {
  		if (dataDataRecordCallbackFunction == 0) return;
  	
		DataTemplateInfo* ti = bt->dataTemplateInfo;

		uint16 length = ntoh(set->length)-((byte*)(&set->data)-(byte*)set);
		byte* record = &set->data;

		if (bt->recordLength < 65535) {
			byte* recordX = record+length;
			while (record < recordX) {
				dataDataRecordCallbackFunction(sourceId, ti, bt->recordLength, record);
				record = record + bt->recordLength;
				}
			} else {
			byte* recordX = record+length;
			while (record < recordX) {
				int recordLength=0;
				int i;
				for (i = 0; i < ti->fieldCount; i++) {
					int fieldLength = 0;
					if (ti->fieldInfo[i].typeX.length < 65535) {
						fieldLength = ti->fieldInfo[i].typeX.length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->fieldInfo[i].typeX.length = fieldLength;
					ti->fieldInfo[i].offset = recordLength;
					recordLength += fieldLength;
					}
				dataDataRecordCallbackFunction(sourceId, ti, recordLength, record);
				record = record + recordLength;
				}
			}
		} else {
		fatalf("Data Set based on known but unhandled template type %d", bt->setID);
		}
	}
	

boolean processNetflowV9Packet(byte* message, uint16 length) {
	NetflowV9Header* header = (NetflowV9Header*)message;
	
	// pointer to first set
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	int i;
	for (i = 0; i < header->setCount; i++) {
		if (ntoh(set->id) == NetflowV9_SetId_Template) {
  			processTemplateSet(ntoh(header->sourceId), set);
  			} else
		if (ntoh(set->id) >= IPFIX_SetId_Data_Start) {
  			processDataSet(ntoh(header->sourceId), set);
  			} else {
			errorf("Unsupported Set ID - expected 0/256+, got %d", ntoh(set->id));
  			}
		set = (IpfixSetHeader*)((byte*)set + ntoh(set->length));
		}

	return true;
	}

boolean processIpfixPacket(byte* message, uint16 length) {
	IpfixHeader* header = (IpfixHeader*)message;

	if (ntoh(header->length) != length) {
 		errorf("Bad message length - expected %#06x, got %#06x", length, ntoh(header->length));
		return false;
 		}

	// pointer to first set
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	// pointer beyond message
	IpfixSetHeader* setX = (IpfixSetHeader*)((char*)message + length); 

	while (set < setX) {
		if (ntoh(set->id) == IPFIX_SetId_Template) {
  			processTemplateSet(ntoh(header->sourceId), set);
  			} else
		if (ntoh(set->id) == IPFIX_SetId_OptionsTemplate) {
  			processOptionsTemplateSet(ntoh(header->sourceId), set);
  			} else
		if (ntoh(set->id) == IPFIX_SetId_DataTemplate) {
  			processDataTemplateSet(ntoh(header->sourceId), set);
  			} else
		if (ntoh(set->id) >= IPFIX_SetId_Data_Start) {
  			processDataSet(ntoh(header->sourceId), set);
  			} else {
			errorf("Unsupported Set ID - expected 2/3/4/256+, got %d", ntoh(set->id));
  			}
		set = (IpfixSetHeader*)((byte*)set + ntoh(set->length));
		}

	return true;
	}


boolean processMessage(byte* message, uint16 length) {
	IpfixHeader* header = (IpfixHeader*)message;
	if (ntoh(header->version) == 0x000a) {
		return processIpfixPacket(message, length);
		}
	#ifdef SUPPORT_NETFLOWV9
	if (ntoh(header->version) == 0x0009) {
		return processNetflowV9Packet(message, length);
		}
	errorf("Bad message version - expected 0x009 or 0x000a, got %#06x", ntoh(header->version));
	return false;
	#else
	error("Bad message version - expected 0x000a, got %#06x", ntoh(header->version));
	return false;
	#endif
	}
	
static void printIPv4(FieldType type, FieldData* data) {
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
		return;
		}
	
	if (type.length < 5) {
		printf("%d.%d.%d.%d", octet1, octet2, octet3, octet4);
		} else {
		printf("%d.%d.%d.%d/%d", octet1, octet2, octet3, octet4, 32-imask);
		}
	}

static void printPort(FieldType type, FieldData* data) {
	if (type.length == 0) {
		printf("zero-length Port");
		return;
		}
	if (type.length == 2) {
		int port = ((uint16)data[0] << 8)+data[1];
		printf("%d", port);
		return;
		}
	if ((type.length >= 4) && ((type.length % 4) == 0)) {
		int i;
		for (i = 0; i < type.length; i+=4) {
			int starti = ((uint16)data[i+0] << 8)+data[i+1];
			int endi = ((uint16)data[i+2] << 8)+data[i+3];
			if (i > 0) printf(",");
			if (starti != endi) {
				printf("%d:%d", starti, endi);
				} else {
				printf("%d", starti);
				}
			} 
		return;
		}
	
	errorf("Port with length %d unparseable", type.length);
	}

static void printProtocol(FieldType type, FieldData* data) {
	if (type.length != 1) {
		errorf("Protocol with length %d unparseable", type.length);
		return;
		}
	if (data[0] == IPFIX_protocolIdentifier_ICMP) printf("ICMP");
	else if (data[0] == IPFIX_protocolIdentifier_TCP) printf("TCP");
	else if (data[0] == IPFIX_protocolIdentifier_UDP) printf("UDP");
	else if (data[0] == IPFIX_protocolIdentifier_RAW) printf("RAW");
	else printf("unknownProtocol");
	}

static void printUint(FieldType type, FieldData* data) {
	if (type.length == 1) {
		uint32 i = (data[0] << 0);
		printf("%d",i);		
		return;
		}
	if (type.length == 2) {
		uint32 i = (data[0] << 8)+(data[1] << 0);
		printf("%d",i);		
		return;
		}
	if (type.length == 4) {
		uint32 i = (data[0] << 24)+(data[1] << 16)+(data[2] << 8)+(data[3] << 0);
		printf("%d",i);		
		return;
		}
	if (type.length == 8) {
		uint32 i = (data[0] << 24)+(data[1] << 16)+(data[2] << 8)+(data[3] << 0);
		uint32 j = (data[4] << 24)+(data[5] << 16)+(data[6] << 8)+(data[7] << 0);
		printf("%d'%d",i,j);
		return;
		}
	
	errorf("Uint with length %d unparseable", type.length);
	}
	
/***** Exported Functions ****************************************************/

boolean initializeRcvIpfix() {
	initializeTemplateBuffer();
	templateCallbackFunction = 0;
	dataTemplateCallbackFunction = 0;
	optionsTemplateCallbackFunction = 0;
	templateDestructionCallbackFunction = 0;
	dataTemplateDestructionCallbackFunction = 0;
	optionsTemplateDestructionCallbackFunction = 0;
	dataRecordCallbackFunction = 0;
	optionsRecordCallbackFunction = 0;
	dataDataRecordCallbackFunction = 0;
	initializeRcvMessage();
	setMessageCallback(processMessage);

	return true;
	}

boolean deinitializeRcvIpfix() {
	deinitializeRcvMessage();
	deinitializeTemplateBuffer();

	return true;
	}

void printFieldData(FieldType type, FieldData* pattern) {
	char* s;
	
	switch (type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
			printf("protocolIdentifier:");
			printProtocol(type, pattern);
			break;
		case IPFIX_TYPEID_sourceIPv4Address:
			printf("sourceIPv4Address:");
			printIPv4(type, pattern);
			break;
		case IPFIX_TYPEID_destinationIPv4Address:
			printf("destinationIPv4Address:");
			printIPv4(type, pattern);
			break;				
		case IPFIX_TYPEID_sourceTransportPort:
			printf("sourceTransportPort:");
			printPort(type, pattern);
			break;
		case IPFIX_TYPEID_destinationtransportPort:
			printf("destinationtransportPort:");
			printPort(type, pattern);
			break;
		default:
			s = typeid2string(type.id);
			if (s != NULL) {
				printf("%s:", s);
				printUint(type, pattern);
				} else {
				errorf("Field with ID %d unparseable", type.id);
				}
			break;
		}
	}

FieldInfo* getTemplateFieldInfo(TemplateInfo* ti, FieldType* type) {
	int i;
	
	for (i = 0; i < ti->fieldCount; i++) {
		if ((ti->fieldInfo[i].typeX.id == type->id) && (ti->fieldInfo[i].typeX.eid == type->eid)) {
			return &ti->fieldInfo[i];
			}
		}

	return NULL;		
	}

FieldInfo* getDataTemplateFieldInfo(DataTemplateInfo* ti, FieldType* type) {
	int i;
	
	for (i = 0; i < ti->fieldCount; i++) {
		if ((ti->fieldInfo[i].typeX.id == type->id) && (ti->fieldInfo[i].typeX.eid == type->eid)) {
			return &ti->fieldInfo[i];
			}
		}

	return NULL;		
	}

FieldInfo* getDataTemplateDataInfo(DataTemplateInfo* ti, FieldType* type) {
	int i;
	
	for (i = 0; i < ti->dataCount; i++) {
		if ((ti->dataInfo[i].typeX.id == type->id) && (ti->dataInfo[i].typeX.eid == type->eid)) {
			return &ti->dataInfo[i];
			}
		}

	return NULL;		
	}
	
	
void setTemplateCallback(TemplateCallbackFunction *f) {
	templateCallbackFunction = f;
	}

void setOptionsTemplateCallback(OptionsTemplateCallbackFunction *f) {
	optionsTemplateCallbackFunction = f;
	}

void setDataTemplateCallback(DataTemplateCallbackFunction *f) {
	dataTemplateCallbackFunction = f;
	}


void setTemplateDestructionCallback(TemplateDestructionCallbackFunction *f) {
	templateDestructionCallbackFunction = f;
	}

void setOptionsTemplateDestructionCallback(OptionsTemplateDestructionCallbackFunction *f) {
	optionsTemplateDestructionCallbackFunction = f;
	}

void setDataTemplateDestructionCallback(DataTemplateDestructionCallbackFunction *f) {
	dataTemplateDestructionCallbackFunction = f;
	}


void setDataRecordCallback(DataRecordCallbackFunction *f) {
	dataRecordCallbackFunction = f;
	}

void setOptionsRecordCallback(OptionsRecordCallbackFunction *f) {
	optionsRecordCallbackFunction = f;
	}

void setDataDataRecordCallback(DataDataRecordCallbackFunction *f) {
	dataDataRecordCallbackFunction = f;
	}

int rcvIpfixUdpIpv4(uint16 port) {
	return rcvMessageUdpIpv4(port);
	}

void startRcvIpfix() {
	startRcvMessage();
	}
	
void stopRcvIpfix() {
	stopRcvMessage();
	}

void rcvIpfixClose(int handle) {
	rcvMessageClose(handle);
	}
