#ifndef _UDP_RECEIVER_H_
#define _UDP_RECEIVER_H_

#include "receiverFunctions.h"

#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TODO: make *blabla*
 */
typedef struct {
	int socket;
	pthread_mutex_t mutex;      /**< Mutex to pause receiving thread */
	pthread_t thread;	    /**< Thread ID for this particular instance, to sync against etc */

	void* packetProcessor;
	int processorCount;

	} IpfixUdpIpv4Receiver;

Receiver_Functions getUdpIpv4ReceiverFunctions();

#ifdef __cplusplus
}
#endif

#endif
