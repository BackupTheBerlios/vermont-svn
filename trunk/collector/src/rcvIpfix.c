/******************************************************************************

IPFIX Collector module
Copyright (C) 2004 Christoph Sommer
http://www.deltadevelopment.de/users/christoph/ipfix

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

static DataRecordCallbackFunction* dataRecordCallbackFunction;
static OptionsRecordCallbackFunction* optionsRecordCallbackFunction;
static DataDataRecordCallbackFunction* dataDataRecordCallbackFunction;

/***** Internal Functions ****************************************************/

void processDataSet(SourceID sourceID, IpfixSetHeader* set);
void processTemplateSet(SourceID sourceID, IpfixSetHeader* set);
void processDataTemplateSet(SourceID sourceID, IpfixSetHeader* set);
void processOptionsTemplateSet(SourceID sourceId, IpfixSetHeader* set);

/**
 * Processes an IPFIX template set.
 * Called by processMessage
 */
void processTemplateSet(SourceID sourceId, IpfixSetHeader* set) {
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
		ti->userData = 0;
		ti->fieldCount = ntoh(th->fieldCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		boolean lengthVarying = false;
		uint16 fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type = ntoh(*(uint16*)((byte*)record+0));
			ti->fieldInfo[fieldNo].length = ntoh(*(uint16*)((byte*)record+2));
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].length;
			if (ti->fieldInfo[fieldNo].length == 65535) lengthVarying=true;
			if (ti->fieldInfo[fieldNo].type & 0x80) {
				ti->fieldInfo[fieldNo].enterpriseNo = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[fieldNo].enterpriseNo = 0;
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
void processOptionsTemplateSet(SourceID sourceId, IpfixSetHeader* set) {
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
		ti->userData = 0;
		ti->scopeCount = ntoh(th->scopeCount);
		ti->scopeInfo = (FieldInfo*)malloc(ti->scopeCount * sizeof(FieldInfo));
		ti->fieldCount = ntoh(th->fieldCount)-ntoh(th->scopeCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		boolean lengthVarying = false;
		uint16 scopeNo;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
			ti->scopeInfo[scopeNo].type = ntoh(*(uint16*)((byte*)record+0));
			ti->scopeInfo[scopeNo].length = ntoh(*(uint16*)((byte*)record+2));
			ti->scopeInfo[scopeNo].offset = bt->recordLength; bt->recordLength+=ti->scopeInfo[scopeNo].length;
			if (ti->scopeInfo[scopeNo].length == 65535) lengthVarying=true;
			if (ti->scopeInfo[scopeNo].type & 0x80) {
				ti->scopeInfo[scopeNo].enterpriseNo = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[scopeNo].enterpriseNo = 0;
				record = (byte*)((byte*)record+4);
				}
			}
		uint16 fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type = ntoh(*(uint16*)((byte*)record+0));
			ti->fieldInfo[fieldNo].length = ntoh(*(uint16*)((byte*)record+2));
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].length;
			if (ti->fieldInfo[fieldNo].length == 65535) lengthVarying=true;
			if (ti->fieldInfo[fieldNo].type & 0x80) {
				ti->fieldInfo[fieldNo].enterpriseNo = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[fieldNo].enterpriseNo = 0;
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
void processDataTemplateSet(SourceID sourceId, IpfixSetHeader* set) {
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
		ti->userData = 0;
		ti->fieldCount = th->fieldCount;
		ti->dataCount = th->dataCount;
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		boolean lengthVarying = false;
		uint16 fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type = ntoh(*(uint16*)((byte*)record+0));
			ti->fieldInfo[fieldNo].length = ntoh(*(uint16*)((byte*)record+2));
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].length;
			if (ti->fieldInfo[fieldNo].length == 65535) lengthVarying=true;
			if (ti->fieldInfo[fieldNo].type & 0x80) {
				ti->fieldInfo[fieldNo].enterpriseNo = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->fieldInfo[fieldNo].enterpriseNo = 0;
				record = (byte*)((byte*)record+4);
				}
			}
		if (lengthVarying) {
  			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) ti->fieldInfo[fieldNo].offset = 65535;
  			}

		ti->dataInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].type = ntoh(*(uint16*)((byte*)record+0));
			ti->dataInfo[fieldNo].length = ntoh(*(uint16*)((byte*)record+2));
			if (ti->dataInfo[fieldNo].type & 0x80) {
				ti->dataInfo[fieldNo].enterpriseNo = ntoh(*(uint32*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
				} else {
				ti->dataInfo[fieldNo].enterpriseNo = 0;
				record = (byte*)((byte*)record+4);
				}
			ti->dataInfo[fieldNo].offset = (record - &th->data);
			if (ti->dataInfo[fieldNo].length == 65535) {
				if (*(byte*)record < 255) {
					ti->dataInfo[fieldNo].length = *(byte*)record;
					} else {
					ti->dataInfo[fieldNo].length = *(uint16*)(record+1);
					}
				}
			record = record + ti->dataInfo[fieldNo].length;
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
void processDataSet(SourceID sourceId, IpfixSetHeader* set) {
	BufferedTemplate* bt = getBufferedTemplate(sourceId, ntoh(set->id));

	if (bt == 0) {
		error("Template %d unknown to collecting process", ntoh(set->id));
		return;
		}
	
	if (bt->setID == IPFIX_SetId_Template) {
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
					if (ti->fieldInfo[i].length < 65535) {
						fieldLength = ti->fieldInfo[i].length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->fieldInfo[i].length = fieldLength;
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
					if (ti->scopeInfo[i].length < 65535) {
						fieldLength = ti->scopeInfo[i].length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->scopeInfo[i].length = fieldLength;
					ti->scopeInfo[i].offset = recordLength;
					recordLength += fieldLength;
					}
				for (i = 0; i < ti->fieldCount; i++) {
					int fieldLength = 0;
					if (ti->fieldInfo[i].length < 65535) {
						fieldLength = ti->fieldInfo[i].length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->fieldInfo[i].length = fieldLength;
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
					if (ti->fieldInfo[i].length < 65535) {
						fieldLength = ti->fieldInfo[i].length;
						} else {
						if (*(byte*)record < 255) {
							fieldLength = *(byte*)record;
							} else {
							fieldLength = *(uint16*)(record+1);
							}
						}
					ti->fieldInfo[i].length = fieldLength;
					ti->fieldInfo[i].offset = recordLength;
					recordLength += fieldLength;
					}
				dataDataRecordCallbackFunction(sourceId, ti, recordLength, record);
				record = record + recordLength;
				}
			}
		} else {
		fatal("Data Set based on known but unhandled template type %d", bt->setID);
		}
	}
	
boolean processMessage(byte* message, uint16 length) {
	IpfixHeader* header = (IpfixHeader*)message;
	if (ntoh(header->version) != 0x000a) {
 		error("Bad message version - expected 0x000a, got %#06x", ntoh(header->version));
		return false;
		}

	if (ntoh(header->length) != length) {
 		error("Bad message length - expected %#06x, got %#06x", length, ntoh(header->length));
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
			error("Unsupported Set ID - expected 2/3/4/256+, got %d", ntoh(set->id));
  			}
		set = (IpfixSetHeader*)((byte*)set + ntoh(set->length));
		}

	return true;
	}


/***** Exported Functions ****************************************************/

boolean initializeRcvIpfix() {
	initializeTemplateBuffer();
	templateCallbackFunction = 0;
	dataTemplateCallbackFunction = 0;
	optionsTemplateCallbackFunction = 0;
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



void setTemplateCallback(TemplateCallbackFunction* templateCallbackFunction_) {
	templateCallbackFunction = templateCallbackFunction_;
	}

void setOptionsTemplateCallback(OptionsTemplateCallbackFunction* optionsTemplateCallbackFunction_) {
	optionsTemplateCallbackFunction = optionsTemplateCallbackFunction_;
	}

void setDataTemplateCallback(DataTemplateCallbackFunction* dataTemplateCallbackFunction_) {
	dataTemplateCallbackFunction = dataTemplateCallbackFunction_;
	}


void setDataRecordCallback(DataRecordCallbackFunction* dataRecordCallbackFunction_) {
	dataRecordCallbackFunction = dataRecordCallbackFunction_;
	}

void setOptionsRecordCallback(OptionsRecordCallbackFunction* optionsRecordCallbackFunction_) {
	optionsRecordCallbackFunction = optionsRecordCallbackFunction_;
	}

void setDataDataRecordCallback(DataDataRecordCallbackFunction* dataDataRecordCallbackFunction_) {
	dataDataRecordCallbackFunction = dataDataRecordCallbackFunction_;
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
