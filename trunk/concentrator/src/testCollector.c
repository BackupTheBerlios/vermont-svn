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

#define DEFAULT_LISTEN_PORT 1500

void sigint() {
	}

int main(int argc, char *argv[]) {

        int lport = DEFAULT_LISTEN_PORT;

        signal(SIGINT, sigint);

        if(argv[1]) {
                lport=atoi(argv[1]);
        }

	initializeIpfixPrinters();

	initializeIpfixReceivers();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	startIpfixPrinter(ipfixPrinter);

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(lport);

	if (argc > 2) {
		debugf("Adding %s to list of authorized hosts", argv[2]);
		addIpfixReceiverAuthorizedHost(ipfixReceiver, argv[2]);
		}

	addIpfixReceiverCallbacks(ipfixReceiver, getIpfixPrinterCallbackInfo(ipfixPrinter));
	startIpfixReceiver(ipfixReceiver);

	debugf("Listening on %d. Hit Ctrl+C to quit", lport);
	pause();
	debug("Stopping threads and tidying up.");
	
	stopIpfixReceiver(ipfixReceiver);
	destroyIpfixReceiver(ipfixReceiver);
 	deinitializeIpfixReceivers();	

	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);
	deinitializeIpfixPrinters();

	return 0;
	}

