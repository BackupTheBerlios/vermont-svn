#ifndef HASHING_H
#define HASHING_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "rcvIpfix.h"
#include "rules.h"

/***** Constants ************************************************************/

#define HASHTABLE_SIZE	65536

/***** Data Types ***********************************************************/

/**
 * Callback function invoked when a new DataTemplate should be exported.
 * @param handle Handle passed to the Callback function each time it is invoked. Can be used by the Callback function to distinguish between multiple instances and/or operation modes
 * @param dataTemplateInfo Pointer to a structure defining this Template
 * @return 0 if packet handled successfully
 */	
typedef int(NewDataTemplateCallbackFunction)(void* handle, DataTemplateInfo* dataTemplateInfo);

/**
 * Callback function invoked when a new DataDataRecord should be exported.
 * @param handle Handle passed to the Callback function each time it is invoked. Can be used by the Callback function to distinguish between multiple instances and/or operation modes
 * @param dataTemplateInfo Pointer to a structure defining this Template
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return 0 if packet handled successfully
 */	
typedef int(NewDataDataRecordCallbackFunction)(void* handle, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

/**
 * Callback function invoked when a DataTemplate should be destroyed.
 * @param handle Handle passed to the Callback function each time it is invoked. Can be used by the Callback function to distinguish between multiple instances and/or operation modes
 * @param dataTemplateInfo Pointer to a structure defining this Template
 * @return 0 if packet handled successfully
 */	
typedef int(NewDataTemplateDestructionCallbackFunction)(void* handle, DataTemplateInfo* dataTemplateInfo);

/**
 * Single Bucket containing one buffered flow's variable data.
 * Is either a direct entry in @c Hashtable::bucket or a member of another HashBucket's spillchain
 */
typedef struct {
	uint32_t expireTime;      /**< timestamp when this bucket will expire if no new flows are added */
	uint32_t forceExpireTime; /**< timestamp when this bucket is forced to expire */
	FieldData* data;        /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo */
	void* next;             /**< next bucket in spill chain */
	} HashBucket;

/**
 * Hash-powered buffer for outgoing flows.
 * This is where outbound flows are aggregated while waiting to be exported.
 */	
typedef struct {
	int bucketCount;                     /**< size of this hashtable (must be HASHTABLE_SIZE) */
	HashBucket* bucket[HASHTABLE_SIZE];  /**< array of pointers to hash buckets at start of spill chain. Members are NULL where no entry present */
	
	DataTemplateInfo* dataTemplate;      /**< structure describing both variable and fixed fields and containing fixed data */
	uint16_t fieldLength;                /**< length in bytes of all variable-length fields */
	FieldModifier* fieldModifier;        /**< specifies what modifier to apply to a given field */

	uint16_t minBufferTime;              /**< If for a buffered flow no new aggregatable flows arrive for this many seconds, export it */
	uint16_t maxBufferTime;              /**< If a buffered flow was kept buffered for this many seconds, export it */

	void* ipfixSender; /**< Handle to pass to callback function */
	NewDataTemplateCallbackFunction* dataTemplateCallback;     /**< Callback function invoked when a new DataTemplate should be exported */
	NewDataDataRecordCallbackFunction* dataDataRecordCallback; /**< Callback function invoked when a new DataDataRecord should be exported */
	NewDataTemplateDestructionCallbackFunction* dataTemplateDestructionCallback;     /**< Callback function invoked when a new DataTemplate should be destroyed */
	} Hashtable;
	
/***** Prototypes ***********************************************************/

Hashtable* createHashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime);

void setNewDataTemplateCallback(Hashtable* ht, void* ipfixSender, NewDataTemplateCallbackFunction* f);
void setNewDataDataRecordCallback(Hashtable* ht, void* ipfixSender, NewDataDataRecordCallbackFunction* f);
void setNewDataTemplateDestructionCallback(Hashtable* ht, void* ipfixSender, NewDataTemplateDestructionCallbackFunction* f);

void aggregateTemplateData(Hashtable* ht, TemplateInfo* ti, FieldData* data);
void aggregateDataTemplateData(Hashtable* ht, DataTemplateInfo* ti, FieldData* data);

void destroyHashtable(Hashtable* ht);

void expireFlows(Hashtable* ht);

#endif
