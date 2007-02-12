#ifndef EXP_HASHING_H
#define EXP_HASHING_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "exp_rcvIpfix.h"
#include "exp_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constants ************************************************************/

#define HASHTABLE_SIZE	65536

/***** Data Types ***********************************************************/

/**
 * Single Bucket containing one buffered flow's variable data.
 * Is either a direct entry in @c Hashtable::bucket or a member of another HashBucket's spillchain
 */
typedef struct {
	uint32_t expireTime;      /**< timestamp when this bucket will expire if no new flows are added */
	uint32_t forceExpireTime; /**< timestamp when this bucket is forced to expire */
	FieldData* data;        /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo */
	void* next;             /**< next bucket in spill chain */
} ExpressHashBucket;

/**
 * Hash-powered buffer for outgoing flows.
 * This is where outbound flows are aggregated while waiting to be exported.
 */	
typedef struct {
	int bucketCount;                     /**< size of this hashtable (must be HASHTABLE_SIZE) */
	ExpressHashBucket* bucket[HASHTABLE_SIZE];  /**< array of pointers to hash buckets at start of spill chain. Members are NULL where no entry present */
	
	ExpressDataTemplateInfo* dataTemplate;      /**< structure describing both variable and fixed fields and containing fixed data */
	uint16_t fieldLength;                /**< length in bytes of all variable-length fields */
	ExpressFieldModifier* fieldModifier;        /**< specifies what modifier to apply to a given field */

	uint16_t minBufferTime;              /**< If for a buffered flow no new aggregatable flows arrive for this many seconds, export it */
	uint16_t maxBufferTime;              /**< If a buffered flow was kept buffered for this many seconds, export it */

	int callbackCount;                   /**< Length of callbackInfo array */
	ExpressCallbackInfo* callbackInfo;          /**< Array of callback functions to invoke when Templates or Records should be sent */

	int recordsReceived;                 /**< Statistics: Number of records received from higher-level modules */
	int recordsSent;                     /**< Statistics: Number of records sent to lower-level modules */
} ExpressHashtable;
	
/***** Prototypes ***********************************************************/

ExpressHashtable* ExpresscreateHashtable(ExpressRule* rule, uint16_t minBufferTime, uint16_t maxBufferTime);
void ExpressdestroyHashtable(ExpressHashtable* ht);

void ExpresshashingAddCallbacks(ExpressHashtable* ht, ExpressCallbackInfo handles);

void ExpressaggregateTemplateData(ExpressHashtable* ht, FieldData* data, int transport_offset);
void ExpressaggregateDataTemplateData(ExpressHashtable* ht, ExpressDataTemplateInfo* ti, FieldData* data);

void ExpressdestroyHashtable(ExpressHashtable* ht);

void ExpressexpireFlows(ExpressHashtable* ht);


#ifdef __cplusplus
}
#endif


#endif
