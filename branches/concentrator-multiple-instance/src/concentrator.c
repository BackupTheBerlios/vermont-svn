/** \file
 * Concentrator meta-module.
 * Simple interface to the Collector, Aggregator and Exporter modules.
 * Use either this module or the individual modules - do not use both.
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "rcvIpfix.h"
#include "common.h"
#include "rules.h"
#include "hashing.h"
#include "config.h"
#include "sndIpfix.h"

#include "concentrator.h"

//Rules* rules;
//IpfixSender* ipfixSender;
//IpfixReceiver* ipfixReceiver;

/**
 * Injects new DataRecords into the Aggregator.
 * @param sourceID ignored
 */
int processDataRecord(Rules *rules, SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("Got a Data Record");

	if(!rules) {
		fatal("Aggregator not started");
		return -1;
	}

	for(i = 0; i < rules->count; i++) {
		if (templateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			
			aggregateTemplateData(rules->rule[i]->hashtable, ti, data);
		}
	}
	
	return 0;
}

/**
 * Injects new DataRecords with fixed fields into the Aggregator.
 * @param sourceID ignored
 */
int processDataDataRecord(Rules *rules, SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;

	debug("Got a DataData Record");

	if(!rules) {
		fatal("Aggregator not started");
		return -1;
		}

	for (i = 0; i < rules->count; i++) {
		if (dataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			
			aggregateDataTemplateData(rules->rule[i]->hashtable, ti, data);
			}
		}
	
	return 0;
	}

/**
 * Initializes Memory. Call this on application startup.
 */
struct concentrator * initializeConcentrator() {
	return((struct concentrator *)calloc(1, sizeof(struct concentrator)));
}

/**
 * Starts the Exporter.
 */
int startExporter(struct concentrator *c, char* ip, uint16_t port) {
	if(c->ipfixSender) {
		fatal("Exporter already started");
		return -1;
	}

	debug("initializing exporter");
	initializeSndIpfix();

	debug("starting exporter");
	if(!(c->ipfixSender=sndIpfixUdpIpv4(ip, port))) {
		fatal("Cannot initialize sndIpfixUdpIpv4");
                return -1;
	};
	return startSndIpfix(c->ipfixSender);
}

/**
 * Starts the Aggregator. Exporter must be running.
 */
int startAggregator(Rules *rules, char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime) {
	int i;

	debug("reading ruleset and creating hashtables");
	if(!(rules = parseRulesFromFile(ruleFile))) {
		fatal("Cannot parse rules");
                return 1;
	}

	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = createHashtable(rules->rule[i], minBufferTime, maxBufferTime);
		setNewDataTemplateCallback(rules->rule[i]->hashtable, sndNewDataTemplate);
		setNewDataDataRecordCallback(rules->rule[i]->hashtable, sndDataDataRecord);
		setNewDataTemplateDestructionCallback(rules->rule[i]->hashtable, sndDestroyDataTemplate);
	}

        c->rules=rules;
	return 0;
}

/**
 * Starts the Collector. Aggregator must be running.
 */
int startCollector(IpfixReceiver *ipfixReceiver, uint16_t port) {

	debug("initializing collector");
	initializeRcvIpfix();
			
	debug("starting collector");
	if(!(ipfixReceiver = rcvIpfixUdpIpv4(port))) {
		fatal("Cannot start rcvIpfixUdpIpv4");
                return 1;
	}
	setDataRecordCallback(ipfixReceiver, processDataRecord);
	setDataDataRecordCallback(ipfixReceiver, processDataDataRecord);

	return(startRcvIpfix(ipfixReceiver));
}

/**
 * Exports aggregated flows; needs to be periodically called.
 */
void pollAggregator(Rules *rules) {
	int i;

	stopRcvIpfix(c->ipfixReceiver);
	for (i = 0; i < rules->count; i++) {
		expireFlows(rules->rule[i]->hashtable);
		}
	startRcvIpfix(c->ipfixReceiver);
	}

/**
 * Frees all memory. Call this when the application terminates
 */
void destroyConcentrator(struct concentrator *c) {
	int i;

	IpfixReceiver *ipfixReceiver=c->ipfixReceiver;
	Rules *rules=c->rules;
        IpfixSender *ipfixSender=c->ipfixSender;

	if(ipfixReceiver) {
		debug("stopping collector");
		stopRcvIpfix(ipfixReceiver);
		rcvIpfixClose(ipfixReceiver);
		deinitializeRcvIpfix();
		ipfixReceiver = 0;
	}
	
	if(rules) {
		debug("stopping aggregator");
		for (i = 0; i < rules->count; i++) {
			destroyHashtable(rules->rule[i]->hashtable);
			}
		destroyRules(rules);
		rules = 0;
	}

	if(ipfixSender) {
		debug("stopping exporter");
		stopSndIpfix(ipfixSender);
		sndIpfixClose(ipfixSender);
		deinitializeSndIpfix();	
		ipfixSender = 0;
	}
}
