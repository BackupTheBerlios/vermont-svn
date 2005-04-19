/** \file
 * Separate Program to test the concentrator
 * Dumps received flows to stdout
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
Config* config;
int mayRun;

void sigint() {
	mayRun = 0;
	}

int processDataRecord(SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("Got a Data Record");

	for (i = 0; i < rules->count; i++) {
		if (templateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			
			aggregateTemplateData(rules->rule[i]->hashtable, ti, data);
			}
		}
	
	return 0;
	}

int processDataDataRecord(SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("Got a DataData Record");

	for (i = 0; i < rules->count; i++) {
		if (dataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			
			aggregateDataTemplateData(rules->rule[i]->hashtable, ti, data);
			}
		}
	
	return 0;
	}

int main(int argc, char *argv[]) {
	int i;
	
	mayRun = 1;
	signal(SIGINT, sigint);

	debug("reading config");
	config = readConfigFromFile("concentrator.conf");

	debug("initializing collector");
	initializeRcvIpfix();
	
	debug("initializing exporter");
	initializeSndIpfix();

	debug("starting exporter");
	IpfixSender* ipfixSender = sndIpfixUdpIpv4("127.0.0.1", 1501);
	startSndIpfix(ipfixSender);
		
	debug("reading ruleset and creating hashtables");
	rules = parseRulesFromFile("aggregation_rules.conf");
	for (i = 0; i < rules->count; i++) {
		printRule(rules->rule[i]);
		rules->rule[i]->hashtable = createHashtable(rules->rule[i], config->minBufferTime, config->maxBufferTime);
		setNewDataTemplateCallback(rules->rule[i]->hashtable, sndNewDataTemplate);
		setNewDataDataRecordCallback(rules->rule[i]->hashtable, sndDataDataRecord);
		setNewDataTemplateDestructionCallback(rules->rule[i]->hashtable, sndDestroyDataTemplate);
		}

			
	debug("starting collector");
	IpfixReceiver* ipfixReceiver = rcvIpfixUdpIpv4(1500);
	setDataRecordCallback(ipfixReceiver, processDataRecord);
	setDataDataRecordCallback(ipfixReceiver, processDataDataRecord);
	startRcvIpfix(ipfixReceiver);

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	while (mayRun) {
		stopRcvIpfix(ipfixReceiver);
		for (i = 0; i < rules->count; i++) {
			expireFlows(rules->rule[i]->hashtable);
			}
		startRcvIpfix(ipfixReceiver);
		sleep(1);
		}
	debug("Stopping threads and tidying up.");

	debug("stopping collector");
	stopRcvIpfix(ipfixReceiver);
	rcvIpfixClose(ipfixReceiver);
	

	debug("destroying hashtables");
	for (i = 0; i < rules->count; i++) {
		destroyHashtable(rules->rule[i]->hashtable);
		}

	debug("stopping exporter");
	stopSndIpfix(ipfixSender);
	sndIpfixClose(ipfixSender);
	
	debug("destroying collector");
	deinitializeRcvIpfix();
	
	debug("destroying exporter");
	deinitializeSndIpfix();	
	
	debug("destroying rulesets");
	destroyRules(rules);
	
	debug("destroying config");
	destroyConfig(config);
	
	return 0;
	}

