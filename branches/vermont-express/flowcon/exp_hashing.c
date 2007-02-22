/** @file
 * Hashing sub-module.
 *
 * The hashing module receives flows from higher levels (see @c aggregateTemplateData(), @c aggregateDataTemplateData()),
 * collects them in Buffers (@c Hashtable), then passes them on to lower levels by calling the
 * appropriate callback functions (see @c setNewDataTemplateCallback(), @c setNewDataDataRecordCallback(), @c setNewDataTemplateDestructionCallback()).
 *
 * Flows that differ only in aggregatable fields (like @c IPFIX_TYPEID_inOctetDeltaCount) are
 * aggregated (see @c aggregateFlow()).
 * If for a buffered flow no new aggregatable flows arrive for a certain timespan (@c Hashtable::minBufferTime)
 * or the flow was kept buffered for a certain amount of time (@c Hashtable::maxBufferTime) it is
 * passed on to lower levels (i.e. exported) and removed from the hashtable.
 *
 * Polling for expired flows is accomplished by periodically calling @c expireFlows().
 *
 * Each @c Hashtable contains some fixed-value IPFIX fields @c Hashtable.data
 * described by the @c Hashtable.dataInfo array. The remaining, variable-value
 * fields are stored in @c Hashtable.bucket[].data structures described by the
 * @c Hashtable.fieldInfo array.
 *
 */

/******************************************************************************

IPFIX Concentrator
Copyright (C) 2005 Christoph Sommer
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

#include <string.h>
#include <netinet/in.h>
#include <time.h>

#include "exp_hashing.h"
#include "exp_crc16.h"
#include "exp_ipfix.h"

#include "ipfixlolib/ipfixlolib.h"

#include "msg.h"

/**
 * Initializes memory for a new bucket in @c ht containing @c data
 */
static ExpressHashBucket* ExpresscreateBucket(ExpressHashtable* ht, FieldData* data)
{
	ExpressHashBucket* bucket = (ExpressHashBucket*)malloc(sizeof(ExpressHashBucket));

	bucket->expireTime = time(0) + ht->minBufferTime;
	bucket->forceExpireTime = time(0) + ht->maxBufferTime;
	bucket->data = data;
	bucket->next = 0;

	return bucket;
}

/**
 * Exports the given @c bucket
 */
static void ExpressexportBucket(ExpressHashtable* ht, ExpressHashBucket* bucket)
{
	/* Pass Data Record to exporter interface */
	int n;
	for (n = 0; n < ht->callbackCount; n++) {
		ExpressCallbackInfo* ci = &ht->callbackInfo[n];
		if (ci->dataDataRecordCallbackFunction) {
			ci->dataDataRecordCallbackFunction(ci->handle, 0, ht->dataTemplate, ht->fieldLength, bucket->data);
		}
	}

	ht->recordsSent++;
}

/**
 * De-allocates memory used by the given @c bucket
 */
static void ExpressdestroyBucket(ExpressHashtable* ht, ExpressHashBucket* bucket)
{
	free(bucket->data); //TODO: is this correct?
	free(bucket);
}

/**
 * Creates and initializes a new hashtable buffer for flows matching @c rule
 */
ExpressHashtable* ExpresscreateHashtable(ExpressRule* rule, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	int i;
	int dataLength = 0; /**< length in bytes of the @c ht->data field */
	ExpressHashtable* ht = (ExpressHashtable*)malloc(sizeof(ExpressHashtable));

	ht->callbackCount = 0;
	ht->callbackInfo = 0;
	ht->minBufferTime = minBufferTime;
	ht->maxBufferTime = maxBufferTime;
	ht->bucketCount = HASHTABLE_SIZE;

	ht->recordsReceived = 0;
	ht->recordsSent = 0;

	for(i = 0; i < ht->bucketCount; i++) {
		ht->bucket[i] = NULL;
	}

	ht->dataTemplate = (ExpressDataTemplateInfo*)malloc(sizeof(ExpressDataTemplateInfo));
	ht->dataTemplate->id=rule->id;
	ht->dataTemplate->preceding=rule->preceding;
	ht->dataTemplate->fieldCount = 0;
	ht->dataTemplate->fieldInfo = NULL;
	ht->fieldLength = 0;
	ht->dataTemplate->dataCount = 0;
	ht->dataTemplate->dataInfo = NULL;
	ht->dataTemplate->data = NULL;
	ht->dataTemplate->userData = NULL;

	ht->fieldModifier = (ExpressFieldModifier*)malloc(rule->fieldCount * sizeof(ExpressFieldModifier));

	for (i = 0; i < rule->fieldCount; i++) {
		ExpressRuleField* rf = rule->field[i];

		if (rf->pattern != NULL) {
			/* create new fixed-data field containing pattern */
			ht->dataTemplate->dataCount++;
			ht->dataTemplate->dataInfo = realloc(ht->dataTemplate->dataInfo, sizeof(ExpressFieldInfo) * ht->dataTemplate->dataCount);
			ExpressFieldInfo* fi = &ht->dataTemplate->dataInfo[ht->dataTemplate->dataCount - 1];
			fi->type = rf->type;
			fi->offset = dataLength;
			dataLength += fi->type.length;
			ht->dataTemplate->data = realloc(ht->dataTemplate->data, dataLength);
			memcpy(ht->dataTemplate->data + fi->offset, rf->pattern, fi->type.length);
		}

		if (rf->modifier != FIELD_MODIFIER_DISCARD) {
			/* define new data field with RuleField's type */
			ht->dataTemplate->fieldCount++;
			ht->dataTemplate->fieldInfo = realloc(ht->dataTemplate->fieldInfo, sizeof(ExpressFieldInfo) * ht->dataTemplate->fieldCount);
			ExpressFieldInfo* fi = &ht->dataTemplate->fieldInfo[ht->dataTemplate->fieldCount - 1];
			fi->type = rf->type;
			fi->offset = ht->fieldLength;
			ht->fieldLength += fi->type.length;
			ht->fieldModifier[ht->dataTemplate->fieldCount - 1] = rf->modifier;
		}

	}

	/* Informing the Exporter of a new Data Template is done when adding the callback functions */
	return ht;
}

/**
 * De-allocates memory of the given hashtable buffer.
 * All remaining Buckets are exported, then destroyed
 */
void ExpressdestroyHashtable(ExpressHashtable* ht)
{
	int i;

	for (i = 0; i < HASHTABLE_SIZE; i++) if (ht->bucket[i] != NULL) {
		ExpressHashBucket* bucket = ht->bucket[i];

		while (bucket != 0) {
			ExpressHashBucket* nextBucket = (ExpressHashBucket*)bucket->next;
			ExpressexportBucket(ht, bucket);
			ExpressdestroyBucket(ht, bucket);
			bucket = nextBucket;
		}
	}

	/* Inform Exporter of Data Template destruction */
	for (i = 0; i < ht->callbackCount; i++) {
		ExpressCallbackInfo* ci = &ht->callbackInfo[i];

		if (ci->dataTemplateDestructionCallbackFunction) {
			ci->dataTemplateDestructionCallbackFunction(ci->handle, 0, ht->dataTemplate);
		}
	}

	free(ht->dataTemplate->fieldInfo);
	free(ht->fieldModifier);
	free(ht->dataTemplate->dataInfo);
	free(ht->dataTemplate->data);
	free(ht->dataTemplate);
	free(ht);
}

/**
 * Exports all expired flows and removes them from the buffer
 */
void ExpressexpireFlows(ExpressHashtable* ht)
{
	int i;
	uint32_t now = time(0);

	/* check each hash bucket's spill chain */
	for (i = 0; i < ht->bucketCount; i++) if (ht->bucket[i] != 0) {
		ExpressHashBucket* bucket = ht->bucket[i];
		ExpressHashBucket* pred = 0;

		/* iterate over spill chain */
		while (bucket != 0) {
			ExpressHashBucket* nextBucket = bucket->next;
			if ((now > bucket->expireTime) || (now > bucket->forceExpireTime)) {
				if(now > bucket->expireTime) {
					DPRINTF("expireFlows: normal expiry\n");
				}
				if(now > bucket->forceExpireTime) {
					DPRINTF("expireFlows: forced expiry\n");
				}

				ExpressexportBucket(ht, bucket);
				ExpressdestroyBucket(ht, bucket);
				if (pred) {
					pred->next = nextBucket;
				} else {
					ht->bucket[i] = nextBucket;
				}
			} else {
				pred = bucket;
			}

			bucket = nextBucket;
		}
	}
}

/**
 * Returns the sum of two uint32_t values in network byte order
 */
static uint64_t ExpressaddUint64Nbo(uint64_t i, uint64_t j)
{
	return (htonll(ntohll(i) + ntohll(j)));
}

/**
 * Returns the sum of two uint32_t values in network byte order
 */
static uint32_t ExpressaddUint32Nbo(uint32_t i, uint32_t j)
{
	return (htonl(ntohl(i) + ntohl(j)));
}

/**
 * Returns the sum of two uint16_t values in network byte order
 */
static uint16_t ExpressaddUint16Nbo(uint16_t i, uint16_t j)
{
	return (htons(ntohs(i) + ntohs(j)));
}

/**
 * Returns the sum of two uint8_t values in network byte order.
 * As if we needed this...
 */
static uint8_t ExpressaddUint8Nbo(uint8_t i, uint8_t j)
{
	return (i + j);
}

/**
 * Returns the lesser of two uint32_t values in network byte order
 */
static uint32_t ExpresslesserUint32Nbo(uint32_t i, uint32_t j)
{
	return (ntohl(i) < ntohl(j))?(i):(j);
}

/**
 * Returns the greater of two uint32_t values in network byte order
 */
static uint32_t ExpressgreaterUint32Nbo(uint32_t i, uint32_t j)
{
	return (ntohl(i) > ntohl(j))?(i):(j);
}

/**
 * Checks whether the given @c type is one of the types that has to be aggregated
 * @return 1 if flow is to be aggregated
 */
static int ExpressisToBeAggregated(ExpressFieldType type)
{
	switch (type.id) {
	case IPFIX_TYPEID_flowStartSysUpTime:
	case IPFIX_TYPEID_flowStartSeconds:
	case IPFIX_TYPEID_flowStartMilliSeconds:
	case IPFIX_TYPEID_flowStartMicroSeconds:
	case IPFIX_TYPEID_flowStartNanoSeconds:
		return 1;

	case IPFIX_TYPEID_flowEndSysUpTime:
	case IPFIX_TYPEID_flowEndSeconds:
	case IPFIX_TYPEID_flowEndMilliSeconds:
	case IPFIX_TYPEID_flowEndMicroSeconds:
	case IPFIX_TYPEID_flowEndNanoSeconds:
		return 1;

	case IPFIX_TYPEID_octetDeltaCount:
	case IPFIX_TYPEID_postOctetDeltaCount:
	case IPFIX_TYPEID_packetDeltaCount:
	case IPFIX_TYPEID_postPacketDeltaCount:
	case IPFIX_TYPEID_droppedOctetDeltaCount:
	case IPFIX_TYPEID_droppedPacketDeltaCount:
		return 1;

	case IPFIX_TYPEID_octetTotalCount:
	case IPFIX_TYPEID_packetTotalCount:
	case IPFIX_TYPEID_droppedOctetTotalCount:
	case IPFIX_TYPEID_droppedPacketTotalCount:
	case IPFIX_TYPEID_postMCastPacketDeltaCount:
	case IPFIX_TYPEID_postMCastOctetDeltaCount:
	case IPFIX_TYPEID_observedFlowTotalCount:
	case IPFIX_TYPEID_exportedOctetTotalCount:
	case IPFIX_TYPEID_exportedMessageTotalCount:
	case IPFIX_TYPEID_exportedFlowTotalCount:
		DPRINTF("isToBeAggregated: Will not aggregate %s field", Expresstypeid2string(type.id));
		return 0;

	default:
		return 0;
	}
}

/**
 * Adds (or otherwise aggregates) @c deltaData to @c baseData
 */
static int ExpressaggregateField(ExpressFieldType* type, FieldData* baseData, FieldData* deltaData)
{
	switch (type->id) {

	case IPFIX_TYPEID_flowStartSysUpTime:
	case IPFIX_TYPEID_flowStartSeconds:
	case IPFIX_TYPEID_flowStartMilliSeconds:
	case IPFIX_TYPEID_flowStartMicroSeconds:
	case IPFIX_TYPEID_flowStartNanoSeconds:
		if (type->length != 4) {
			DPRINTF("aggregateField: unsupported length: %d", type->length);
                        goto out;
		}

		*(uint32_t*)baseData = ExpresslesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
		break;

	case IPFIX_TYPEID_flowEndSysUpTime:
	case IPFIX_TYPEID_flowEndSeconds:
	case IPFIX_TYPEID_flowEndMilliSeconds:
	case IPFIX_TYPEID_flowEndMicroSeconds:
	case IPFIX_TYPEID_flowEndNanoSeconds:
		if (type->length != 4) {
			DPRINTF("aggregateField: unsupported length: %d", type->length);
			goto out;
		}

		*(uint32_t*)baseData = ExpressgreaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
		break;

	case IPFIX_TYPEID_octetDeltaCount:
	case IPFIX_TYPEID_postOctetDeltaCount:
	case IPFIX_TYPEID_packetDeltaCount:
	case IPFIX_TYPEID_postPacketDeltaCount:
	case IPFIX_TYPEID_droppedOctetDeltaCount:
	case IPFIX_TYPEID_droppedPacketDeltaCount:
		switch (type->length) {
		case 1:
			*(uint8_t*)baseData = ExpressaddUint8Nbo(*(uint8_t*)baseData, *(uint8_t*)deltaData);
			return 0;
		case 2:
			*(uint16_t*)baseData = ExpressaddUint16Nbo(*(uint16_t*)baseData, *(uint16_t*)deltaData);
                        return 0;
		case 4:
			*(uint32_t*)baseData = ExpressaddUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
                        return 0;
		case 8:
			*(uint64_t*)baseData = ExpressaddUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
                        return 0;
		default:
			DPRINTF("aggregateField: unsupported length: %d", type->length);
			goto out;
		}
		break;

	default:
		DPRINTF("aggregateField: non-aggregatable type: %d", type->id);
                goto out;
		break;
	}

	return 0;
out:
        return 1;
}

/**
 * Adds (or otherwise aggregates) pertinent fields of @c flow to @c baseFlow
 */
static int ExpressaggregateFlow(ExpressHashtable* ht, FieldData* baseFlow, FieldData* flow)
{
	int i;

	if(!baseFlow) {
		DPRINTF("aggregateFlow: baseFlow is NULL");
		return 1;
	}

	if(!flow){
		DPRINTF("aggregateFlow: flow is NULL");
		return 1;
	}

	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		ExpressFieldInfo* fi = &ht->dataTemplate->fieldInfo[i];

		if(!ExpressisToBeAggregated(fi->type)) {
			continue;
		}
		ExpressaggregateField(&fi->type, baseFlow + fi->offset, flow + fi->offset);
	}

	return 0;
}

/**
 * Returns a hash value corresponding to all variable, non-aggregatable fields of a flow
 */
static uint16_t ExpressgetHash(ExpressHashtable* ht, FieldData* data)
{
	int i;

	uint16_t hash = 0;
	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		if(ExpressisToBeAggregated(ht->dataTemplate->fieldInfo[i].type)) {
			continue;
		}
		hash = crc16(hash,
			     ht->dataTemplate->fieldInfo[i].type.length,
			     data + ht->dataTemplate->fieldInfo[i].offset
			    );
	}

	return hash;
}

/**
 * Checks if two data fields are binary equal
 * @return 1 if fields are equal
 */
static int ExpressequalRaw(ExpressFieldType* data1Type, FieldData* data1, ExpressFieldType* data2Type, FieldData* data2)
{
	int i;

	if(data1Type->id != data2Type->id) return 0;
	if(data1Type->length != data2Type->length) return 0;
	if(data1Type->eid != data2Type->eid) return 0;

	for(i = 0; i < data1Type->length; i++) {
		if(data1[i] != data2[i]) {
			return 0;
		}
	}

	return 1;
}

/**
 * Checks if all of two flows' (non-aggregatable) fields are binary equal
 * @return 1 if fields are equal
 */
static int ExpressequalFlow(ExpressHashtable* ht, FieldData* flow1, FieldData* flow2)
{
	int i;

	if(flow1 == flow2) return 1;

	for(i = 0; i < ht->dataTemplate->fieldCount; i++) {
		ExpressFieldInfo* fi = &ht->dataTemplate->fieldInfo[i];

		if(ExpressisToBeAggregated(fi->type)) {
			continue;
		}

		if(!ExpressequalRaw(&fi->type, flow1 + fi->offset, &fi->type, flow2 + fi->offset)) {
			return 0;
		}
	}

	return 1;
}

/**
 * Inserts a data block into the hashtable
 */
static void ExpressbufferDataBlock(ExpressHashtable* ht, FieldData* data)
{
	ht->recordsReceived++;

	uint16_t hash = ExpressgetHash(ht, data);
	ExpressHashBucket* bucket = ht->bucket[hash];

	if (bucket == 0) {
		/* This slot is still free, place the bucket here */
		DPRINTF("bufferDataBlock: creating bucket\n");
		ht->bucket[hash] = ExpresscreateBucket(ht, data);
		return;
	}

	/* This slot is already used, search spill chain for equal flow */
	while(1) {
		if (ExpressequalFlow(ht, bucket->data, data)) {
			DPRINTF("appending to bucket\n");

			ExpressaggregateFlow(ht, bucket->data, data);
			bucket->expireTime = time(0) + ht->minBufferTime;

			/* The flow's data block is no longer needed */
			free(data);
			break;
		}

		if (bucket->next == 0) {
			DPRINTF("creating bucket\n");

			bucket->next = ExpresscreateBucket(ht, data);
			break;
		}

		bucket = (ExpressHashBucket*)bucket->next;
	}
}

/**
 * Copies \c srcData to \c dstData applying \c modifier.
 * Takes care to pad \c srcData with zero-bytes in case it is shorter than \c dstData.
 */
static void ExpresscopyData(ExpressFieldType* dstType, FieldData* dstData, ExpressFieldType* srcType, FieldData* srcData, ExpressFieldModifier modifier)
{
	if((dstType->id != srcType->id) || (dstType->eid != srcType->eid)) {
		DPRINTF("copyData: Tried to copy field to destination of different type\n");
		return;
	}

	/* Copy data, care for length differences */
	if(dstType->length == srcType->length) {
		memcpy(dstData, srcData, srcType->length);

	} else if(dstType->length > srcType->length) {

		/* TODO: We simply pad with zeroes - will this always be correct? */
		if((dstType->id == IPFIX_TYPEID_sourceIPv4Address) || (dstType->id == IPFIX_TYPEID_destinationIPv4Address)) {
			/* Fields of type IPv4Address-type are padded on the right */
			bzero(dstData, dstType->length);
			memcpy(dstData, srcData, srcType->length);
		} else {
			/* TODO: all other types are padded on the left, i.e. the "big" end */
			bzero(dstData, dstType->length);
			memcpy(dstData + dstType->length - srcType->length, srcData, srcType->length);
		}

	} else {
		DPRINTF("Target buffer too small. Buffer expected %s of length %d, got one with length %d\n", Expresstypeid2string(dstType->id), dstType->length, srcType->length);
		return;
	}

	/* Apply modifier */
	if(modifier == FIELD_MODIFIER_DISCARD) {
		DPRINTF("Tried to copy data w/ having field modifier set to discard\n");
		return;
	} else if((modifier == FIELD_MODIFIER_KEEP) || (modifier == FIELD_MODIFIER_AGGREGATE)) {

	} else if((modifier >= FIELD_MODIFIER_MASK_START) && (modifier <= FIELD_MODIFIER_MASK_END)) {

		if((dstType->id != IPFIX_TYPEID_sourceIPv4Address) && (dstType->id != IPFIX_TYPEID_destinationIPv4Address)) {
			DPRINTF("Tried to apply mask to %s field\n", Expresstypeid2string(dstType->id));
			return;
		}

		if (dstType->length != 5) {
			DPRINTF("Destination data to short - no room to store mask\n");
			return;
		}

		uint8_t imask = 32 - (modifier - FIELD_MODIFIER_MASK_START);
		dstData[4] = imask; /* store the inverse network mask */

		if (imask > 0) {
			if (imask == 8) {
				dstData[3] = 0x00;
			} else if (imask == 16) {
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 24) {
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 32) {
				dstData[0] = 0x00;
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else {
				int pattern = 0;
				int i;
				for(i = 0; i < imask; i++) {
					pattern |= (1 << i);
				}

				*(uint32_t*)dstData = htonl(ntohl(*(uint32_t*)(dstData)) & ~pattern);
			}
		}

	} else {
		DPRINTF("Unhandled field modifier: %d\n", modifier);
		return;
	}
}


/**
 * Copies \c srcData to \c dstData applying \c modifier.
 * Takes care to pad \c srcData with zero-bytes in case it is shorter than \c dstData.
 * modified version for flowcon
 */
static void ExpresscopyDatalight(ExpressFieldType* dstType, FieldData* dstData, ExpressFieldType* srcType, FieldData* srcData, ExpressFieldModifier modifier)
{
	if((dstType->id != srcType->id)) {
		DPRINTF("copyData: Tried to copy field to destination of different type\n");
		return;
	}

	/* Copy data, care for length differences */
	if(dstType->length == srcType->length) {
		memcpy(dstData, srcData, srcType->length);

	} else if(dstType->length > srcType->length) {

		/* TODO: We simply pad with zeroes - will this always be correct? */
		if((dstType->id == IPFIX_TYPEID_sourceIPv4Address) || (dstType->id == IPFIX_TYPEID_destinationIPv4Address)) {
			/* Fields of type IPv4Address-type are padded on the right */
			bzero(dstData, dstType->length);
			memcpy(dstData, srcData, srcType->length);
		} else {
			/* TODO: all other types are padded on the left, i.e. the "big" end */
			bzero(dstData, dstType->length);
			memcpy(dstData + dstType->length - srcType->length, srcData, srcType->length);
		}

	} else {
		DPRINTF("Target buffer too small. Buffer expected %s of length %d, got one with length %d\n", Expresstypeid2string(dstType->id), dstType->length, srcType->length);
		return;
	}

	/* Apply modifier */
	if(modifier == FIELD_MODIFIER_DISCARD) {
		DPRINTF("Tried to copy data w/ having field modifier set to discard\n");
		return;
	} else if((modifier == FIELD_MODIFIER_KEEP) || (modifier == FIELD_MODIFIER_AGGREGATE)) {

	} else if((modifier >= FIELD_MODIFIER_MASK_START) && (modifier <= FIELD_MODIFIER_MASK_END)) {

		if((dstType->id != IPFIX_TYPEID_sourceIPv4Address) && (dstType->id != IPFIX_TYPEID_destinationIPv4Address)) {
			DPRINTF("Tried to apply mask to %s field\n", Expresstypeid2string(dstType->id));
			return;
		}

		if (dstType->length != 5) {
			DPRINTF("Destination data to short - no room to store mask\n");
			return;
		}

		uint8_t imask = 32 - (modifier - FIELD_MODIFIER_MASK_START);
		dstData[4] = imask; /* store the inverse network mask */

		if (imask > 0) {
			if (imask == 8) {
				dstData[3] = 0x00;
			} else if (imask == 16) {
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 24) {
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 32) {
				dstData[0] = 0x00;
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else {
				int pattern = 0;
				int i;
				for(i = 0; i < imask; i++) {
					pattern |= (1 << i);
				}

				*(uint32_t*)dstData = htonl(ntohl(*(uint32_t*)(dstData)) & ~pattern);
			}
		}

	} else {
		DPRINTF("Unhandled field modifier: %d\n", modifier);
		return;
	}
}



/**
 * Buffer passed flow in Hashtable @c ht
 */
	void ExpressaggregateTemplateData(ExpressHashtable* ht, FieldData* data, int transport_offset)
{
	int i;

	/* Create data block to be inserted into buffer... */
	FieldData* htdata = (FieldData*)malloc(ht->fieldLength);

	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		ExpressFieldInfo* hfi = &ht->dataTemplate->fieldInfo[i];
		int tfi = ExpressgetFieldInfo(hfi->type, transport_offset);
		int tfil = ExpressgetFieldLength(hfi->type);

		if(tfi == 999) {
			DPRINTF("Flow to be buffered did not contain %s field\n", Expresstypeid2string(hfi->type.id));
			continue;
		}

		ExpresscopyDatalight(&hfi->type, htdata + hfi->offset, &(ExpressFieldType){.id = hfi->type.id, .length=tfil} , data + tfi, ht->fieldModifier[i]);

	}

	/* ...then buffer it */
	ExpressbufferDataBlock(ht, htdata);
}

/**
 * Buffer passed flow (containing fixed-value fields) in Hashtable @c ht
 */
void ExpressaggregateDataTemplateData(ExpressHashtable* ht, ExpressDataTemplateInfo* ti, FieldData* data)
{
	int i;

	/* Create data block to be inserted into buffer... */
	FieldData* htdata = (FieldData*)malloc(ht->fieldLength);

	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		ExpressFieldInfo* hfi = &ht->dataTemplate->fieldInfo[i];

		/* Copy from matching variable field, should it exist */
		ExpressFieldInfo* tfi = ExpressgetDataTemplateFieldInfo(ti, &hfi->type);
		if(tfi) {
			ExpresscopyData(&hfi->type, htdata + hfi->offset, &tfi->type, data + tfi->offset, ht->fieldModifier[i]);

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = ExpressgetDataTemplateFieldInfo(ti, &(ExpressFieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = ExpressgetDataTemplateFieldInfo(ti, &(ExpressFieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d", tfi->type.length);
						}
					}
				}
				break;

			default:
				break;
			}

			continue;
		}

		/* No matching variable field. Copy from matching fixed field, should it exist */
		tfi = ExpressgetDataTemplateDataInfo(ti, &hfi->type);
		if(tfi) {
			ExpresscopyData(&hfi->type, htdata + hfi->offset, &tfi->type, ti->data + tfi->offset, ht->fieldModifier[i]);

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = ExpressgetDataTemplateDataInfo(ti, &(ExpressFieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = ExpressgetDataTemplateDataInfo(ti, &(ExpressFieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if (tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			default:
				break;
			}
			continue;
		}

		msg(MSG_FATAL, "AggregateDataTemplateData: Flow to be buffered did not contain %s field", Expresstypeid2string(hfi->type.id));
		continue;
	}

	/* ...then buffer it */
	ExpressbufferDataBlock(ht, htdata);
}

/**
 * Adds a set of callback functions to the list of functions to call when Templates or Records have to be sent
 * @param ht Hashtable to set the callback function for
 * @param handles set of callback functions
 */
void ExpresshashingAddCallbacks(ExpressHashtable* ht, ExpressCallbackInfo handles)
{
	int n;
	int i = ++ht->callbackCount;

	ht->callbackInfo = (ExpressCallbackInfo*)realloc(ht->callbackInfo, i * sizeof(ExpressCallbackInfo));
	memcpy(&ht->callbackInfo[i-1], &handles, sizeof(ExpressCallbackInfo));

	/* Immediately pass the Hashtable's DataTemplate to the new Callback receiver */
	for (n = 0; n < ht->callbackCount; n++) {
		ExpressCallbackInfo* ci = &ht->callbackInfo[n];

		if (ci->dataTemplateCallbackFunction) {
			ci->dataTemplateCallbackFunction(ci->handle, 0, ht->dataTemplate);
		}
	}
}
