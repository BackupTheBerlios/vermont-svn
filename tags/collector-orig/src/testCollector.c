/** \file
 * Separate Program to test "ipfixCollector"
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "rcvIpfix.h"
#include "common.h"

void sigint() {
	}

void printField(FieldInfo fi, FieldData* data) {
	printFieldData(fi.typeX, data+fi.offset);
	printf("\n");
	}

boolean templateCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	debug("--- Got a Template");
	return true;
	}

boolean templateDestructionCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	debug("--- Template destroyed");
	return true;
	}

boolean dataRecordCallbackTest(SourceID sourceID, TemplateInfo* ti, uint16 length, FieldData* data) {
	int i;
	debug("--- Got a Data Record");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return true;
	}

boolean optionsRecordCallbackTest(SourceID sourceID, OptionsTemplateInfo* ti, uint16 length, FieldData* data) {
	int i;
	debug("--- Got an Options Data Record");
	for (i = 0; i < ti->scopeCount; i++) printField(ti->scopeInfo[i], data);
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return true;
	}

boolean dataDataRecordCallbackTest(SourceID sourceID, DataTemplateInfo* ti, uint16 length, FieldData* data) {
	int i;
	debug("--- Got a Data Record with fixed fields");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);
	for (i = 0; i < ti->dataCount;  i++) printField(ti->dataInfo[i],  ti->data);

	return true;
	}

int main(int argc, char *argv[]) {
	signal(SIGINT, sigint);

	initializeRcvIpfix();
		
 	setTemplateCallback(templateCallbackTest);
	setTemplateDestructionCallback(templateDestructionCallbackTest);

 	setDataRecordCallback(dataRecordCallbackTest);
	setOptionsRecordCallback(optionsRecordCallbackTest);
	setDataDataRecordCallback(dataDataRecordCallbackTest);

	int handle = rcvIpfixUdpIpv4(1500);
	startRcvIpfix();

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	pause();
	debug("Stopping threads and tidying up.");

	
	stopRcvIpfix();
	rcvIpfixClose(handle);
 	deinitializeRcvIpfix();	

	return 0;
	}

