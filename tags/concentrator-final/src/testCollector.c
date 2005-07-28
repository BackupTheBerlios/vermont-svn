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
        char* lhost = 0;

        signal(SIGINT, sigint);

        if(argv[1]) {
        	lhost=argv[1]; 
        	}
        if(argv[2]) {
                lport=atoi(argv[2]);
        }

	initializeIpfixPrinters();

	initializeIpfixReceivers();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	startIpfixPrinter(ipfixPrinter);

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(lhost, lport);
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

	return 0;
	}

