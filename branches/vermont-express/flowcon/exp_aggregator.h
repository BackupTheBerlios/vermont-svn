#ifndef EXP_AGGREGATOR_H
#define EXP_AGGREGATOR_H

#include "exp_rcvIpfix.h"
#include "exp_rules.h"
#include "exp_hashing.h"
#include <pthread.h>
#include "sampler/packet_hook.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constants ************************************************************/

/***** Typedefs *************************************************************/

/**
 * Represents an Aggregator.
 * Create with @c createAggregator()
 */
typedef struct {
	ExpressRules* rules;          /**< Set of rules that define the aggregator */
	pthread_mutex_t mutex; /**< Mutex to synchronize and/or pause aggregator */
} IpfixExpressAggregator;

/***** Prototypes ***********************************************************/

int initializeExpressAggregators();
int deinitializeExpressAggregators();

IpfixExpressAggregator* createExpressAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime);
IpfixExpressAggregator* createExpressAggregatorFromRules(ExpressRules* rules, uint16_t minBufferTime, uint16_t maxBufferTime);
void destroyExpressAggregator(IpfixExpressAggregator* ipfixAggregator);

void startExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator);
void stopExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator);

int ExpressaggregateDataRecord(void* ipfixExpressAggregator, Exp_SourceID* sourceID, uint16_t length, FieldData* data, struct packet_hook *pdata);
int ExpressaggregateDataDataRecord(void* ipfixAggregator, Exp_SourceID* sourceID, ExpressDataTemplateInfo* ti, uint16_t length, FieldData* data);

void pollExpressAggregator(IpfixExpressAggregator* ipfixExpressAggregator);

void addExpressAggregatorCallbacks(IpfixExpressAggregator* ipfixExpressAggregator, ExpressCallbackInfo handles);

ExpressCallbackInfo getExpressAggregatorCallbackInfo(IpfixExpressAggregator* ipfixExpressAggregator);

void statsExpressAggregator(void* ipfixExpressAggregator);


#ifdef __cplusplus
}
#endif

#endif
