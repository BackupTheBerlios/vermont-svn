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

#define DEFAULT_LISTEN_PORT 1501
#define USE_OLD_IFACE 0
#define USE_NEW_IFACE 1

void sigint() {
	}

void start_collector_using_old_interface(int);
void start_collector_using_new_interface(int);
void usage();

int main(int argc, char *argv[]) {

        int lport=DEFAULT_LISTEN_PORT;
	int c;
	int iface = USE_NEW_IFACE;

        signal(SIGINT, sigint);

	while ((c = getopt(argc, argv, "p:onh")) != -1) {
		switch (c) {
		case 'p':
			lport = atoi(optarg);
			break;
		case 'o':
			iface = USE_OLD_IFACE;
			break;
		case 'n':
			iface = USE_NEW_IFACE;
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
		}
	}

	if (iface == USE_OLD_IFACE) {
		debug("Starting up using old interface");
		start_collector_using_old_interface(lport);
	} else {
		start_collector_using_new_interface(lport);
		debug("Starting up using new interface");
	}
	return 0;
	}



void start_collector_using_old_interface(int lport) {
	initializeIpfixPrinters();

	initializeIpfixReceivers();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	startIpfixPrinter(ipfixPrinter);

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(lport);
	addIpfixReceiverCallbacks(ipfixReceiver, getIpfixPrinterCallbackInfo(ipfixPrinter));
	startIpfixReceiver(ipfixReceiver);

	debugf("Listening on Port %d. Hit Ctrl+C to quit", lport);
	pause();
	debug("Stopping threads and tidying up.");
	
	stopIpfixReceiver(ipfixReceiver);
	destroyIpfixReceiver(ipfixReceiver);
 	deinitializeIpfixReceivers();	

	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);
	deinitializeIpfixPrinters();
        }

void start_collector_using_new_interface(int lport) {
	initializeIpfixPrinters();
	initializeIpfixCollectors();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();

	IpfixCollector* ipfixCollector = createIpfixCollector();	
	setReceiverType(ipfixCollector, TCP_IPV4, lport);
	
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
        }

void usage(char* progname) {
	printf("Usage: %s [-p port] [-n] [-o]\n", progname);
        }
