/** @file
 * IPFIX Collector module.
 *
 * The IPFIX Collector module receives messages from lower levels (see @c processMessage())
 * and parses the message into separate Templates, Options and Flows. It then
 * invokes the appropriate callback routine for each Template, Option and Flow received
 * (see the @c setTemplateCallback() and @c setDataRecordCallback() function groups).
 *
 * The Collector module supports higher-level modules by providing field types and offsets along 
 * with the raw data block of individual messages passed via the callback functions (see @c TemplateInfo)
 *
 */

/******************************************************************************

IPFIX Collector module
Copyright (C) 2004 Christoph Sommer
http://www.deltadevelopment.de/users/christoph/ipfix

FIXME: Basic support for NetflowV9 packets, templates and flow records
is provided. Will break when fed field types with type ID >= 0x8000.

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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

/* for ntohll et al */
#include "ipfixlolib/ipfixlolib.h"
#include "sampler/packet_hook.h"

#include "exp_rcvIpfix.h"
#include "exp_templateBuffer.h"
#include "exp_ipfix.h"

#include "msg.h"
/***** Defines ************************************************************/


/***** Constants ************************************************************/

#define NetflowV9_SetId_Template  0

/***** Macros ************************************************************/


/***** Data Types ************************************************************/

// Network header classifications
#define PCLASS_NET_IP4             (1UL <<  0)
#define PCLASS_NET_IP6             (1UL <<  1)

#define PCLASS_NETMASK             0x00000fff

// Transport header classifications
#define PCLASS_TRN_TCP             (1UL << 12)
#define PCLASS_TRN_UDP             (1UL << 13)
#define PCLASS_TRN_ICMP            (1UL << 14)
#define PCLASS_TRN_IGMP            (1UL << 15)



/**
 * IPFIX header helper.
 * Constitutes the first 16 bytes of every IPFIX Message
 */
typedef struct {
	uint16_t version;			/**< Expected to be 0x000a */
	uint16_t length; 
	uint32_t exportTime;
	uint32_t sequenceNo;
	uint32_t observationDomainId;
	byte   data;
} ExpressIpfixHeader;

/**
 * NetflowV9 header helper.
 * Constitutes the first bytes of every NetflowV9 Message
 */
typedef struct {
	uint16_t version;                 /**< Expected to be 0x0009 */
	uint16_t setCount;
	uint32_t uptime;
	uint32_t exportTime;
	uint32_t sequenceNo;
	uint32_t observationDomainId;
	byte   data;
} ExpressNetflowV9Header;

/**
 * IPFIX "Set" helper.
 * Constitutes the first bytes of every IPFIX Template Set, Options Template Set or Data Set
 */
typedef struct {
	uint16_t id;
	uint16_t length;
	byte data; 
} ExpressIpfixSetHeader;

/**
 * IPFIX "Template Set" helper.
 * Constitutes the first bytes of every IPFIX Template
 */
typedef struct {
	uint16_t templateId;
	uint16_t fieldCount;
	byte data;
} ExpressIpfixTemplateHeader;

/**
 * IPFIX "DataTemplate Set" helper.
 * Constitutes the first bytes of every IPFIX DataTemplate
 */
typedef struct {
	uint16_t templateId;
	uint16_t fieldCount;
	uint16_t dataCount;
	uint16_t precedingRule;
	byte data;
} ExpressIpfixDataTemplateHeader;

/**
 * IPFIX "Options Template Set" helper.
 * Constitutes the first bytes of every IPFIX Options Template
 */
typedef struct {
	uint16_t templateId;
	uint16_t fieldCount;
	uint16_t scopeCount; 
	byte data;
} ExpressIpfixOptionsTemplateHeader;

/***** Global Variables ******************************************************/


/***** Internal Functions ****************************************************/

static void ExpressprocessDataSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceID, ExpressIpfixSetHeader* set);
static void ExpressprocessTemplateSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceID, ExpressIpfixSetHeader* set);
static void ExpressprocessDataTemplateSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceID, ExpressIpfixSetHeader* set);
static void ExpressprocessOptionsTemplateSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceId, ExpressIpfixSetHeader* set);

/**
 * Processes an IPFIX template set.
 * Called by processMessage
 */
static void ExpressprocessTemplateSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceId, ExpressIpfixSetHeader* set) {
	byte* endOfSet = (byte*)set + ntohs(set->length);
	byte* record = (byte*)&set->data;

	/* TemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		ExpressIpfixTemplateHeader* th = (ExpressIpfixTemplateHeader*)record;
		record = (byte*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			destroyBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(th->templateId));
			continue;
		}
		ExpressBufferedTemplate* bt = (ExpressBufferedTemplate*)malloc(sizeof(ExpressBufferedTemplate));
		ExpressTemplateInfo* ti = (ExpressTemplateInfo*)malloc(sizeof(ExpressTemplateInfo));
		memcpy(&bt->sourceID, sourceId, sizeof(Exp_SourceID));
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->templateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->fieldInfo = (ExpressFieldInfo*)malloc(ti->fieldCount * sizeof(ExpressFieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 65535;
			}
		}
        
		bufferTemplate(ipfixParser->templateBuffer, bt); 
		bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;

		int n;          
		for (n = 0; n < ipfixParser->callbackCount; n++) {
			ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
			if (ci->templateCallbackFunction) {
				ci->templateCallbackFunction(ci->handle, sourceId, ti);
			}
		}
	}
}

/**
 * Processes an IPFIX Options Template Set.
 * Called by processMessage
 */
static void ExpressprocessOptionsTemplateSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceId, ExpressIpfixSetHeader* set) {
	byte* endOfSet = (byte*)set + ntohs(set->length);
	byte* record = (byte*)&set->data;

	/* OptionsTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		ExpressIpfixOptionsTemplateHeader* th = (ExpressIpfixOptionsTemplateHeader*)record;
		record = (byte*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			destroyBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(th->templateId));
			continue;
		}
		ExpressBufferedTemplate* bt = (ExpressBufferedTemplate*)malloc(sizeof(ExpressBufferedTemplate));
		ExpressOptionsTemplateInfo* ti = (ExpressOptionsTemplateInfo*)malloc(sizeof(ExpressOptionsTemplateInfo));
		memcpy(&bt->sourceID, sourceId, sizeof(Exp_SourceID));
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->optionsTemplateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->scopeCount = ntohs(th->scopeCount);
		ti->scopeInfo = (ExpressFieldInfo*)malloc(ti->scopeCount * sizeof(ExpressFieldInfo));
		ti->fieldCount = ntohs(th->fieldCount)-ntohs(th->scopeCount);
		ti->fieldInfo = (ExpressFieldInfo*)malloc(ti->fieldCount * sizeof(ExpressFieldInfo));
		int isLengthVarying = 0;
		uint16_t scopeNo;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
			ti->scopeInfo[scopeNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->scopeInfo[scopeNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->scopeInfo[scopeNo].type.isVariableLength = (ti->scopeInfo[scopeNo].type.length == 65535);
			ti->scopeInfo[scopeNo].offset = bt->recordLength; bt->recordLength+=ti->scopeInfo[scopeNo].type.length;
			if (ti->scopeInfo[scopeNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->scopeInfo[scopeNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->scopeInfo[scopeNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[scopeNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->scopeCount; fieldNo++) {
				ti->scopeInfo[fieldNo].offset = 65535;
			}
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 65535;
			}
		}
		bufferTemplate(ipfixParser->templateBuffer, bt); 
		bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;


		int n;
		for (n = 0; n < ipfixParser->callbackCount; n++) {
			ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
			if (ci->optionsTemplateCallbackFunction) {
				ci->optionsTemplateCallbackFunction(ci->handle, sourceId, ti);
			}
		}
	}
}

/**
 * Processes an IPFIX DataTemplate set.
 * Called by processMessage
 */
static void ExpressprocessDataTemplateSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceId, ExpressIpfixSetHeader* set) {
	byte* endOfSet = (byte*)set + ntohs(set->length);
	byte* record = (byte*)&set->data;

	/* DataTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		ExpressIpfixDataTemplateHeader* th = (ExpressIpfixDataTemplateHeader*)record;
		record = (byte*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			destroyBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(th->templateId));
			continue;
		}
		ExpressBufferedTemplate* bt = (ExpressBufferedTemplate*)malloc(sizeof(ExpressBufferedTemplate));
		ExpressDataTemplateInfo* ti = (ExpressDataTemplateInfo*)malloc(sizeof(ExpressDataTemplateInfo));
		memcpy(&bt->sourceID, sourceId, sizeof(Exp_SourceID));
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->dataTemplateInfo = ti;
		ti->userData = 0;
		ti->id = ntohs(th->templateId);
		ti->preceding = ntohs(th->precedingRule);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->dataCount = ntohs(th->dataCount);
		ti->fieldInfo = (ExpressFieldInfo*)malloc(ti->fieldCount * sizeof(ExpressFieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 65535;
			}
		}

		ti->dataInfo = (ExpressFieldInfo*)malloc(ti->fieldCount * sizeof(ExpressFieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->dataInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			if (ti->dataInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->dataInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->dataInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}

		/* done with reading dataInfo, @c record now points to the fixed data block */
		byte* dataStart = record;

		int dataLength = 0;
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].offset = dataLength;
			if (ti->dataInfo[fieldNo].type.length == 65535) {
				/* This is a variable-length field, get length from first byte and advance offset */
				ti->dataInfo[fieldNo].type.length = *(uint8_t*)(dataStart + ti->dataInfo[fieldNo].offset);
				ti->dataInfo[fieldNo].offset += 1;
				if (ti->dataInfo[fieldNo].type.length == 255) {
					/* First byte did not suffice, length is stored in next two bytes. Advance offset */
					ti->dataInfo[fieldNo].type.length = *(uint16_t*)(dataStart + ti->dataInfo[fieldNo].offset);
					ti->dataInfo[fieldNo].offset += 2;
				}
			}
			dataLength += ti->dataInfo[fieldNo].type.length;
		}

		/* Copy fixed data block */
		ti->data = (byte*)malloc(dataLength);
		memcpy(ti->data,dataStart,dataLength);

		/* Advance record to end of fixed data block, i.e. start of next template*/
		record += dataLength;

		bufferTemplate(ipfixParser->templateBuffer, bt); 
		bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;

		int n;
		for (n = 0; n < ipfixParser->callbackCount; n++) {
			ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
			if (ci->dataTemplateCallbackFunction){
				ci->dataTemplateCallbackFunction(ci->handle, sourceId, ti);
			}
		}
	}
}

/**
 * Processes an IPFIX data set.
 * Called by processMessage
 */
static void ExpressprocessDataSet(ExpressIpfixParser* ipfixParser, Exp_SourceID* sourceId, ExpressIpfixSetHeader* set) {
	ExpressBufferedTemplate* bt = getBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(set->id));

	if (bt == 0) {
		/* this error may come in rapid succession; I hope I don't regret it */
		msg(MSG_INFO, "Template %d unknown to collecting process", ntohs(set->id));
		return;
	}
        
#ifdef SUPPORT_NETFLOWV9
	if ((bt->setID == IPFIX_SetId_Template) || (bt->setID == NetflowV9_SetId_Template)) {
#else
	if (bt->setID == IPFIX_SetId_Template) {
#endif

		ExpressTemplateInfo* ti = bt->templateInfo;
        
		uint16_t length = ntohs(set->length)-((byte*)(&set->data)-(byte*)set);

		byte* record = &set->data;
        
		if (bt->recordLength < 65535) {
			byte* recordX = record+length;
        
			if (record >= recordX - (bt->recordLength - 1)) {
				DPRINTF("Got a Data Set that contained not a single full record\n");
			}

			/* We stop processing when no full record is left */
			while (record < recordX - (bt->recordLength - 1)) {
				int n;
				for (n = 0; n < ipfixParser->callbackCount; n++) {
					ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
					if (ci->dataRecordCallbackFunction) {
						ci->dataRecordCallbackFunction(ci->handle, sourceId, ti, bt->recordLength, record);
					}
				}
				record = record + bt->recordLength;
			}
		} else {
			byte* recordX = record+length;

			if (record >= recordX - 3) {
				DPRINTF("Got a Data Set that contained not a single full record");
			}

			/* We assume that all variable-length records are >= 4 byte, so we stop processing when only 3 bytes are left */
			while (record < recordX - 3) {
				int recordLength=0;
				int i;
				for (i = 0; i < ti->fieldCount; i++) {
					int fieldLength = 0;
					if (!ti->fieldInfo[i].type.isVariableLength) {
						fieldLength = ti->fieldInfo[i].type.length;
					} else {
						fieldLength = *(uint8_t*)(record + recordLength);
						recordLength += 1;
						if (fieldLength == 255) {
							fieldLength = ntohs(*(uint16_t*)(record + recordLength));
							recordLength += 2;
						}
					}
					ti->fieldInfo[i].offset = recordLength;
					ti->fieldInfo[i].type.length = fieldLength;
					recordLength += fieldLength;
				}
				int n;
				for (n = 0; n < ipfixParser->callbackCount; n++) {
					ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
					if (ci->dataRecordCallbackFunction) {
						ci->dataRecordCallbackFunction(ci->handle, sourceId, ti, recordLength, record);
					}
				}
				record = record + recordLength;
			}
		}
	} else {
		if (bt->setID == IPFIX_SetId_OptionsTemplate) {

			ExpressOptionsTemplateInfo* ti = bt->optionsTemplateInfo;

			uint16_t length = ntohs(set->length)-((byte*)(&set->data)-(byte*)set);
			byte* record = &set->data;

			if (bt->recordLength < 65535) {
				byte* recordX = record+length;
				while (record < recordX) {
					int n;
					for (n = 0; n < ipfixParser->callbackCount; n++) {
						ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
						if (ci->optionsRecordCallbackFunction) {
							ci->optionsRecordCallbackFunction(ci->handle, sourceId, ti, bt->recordLength, record);
						}
					}
					record = record + bt->recordLength;
				}
			} else {
				byte* recordX = record+length;
				while (record < recordX) {
					int recordLength=0;
					int i;
					for (i = 0; i < ti->scopeCount; i++) {
						int fieldLength = 0;
						if (ti->scopeInfo[i].type.isVariableLength) {
							fieldLength = ti->scopeInfo[i].type.length;
						} else {
							fieldLength = *(uint8_t*)(record + recordLength);
							recordLength += 1;
							if (fieldLength == 255) {
								fieldLength = *(uint16_t*)(record + recordLength);
								recordLength += 2;
							}
						}
						ti->scopeInfo[i].offset = recordLength;
						ti->scopeInfo[i].type.length = fieldLength;
						recordLength += fieldLength;
					}
					for (i = 0; i < ti->fieldCount; i++) {
						int fieldLength = 0;
						if (!ti->fieldInfo[i].type.isVariableLength) {
							fieldLength = ti->fieldInfo[i].type.length;
						} else {
							fieldLength = *(uint8_t*)(record + recordLength);
							recordLength += 1;
							if (fieldLength == 255) {
								fieldLength = *(uint16_t*)(record + recordLength);
								recordLength += 2;
							}
						}
						ti->fieldInfo[i].offset = recordLength;
						ti->fieldInfo[i].type.length = fieldLength;
						recordLength += fieldLength;
					}
					int n;
					for (n = 0; n < ipfixParser->callbackCount; n++) {
						ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
						if (ci->optionsRecordCallbackFunction)
							ci->optionsRecordCallbackFunction(ci->handle, sourceId, ti, recordLength, record);
					}

					record = record + recordLength;
				}
			}
		} else {
			if (bt->setID == IPFIX_SetId_DataTemplate) {
				ExpressDataTemplateInfo* ti = bt->dataTemplateInfo;

				uint16_t length = ntohs(set->length)-((byte*)(&set->data)-(byte*)set);
				byte* record = &set->data;

				if (bt->recordLength < 65535) {
					byte* recordX = record+length;
					while (record < recordX) {
						int n;
						for (n = 0; n < ipfixParser->callbackCount; n++) {
							ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
							if (ci->dataDataRecordCallbackFunction) {
								ci->dataDataRecordCallbackFunction(ci->handle, sourceId, ti, bt->recordLength, record);
							}
						}
						record = record + bt->recordLength;
					}
				} else {
					byte* recordX = record+length;
					while (record < recordX) {
						int recordLength=0;
						int i;
						for (i = 0; i < ti->fieldCount; i++) {
							int fieldLength = 0;
							if (!ti->fieldInfo[i].type.isVariableLength) {
								fieldLength = ti->fieldInfo[i].type.length;
							} else {
								fieldLength = *(uint8_t*)(record + recordLength);
								recordLength += 1;
								if (fieldLength == 255) {
									fieldLength = *(uint16_t*)(record + recordLength);
									recordLength += 2;
								}
							}
							ti->fieldInfo[i].offset = recordLength;
							ti->fieldInfo[i].type.length = fieldLength;
							recordLength += fieldLength;
						}
						int n;
						for (n = 0; n < ipfixParser->callbackCount; n++) {
							ExpressCallbackInfo* ci = &ipfixParser->callbackInfo[n];
							if (ci->dataDataRecordCallbackFunction) {
								ci->dataDataRecordCallbackFunction(ci->handle, sourceId, ti, recordLength, record);
							}
						}
						record = record + recordLength;
					}
				}	
			} else {
				msg(MSG_FATAL, "Data Set based on known but unhandled template type %d", bt->setID);
			}
		}
	}
}

        
/**
 * Process a NetflowV9 Packet
 * @return 0 on success
 */     
static int ExpressprocessNetflowV9Packet(ExpressIpfixParser* ipfixParser, byte* message, uint16_t length, Exp_SourceID* sourceId) 
{
	ExpressNetflowV9Header* header = (ExpressNetflowV9Header*)message;

	/* pointer to first set */
	ExpressIpfixSetHeader* set = (ExpressIpfixSetHeader*)&header->data;

	int i;
        
	sourceId->observationDomainId = ntohl(header->observationDomainId);

	for (i = 0; i < ntohs(header->setCount); i++) {
		if (ntohs(set->id) == NetflowV9_SetId_Template) {
			ExpressprocessTemplateSet(ipfixParser, sourceId, set);
		} else
			if (ntohs(set->id) >= IPFIX_SetId_Data_Start) {
				ExpressprocessDataSet(ipfixParser, sourceId, set);
			} else {
				msg(MSG_ERROR, "Unsupported Set ID - expected 0/256+, got %d", ntohs(set->id));
			}
		set = (ExpressIpfixSetHeader*)((byte*)set + ntohs(set->length));
	}

	return 0;
}

/**
 * Process an IPFIX Packet
 * @return 0 on success
 */     
static int ExpressprocessIpfixPacket(ExpressIpfixParser* ipfixParser, byte* message, uint16_t length, Exp_SourceID* sourceId) 
{
	ExpressIpfixHeader* header = (ExpressIpfixHeader*)message;
	sourceId->observationDomainId = ntohl(header->observationDomainId);

	if (ntohs(header->length) != length) {
		DPRINTF("Bad message length - expected %#06x, got %#06x\n", length, ntohs(header->length));
		return -1;
	}

	/* pointer to first set */
	ExpressIpfixSetHeader* set = (ExpressIpfixSetHeader*)&header->data;

	/* pointer beyond message */
	ExpressIpfixSetHeader* setX = (ExpressIpfixSetHeader*)((char*)message + length); 

	uint16_t tmpid;
	while(set < setX) {
		tmpid=ntohs(set->id);

		switch(tmpid) {
		case IPFIX_SetId_DataTemplate:
			ExpressprocessDataTemplateSet(ipfixParser, sourceId, set);
			break;
		case IPFIX_SetId_Template:
			ExpressprocessTemplateSet(ipfixParser, sourceId, set);
			break;
		case IPFIX_SetId_OptionsTemplate:
			ExpressprocessOptionsTemplateSet(ipfixParser, sourceId, set);
			break;
		default:
			if(tmpid >= IPFIX_SetId_Data_Start) {
				ExpressprocessDataSet(ipfixParser, sourceId, set);
			} else {
				msg(MSG_ERROR, "processIpfixPacket: Unsupported Set ID - expected 2/3/4/256+, got %d", tmpid);
			}
		}
		set = (ExpressIpfixSetHeader*)((byte*)set + ntohs(set->length));
	}

	return 0;
}

/**
 * Process new Message
 * @return 0 on success
 */     
static int ExpressprocessMessage(ExpressIpfixParser* ipfixParser, byte* message, uint16_t length, Exp_SourceID* sourceID) 
{
	ExpressIpfixHeader* header = (ExpressIpfixHeader*)message;
	if (ntohs(header->version) == 0x000a) {
		return ExpressprocessIpfixPacket(ipfixParser, message, length, sourceID);
	}
#ifdef SUPPORT_NETFLOWV9
	if (ntohs(header->version) == 0x0009) {
		return ExpressprocessNetflowV9Packet(ipfixParser, message, length, sourceID);
	}
	DPRINTF("Bad message version - expected 0x009 or 0x000a, got %#06x\n", ntohs(header->version));
	return -1;
#else
	DPRINTF("Bad message version - expected 0x000a, got %#06x\n", ntohs(header->version));
	return -1;
#endif
}

static void ExpressprintIPv4(ExpressFieldType type, FieldData* data) {
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
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return;
	}

	if ((type.length == 5) /*&& (imask != 0)*/) {
		printf("%d.%d.%d.%d/%d", octet1, octet2, octet3, octet4, 32-imask);
	}  else {
		printf("%d.%d.%d.%d", octet1, octet2, octet3, octet4);
	}
}

static void ExpressprintPort(ExpressFieldType type, FieldData* data) {
	if (type.length == 0) {
		printf("zero-length Port");
		return;
	}
	if (type.length == 2) {
		int port = ((uint16_t)data[0] << 8)+data[1];
		printf("%d", port);
		return;
	}
	if ((type.length >= 4) && ((type.length % 4) == 0)) {
		int i;
		for (i = 0; i < type.length; i+=4) {
			int starti = ((uint16_t)data[i+0] << 8)+data[i+1];
			int endi = ((uint16_t)data[i+2] << 8)+data[i+3];
			if (i > 0) {
				printf(",");
			}
			if (starti != endi) {
				printf("%d:%d", starti, endi);
			} else {
				printf("%d", starti);
			}
		}
		return;
	}

	printf("Port with length %d unparseable", type.length);
}

void ExpressprintProtocol(ExpressFieldType type, FieldData* data) {
	if (type.length != 1) {
		printf("Protocol with length %d unparseable", type.length);
		return;
	}
	switch (data[0]) {
	case IPFIX_protocolIdentifier_ICMP:
		printf("ICMP");
		return;
	case IPFIX_protocolIdentifier_TCP:
		printf("TCP");
		return;
	case IPFIX_protocolIdentifier_UDP: 
		printf("UDP");
		return;
	case IPFIX_protocolIdentifier_RAW: 
		printf("RAW");
		return;
	default:
		printf("unknownProtocol");
		return;
	}
}

static void ExpressprintUint(ExpressFieldType type, FieldData* data) {
	switch (type.length) {
	case 1:
		printf("%hhu",*(uint8_t*)data);
		return;
	case 2:
		printf("%hu",ntohs(*(uint16_t*)data));
		return;
	case 4:
		printf("%u",ntohl(*(uint32_t*)data));
		return;
	case 8:
		printf("%Lu",ntohll(*(uint64_t*)data));
		return;
	default:
		msg(MSG_ERROR, "Uint with length %d unparseable", type.length);
		return;
	}
}


/***** Exported Functions ****************************************************/


/**
 * Prints a string representation of FieldData to stdout.
 */
void ExpressprintFieldData(ExpressFieldType type, FieldData* pattern) {
	char* s;

	switch (type.id) {
	case IPFIX_TYPEID_protocolIdentifier:
		printf("protocolIdentifier:");
		ExpressprintProtocol(type, pattern);
		break;
	case IPFIX_TYPEID_sourceIPv4Address:
		printf("sourceIPv4Address:");
		ExpressprintIPv4(type, pattern);
		break;
	case IPFIX_TYPEID_destinationIPv4Address:
		printf("destinationIPv4Address:");
		ExpressprintIPv4(type, pattern);
		break;                          
	case IPFIX_TYPEID_sourceTransportPort:
		printf("sourceTransportPort:");
		ExpressprintPort(type, pattern);
		break;
	case IPFIX_TYPEID_destinationTransportPort:
		printf("destinationTransportPort:");
		ExpressprintPort(type, pattern);
		break;
	default:
		s = Expresstypeid2string(type.id);
		if (s != NULL) {
			printf("%s:", s);
			ExpressprintUint(type, pattern);
		} else {
			DPRINTF("Field with ID %d unparseable\n", type.id);
		}
		break;
	}
}

/**
 * Gets a Template's FieldInfo by field id.
 * @param ti Template to search in
 * @param type Field id and field eid to look for, length is ignored
 * @return NULL if not found
 */
ExpressFieldInfo* ExpressgetTemplateFieldInfo(ExpressTemplateInfo* ti, ExpressFieldType* type) {
	int i;


	for (i = 0; i < ti->fieldCount; i++) {
		if ((ti->fieldInfo[i].type.id == type->id) && (ti->fieldInfo[i].type.eid == type->eid)) {
			printf("-%i-", ti->fieldInfo[i].type.id);
			return &ti->fieldInfo[i];
		}
	}

	return NULL;
}
/** 
 * Gets FieldInfo by field id
 * @param type Field id to look for
 * @param pdata packet_hook struct
 * @return NULL if not found
 */

FieldData *ExpressgetFieldPointer(ExpressFieldType type, struct packet_hook *pdata) {


	FieldData *idata=(FieldData *)pdata->ip_header;


	switch (type.id) {
	case IPFIX_TYPEID_packetDeltaCount:
		return idata + 10;
		break;

	case IPFIX_TYPEID_flowStartSeconds:
		return idata + 4;
		break;

	case IPFIX_TYPEID_flowEndSeconds:
		return idata + 4;
		break;

	case IPFIX_TYPEID_octetDeltaCount:
		return idata + 2;
		break;

	case IPFIX_TYPEID_protocolIdentifier:
		return idata + 9;
		break;

	case IPFIX_TYPEID_sourceIPv4Address:
		return idata + 12;
		break;

	case IPFIX_TYPEID_destinationIPv4Address:
		return idata + 16;
		break;

	case IPFIX_TYPEID_icmpTypeCode:
		if((pdata->classification & PCLASS_TRN_ICMP) == 0) {
			FieldData *tdata=(FieldData *)pdata->transport_header;
			return tdata + 0;
		} else {
			return NULL;
		}
		break;

	case IPFIX_TYPEID_sourceTransportPort:
		if((pdata->classification & PCLASS_TRN_TCP) == 0 || (pdata->classification & PCLASS_TRN_UDP) == 0) {
			FieldData *tdata=(FieldData *)pdata->transport_header;
			return tdata + 0;
		} else {
			return NULL;
		}
		break;

	case IPFIX_TYPEID_destinationTransportPort:
		if((pdata->classification & PCLASS_TRN_TCP) == 0  || (pdata->classification & PCLASS_TRN_UDP) == 0) {
			FieldData *tdata=(FieldData *)pdata->transport_header;
			return tdata + 2;
		} else {
			return NULL;
		}
		break;

	case IPFIX_TYPEID_tcpControlBits:
		if((pdata->classification & PCLASS_TRN_TCP) == 0) {
			FieldData *tdata=(FieldData *)pdata->transport_header;
			return tdata + 13;
		} else {
			return NULL;
		}
		break;

	default:
		return NULL;
		break;
	}

	return NULL;

}

int ExpressgetFieldInfo(ExpressFieldType type, int offset) {

	int type_offset;

	switch (type.id) {
	case IPFIX_TYPEID_packetDeltaCount:
		type_offset = 10;
		return type_offset;
		break;

	case IPFIX_TYPEID_flowStartSeconds:
		type_offset = 4;
		return type_offset;
		break;

	case IPFIX_TYPEID_flowEndSeconds:
		type_offset = 4;
		return type_offset;
		break;

	case IPFIX_TYPEID_octetDeltaCount:
		type_offset = 2;
		return type_offset;
		break;

	case IPFIX_TYPEID_protocolIdentifier:
		type_offset = 9;
		return type_offset;
		break;

	case IPFIX_TYPEID_sourceIPv4Address:
		type_offset = 12;
		return type_offset;
		break;

	case IPFIX_TYPEID_destinationIPv4Address:
		type_offset = 16;
		return type_offset;
		break;

	case IPFIX_TYPEID_icmpTypeCode:
		type_offset = 0 + offset;
		return type_offset;
		break;

	case IPFIX_TYPEID_sourceTransportPort:
		type_offset = 0 + offset;
		return type_offset;
		break;

	case IPFIX_TYPEID_destinationTransportPort:
		type_offset = 2 + offset;
		return type_offset;
		break;

	case IPFIX_TYPEID_tcpControlBits:
		type_offset = 13 + offset;
		return type_offset;
		break;

	default:
		return 999;
		break;
	}

	return 999;
}

/**
 * gets a types length
 **/
int ExpressgetFieldLength(ExpressFieldType type) {

	int type_length;

	switch (type.id) {
	case IPFIX_TYPEID_packetDeltaCount:
		type_length = 1;
		return type_length;
		break;

	case IPFIX_TYPEID_flowStartSeconds:
		type_length = 4;
		return type_length;
		break;

	case IPFIX_TYPEID_flowEndSeconds:
		type_length = 4;
		return type_length;
		break;

	case IPFIX_TYPEID_octetDeltaCount:
		type_length = 2;
		return type_length;
		break;

	case IPFIX_TYPEID_protocolIdentifier:
		type_length = 1;
		return type_length;
		break;

	case IPFIX_TYPEID_sourceIPv4Address:
		type_length = 4;
		return type_length;
		break;

	case IPFIX_TYPEID_destinationIPv4Address:
		type_length = 4;
		return type_length;
		break;

	case IPFIX_TYPEID_icmpTypeCode:
		type_length = 4;
		return type_length;
		break;

	case IPFIX_TYPEID_sourceTransportPort:
		type_length = 2;
		return type_length;
		break;

	case IPFIX_TYPEID_destinationTransportPort:
		type_length = 2;
		return type_length;
		break;

	case IPFIX_TYPEID_tcpControlBits:
		type_length = 1;
		return type_length;
		break;

	default:
		return 999;
		break;
	}

	return 999;
}



/**
 * Gets a DataTemplate's FieldInfo by field id.
 * @param ti DataTemplate to search in
 * @param type Field id and field eid to look for, length is ignored
 * @return NULL if not found
 */
ExpressFieldInfo* ExpressgetDataTemplateFieldInfo(ExpressDataTemplateInfo* ti, ExpressFieldType* type) {
	int i;

	for (i = 0; i < ti->fieldCount; i++) {
		printf("hier");
		if ((ti->fieldInfo[i].type.id == type->id) && (ti->fieldInfo[i].type.eid == type->eid)) {
			return &ti->fieldInfo[i];
		}
	}

	return NULL;
}

/**
 * Gets a DataTemplate's Data-FieldInfo by field id.
 * @param ti DataTemplate to search in
 * @param type Field id and field eid to look for, length is ignored
 * @return NULL if not found
 */
ExpressFieldInfo* ExpressgetDataTemplateDataInfo(ExpressDataTemplateInfo* ti, ExpressFieldType* type) {
	int i;

	for (i = 0; i < ti->dataCount; i++) {
		if ((ti->dataInfo[i].type.id == type->id) && (ti->dataInfo[i].type.eid == type->eid)) {
			return &ti->dataInfo[i];
		}
	}

	return NULL;            
}
        

/**
 * Creates a new  @c IpfixParser.
 * @return handle to created instance
 */
ExpressIpfixParser* ExpresscreateIpfixParser() {
	ExpressIpfixParser* ipfixParser;

	if(!(ipfixParser=(ExpressIpfixParser*)malloc(sizeof(ExpressIpfixParser)))) {
		msg(MSG_FATAL, "Ran out of memory");
		goto out0;
	}

	ipfixParser->callbackInfo = NULL;
	ipfixParser->callbackCount = 0;

	if(!(ipfixParser->templateBuffer = createTemplateBuffer(ipfixParser))) {
		msg(MSG_FATAL, "Could not create template Buffer");
		goto out1;
	}

	return ipfixParser;

out1:
	free(ipfixParser);
out0:
	return NULL;
}

/**
 * Frees memory used by an IpfixParser.
 */
void ExpressdestroyIpfixParser(ExpressIpfixParser* ipfixParser) {
	destroyTemplateBuffer(ipfixParser->templateBuffer);

	free(ipfixParser->callbackInfo);

	free(ipfixParser);
}

/**
 * Creates a new @c PacketProcessor.
 * @return handle to the created object
 */
ExpressIpfixPacketProcessor*  ExpresscreateIpfixPacketProcessor() {
	ExpressIpfixPacketProcessor* packetProcessor;

	if(!(packetProcessor=(ExpressIpfixPacketProcessor*)malloc(sizeof(ExpressIpfixPacketProcessor)))) {
		msg(MSG_FATAL,"Ran out of memory");
		goto out0;
	}

        if (pthread_mutex_init(&packetProcessor->mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		goto out1;
	}

	packetProcessor->ipfixParser = NULL;
	packetProcessor->processPacketCallbackFunction = ExpressprocessMessage;

	return packetProcessor;
out1:
	free(packetProcessor);
out0:
	return NULL;
}


/**
 * Frees memory used by a @c PacketProcessor
 */
void ExpressdestroyIpfixPacketProcessor(ExpressIpfixPacketProcessor* packetProcessor) {
	ExpressdestroyIpfixParser(packetProcessor->ipfixParser);
	pthread_mutex_destroy(&packetProcessor->mutex);
	free(packetProcessor);
}

/**
 * Adds a set of callback functions to the list of functions to call when a new Message arrives
 * @param ipfixParser IpfixParser to set the callback function for
 * @param handles set of callback functions
 */
void ExpressaddIpfixParserCallbacks(ExpressIpfixParser* ipfixParser, ExpressCallbackInfo handles) {
	int n = ++ipfixParser->callbackCount;
	ipfixParser->callbackInfo = (ExpressCallbackInfo*)realloc(ipfixParser->callbackInfo, n * sizeof(ExpressCallbackInfo));
	memcpy(&ipfixParser->callbackInfo[n-1], &handles, sizeof(ExpressCallbackInfo));
}

/** 
 * Assigns an IpfixParser to packetProcessor
 * @param packetProcessor PacketProcessor to assign the IpfixParser to.
 * @param ipfixParser Pointer to an ipfixParser object.
 */
void ExpresssetIpfixParser(ExpressIpfixPacketProcessor* packetProcessor, ExpressIpfixParser* ipfixParser) {
	packetProcessor->ipfixParser = ipfixParser;
}

/**
 * Adds a PacketProcessor to the list of PacketProcessors
 * @param ipfixCollector Collector to assign the PacketProcessor to
 * @param packetProcessor handle of packetProcessor
 */
void ExpressaddIpfixPacketProcessor(ExpressIpfixCollector* ipfixCollector, ExpressIpfixPacketProcessor* packetProcessor) {
	int i;
	int n = ++ipfixCollector->processorCount;
	ipfixCollector->packetProcessors = (ExpressIpfixPacketProcessor*)realloc(ipfixCollector->packetProcessors,
									  n*sizeof(ExpressIpfixPacketProcessor));
	memcpy(&ipfixCollector->packetProcessors[n-1], packetProcessor, sizeof(ExpressIpfixPacketProcessor));

	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		ExpresssetPacketProcessors(ipfixCollector->ipfixReceivers[i], ipfixCollector->packetProcessors,
				    ipfixCollector->processorCount);
	}
}


/**
 * Initializes internal data.
 * Call onces before using any function in this module
 * @return 0 if call succeeded
 */
int ExpressinitializeIpfixCollectors() {
	ExpressinitializeIpfixReceivers();
	return 0;
}

/**
 * Destroys internal data.
 * Call once to tidy up. Do not use any function in this module afterwards
 * @return 0 if call succeeded
 */
int ExpressdeinitializeIpfixCollectors() {
	ExpressdeinitializeIpfixReceivers();
	return 0;
} 

/**
 * Creates a new IpfixCollector.
 * Call @c startIpfixCollector() to start receiving and processing messages.
 * @param rec_type Type of receiver (SCTP, UDP, ...)
 * @param port Port to listen on
 * @return handle for further interaction
 */
ExpressIpfixCollector* ExpresscreateIpfixCollector(ExpressReceiver_Type rec_type, int port) {
	ExpressIpfixCollector* ipfixCollector;

	if (!(ipfixCollector = (ExpressIpfixCollector*)malloc(sizeof(ExpressIpfixCollector)))) {
		msg(MSG_FATAL, "Ran out of memory");
		return NULL;
	}

	ipfixCollector->receiverCount = 0;
	ipfixCollector->ipfixReceivers = NULL;

	ipfixCollector->processorCount = 0;
	ipfixCollector->packetProcessors = NULL;

	return ipfixCollector;
}

/**
 * Frees memory used by a IpfixCollector.
 * @param ipfixCollector Handle returned by @c createIpfixCollector()
 */
void ExpressdestroyIpfixCollector(ExpressIpfixCollector* ipfixCollector) {
	int i;

	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		ExpressdestroyIpfixReceiver(ipfixCollector->ipfixReceivers[i]);
	}
	free(ipfixCollector->ipfixReceivers);

	for (i = 0; i != ipfixCollector->processorCount; ++i) {
		ExpressdestroyIpfixPacketProcessor(&ipfixCollector->packetProcessors[i]);
	}
	free(ipfixCollector);
}

/**
 * Starts receiving and processing messages.
 * All sockets prepared by calls to createIpfixCollector() will start
 * receiving messages until stopIpfixCollector() is called.
 * @return 0 on success, non-zero on error
 */
int ExpressstartIpfixCollector(ExpressIpfixCollector* ipfixCollector) {
	int err = 0;
	int i;
	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		err += startExpressIpfixReceiver(ipfixCollector->ipfixReceivers[i]); 
	}
	return err;
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixCollector() call.
 * @return 0 on success, non-zero on error
 */
int ExpressstopIpfixCollector(ExpressIpfixCollector* ipfixCollector) {
	int err = 0;
	int i;
	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		err += stopExpressIpfixReceiver(ipfixCollector->ipfixReceivers[i]);
	}
	return err;
}

/**
 * Adds a IpfixReceiver to the list of IpfixReceivers
 * @param ipfixCollector Collector to assign the IpfixReceiver to
 * @param ipfixReceiver handle of ipfixReceiver
 */
void ExpressaddIpfixReceiver(ExpressIpfixCollector* ipfixCollector, ExpressIpfixReceiver* ipfixReceiver) {
	int n = ++ipfixCollector->receiverCount;
	ipfixCollector->ipfixReceivers = (ExpressIpfixReceiver**)realloc(ipfixCollector->ipfixReceivers,
								  n*sizeof(ExpressIpfixReceiver*));
	ipfixCollector->ipfixReceivers[n - 1] = ipfixReceiver;
}

