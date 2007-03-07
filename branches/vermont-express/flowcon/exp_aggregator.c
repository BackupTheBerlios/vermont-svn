/** \file
 * Aggregator module.
 * Uses rules.c and hashing.c to implement an IPFIX Aggregator.
 * Call createAggregator() to instantiate an aggregator,
 * start it with startAggregator(), feed it with aggregateDataRecord(),
 * poll for expired flows with pollAggregator().
 * To destroy the Aggregator, first stop it with stopAggregator(), then
 * call destroyAggregator().
 */

#include <netinet/in.h>
#include <unistd.h>
#include "exp_aggregator.h"
#include "exp_rcvIpfix.h"
#include "exp_sndIpfix.h"
#include "sampler/packet_hook.h"

#include "msg.h"


static IpfixExpressAggregator* buildExpressAggregator(ExpressRules* rules, uint16_t minBufferTime, uint16_t maxBufferTime);


int initializeExpressAggregators()
{
	return 0;
}

int deinitializeExpressAggregators()
{
	return 0;
}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggregator().
 * @param ruleFile filename of file containing a set of rules
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 * @return handle to the Aggregator on success or NULL on error
 */

IpfixExpressAggregator* createExpressAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	ExpressRules* rules = ExpressparseRulesFromFile(ruleFile);
	if (!rules) {
		msg(MSG_FATAL, "ExpressAggregator: could not parse rules file %s", ruleFile);
		return NULL;
	}
	return buildExpressAggregator(rules, minBufferTime, maxBufferTime);
}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggreagtor().
 * @param rules Rules for aggregator to work with
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 * @return handle to the Aggreagtor on success or NULL on error
 */
IpfixExpressAggregator* createExpressAggregatorFromRules(ExpressRules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	return buildExpressAggregator(rules, minBufferTime, maxBufferTime);
}

/**
 * Builds a new aggregator from the given rules (helper function for @c createAggregator and @c createAggregatorFromRules)
 */
static IpfixExpressAggregator* buildExpressAggregator(ExpressRules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	int i;

	IpfixExpressAggregator* ipfixExpressAggregator = (IpfixExpressAggregator*)malloc(sizeof(IpfixExpressAggregator));
	ipfixExpressAggregator->rules = rules;

	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = ExpresscreateHashtable(rules->rule[i],
							    minBufferTime,
							    maxBufferTime
							   );
	}

	if (pthread_mutex_init(&ipfixExpressAggregator->mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		}
		
	if (pthread_mutex_lock(&ipfixExpressAggregator->mutex) != 0) {
		msg(MSG_FATAL, "Could not lock mutex");
		}

	msg(MSG_INFO, "ExpressAggregator: Done. Parsed %d rules; minBufferTime %d, maxBufferTime %d",
	    rules->count, minBufferTime, maxBufferTime
	   );

        return ipfixExpressAggregator;
}

/**
 * Frees memory used by an Aggregator.
 * Make sure the Aggregator is not being used before calling this method.
 * @param ipfixExpressAggregator handle of Aggregator
 */
void destroyExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator)
{
	ExpressRules* rules = ((IpfixExpressAggregator*)ipfixExpressAggregator)->rules;

	int i;
	for (i = 0; i < rules->count; i++) {
		ExpressdestroyHashtable(rules->rule[i]->hashtable);

	pthread_mutex_unlock(&((IpfixExpressAggregator*)ipfixExpressAggregator)->mutex);
	pthread_mutex_destroy(&((IpfixExpressAggregator*)ipfixExpressAggregator)->mutex);

	free(ipfixExpressAggregator);
	}

	ExpressdestroyRules(rules);
}

/**
 * Starts or resumes processing Records
 * @param ipfixExpressAggregator handle of Aggregator
 */
void startExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator)
{
	pthread_mutex_unlock(&ipfixExpressAggregator->mutex);
}

/**
 * Temporarily pauses processing Records
 * @param ipfixExpressAggregator handle of Aggregator
 */
void stopExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator)
{
	pthread_mutex_lock(&ipfixExpressAggregator->mutex);
}

/**
 * Injects new DataRecords into the Aggregator.
 * @param ipfixExpressAggregator handle of aggregator this Record is for
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int ExpressaggregateDataRecord(void* ipfixExpressAggregator, Exp_SourceID* sourceID, uint16_t length, FieldData* data, struct packet_hook *pdata)
{
	ExpressRules* rules = ((IpfixExpressAggregator*)ipfixExpressAggregator)->rules;

	int i;
	DPRINTF("ExpressaggregateDataRecord: Got a Data Record\n");

	if(!rules) {
		msg(MSG_FATAL, "ExpressAggregator not started");
		return -1;
	}

	pthread_mutex_lock(&((IpfixExpressAggregator*)ipfixExpressAggregator)->mutex);
	for (i = 0; i < rules->count; i++) {
		if (ExpresstemplateDataMatchesRule(data, rules->rule[i], pdata)) {
			DPRINTF("rule %d matches\n", i);
			ExpressaggregateTemplateData(rules->rule[i]->hashtable, data, pdata);
		}
	}
	pthread_mutex_unlock(&((IpfixExpressAggregator*)ipfixExpressAggregator)->mutex);
	
	return 0;
}

/**
 * Injects new DataRecords into the Aggregator.
 * @param ipfixExpressAggregator handle of aggregator this Record is for
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int ExpressaggregateDataDataRecord(void* ipfixExpressAggregator, Exp_SourceID* sourceID, ExpressDataTemplateInfo* ti, uint16_t length, FieldData* data)
{
	ExpressRules* rules = ((IpfixExpressAggregator*)ipfixExpressAggregator)->rules;

	int i;
	DPRINTF("ExpressaggregateDataDataRecord: Got a DataData Record\n");

	if(!rules) {
		msg(MSG_FATAL, "ExpressAggregator not started");
		return -1;
	}

	pthread_mutex_lock(&((IpfixExpressAggregator*)ipfixExpressAggregator)->mutex);
	for (i = 0; i < rules->count; i++) {
		if (ExpressdataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			DPRINTF("rule %d matches\n", i);
			ExpressaggregateDataTemplateData(rules->rule[i]->hashtable, ti, data);
		}
	}
	pthread_mutex_unlock(&((IpfixExpressAggregator*)ipfixExpressAggregator)->mutex);
	
	return 0;
}

/**
 * Checks for flows buffered longer than @c ipfixExpressAggregator::minBufferTime and/or @c ipfixExpressAggregator::maxBufferTime and passes them to the previously defined callback functions.
 * @param ipfixExpressAggregator handle of Aggregator to poll
 */
void pollExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator)
{
        int i;
	ExpressRules* rules = ((IpfixExpressAggregator*)ipfixExpressAggregator)->rules;

	pthread_mutex_lock(&ipfixExpressAggregator->mutex);
	for (i = 0; i < rules->count; i++) {
		ExpressexpireFlows(rules->rule[i]->hashtable);
	}
	pthread_mutex_unlock(&ipfixExpressAggregator->mutex);
}

/**
 * Adds a set of callback functions to the list of functions to call when Templates or Records have to be sent
 * @param ipfixExpressAggregator IpfixExpressAggregator to set the callback function for
 * @param handles set of callback functions
 */
void addExpressAggregatorCallbacks(IpfixExpressAggregator* ipfixExpressAggregator, ExpressCallbackInfo handles)
{
	int i;
	ExpressRules* rules = ((IpfixExpressAggregator*)ipfixExpressAggregator)->rules;

	for (i = 0; i < rules->count; i++) {
		ExpresshashingAddCallbacks(rules->rule[i]->hashtable, handles);
	}
}
/*
ExpressCallbackInfo getExpressAggregatorCallbackInfo(IpfixExpressAggregator* ipfixExpressAggregator)
{
	ExpressCallbackInfo ci;

	bzero(&ci, sizeof(ExpressCallbackInfo));
	ci.handle = ipfixExpressAggregator;
	ci.dataRecordCallbackFunction = ExpressaggregateDataRecord;
	ci.dataDataRecordCallbackFunction = ExpressaggregateDataDataRecord;

	return ci;
}
*/
/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void statsExpressAggregator(void* ipfixExpressAggregator_)
{
	int i;
	IpfixExpressAggregator* ipfixExpressAggregator = (IpfixExpressAggregator*)ipfixExpressAggregator_;
	ExpressRules* rules = ipfixExpressAggregator->rules;

	pthread_mutex_lock(&ipfixExpressAggregator->mutex);
	for (i = 0; i < rules->count; i++) {
		int j;
		uint32_t usedBuckets = 0;
		uint32_t usedHeads = 0;
		uint32_t longestSpillchain = 0;
		uint32_t avgAge = 0;

		ExpressHashtable* ht = rules->rule[i]->hashtable;
		msg_stat("FlowConcentrator: Rule %2d: Records: %6d received, %6d sent", i, ht->recordsReceived, ht->recordsSent);
		ht->recordsReceived = 0;
		ht->recordsSent = 0;

		for (j = 0; j < HASHTABLE_SIZE; j++) {
			ExpressHashBucket* hb = ht->bucket[j];
			if (hb) usedHeads++;

			uint32_t bucketsInSpillchain = 0;
			while (hb) {
				avgAge += time(0) - (hb->forceExpireTime - ht->maxBufferTime);
				usedBuckets++;
				bucketsInSpillchain++;
				hb = hb->next;
			}
			if (bucketsInSpillchain > longestSpillchain) longestSpillchain = bucketsInSpillchain;
		}

		msg_stat("FlowConcentrator: Rule %2d: Hashbuckets: %6d used, %6d at head, %6d max chain, %6d avg age", i, usedBuckets, usedHeads, longestSpillchain, usedBuckets?(avgAge / usedBuckets):0);
	}
	pthread_mutex_unlock(&ipfixExpressAggregator->mutex);
}
