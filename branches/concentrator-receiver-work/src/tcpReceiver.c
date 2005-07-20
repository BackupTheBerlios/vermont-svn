#include "tcpReceiver.h"
#include "rcvIpfix.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_MSG_LEN	65536

/**
 * TODO: make *blabla*
 */
static void* listenerTcpIpv4(void* tcpReceiver) {
	return NULL;
}

/**
 * Initializes internal data.
 * Call once before using any function in this module
 * @return 0 if call succeeded
 */
static int initializeIpfixTcpIpv4Receivers() {
	return 0;
	}

/**
 * Destroys internal data.
 * Call once to tidy up. Do not use any function in this module afterwards
 * @return 0 if call succeeded
 */
static int deinitializeIpfixTcpIpv4Receivers() {
	return 0;
	}


/**
 * Creates a new IpfixUdpIpv4Receiver.
 * Call @c startIpfixUpdIpv4Receiver() to start processing messages.
 * @param port Port to listen on
 * @return handle for further interaction
 */
static void* createIpfixTcpIpv4Receiver(uint16_t port) {
	IpfixTcpIpv4Receiver* receiver;
	struct sockaddr_in serverAddress;
	
	if(!(receiver=(IpfixTcpIpv4Receiver*)malloc(sizeof(IpfixTcpIpv4Receiver)))) {
		fatal("Ran out of memory");
		goto out0;
		}

	receiver->connected_sockets = NULL;
	receiver->connection_count = 0;

	receiver->processorCount = 0;
	receiver->packetProcessor = NULL;

	if (pthread_mutex_init(&receiver->mutex, NULL) != 0) {
		fatal("Could not init mutex");
		goto out1;
		}
		
	if (pthread_mutex_lock(&receiver->mutex) != 0) {
		fatal("Could not lock mutex");
		goto out1;
		}

	receiver->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(receiver->listen_socket < 0) {
		perror("Could not create socket");
		goto out1;
		}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	if(bind(receiver->listen_socket, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		goto out2;
		}

	if(pthread_create(&(receiver->thread), 0, listenerTcpIpv4, receiver) != 0) {
		fatal("Could not create listener thread");
		goto out2;
		}
	
	return receiver;

out2:
	close(receiver->listen_socket);
out1:
	free(receiver);
out0:
	return NULL;
	}


/**
 * Starts processing messages.
 * All sockets prepared by calls to createIpfixTcpIpv4Receiver() will start
 * receiving messages until stopIpfixTcpIpv4Receiver() is called.
 * @return 0 on success, non-zero on error
 */
static int startIpfixTcpIpv4Receiver(void* ipfixTcpIpv4Receiver_) {
	IpfixTcpIpv4Receiver* receiver = (IpfixTcpIpv4Receiver*)ipfixTcpIpv4Receiver_;

	if (pthread_mutex_unlock(&receiver->mutex) != 0) {
		fatal("Could not unlock mutex");
		return -1;
		}
	
	return 0;
	}

	
/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixTcpIpv4Receiver() call.
 * @return 0 on success, non-zero on error
 */
static int stopIpfixTcpIpv4Receiver(void* ipfixTcpIpv4Receiver_) {
	IpfixTcpIpv4Receiver* receiver = (IpfixTcpIpv4Receiver*)ipfixTcpIpv4Receiver_;

	if (pthread_mutex_lock(&receiver->mutex) != 0) {
		fatal("Could not lock mutex");
		return -1;
		}
	
	return 0;
	}

/**
 * Frees memory used by a IpfixTcpIpv4Receiver.
 * @param ipfixTcpIpv4Receiver Handle returned by @c createIpfixTcpIpv4Receiver()
 */
static void destroyIpfixTcpIpv4Receiver(void* ipfixTcpIpv4Receiver_) {
	IpfixTcpIpv4Receiver* receiver = (IpfixTcpIpv4Receiver*)ipfixTcpIpv4Receiver_;
	int i;

	close(receiver->listen_socket);

	for (i = 0; i != receiver->connection_count; ++i) {
		close(receiver->connected_sockets[i]);
	        }
	
	if (pthread_mutex_unlock(&receiver->mutex) != 0) {
		error("Could not unlock mutex");
		}
	pthread_mutex_destroy(&receiver->mutex);
       
	free(receiver);
	}

/**
 * TODO: make *blabla*
 */
static int setPacketProcessor(void* ipfixTcpIpv4Receiver_, void* packetProcessor_, int processorCount) {
	IpfixTcpIpv4Receiver* receiver = (IpfixTcpIpv4Receiver*)ipfixTcpIpv4Receiver_;
       
	receiver->packetProcessor = packetProcessor_;
	receiver->processorCount = processorCount;

	return 0;
}

/**
 * TODO: make *blabla*
 */
static int hasPacketProcessor(void* ipfixTcpIpv4Receiver_) {
	return ((IpfixTcpIpv4Receiver*)ipfixTcpIpv4Receiver_)->processorCount;
}

/**
 * TODO: make *blabla*
 */
Receiver_Functions getTcpIpv4ReceiverFunctions() {
	Receiver_Functions receiver_functions;

	receiver_functions.initializeReceivers   = initializeIpfixTcpIpv4Receivers;
	receiver_functions.deinitializeReceivers = deinitializeIpfixTcpIpv4Receivers;
	receiver_functions.createReceiver        = createIpfixTcpIpv4Receiver;
	receiver_functions.destroyReceiver       = destroyIpfixTcpIpv4Receiver;
	receiver_functions.startReceiver         = startIpfixTcpIpv4Receiver;
	receiver_functions.stopReceiver          = stopIpfixTcpIpv4Receiver;
	receiver_functions.setPacketProcessor    = setPacketProcessor;
	receiver_functions.hasPacketProcessor    = hasPacketProcessor;

	return receiver_functions;
}

