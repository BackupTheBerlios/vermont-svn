#include "udpReceiver.h"
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

/*
 FIXME: implement clean exiting
 Use pthread_sigmask() ?
 */
static void* listenerUdpIpv4(void* ipfixUdpIpv4Receiver_) {
	IpfixUdpIpv4Receiver* ipfixUdpIpv4Receiver = (IpfixUdpIpv4Receiver*)ipfixUdpIpv4Receiver_;
	
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	byte* data = (byte*)malloc(sizeof(byte)*MAX_MSG_LEN);
	int n, i;
	
	while(1) {
	
		//static uint16_t packets = 0;
		//if (packets++ >= 10000) break;
		
		clientAddressLen = sizeof(struct sockaddr_in);
		n = recvfrom(ipfixUdpIpv4Receiver->socket, data, MAX_MSG_LEN, 0, (struct sockaddr*)&clientAddress, &clientAddressLen);

		if (n < 0) {
			debug("recvfrom returned without data, terminating listener thread");
			break;
			}
		
		pthread_mutex_lock(&ipfixUdpIpv4Receiver->mutex);
		PacketProcessor* pp = (PacketProcessor*)(ipfixUdpIpv4Receiver->packetProcessor);
		for (i = 0; i != ipfixUdpIpv4Receiver->processorCount; ++i) 
			pp[i].processPacketCallbackFunction(pp[i].ipfixParser, data, n);
		
		pthread_mutex_unlock(&ipfixUdpIpv4Receiver->mutex);
		}

	free(data);

	return 0;
	}

/**
 * Initializes internal data.
 * Call once before using any function in this module
 * @return 0 if call succeeded
 */
static int initializeIpfixUdpIpv4Receivers() {
	return 0;
	}

/**
 * Destroys internal data.
 * Call once to tidy up. Do not use any function in this module afterwards
 * @return 0 if call succeeded
 */
static int deinitializeIpfixUdpIpv4Receivers() {
	return 0;
	}


/**
 * Creates a new IpfixUdpIpv4Receiver.
 * Call @c startIpfixUpdIpv4Receiver() to start processing messages.
 * @param port Port to listen on
 * @return handle for further interaction
 */
static void* createIpfixUdpIpv4Receiver(uint16_t port) {
	IpfixUdpIpv4Receiver* ipfixUdpIpv4Receiver;
	struct sockaddr_in serverAddress;
	
	if(!(ipfixUdpIpv4Receiver=(IpfixUdpIpv4Receiver*)malloc(sizeof(IpfixUdpIpv4Receiver)))) {
		fatal("Ran out of memory");
		goto out0;
		}

	ipfixUdpIpv4Receiver->processorCount = 0;
	ipfixUdpIpv4Receiver->packetProcessor = NULL;

	if (pthread_mutex_init(&ipfixUdpIpv4Receiver->mutex, NULL) != 0) {
		fatal("Could not init mutex");
		goto out1;
		}
		
	if (pthread_mutex_lock(&ipfixUdpIpv4Receiver->mutex) != 0) {
		fatal("Could not lock mutex");
		goto out1;
		}

	ipfixUdpIpv4Receiver->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ipfixUdpIpv4Receiver->socket < 0) {
		perror("Could not create socket");
		goto out1;
		}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	if(bind(ipfixUdpIpv4Receiver->socket, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		goto out2;
		}

	if(pthread_create(&(ipfixUdpIpv4Receiver->thread), 0, listenerUdpIpv4, ipfixUdpIpv4Receiver) != 0) {
		fatal("Could not create listener thread");
		goto out2;
		}
	//listenerUdpIpv4(ipfixReceiver); //debug - single-threaded
	
	return ipfixUdpIpv4Receiver;

out2:
	close(ipfixUdpIpv4Receiver->socket);
out1:
	free(ipfixUdpIpv4Receiver);
out0:
	return NULL;
	}

/**
 * Starts processing messages.
 * All sockets prepared by calls to createIpfixUdpIpv4Receiver() will start
 * receiving messages until stopIpfixUdpIpv4Receiver() is called.
 * @return 0 on success, non-zero on error
 */
static int startIpfixUdpIpv4Receiver(void* ipfixUdpIpv4Receiver_) {
	IpfixUdpIpv4Receiver* ipfixUdpIpv4Receiver = (IpfixUdpIpv4Receiver*)ipfixUdpIpv4Receiver_;

	if (pthread_mutex_unlock(&ipfixUdpIpv4Receiver->mutex) != 0) {
		fatal("Could not unlock mutex");
		return -1;
		}
	
	return 0;
	}
	
/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixUdpIpv4Receiver() call.
 * @return 0 on success, non-zero on error
 */
static int stopIpfixUdpIpv4Receiver(void* ipfixUdpIpv4Receiver_) {
	IpfixUdpIpv4Receiver* ipfixUdpIpv4Receiver = (IpfixUdpIpv4Receiver*)ipfixUdpIpv4Receiver_;

	if (pthread_mutex_lock(&ipfixUdpIpv4Receiver->mutex) != 0) {
		fatal("Could not lock mutex");
		return -1;
		}
	
	return 0;
	}

/**
 * Frees memory used by a IpfixUdpIpv4Receiver.
 * @param ipfixReceiver Handle returned by @c createIpfixReceiver()
 */
static void destroyIpfixUdpIpv4Receiver(void* ipfixUdpIpv4Receiver_) {
	IpfixUdpIpv4Receiver* ipfixUdpIpv4Receiver = (IpfixUdpIpv4Receiver*)ipfixUdpIpv4Receiver_;

	close(ipfixUdpIpv4Receiver->socket);
	
	if (pthread_mutex_unlock(&ipfixUdpIpv4Receiver->mutex) != 0) {
		error("Could not unlock mutex");
		}
	pthread_mutex_destroy(&ipfixUdpIpv4Receiver->mutex);
       
	free(ipfixUdpIpv4Receiver);
	}

/**
 * TODO: make *blabla*
 */
static int setPacketProcessor(void* ipfixUdpIpv4Receiver_, void* packetProcessor_, int processorCount) {
	IpfixUdpIpv4Receiver* ipfixUdpIpv4Receiver = (IpfixUdpIpv4Receiver*)ipfixUdpIpv4Receiver_;
       
	ipfixUdpIpv4Receiver->packetProcessor = packetProcessor_;
	ipfixUdpIpv4Receiver->processorCount = processorCount;

	return 0;
}

/**
 * TODO: make *blabla*
 */
static int hasPacketProcessor(void* ipfixUdpIpv4Receiver_) {
	return ((IpfixUdpIpv4Receiver*)ipfixUdpIpv4Receiver_)->processorCount;
}

/**
 * TODO: make *blabla*
 */
Receiver_Functions getUdpIpv4ReceiverFunctions() {
	Receiver_Functions receiver_functions;

	receiver_functions.initializeReceivers   = initializeIpfixUdpIpv4Receivers;
	receiver_functions.deinitializeReceivers = deinitializeIpfixUdpIpv4Receivers;
	receiver_functions.createReceiver        = createIpfixUdpIpv4Receiver;
	receiver_functions.destroyReceiver       = destroyIpfixUdpIpv4Receiver;
	receiver_functions.startReceiver         = startIpfixUdpIpv4Receiver;
	receiver_functions.stopReceiver          = stopIpfixUdpIpv4Receiver;
	receiver_functions.setPacketProcessor    = setPacketProcessor;
	receiver_functions.hasPacketProcessor    = hasPacketProcessor;

	return receiver_functions;
}

