#ifndef _TCP_RECEIVER_H_
#define _TCP_RECEIVER_H_

#include "receiverFunctions.h"

#include <pthread.h>
#include <stdint.h>

/**
 * TODO: make *blabla*
 */
typedef struct {
	int listen_socket;

	int* connected_sockets;
	int connection_count;

	pthread_mutex_t mutex;
	pthread_t thread;

	void* packetProcessor;
	int processorCount;

        } IpfixTcpIpv4Receiver;

Receiver_Functions getTcpIpv4ReceiverFunctions();

#endif
