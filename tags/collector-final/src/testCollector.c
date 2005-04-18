/** \file
 * Separate Program to test "ipfixCollector"
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "rcvIpfix.h"
#include "common.h"

#define COL_LISTEN_PORT 1500

void sigint() {
	}

void printField(FieldInfo fi, FieldData* data) {
	printFieldData(fi.type, data+fi.offset);
	printf("\n");
	}

int templateCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	debug("--- Got a Template");
	return 0;
	}

int templateDestructionCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	debug("--- Template destroyed");
	return 0;
	}

int dataRecordCallbackTest(SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("--- Got a Data Record");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return 0;
	}

int optionsRecordCallbackTest(SourceID sourceID, OptionsTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("--- Got an Options Data Record");
	for (i = 0; i < ti->scopeCount; i++) printField(ti->scopeInfo[i], data);
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return 0;
	}

int dataDataRecordCallbackTest(SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("--- Got a Data Record with fixed fields");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);
	for (i = 0; i < ti->dataCount;  i++) printField(ti->dataInfo[i],  ti->data);

	return 0;
	}

int main(int argc, char *argv[]) {
	signal(SIGINT, sigint);

	initializeRcvIpfix();

	IpfixReceiver* ipfixReceiver = rcvIpfixUdpIpv4(COL_LISTEN_PORT);
 	setTemplateCallback(ipfixReceiver, templateCallbackTest);
	setTemplateDestructionCallback(ipfixReceiver, templateDestructionCallbackTest);
 	setDataRecordCallback(ipfixReceiver, dataRecordCallbackTest);
	setOptionsRecordCallback(ipfixReceiver, optionsRecordCallbackTest);
	setDataDataRecordCallback(ipfixReceiver, dataDataRecordCallbackTest);
	
	startRcvIpfix(ipfixReceiver);

	printf("Listening on Port %d. Hit Ctrl+C to quit", COL_LISTEN_PORT);
	pause();
	printf("Stopping threads and tidying up.");
	
	stopRcvIpfix(ipfixReceiver);
	rcvIpfixClose(ipfixReceiver);
 	deinitializeRcvIpfix();	

	return 0;
	}

