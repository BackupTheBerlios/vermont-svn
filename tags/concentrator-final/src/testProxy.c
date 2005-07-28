/** \file
 * Separate Program to test collector, printer and sender
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "rcvIpfix.h"
#include "printIpfix.h"
#include "sndIpfix.h"
#include "common.h"

#define DEFAULT_LISTEN_PORT 1500
#define DEFAULT_TALK_IP "127.0.0.1"
#define DEFAULT_TALK_PORT 1501

#define DEFAULT_SOURCE_ID 4711

void sigint() {
	}

int main(int argc, char* argv[]) {

	char* lip = 0;
        int lport = DEFAULT_LISTEN_PORT;
        char* tip = DEFAULT_TALK_IP;
        int tport = DEFAULT_TALK_PORT;

        signal(SIGINT, sigint);

        if (argc > 1) lip=argv[1];
        if (argc > 2) lport=atoi(argv[2]);
        if (argc > 3) tip=argv[3];
        if (argc > 4) tport=atoi(argv[4]);

	initializeIpfixReceivers();
	initializeIpfixPrinters();
	initializeIpfixSenders();

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(lip, lport);
	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	IpfixSender* ipfixSender = createIpfixSender(DEFAULT_SOURCE_ID, tip, tport);

	addIpfixReceiverCallbacks(ipfixReceiver, getIpfixSenderCallbackInfo(ipfixSender));
	addIpfixReceiverCallbacks(ipfixReceiver, getIpfixPrinterCallbackInfo(ipfixPrinter));

	startIpfixSender(ipfixSender);
	startIpfixPrinter(ipfixPrinter);
	startIpfixReceiver(ipfixReceiver);

	debugf("0.0.0.0:%d => %s:%d", lport, tip, tport);
	debug("Forwarding all Templates and Data Records. Press Ctrl+C to quit.");
	pause();
	debug("Stopping threads and tidying up.");
	
	stopIpfixReceiver(ipfixReceiver);
	destroyIpfixReceiver(ipfixReceiver);

	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);

	stopIpfixSender(ipfixSender);
	destroyIpfixSender(ipfixSender);

 	deinitializeIpfixReceivers();	
	deinitializeIpfixPrinters();
	deinitializeIpfixSenders();

	return 0;
	}

