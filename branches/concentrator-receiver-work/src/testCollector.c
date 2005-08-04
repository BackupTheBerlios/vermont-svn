/** \file
 * Separate Program to test the collector
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "rcvIpfix.h"
#include "printIpfix.h"
#include "common.h"

#define DEFAULT_LISTEN_PORT 4711

void sigint() {
}

void usage();

int main(int argc, char *argv[]) {

        int lport=DEFAULT_LISTEN_PORT;
	
	if (argc > 1) {
		lport = atoi(argv[1]);
	}

        signal(SIGINT, sigint);

	initializeIpfixPrinters();
	initializeIpfixCollectors();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();

	IpfixCollector* ipfixCollector = createIpfixCollector(UDP_IPV4, lport);	
	
	IpfixParser* ipfixParser = createIpfixParser();
	addIpfixParserCallbacks(ipfixParser, getIpfixPrinterCallbackInfo(ipfixPrinter));
	
	PacketProcessor* packetProcessor = createPacketProcessor();
	setIpfixParser(packetProcessor, ipfixParser);
	
	addPacketProcessor(ipfixCollector, packetProcessor);

	startIpfixCollector(ipfixCollector);
        debugf("Listening on Port %d. Hit Ctrl+C to quit", lport);
        pause();
        debug("Stopping threads and tidying up.");
	stopIpfixCollector(ipfixCollector);
	
	destroyIpfixCollector(ipfixCollector);
	deinitializeIpfixCollectors();
	
	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);
        deinitializeIpfixPrinters();

	return 0;
}

void usage(char* progname) {
	printf("Usage: %s [port] \n", progname);
}
