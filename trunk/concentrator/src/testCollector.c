/** \file
 * Separate Program to test the collector
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "rcvIpfix.h"
#include "common.h"

void sigint() {
	}

void printField(FieldInfo fi, FieldData* data) {
	printFieldData(fi.type, data+fi.offset);
	printf("\n");
	}

int templateCallbackTest(void* ipfixAggregator, SourceID sourceID, TemplateInfo* ti) {
	debug("--- Got a Template");
	return 0;
	}

int templateDestructionCallbackTest(void* ipfixAggregator, SourceID sourceID, TemplateInfo* ti) {
	debug("--- Template destroyed");
	return 0;
	}

int dataTemplateCallbackTest(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* ti) {
	int i;
	
	debug("--- Got a DataTemplate");
	debug("fixed fields:");
	for (i = 0; i < ti->dataCount;  i++) printField(ti->dataInfo[i],  ti->data);
	return 0;
	}

int dataTemplateDestructionCallbackTest(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* ti) {
	debug("--- DataTemplate destroyed");
	return 0;
	}

int dataRecordCallbackTest(void* ipfixAggregator, SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("--- Got a Data Record");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return 0;
	}

int optionsRecordCallbackTest(void* ipfixAggregator, SourceID sourceID, OptionsTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("--- Got an Options Data Record");
	for (i = 0; i < ti->scopeCount; i++) printField(ti->scopeInfo[i], data);
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return 0;
	}

int dataDataRecordCallbackTest(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data) {
	int i;
	debug("--- Got a Data Record with fixed fields");
	debug("fixed fields:");
	for (i = 0; i < ti->dataCount;  i++) printField(ti->dataInfo[i],  ti->data);
	debug("regular fields:");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return 0;
	}

int main(int argc, char *argv[]) {
	signal(SIGINT, sigint);

	initializeIpfixReceivers();

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(1501);

	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.templateCallbackFunction = templateCallbackTest;
	ci.dataTemplateCallbackFunction = dataTemplateCallbackTest;
	ci.dataRecordCallbackFunction = dataRecordCallbackTest;
	ci.optionsRecordCallbackFunction = optionsRecordCallbackTest;
	ci.dataDataRecordCallbackFunction = dataDataRecordCallbackTest;
	ci.templateDestructionCallbackFunction = templateDestructionCallbackTest;
	ci.dataTemplateDestructionCallbackFunction = dataTemplateDestructionCallbackTest;
	addIpfixReceiverCallbacks(ipfixReceiver, ci);
	
	startIpfixReceiver(ipfixReceiver);

	debug("Listening on Port 1501. Hit Ctrl+C to quit");
	pause();
	debug("Stopping threads and tidying up.");
	
	stopIpfixReceiver(ipfixReceiver);
	destroyIpfixReceiver(ipfixReceiver);
 	deinitializeIpfixReceivers();	

	return 0;
	}

