/*
IPFIX Collector module
Copyright (C) 2004-2005 Christoph Sommer
http://www.deltadevelopment.de/users/christoph/ipfix
Copyright (C) 2005 Lothar Braun <braunl@informatik.uni-tuebingen.de>

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 2.1 of the License,
or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the
Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
*/


#ifndef EXP_IPFIX_RECEIVER_H_
#define EXP_IPFIX_RECEIVER_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define MAX_MSG_LEN   65536

/**
 * Defines type of Receiver
 */
typedef enum {
        EXP_UDP_IPV4, EXP_UDP_IPV6, EXP_TCP_IPV4, EXP_TCP_IPV6, EXP_SCTP_IPV4, EXP_SCTP_IPV6
} ExpressReceiver_Type;


/**
 * Control structure for receiving process.
 */
typedef struct {
        int listen_socket;

        int* connected_sockets;
        int connection_count;

        pthread_mutex_t mutex; /**< Mutex to pause receiving thread */
        pthread_t thread;      /**< Thread ID for this particular instance, to sync against etc */

        int authCount;
        struct in_addr* authHosts; /**< List of authorized hosts. Only packets from hosts in this list, will be 
                                      forwarded to the PacketProcessors */

        void* packetProcessor;     /**< Authorized incoming packets are forwarded to the packetProcessors. The list
                                      of packetProcessor must be created, managed and destroyed by an superior instance. The
                                      IpfixReceiver will only work with the given list */
        int processorCount;
	int exit; /**< exit flag to terminate thread */

	uint32_t receivedRecords; /**< Statistics: Total number of data (or dataData) records received since last statistics were polled */
        
        ExpressReceiver_Type receiver_type;
} ExpressIpfixReceiver;


int ExpressinitializeIpfixReceivers();
int ExpressdeinitializeIpfixReceivers();

ExpressIpfixReceiver* createExpressIpfixReceiver(ExpressReceiver_Type receiver_type, int port);
void ExpressdestroyIpfixReceiver(ExpressIpfixReceiver* ipfixReceiver);

int startExpressIpfixReceiver(ExpressIpfixReceiver* ipfixReceiver);
int stopExpressIpfixReceiver(ExpressIpfixReceiver* ipfixReceiver);

int ExpressaddAuthorizedHost(ExpressIpfixReceiver* ipfixReceiver, const char*);
int ExpressisHostAuthorized(ExpressIpfixReceiver* ipfixReceiver, struct in_addr* inaddr, int addrlen);
int ExpresssetPacketProcessors(ExpressIpfixReceiver* ipfixReceiver, void* packetProcessor, int processorCount);

void ExpressstatsIpfixReceiver(void* ipfixReceiver);

#ifdef __cplusplus
}
#endif

#endif
