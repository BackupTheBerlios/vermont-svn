#ifndef _UDP_RECEIVER_H_
#define _UDP_RECEIVER_H_

#include "receiverFunctions.h"



#include <pthread.h>
#include <stdint.h>

/**
 * Represents a Collector.
 * Create with @c createIpfixReceiver()
 */
typedef struct {
	int socket;
	pthread_mutex_t mutex;      /**< Mutex to pause receiving thread */
	pthread_t thread;	    /**< Thread ID for this particular instance, to sync against etc */

	void* packetProcessor;
	int processorCount;

	} IpfixUdpIpv4Receiver;


//int initializeIpfixUdpIpv4Receivers();
//int deinitializeIpfixUdpIpv4Receivers();

//void* createIpfixUdpIpv4Receiver(uint16_t port);
//void destroyIpfixUdpIpv4Receiver(void* ipfixReceiver);

//int startIpfixUdpIpv4Receiver(void* ipfixReceiver);
//int stopIpfixUdpIpv4Receiver(void* ipfixReceiver);

//int setPacketProcessor(void* ipfixReceiver, void* packetProcessor, int processorCount);

Receiver_Functions getUdpIpv4ReceiverFunctions();

#endif
