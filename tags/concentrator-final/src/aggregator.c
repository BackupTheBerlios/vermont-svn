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
#include "aggregator.h"
#include "rcvIpfix.h"
#include "common.h"
#include "sndIpfix.h"

int initializeAggregators() {
	return 0;
	}

int deinitializeAggregators() {
	return 0;
	}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggregator().
 * @param ruleFile filename of file containing a set of rules
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 * @return handle to the Aggregator on success or NULL on error
 */
IpfixAggregator* createAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime) {
	int i;

	IpfixAggregator* ipfixAggregator = (IpfixAggregator*)malloc(sizeof(IpfixAggregator));
	ipfixAggregator->rules = parseRulesFromFile(ruleFile);

	if (!ipfixAggregator->rules) {
		fatal("Could not parse ruleFile");
		return NULL;
		}

	Rules* rules = ipfixAggregator->rules;
	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = createHashtable(rules->rule[i], minBufferTime, maxBufferTime);
		}

	if (pthread_mutex_init(&ipfixAggregator->mutex, NULL) != 0) {
		fatal("Could not init mutex");
		}
		
	if (pthread_mutex_lock(&ipfixAggregator->mutex) != 0) {
		fatal("Could not lock mutex");
		}

	return ipfixAggregator;
	}

/**
 * Frees memory used by an Aggregator.
 * Make sure the Aggregator is not being used before calling this method.
 * @param ipfixAggregator handle of Aggregator
 */
void destroyAggregator(IpfixAggregator* ipfixAggregator) {
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	for (i = 0; i < rules->count; i++) {
		destroyHashtable(rules->rule[i]->hashtable);
		}
	destroyRules(rules);

	pthread_mutex_unlock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	pthread_mutex_destroy(&((IpfixAggregator*)ipfixAggregator)->mutex);

	free(ipfixAggregator);
	}

/**
 * Starts or resumes processing Records
 * @param ipfixAggregator handle of Aggregator
 */
void startAggregator(IpfixAggregator* ipfixAggregator) {
	pthread_mutex_unlock(&ipfixAggregator->mutex);
	}

/**
 * Temporarily pauses processing Records
 * @param ipfixAggregator handle of Aggregator
 */
void stopAggregator(IpfixAggregator* ipfixAggregator) {
	pthread_mutex_lock(&ipfixAggregator->mutex);
	}

/**
 * Injects new DataRecords into the Aggregator.
 * @param ipfixAggregator handle of aggregator this Record is for
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int aggregateDataRecord(void* ipfixAggregator, SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	debug("Got a Data Record");

	if (!rules) {
		fatal("Aggregator not started");
		return -1;
		}

	pthread_mutex_lock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	for (i = 0; i < rules->count; i++) {
		if (templateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			
			aggregateTemplateData(rules->rule[i]->hashtable, ti, data);
			}
		}
	pthread_mutex_unlock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	
	return 0;
	}

/**
 * Injects new DataRecords into the Aggregator.
 * @param ipfixAggregator handle of aggregator this Record is for
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int aggregateDataDataRecord(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	debug("Got a DataData Record");

	if (!rules) {
		fatal("Aggregator not started");
		return -1;
		}

	pthread_mutex_lock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	for (i = 0; i < rules->count; i++) {
		if (dataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			
			aggregateDataTemplateData(rules->rule[i]->hashtable, ti, data);
			}
		}
	pthread_mutex_unlock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	
	return 0;
	}

/**
 * Checks for flows buffered longer than @c ipfixAggregator::minBufferTime and/or @c ipfixAggregator::maxBufferTime and passes them to the previously defined callback functions.
 * @param ipfixAggregator handle of Aggregator to poll
 */
void pollAggregator(IpfixAggregator* ipfixAggregator) {
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	pthread_mutex_lock(&ipfixAggregator->mutex);
	for (i = 0; i < rules->count; i++) {
		expireFlows(rules->rule[i]->hashtable);
		}
	pthread_mutex_unlock(&ipfixAggregator->mutex);
	}

/**
 * Adds a set of callback functions to the list of functions to call when Templates or Records have to be sent
 * @param ipfixAggregator IpfixAggregator to set the callback function for
 * @param handles set of callback functions
 */
void addAggregatorCallbacks(IpfixAggregator* ipfixAggregator, CallbackInfo handles) {
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	for (i = 0; i < rules->count; i++) {
		hashingAddCallbacks(rules->rule[i]->hashtable, handles);
		}
	}

CallbackInfo getAggregatorCallbackInfo(IpfixAggregator* ipfixAggregator) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixAggregator;
	ci.dataRecordCallbackFunction = aggregateDataRecord;
	ci.dataDataRecordCallbackFunction = aggregateDataDataRecord;
	return ci;
	}
