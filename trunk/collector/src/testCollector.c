/** \file
 * Separate Program to test "ipfixCollector"
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "rcvIpfix.h"
#include "tools.h"

void sigint() {
	}

void printField(FieldInfo fi, byte* data) {
	data = data + fi.offset;
	if (fi.type == IPFIX_Type_protocolIdentifier) {
		if (*data == IPFIX_protocolIdentifier_TCP) debugs("Proto is TCP"); else
		if (*data == IPFIX_protocolIdentifier_UDP) debugs("Proto is UDP"); else
		debugs("Proto is unknown");
		}
	if (fi.type == IPFIX_Type_sourceAddressV4) {
		debug("From %d", ntohl(*(uint32*)data));
		}
	if (fi.type == IPFIX_Type_destinationAddressV4) {
		debug("To %d", ntohl(*(uint32*)data));
		}
	if (fi.type == IPFIX_Type_deltaOctetCount) {
		debug("%d Bytes", ntohl(*(uint32*)data));
		}
	if (fi.type == IPFIX_Type_deltaPacketCount) {
		debug("%d Packets", ntohl(*(uint32*)data));
		}
	if (fi.type == IPFIX_Type_transportSourcePort) {
		debug("Source Port is %d", ntoh(*(uint16*)data));
		}
	if (fi.type == IPFIX_Type_transportDestinationPort) {
		debug("Dest Port is %d", ntoh(*(uint16*)data));
		}
	}

boolean templateCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	debugs("--- Got a Template");
	return true;
	}

boolean templateDestructionCallbackTest(SourceID sourceID, TemplateInfo* ti) {
	debugs("--- Template destroyed");
	return true;
	}

boolean dataRecordCallbackTest(SourceID sourceID, TemplateInfo* ti, uint16 length, byte* data) {
	int i;
	debugs("--- Got a Data Record");
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return true;
	}

boolean optionsRecordCallbackTest(SourceID sourceID, OptionsTemplateInfo* ti, uint16 length, byte* data) {
	int i;
	debugs("--- Got an Options Data Record");
	for (i = 0; i < ti->scopeCount; i++) printField(ti->scopeInfo[i], data);
	for (i = 0; i < ti->fieldCount; i++) printField(ti->fieldInfo[i], data);

	return true;
	}

boolean dataDataRecordCallbackTest(SourceID sourceID, DataTemplateInfo* ti, uint16 length, byte* data) {
	int i;
	debugs("--- Got a Data Record with fixed fields");
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

	debugs("Listening on Port 1500. Hit Ctrl+C to quit");
	pause();
	debugs("Stopping threads and tidying up.");

	
	stopRcvIpfix();
	rcvIpfixClose(handle);
 	deinitializeRcvIpfix();	

	return 0;
	}

