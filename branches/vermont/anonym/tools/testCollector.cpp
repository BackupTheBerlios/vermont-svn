/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/** \file
 * Separate Program to test the collector
 * Dumps received flows to stdout
 */


#include "concentrator/IpfixCollector.hpp"
#include "concentrator/IpfixParser.hpp"
#include "concentrator/IpfixPacketProcessor.hpp"
#include "concentrator/IpfixReceiverUdpIpV4.hpp"
#include "concentrator/IpfixReceiverSctpIpV4.hpp"
#include "concentrator/IpfixPrinter.hpp"
#include "reconf/ConnectionQueue.h"

#include "common/msg.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define DEFAULT_LISTEN_PORT 1500

void sigint(int) {
    static bool shutdownInitiated = false;

    if (shutdownInitiated) {
        msg(MSG_DIALOG, "second signal received, shutting down the hard way!");
        exit(2);
    }

    shutdownInitiated = true;
}

int main(int argc, char *argv[]) {

	int lport = DEFAULT_LISTEN_PORT;

	msg_setlevel(MSG_DEBUG);
	
	signal(SIGINT, sigint);

	if(argv[1]) {
		lport=atoi(argv[1]);
	}

	// needs to be a pointer because its freed in d'tor of IpfixCollector
	//IpfixReceiverUdpIpV4* ipfixReceiver = new IpfixReceiverUdpIpV4(lport);
	
	ConnectionQueue<IpfixRecord*> queue(100);
	IpfixPrinter printer;




#ifdef SUPPORT_SCTP
	// If you want to create a SCTP testCollector 	
   	IpfixReceiverSctpIpV4 ipfixReceiver(lport, "127.0.0.1");
   	IpfixCollector collector(&ipfixReceiver);
	collector.connectTo(&queue);
#endif
	queue.connectTo(&printer);

	printer.start();
	queue.start();
#ifdef SUPPORT_SCTP
	collector.start();
#endif


	// If you want to create a UDP testCollector
    	IpfixReceiverUdpIpV4 ipfixReceiver2(lport);
	
	
	/* (not in this branch of rcvIpfix)
	if (argc > 2) {
		msg(MSG_DIALOG, "Adding %s to list of authorized hosts", argv[2]);
		ipfixReceiver.addAuthorizedHost(argv[2]);
	}
	*/

	/*IpfixParser ipfixParser;
	ipfixParser.setTemplateLivetime(0);
	ipfixParser.addFlowSink(&ipfixPrinter);

	IpfixCollector ipfixCollector;
#ifdef SUPPORT_SCTP
	ipfixCollector.addIpfixReceiver(&ipfixReceiver);
#endif
	ipfixCollector.addIpfixReceiver(&ipfixReceiver2);
	ipfixCollector.addIpfixPacketProcessor(&ipfixParser);
	ipfixCollector.start();*/

	msg(MSG_DIALOG, "Hit Ctrl+C to quit");
	pause();
	msg(MSG_DIALOG, "Stopping threads and tidying up.\n");

	msg(MSG_DIALOG, "stopping collector\n");
#ifdef SUPPORT_SCTP
	collector.shutdown();
#endif
	queue.shutdown();
	printer.shutdown();
#ifdef SUPPORT_SCTP
	//ipfixReceiver.shutdown();
#endif
	//ipfixReceiver2.shutdown();

	return 0;
}

