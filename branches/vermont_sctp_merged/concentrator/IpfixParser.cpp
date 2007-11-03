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

// FIXME: Basic support for NetflowV9 packets, templates and flow records is provided. Will break when fed field types with type ID >= 0x8000.

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

#include "IpfixReceiver.hpp"
#include "IpfixParser.hpp"
#include "TemplateBuffer.hpp"
#include "ipfix.hpp"
#include "IpfixPrinter.hpp"

#include "common/msg.h"

#define MAX_MSG_LEN 65536

/**
 * Processes an IPFIX template set.
 * Called by processMessage
 */
void IpfixParser::processTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set) {
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* TemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			continue;
		}
		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<IpfixRecord::TemplateInfo> ti(new IpfixRecord::TemplateInfo);
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->templateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->fieldInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}
        
		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLivetime > 0))
			bt->expires = time(0) + templateLivetime;
		else
			bt->expires = 0;

		boost::shared_ptr<IpfixTemplateRecord> ipfixRecord(new IpfixTemplateRecord);
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->templateInfo = ti;
		push(ipfixRecord);
	}
}

/**
 * Processes an IPFIX Options Template Set.
 * Called by processMessage
 */
void IpfixParser::processOptionsTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set) {
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* OptionsTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		IpfixOptionsTemplateHeader* th = (IpfixOptionsTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			continue;
		}
		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> ti(new IpfixRecord::OptionsTemplateInfo);
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->optionsTemplateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->scopeCount = ntohs(th->scopeCount);
		ti->scopeInfo = (IpfixRecord::FieldInfo*)malloc(ti->scopeCount * sizeof(IpfixRecord::FieldInfo));
		ti->fieldCount = ntohs(th->fieldCount)-ntohs(th->scopeCount);
		ti->fieldInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		int isLengthVarying = 0;
		uint16_t scopeNo;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
			ti->scopeInfo[scopeNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->scopeInfo[scopeNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->scopeInfo[scopeNo].type.isVariableLength = (ti->scopeInfo[scopeNo].type.length == 65535);
			ti->scopeInfo[scopeNo].offset = bt->recordLength; bt->recordLength+=ti->scopeInfo[scopeNo].type.length;
			if (ti->scopeInfo[scopeNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->scopeInfo[scopeNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->scopeInfo[scopeNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[scopeNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->scopeCount; fieldNo++) {
				ti->scopeInfo[fieldNo].offset = 0xFFFFFFFF;
			}
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}
		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLivetime > 0))
			bt->expires = time(0) + templateLivetime;
		else
			bt->expires = 0;

		boost::shared_ptr<IpfixOptionsTemplateRecord> ipfixRecord(new IpfixOptionsTemplateRecord);
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->optionsTemplateInfo = ti;
		push(ipfixRecord);
	}
}

/**
 * Processes an IPFIX DataTemplate set.
 * Called by processMessage
 */
void IpfixParser::processDataTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set) {
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* DataTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		IpfixDataTemplateHeader* th = (IpfixDataTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			continue;
		}
		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> ti(new IpfixRecord::DataTemplateInfo);
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->dataTemplateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->preceding = ntohs(th->precedingRule);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->dataCount = ntohs(th->dataCount);
		ti->fieldInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}

		ti->dataInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->dataInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			if (ti->dataInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->dataInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->dataInfo[fieldNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}

		/* done with reading dataInfo, @c record now points to the fixed data block */
		uint8_t* dataStart = record;

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
		ti->data = (uint8_t*)malloc(dataLength);
		memcpy(ti->data,dataStart,dataLength);

		/* Advance record to end of fixed data block, i.e. start of next template*/
		record += dataLength;

		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLivetime > 0))
			bt->expires = time(0) + templateLivetime;
		else
			bt->expires = 0;

		boost::shared_ptr<IpfixDataTemplateRecord> ipfixRecord(new IpfixDataTemplateRecord);
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->dataTemplateInfo = ti;
		push(ipfixRecord);
	}
}

/**
 * Processes an IPFIX data set.
 * Called by processMessage
 */
uint32_t IpfixParser::processDataSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set) {
	TemplateBuffer::BufferedTemplate* bt = templateBuffer->getBufferedTemplate(sourceId, ntohs(set->id));
	uint32_t numberOfRecords = 0;

	if (bt == 0) {
		/* this error may come in rapid succession; I hope I don't regret it */
		msg(MSG_INFO, "Template %d unknown to collecting process, got from ", ntohs(set->id));
		if(sourceId->exporterAddress.len == 4)
		printf("%d.%d.%d.%d", (uint8_t)sourceId->exporterAddress.ip[3], (uint8_t)sourceId->exporterAddress.ip[2], (uint8_t)sourceId->exporterAddress.ip[1], (uint8_t)sourceId->exporterAddress.ip[0]);
		else
			printf("non-IPv4 address");
		printf(":%d (", sourceId->exporterPort);
		/* we need a FieldInfo for printIPv4 */
		IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
		tmpInfo.length = 1; // length=1 for protocol identifier
		printProtocol(tmpInfo, &sourceId->protocol);
		printf(")\n");
		return 0;
	}
        
#ifdef SUPPORT_NETFLOWV9
	if ((bt->setID == IPFIX_SetId_Template) || (bt->setID == NetflowV9_SetId_Template)) {
#else
	if (bt->setID == IPFIX_SetId_Template) {
#endif

		boost::shared_ptr<IpfixRecord::TemplateInfo> ti = bt->templateInfo;
        
		uint16_t length = ntohs(set->length)-((uint8_t*)(&set->data)-(uint8_t*)set);

		uint8_t* record = &set->data;
        
		if (bt->recordLength < 65535) {
			uint8_t* recordX = record+length;
        
			if (record >= recordX - (bt->recordLength - 1)) {
				DPRINTF("Got a Data Set that contained not a single full record\n");
			}

			/* We stop processing when no full record is left */
			while (record < recordX - (bt->recordLength - 1)) {
				boost::shared_ptr<IpfixDataRecord> ipfixRecord(new IpfixDataRecord);
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->templateInfo = ti;
				ipfixRecord->dataLength = bt->recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				push(ipfixRecord);
				record = record + bt->recordLength;
				numberOfRecords++;
			}
		} else {
			uint8_t* recordX = record+length;

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

				boost::shared_ptr<IpfixDataRecord> ipfixRecord(new IpfixDataRecord);
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->templateInfo = ti;
				ipfixRecord->dataLength = recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				push(ipfixRecord);
				record = record + recordLength;
				numberOfRecords++;
			}
		}
	} else {
		if (bt->setID == IPFIX_SetId_OptionsTemplate) {

			boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> ti = bt->optionsTemplateInfo;

			uint16_t length = ntohs(set->length)-((uint8_t*)(&set->data)-(uint8_t*)set);
			uint8_t* record = &set->data;

			if (bt->recordLength < 65535) {
				uint8_t* recordX = record+length;
				while (record < recordX) {
					boost::shared_ptr<IpfixOptionsRecord> ipfixRecord(new IpfixOptionsRecord);
					ipfixRecord->sourceID = sourceId;
					ipfixRecord->optionsTemplateInfo = ti;
					ipfixRecord->dataLength = bt->recordLength;
					ipfixRecord->message = message;
					ipfixRecord->data = record;
					push(ipfixRecord);
					record = record + bt->recordLength;
					numberOfRecords++;
				}
			} else {
				uint8_t* recordX = record+length;
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

					boost::shared_ptr<IpfixOptionsRecord> ipfixRecord(new IpfixOptionsRecord);
					ipfixRecord->sourceID = sourceId;
					ipfixRecord->optionsTemplateInfo = ti;
					ipfixRecord->dataLength = recordLength;
					ipfixRecord->message = message;
					ipfixRecord->data = record;
					push(ipfixRecord);
					record = record + recordLength;
					numberOfRecords++;
				}
			}
		} else {
			if (bt->setID == IPFIX_SetId_DataTemplate) {
				boost::shared_ptr<IpfixRecord::DataTemplateInfo> ti = bt->dataTemplateInfo;

				uint16_t length = ntohs(set->length)-((uint8_t*)(&set->data)-(uint8_t*)set);
				uint8_t* record = &set->data;

				if (bt->recordLength < 65535) {
					uint8_t* recordX = record+length;
					while (record < recordX) {

						boost::shared_ptr<IpfixDataDataRecord> ipfixRecord(new IpfixDataDataRecord);
						ipfixRecord->sourceID = sourceId;
						ipfixRecord->dataTemplateInfo = ti;
						ipfixRecord->dataLength = bt->recordLength;
						ipfixRecord->message = message;
						ipfixRecord->data = record;
						push(ipfixRecord);
						record = record + bt->recordLength;
						numberOfRecords++;
					}
				} else {
					uint8_t* recordX = record+length;
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

						boost::shared_ptr<IpfixDataDataRecord> ipfixRecord(new IpfixDataDataRecord);
						ipfixRecord->sourceID = sourceId;
						ipfixRecord->dataTemplateInfo = ti;
						ipfixRecord->dataLength = recordLength;
						ipfixRecord->message = message;
						ipfixRecord->data = record;
						push(ipfixRecord);
						record = record + recordLength;
						numberOfRecords++;
					}
				}	
			} else {
				msg(MSG_FATAL, "Data Set based on known but unhandled template type %d", bt->setID);
			}
		}
	}
	return numberOfRecords;
}

        
/**
 * Process a NetflowV9 Packet
 * @return 0 on success
 */
int IpfixParser::processNetflowV9Packet(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) {
	NetflowV9Header* header = (NetflowV9Header*)message.get();

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	int i;

	sourceId->observationDomainId = ntohl(header->observationDomainId);

	for (i = 0; i < ntohs(header->setCount); i++) {
		if (ntohs(set->id) == NetflowV9_SetId_Template) {
			processTemplateSet(sourceId, message, set);
		} else
			if (ntohs(set->id) >= IPFIX_SetId_Data_Start) {
				processDataSet(sourceId, message, set);
			} else {
				msg(MSG_ERROR, "Unsupported Set ID - expected 0/256+, got %d", ntohs(set->id));
			}
		set = (IpfixSetHeader*)((uint8_t*)set + ntohs(set->length));
	}

	return 0;
}

/**
 * Process an IPFIX Packet
 * @return 0 on success
 */
int IpfixParser::processIpfixPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) {
	IpfixHeader* header = (IpfixHeader*)message.get();
	sourceId->observationDomainId = ntohl(header->observationDomainId);

	if (ntohs(header->length) != length) {
		DPRINTF("Bad message length - expected %#06x, got %#06x\n", length, ntohs(header->length));
		return -1;
	}

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	/* pointer beyond message */
	IpfixSetHeader* setX = (IpfixSetHeader*)((char*)message.get() + length); 

	uint16_t tmpid;
	uint32_t numberOfDataRecords = 0;
	while(set < setX) {
		tmpid=ntohs(set->id);

		switch(tmpid) {
		case IPFIX_SetId_DataTemplate:
			processDataTemplateSet(sourceId, message, set);
			break;
		case IPFIX_SetId_Template:
			processTemplateSet(sourceId, message, set);
			break;
		case IPFIX_SetId_OptionsTemplate:
			processOptionsTemplateSet(sourceId, message, set);
			break;
		default:
			if(tmpid >= IPFIX_SetId_Data_Start) {
				numberOfDataRecords += processDataSet(sourceId, message, set);
			} else {
				msg(MSG_ERROR, "processIpfixPacket: Unsupported Set ID - expected 2/3/4/256+, got %d", tmpid);
			}
		}
		set = (IpfixSetHeader*)((uint8_t*)set + ntohs(set->length));
	}

	msg(MSG_DEBUG, "Message contained %u records, sequence number was %u", numberOfDataRecords, ntohl(header->sequenceNo));

	return 0;
}

/**
 * Process new Message
 * @return 0 on success
 */
int IpfixParser::processPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId)
{
	pthread_mutex_lock(&mutex);
	IpfixHeader* header = (IpfixHeader*)message.get();
	if (ntohs(header->version) == 0x000a) {
		int r = processIpfixPacket(message, length, sourceId);
		pthread_mutex_unlock(&mutex);
		return r;
	}
#ifdef SUPPORT_NETFLOWV9
	if (ntohs(header->version) == 0x0009) {
		int r = processNetflowV9Packet(message, length, sourceId);
		pthread_mutex_unlock(&mutex);
		return r;
	}
	DPRINTF("Bad message version - expected 0x009 or 0x000a, got %#06x\n", ntohs(header->version));
	pthread_mutex_unlock(&mutex);
	return -1;
#else
	DPRINTF("Bad message version - expected 0x000a, got %#06x\n", ntohs(header->version));
	pthread_mutex_unlock(&mutex);
	return -1;
#endif
}


/**
 * Sets the template livetime
 */
void IpfixParser::setTemplateLivetime(uint16_t time) {
	templateLivetime = time;
}

/**
 * Creates a new  @c IpfixParser.
 * @return handle to created instance
 */
IpfixParser::IpfixParser() : templateLivetime(DEFAULT_TEMPLATE_EXPIRE_SECS) {

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		THROWEXCEPTION("IpfixParser creation failed");
	}

	templateBuffer = new TemplateBuffer(this);

}

/**
 * Frees memory used by an IpfixParser.
 */
IpfixParser::~IpfixParser() {

	delete(templateBuffer);

	pthread_mutex_destroy(&mutex);

}


