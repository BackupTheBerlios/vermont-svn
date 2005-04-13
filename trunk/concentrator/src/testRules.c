/** \file
 * Separate Program to test "ipfixCollector"
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "rcvIpfix.h"
#include "common.h"
#include "rules.h"

Rules* rules;

void sigint() {
	}

int templateCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	/*
	debug("--- Got a Template");
	*/
	return 0;
	}

int dataRecordCallbackTest(SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	printf("\nData Record\n");
	for (i = 0; i < ti->fieldCount; i++) {
		printf("\t");
		printFieldData(ti->fieldInfo[i].type, data + ti->fieldInfo[i].offset);
		printf("\n");
		}

	for (i = 0; i < rules->count; i++) {
		if (templateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			} else {
			debugf("rule %d doesn't match", i);
			}
		}
	
	return 0;
	}

int dataDataRecordCallbackTest(SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	printf("\nDataData Record\n");
	for (i = 0; i < ti->fieldCount; i++) {
		printf("\t");
		printFieldData(ti->fieldInfo[i].type, data + ti->fieldInfo[i].offset);
		printf("\n");
		}
	for (i = 0; i < ti->dataCount; i++) {
		printf("\t(common ");
		printFieldData(ti->dataInfo[i].type, ti->data + ti->dataInfo[i].offset);
		printf(")\n");
		}

	for (i = 0; i < rules->count; i++) {
		if (dataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			debugf("rule %d matches", i);
			} else {
			debugf("rule %d doesn't match", i);
			}
		}
	
	return 0;
	}

int main(int argc, char *argv[]) {
	int i;
	
	signal(SIGINT, sigint);

	rules = parseRulesFromFile("aggregation_rules.conf");
	for (i = 0; i < rules->count; i++) {
		printRule(rules->rule[i]);
		}
	
	initializeRcvIpfix();

	IpfixReceiver* ipfixReceiver = rcvIpfixUdpIpv4(1500);
	setTemplateCallback(ipfixReceiver, templateCallbackTest);

	setDataRecordCallback(ipfixReceiver, dataRecordCallbackTest);
	setDataDataRecordCallback(ipfixReceiver, dataDataRecordCallbackTest);
	
	startRcvIpfix(ipfixReceiver);

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	pause();
	debug("Stopping threads and tidying up.");

	
	stopRcvIpfix(ipfixReceiver);
	rcvIpfixClose(ipfixReceiver);
	deinitializeRcvIpfix();	

	return 0;
	}

