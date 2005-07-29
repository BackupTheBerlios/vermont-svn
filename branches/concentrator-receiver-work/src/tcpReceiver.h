#ifndef _TCP_RECEIVER_H_
#define _TCP_RECEIVER_H_

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
	int listen_socket;

	int* connected_sockets;
	int connection_count;

	pthread_mutex_t mutex;
	pthread_t thread;

	int authCount;              /**< Length of authHosts array */
	struct in_addr* authHosts;  /**< Array of hosts from which this instance accepts packets. If empty, we accept all packets */

	void* packetProcessor;
	int processorCount;

        } IpfixTcpIpv4Receiver;

Receiver_Functions getTcpIpv4ReceiverFunctions();


#ifdef __cplusplus
}
#endif

#endif
