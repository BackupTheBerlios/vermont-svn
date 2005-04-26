/** @file
 * Hashing module.
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
#include "common.h"
#include "hashing.h"
#include "crc16.h"
 
/**
 * Initializes memory for a new bucket in @c ht containing @c data
 */
HashBucket* createBucket(Hashtable* ht, FieldData* data) {
	HashBucket* bucket = (HashBucket*)malloc(sizeof(HashBucket));
	bucket->expireTime = time(0) + ht->minBufferTime;
	bucket->forceExpireTime = time(0) + ht->maxBufferTime;
	bucket->data = data;
	bucket->next = 0;

	return bucket;
	}

/**
 * Exports the given @c bucket
 */
void exportBucket(Hashtable* ht, HashBucket* bucket) {
	/* Pass Data Record to exporter interface */
	if (ht->dataDataRecordCallback) ht->dataDataRecordCallback(ht->ipfixSender, ht->dataTemplate, ht->fieldLength, bucket->data);
	}

/**
 * De-allocates memory used by the given @c bucket
 */
void destroyBucket(Hashtable* ht, HashBucket* bucket) {
	free(bucket->data); //TODO: is this correct?
	free(bucket);
	}

/**
 * Creates and initializes a new hashtable buffer for flows matching @c rule
 */
Hashtable* createHashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime) {
	int i;
	int dataLength = 0; /**< length in bytes of the @c ht->data field */
	
	Hashtable* ht = (Hashtable*)malloc(sizeof(Hashtable));
	
	ht->ipfixSender = 0;
	ht->dataTemplateCallback = 0;
	ht->dataDataRecordCallback = 0;
	ht->dataTemplateDestructionCallback = 0;
	ht->minBufferTime = minBufferTime;
	ht->maxBufferTime = maxBufferTime;

	
	ht->bucketCount = HASHTABLE_SIZE;
	for (i = 0; i < ht->bucketCount; i++) ht->bucket[i] = NULL;
	
	ht->dataTemplate = (DataTemplateInfo*)malloc(sizeof(DataTemplateInfo));
	ht->dataTemplate->fieldCount = 0;
	ht->dataTemplate->fieldInfo = NULL;
	ht->fieldLength = 0;
	ht->dataTemplate->dataCount = 0;
	ht->dataTemplate->dataInfo = NULL;
	ht->dataTemplate->data = NULL;
	ht->dataTemplate->userData = NULL;
	
	ht->fieldModifier = (FieldModifier*)malloc(rule->fieldCount * sizeof(FieldModifier));
	
	for (i = 0; i < rule->fieldCount; i++) {
		RuleField* rf = rule->field[i];
		
		if (rf->pattern != NULL) {
			/* create new fixed-data field containing pattern */
			ht->dataTemplate->dataCount++;
			ht->dataTemplate->dataInfo = realloc(ht->dataTemplate->dataInfo, sizeof(FieldInfo) * ht->dataTemplate->dataCount);
			FieldInfo* fi = &ht->dataTemplate->dataInfo[ht->dataTemplate->dataCount - 1];
			fi->type = rf->type;
			fi->offset = dataLength;
			dataLength += fi->type.length;
			ht->dataTemplate->data = realloc(ht->dataTemplate->data, dataLength);
			memcpy(ht->dataTemplate->data + fi->offset, rf->pattern, fi->type.length);
			}
		
		if (rf->modifier != FIELD_MODIFIER_DISCARD) {
			/* define new data field with RuleField's type */
			ht->dataTemplate->fieldCount++;
			ht->dataTemplate->fieldInfo = realloc(ht->dataTemplate->fieldInfo, sizeof(FieldInfo) * ht->dataTemplate->fieldCount);
			FieldInfo* fi = &ht->dataTemplate->fieldInfo[ht->dataTemplate->fieldCount - 1];
			fi->type = rf->type;
			fi->offset = ht->fieldLength;
			ht->fieldLength += fi->type.length;
			ht->fieldModifier[ht->dataTemplate->fieldCount - 1] = rf->modifier;
			}
			
		}
	
	/* Informing the Exporter of a new Data Template is done when setting the callback function */
	
	return ht;
	}
	
/**
 * De-allocates memory of the given hashtable buffer.
 * All remaining Buckets are exported, then destroyed
 */
void destroyHashtable(Hashtable* ht) {
	int i;
	for (i = 0; i < HASHTABLE_SIZE; i++) if (ht->bucket[i] != NULL) {
		HashBucket* bucket = ht->bucket[i];
		while (bucket != 0) {
			HashBucket* nextBucket = (HashBucket*)bucket->next;
			exportBucket(ht, bucket);
			destroyBucket(ht, bucket);
			bucket = nextBucket;
			}
 		}

	/* Inform Exporter of Data Template destruction */
	if (ht->dataTemplateDestructionCallback) ht->dataTemplateDestructionCallback(ht->ipfixSender, ht->dataTemplate);
			
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
void expireFlows(Hashtable* ht) {
	uint32_t now = time(0);
	int i;
	
	/* check each hash bucket's spill chain */
	for (i = 0; i < ht->bucketCount; i++) if (ht->bucket[i] != 0) {
		HashBucket* bucket = ht->bucket[i];
		HashBucket* pred = 0;
		
		/* iterate over spill chain */
		while (bucket != 0) {
			HashBucket* nextBucket = bucket->next;
			if ((now > bucket->expireTime) || (now > bucket->forceExpireTime)) {
				if (now > bucket->expireTime) debug("normal expiry");
				if (now > bucket->forceExpireTime) debug("forced expiry");
				
				exportBucket(ht, bucket);
				destroyBucket(ht, bucket);
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
uint64_t addUint64Nbo(uint64_t i, uint64_t j) {
	return (htonll(ntohll(i) + ntohll(j)));
	}

/**
 * Returns the sum of two uint32_t values in network byte order
 */
uint32_t addUint32Nbo(uint32_t i, uint32_t j) {
	return (htonl(ntohl(i) + ntohl(j)));
	}

/**
 * Returns the sum of two uint16_t values in network byte order
 */
uint16_t addUint16Nbo(uint16_t i, uint16_t j) {
	return (htons(ntohs(i) + ntohs(j)));
	}

/**
 * Returns the sum of two uint8_t values in network byte order.
 * As if we needed this...
 */
uint8_t addUint8Nbo(uint8_t i, uint8_t j) {
	return (i + j);
	}
	
/**
 * Returns the lesser of two uint32_t values in network byte order
 */
uint32_t lesserUint32Nbo(uint32_t i, uint32_t j) {
	uint32_t i2 = ntohl(i);
	uint32_t j2 = ntohl(j);
	return (i2 < j2)?(i2):(j2);
	}

/**
 * Returns the greater of two uint32_t values in network byte order
 */
uint32_t greaterUint32Nbo(uint32_t i, uint32_t j) {
	uint32_t i2 = ntohl(i);
	uint32_t j2 = ntohl(j);
	return (i2 > j2)?(i2):(j2);
	}

/**
 * Checks whether the given @c type is one of the types that has to be aggregated
 * @return 1 if flow is to be aggregated
 */	
int isToBeAggregated(FieldType type) {
	switch (type.id) {
		case IPFIX_TYPEID_flowCreationTime:
			return 1;
		
		case IPFIX_TYPEID_flowEndTime:
			return 1;
			
		case IPFIX_TYPEID_inOctetDeltaCount:
		case IPFIX_TYPEID_outOctetDeltaCount:
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_inPacketDeltaCount:
		case IPFIX_TYPEID_outPacketDeltaCount:
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_droppedOctetDeltaCount:
		case IPFIX_TYPEID_droppedPacketDeltaCount:
			return 1;
			
		case IPFIX_TYPEID_octetTotalCount:
		case IPFIX_TYPEID_packetTotalCount:
		case IPFIX_TYPEID_droppedOctetTotalCount:
		case IPFIX_TYPEID_droppedPacketTotalCount:
		case IPFIX_TYPEID_outMulticastPacketCount:
		case IPFIX_TYPEID_outMulticastOctetCount:
		case IPFIX_TYPEID_observedFlowTotalCount:
		case IPFIX_TYPEID_exportedOctetTotalCount:
		case IPFIX_TYPEID_exportedPacketTotalCount:
		case IPFIX_TYPEID_exportedFlowTotalCount:
			infof("Will not aggregate %s field", typeid2string(type.id));
			return 0;
		
		default:
			return 0;
		}
	}

/**
 * Adds (or otherwise aggregates) @c deltaData to @c baseData
 */	
void aggregateField(FieldType* type, FieldData* baseData, FieldData* deltaData) {
		
	switch (type->id) {
		case IPFIX_TYPEID_flowCreationTime:
			if (type->length != 4) {
				fatalf("unsupported length: %d", type->length);
				return;
				}
			*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;
		
		case IPFIX_TYPEID_flowEndTime:
			if (type->length != 4) {
				fatalf("unsupported length: %d", type->length);
				return;
				}
			*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;
			
		case IPFIX_TYPEID_inOctetDeltaCount:
		case IPFIX_TYPEID_outOctetDeltaCount:
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_inPacketDeltaCount:
		case IPFIX_TYPEID_outPacketDeltaCount:
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_droppedOctetDeltaCount:
		case IPFIX_TYPEID_droppedPacketDeltaCount:
			switch (type->length) {
				case 1:
					*(uint8_t*)baseData = addUint8Nbo(*(uint8_t*)baseData, *(uint8_t*)deltaData);
					return;
				case 2:
					*(uint16_t*)baseData = addUint16Nbo(*(uint16_t*)baseData, *(uint16_t*)deltaData);
					return;
				case 4:
					*(uint32_t*)baseData = addUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return;
				case 8:
					*(uint64_t*)baseData = addUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return;
				default:
					fatalf("unsupported length: %d", type->length);
					return;
				}
			break;
			
		default:
			fatalf("non-aggregatable type: %d", type->id);
			return;
			break;
		}
	}

/**
 * Adds (or otherwise aggregates) pertinent fields of @c flow to @c baseFlow
 */
void aggregateFlow(Hashtable* ht, FieldData* baseFlow, FieldData* flow) {
	int i;
	
	if (!baseFlow) { fatal("baseFlow is NULL"); return; }
	if (!flow) { fatal("flow is NULL"); return; }
	
	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		FieldInfo* fi = &ht->dataTemplate->fieldInfo[i];
		
		if (!isToBeAggregated(fi->type)) continue;
		aggregateField(&fi->type, baseFlow + fi->offset, flow + fi->offset);
		}
	}

/**
 * Returns a hash value corresponding to all variable, non-aggregatable fields of a flow
 */
uint16_t getHash(Hashtable* ht, FieldData* data) {
	int i;
	
	uint16_t hash = 0;
	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		if (isToBeAggregated(ht->dataTemplate->fieldInfo[i].type)) continue;
		hash = crc16(hash, ht->dataTemplate->fieldInfo[i].type.length, data + ht->dataTemplate->fieldInfo[i].offset);
		}
		
	return hash;	
	}	

/**
 * Checks if two data fields are binary equal
 * @return 1 if fields are equal
 */	
int equalRaw(FieldType* data1Type, FieldData* data1, FieldType* data2Type, FieldData* data2) {
	int i;
	
	if (data1Type->id != data2Type->id) return 0;
	if (data1Type->length != data2Type->length) return 0;
	if (data1Type->eid != data2Type->eid) return 0;
	
	for (i = 0; i < data1Type->length; i++) if (data1[i] != data2[i]) return 0;
	
	return 1;
	}
	
/**
 * Checks if all of two flows' (non-aggregatable) fields are binary equal
 * @return 1 if fields are equal
 */	
int equalFlow(Hashtable* ht, FieldData* flow1, FieldData* flow2) {
	int i;
	
	if (flow1 == flow2) return 1;
	
	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		FieldInfo* fi = &ht->dataTemplate->fieldInfo[i];
		if (isToBeAggregated(fi->type)) continue;
		if (!equalRaw(&fi->type, flow1 + fi->offset, &fi->type, flow2 + fi->offset)) return 0;
		}
		
	return 1;
	}

/**
 * Inserts a data block into the hashtable
 */				
void bufferDataBlock(Hashtable* ht, FieldData* data) {
	uint16_t hash = getHash(ht, data);
	
	HashBucket* bucket = ht->bucket[hash];
	
	if (bucket == 0) {
		/* This slot is still free, place the bucket here */
		debug("creating bucket");
		ht->bucket[hash] = createBucket(ht, data);
		return;
		}
	
	/* This slot is already used, search spill chain for equal flow */
	while (1) {
		if (equalFlow(ht, bucket->data, data)) {
			debug("appending to bucket");
			aggregateFlow(ht, bucket->data, data);
			bucket->expireTime = time(0) + ht->minBufferTime;
			/* The flow's data block is no longer needed */
			free(data);
			break;
			}
		if (bucket->next == 0) {
			debug("creating bucket");
			bucket->next = createBucket(ht, data);
			break;
			}
		bucket = (HashBucket*)bucket->next;
		}
	}

/**
 * Copies \c srcData to \c dstData applying \c modifier.
 * Takes care to pad \c srcData with zero-bytes in case it is shorter than \c dstData.
 */
void copyData(FieldType* dstType, FieldData* dstData, FieldType* srcType, FieldData* srcData, FieldModifier modifier) {
	if ((dstType->id != srcType->id) || (dstType->eid != srcType->eid)) {
		fatal("Tried to copy field to destination of different type");
		return;
		}
	
	/* Copy data, care for length differences */
	if (dstType->length == srcType->length) {
		memcpy(dstData, srcData, srcType->length);
		}
	else if (dstType->length > srcType->length) {
		/* TODO: We simply pad with zeroes - will this always be correct? */
		memcpy(dstData, srcData, srcType->length);
		bzero(dstData + srcType->length, dstType->length - srcType->length);
		}
	else {
		fatalf("Target buffer too small. Buffer expected %s of length %d, got one with length %d", typeid2string(dstType->id), dstType->length, srcType->length);
		return;
		}

	/* Apply modifier */	
	if (modifier == FIELD_MODIFIER_DISCARD) {
		fatal("Tried to copy data w/ having field modifier set to discard");
		return;
		}
	else if ((modifier == FIELD_MODIFIER_KEEP) || (modifier == FIELD_MODIFIER_AGGREGATE)) {
		}
	else if ((modifier >= FIELD_MODIFIER_MASK_START) && (modifier <= FIELD_MODIFIER_MASK_END)) {
		if ((dstType->id != IPFIX_TYPEID_sourceIPv4Address) && (dstType->id != IPFIX_TYPEID_destinationIPv4Address)) {
			fatalf("Tried to apply mask to %s field", typeid2string(dstType->id));
			return;
			}
		if (dstType->length != 5) {
			fatal("Destination data to short - no room to store mask");
			return;
			}
			
		uint8_t imask = 32 - (modifier - FIELD_MODIFIER_MASK_START);
		dstData[4] = imask; /* store the inverse network mask */
		
		if (imask > 0) {
			if (imask == 8) {
				dstData[3] = 0x00;
				}
			else if (imask == 16) {
				dstData[2] = 0x00;
				dstData[3] = 0x00;
				}
			else if (imask == 24) {
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
				}
			else if (imask == 32) {
				dstData[0] = 0x00;
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
				}
			else {
				int pattern = 0;
				int i;
				for (i = 0; i < imask; i++) pattern |= (1 << i);
				*(uint32_t*)dstData = htonl(ntohl(*(uint32_t*)(dstData)) & ~pattern);
				}
			}
		
		}
	else {
		fatalf("Unhandled field modifier: %d", modifier);
		return;
		}
	}

/**
 * Buffer passed flow in Hashtable @c ht
 */	
void aggregateTemplateData(Hashtable* ht, TemplateInfo* ti, FieldData* data) {
	int i;
	
	/* Create data block to be inserted into buffer... */
	FieldData* htdata = (FieldData*)malloc(ht->fieldLength);
	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		FieldInfo* hfi = &ht->dataTemplate->fieldInfo[i];
		FieldInfo* tfi = getTemplateFieldInfo(ti, &hfi->type);
		if (!tfi) {
			fatalf("Flow to be buffered did not contain %s field", typeid2string(hfi->type.id));
			continue;
			}

		copyData(&hfi->type, htdata + hfi->offset, &tfi->type, data + tfi->offset, ht->fieldModifier[i]);
		
		/* copy associated mask, should there be one */
		switch (hfi->type.id) {
			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = getTemplateFieldInfo(ti, &(FieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
				if (tfi) {
					if (hfi->type.length != 5) {
						fatalf("Tried to set mask of length %d IP address", hfi->type.length);
						} else {
						if (tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
							} else {
							fatalf("Cannot process associated mask with invalid length %d", tfi->type.length);
							}
						}
					}
				break;
			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = getTemplateFieldInfo(ti, &(FieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
				if (tfi) {
					if (hfi->type.length != 5) {
						fatalf("Tried to set mask of length %d IP address", hfi->type.length);
						} else {
						if (tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
							} else {
							fatalf("Cannot process associated mask with invalid length %d", tfi->type.length);
							}
						}
					}
				break;
			default:
				break;	
			}
		}
	
	/* ...then buffer it */
	bufferDataBlock(ht, htdata);
	}

/**
 * Buffer passed flow (containing fixed-value fields) in Hashtable @c ht
 */	
void aggregateDataTemplateData(Hashtable* ht, DataTemplateInfo* ti, FieldData* data) {
	int i;
	
	/* Create data block to be inserted into buffer... */
	FieldData* htdata = (FieldData*)malloc(ht->fieldLength);
	for (i = 0; i < ht->dataTemplate->fieldCount; i++) {
		FieldInfo* hfi = &ht->dataTemplate->fieldInfo[i];
		
		/* Copy from matching variable field, should it exist */
		FieldInfo* tfi = getDataTemplateFieldInfo(ti, &hfi->type);
		if (tfi) {
			copyData(&hfi->type, htdata + hfi->offset, &tfi->type, data + tfi->offset, ht->fieldModifier[i]);
			
			/* copy associated mask, should there be one */
			switch (hfi->type.id) {
				case IPFIX_TYPEID_sourceIPv4Address:
					tfi = getDataTemplateFieldInfo(ti, &(FieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
					if (tfi) {
						if (hfi->type.length != 5) {
							fatalf("Tried to set mask of length %d IP address", hfi->type.length);
							} else {
							if (tfi->type.length == 1) {
								*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
								} else {
								fatalf("Cannot process associated mask with invalid length %d", tfi->type.length);
								}
							}
						}
					break;
				case IPFIX_TYPEID_destinationIPv4Address:
					tfi = getDataTemplateFieldInfo(ti, &(FieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
					if (tfi) {
						if (hfi->type.length != 5) {
							fatalf("Tried to set mask of length %d IP address", hfi->type.length);
							} else {
							if (tfi->type.length == 1) {
								*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
								} else {
								fatalf("Cannot process associated mask with invalid length %d", tfi->type.length);
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
		tfi = getDataTemplateDataInfo(ti, &hfi->type);
		if (tfi) {
			copyData(&hfi->type, htdata + hfi->offset, &tfi->type, ti->data + tfi->offset, ht->fieldModifier[i]);
			
			/* copy associated mask, should there be one */
			switch (hfi->type.id) {
				case IPFIX_TYPEID_sourceIPv4Address:
					tfi = getDataTemplateDataInfo(ti, &(FieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
					if (tfi) {
						if (hfi->type.length != 5) {
							fatalf("Tried to set mask of length %d IP address", hfi->type.length);
							} else {
							if (tfi->type.length == 1) {
								*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
								} else {
								fatalf("Cannot process associated mask with invalid length %d", tfi->type.length);
								}
							}
						}
					break;
				case IPFIX_TYPEID_destinationIPv4Address:
					tfi = getDataTemplateDataInfo(ti, &(FieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
					if (tfi) {
						if (hfi->type.length != 5) {
							fatalf("Tried to set mask of length %d IP address", hfi->type.length);
							} else {
							if (tfi->type.length == 1) {
								*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
								} else {
								fatalf("Cannot process associated mask with invalid length %d", tfi->type.length);
								}
							}
						}
					break;
				default:
					break;	
				}
			continue;
			}
		
		fatalf("Flow to be buffered did not contain %s field", typeid2string(hfi->type.id));
		continue;
		}
	
	/* ...then buffer it */
	bufferDataBlock(ht, htdata);
	}

/**
 * Sets the function to call when a new DataTemplate has to be exported
 * @param ht handle of Hashtable for which to set this callback function
 * @param ipfixSender handle to pass to callback function. Forced to be the same for all of a Hashtable's Callback functions
 * @param f function to call, @c handle get passed to this function
 */
void setNewDataTemplateCallback(Hashtable* ht, void* ipfixSender, NewDataTemplateCallbackFunction* f) {
	ht->ipfixSender = ipfixSender;
	ht->dataTemplateCallback = f;

	/* we now know who to pass the Hashtable's DataTemplate to, so we immediately pass it on */
	if (ht->dataTemplateCallback) ht->dataTemplateCallback(ht->ipfixSender, ht->dataTemplate);
	}

/**
 * Sets the function to call when a new Data Record with fixed fields has to be exported
 * @param ht handle of Hashtable for which to set this callback function
 * @param ipfixSender handle to pass to callback function. Forced to be the same for all of a Hashtable's Callback functions
 * @param f function to call, @c handle get passed to this function
 */
void setNewDataDataRecordCallback(Hashtable* ht, void* ipfixSender, NewDataDataRecordCallbackFunction* f) {
	ht->ipfixSender = ipfixSender;
	ht->dataDataRecordCallback = f;
	}

/**
 * Sets the function to call when a DataTemplate is invalidated
 * @param ht handle of Hashtable for which to set this callback function
 * @param ipfixSender handle to pass to callback function. Forced to be the same for all of a Hashtable's Callback functions
 * @param f function to call, @c handle get passed to this function
 */
void setNewDataTemplateDestructionCallback(Hashtable* ht, void* ipfixSender, NewDataTemplateDestructionCallbackFunction* f) {
	ht->ipfixSender = ipfixSender;
	ht->dataTemplateDestructionCallback = f;
	}

