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

Rules* rules;
IpfixSender* ipfixSender;
IpfixReceiver* ipfixReceiver;

/**
 * Injects new DataRecords into the Aggregator.
 * @param sourceID ignored
 */
int processDataRecord(SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("Got a Data Record");

	if (!rules) {
		fatal("Aggregator not started");
		return -1;
		}

	for (i = 0; i < rules->count; i++) {
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
int processDataDataRecord(SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("Got a DataData Record");

	if (!rules) {
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
void initializeConcentrator() {
	ipfixSender = 0;
	rules = 0;
	ipfixReceiver = 0;
	}

/**
 * Starts the Exporter.
 */
void startExporter(char* ip, uint16_t port) {
	if (ipfixSender) {
		fatal("Exporter already started");
		return;
		}

	debug("initializing exporter");
	initializeSndIpfix();

	debug("starting exporter");
	ipfixSender = sndIpfixUdpIpv4(ip, port);
	startSndIpfix(ipfixSender);
	}

/**
 * Starts the Aggregator. Exporter must be running.
 */
void startAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime) {
	int i;

	if (!ipfixSender) {
		fatal("Exporter not started");
		return;
		}
	if (rules) {
		fatal("Concentrator already started");
		return;
		}	

	debug("reading ruleset and creating hashtables");
	rules = parseRulesFromFile(ruleFile);
	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = createHashtable(rules->rule[i], minBufferTime, maxBufferTime);
		setNewDataTemplateCallback(rules->rule[i]->hashtable, ipfixSender, sndNewDataTemplate);
		setNewDataDataRecordCallback(rules->rule[i]->hashtable, ipfixSender, sndDataDataRecord);
		setNewDataTemplateDestructionCallback(rules->rule[i]->hashtable, ipfixSender, sndDestroyDataTemplate);
		}
	}

/**
 * Starts the Collector. Aggregator must be running.
 */
void startCollector(uint16_t port) {
	if (!ipfixSender) {
		fatal("Exporter not started");
		return;
		}
	if (!rules) {
		fatal("Aggregator not started");
		return;
		}
	if (ipfixReceiver) {
		fatal("Collector already started");
		return;
		}

	debug("initializing collector");
	initializeRcvIpfix();
			
	debug("starting collector");
	ipfixReceiver = rcvIpfixUdpIpv4(port);
	setDataRecordCallback(ipfixReceiver, processDataRecord);
	setDataDataRecordCallback(ipfixReceiver, processDataDataRecord);
	startRcvIpfix(ipfixReceiver);
	}

/**
 * Exports aggregated flows; needs to be periodically called.
 */
void pollAggregator() {
	int i;

	stopRcvIpfix(ipfixReceiver);
	for (i = 0; i < rules->count; i++) {
		expireFlows(rules->rule[i]->hashtable);
		}
	startRcvIpfix(ipfixReceiver);
	}

/**
 * Frees all memory. Call this when the application terminates
 */
void destroyConcentrator() {
	int i;

	if (ipfixReceiver) {
		debug("stopping collector");
		stopRcvIpfix(ipfixReceiver);
		rcvIpfixClose(ipfixReceiver);
		deinitializeRcvIpfix();
		ipfixReceiver = 0;
		}
	
	if (rules) {
		debug("stopping aggregator");
		for (i = 0; i < rules->count; i++) {
			destroyHashtable(rules->rule[i]->hashtable);
			}
		destroyRules(rules);
		rules = 0;
		}

	if (ipfixSender) {
		debug("stopping exporter");
		stopSndIpfix(ipfixSender);
		sndIpfixClose(ipfixSender);
		deinitializeSndIpfix();	
		ipfixSender = 0;
		}
	}
