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
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>


#define MAX_MSG_LEN	65536


/**
 * TODO: make *blabla*
 */
static void addConnectedSocket(IpfixTcpIpv4Receiver* receiver, int socket) {
	++receiver->connection_count;
	receiver->connected_sockets = (int*)realloc(receiver->connected_sockets,
						    receiver->connection_count*sizeof(int));
	receiver->connected_sockets[receiver->connection_count - 1] = socket;
}

/**
 * TODO: make *blabla* 
 * no range check
 */
static int deleteConnectedSocket(IpfixTcpIpv4Receiver* receiver, int* socket) {
	int* tmp;
	int c = ((socket - receiver->connected_sockets));
	int ret = -1;
	int i;

	if (receiver->connection_count == 0)
		return ret;

	--receiver->connection_count;
	tmp = (int*)malloc(receiver->connection_count*sizeof(int));

	memcpy(tmp, receiver->connected_sockets, c+sizeof(int));
	memcpy(tmp + c, receiver->connected_sockets + c + 1, (receiver->connection_count - c)*sizeof(int));

	free(receiver->connected_sockets);
	receiver->connected_sockets = tmp;

	for (i = 0; i != receiver->connection_count; ++i) {
		if (ret < receiver->connected_sockets[i])
			ret = receiver->connected_sockets[i];
	}

	return ret;	
}

/**
 * TODO: make *blabla*
 */
static void* listenerTcpIpv4(void* tcpReceiver) {
	IpfixTcpIpv4Receiver* receiver = (IpfixTcpIpv4Receiver*)tcpReceiver;
	

	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen = sizeof(struct sockaddr_in);
	byte* data = (byte*)malloc(sizeof(byte)*MAX_MSG_LEN);
	int n, i, j;
	fd_set rfd;
	int maxFd;
	int changed_sockets;
	
	
	/* start listening */
	if (-1 == listen(receiver->listen_socket, 5)) {
		error("Error on listen");
		return NULL;
	}

	maxFd = receiver->listen_socket;

	while(1) {
		FD_ZERO(&rfd);
		FD_SET(receiver->listen_socket, &rfd);
		for (i = 0; i != receiver->connection_count; ++i) {
			FD_SET(receiver->connected_sockets[i], &rfd);
		}
		
		debug("Entering select");
		if (-1 == select(maxFd + 1, &rfd, NULL, NULL, NULL)) {
			error("Error on select");
		}
		debug("Leaving select");

		/* new connection? */
		if (FD_ISSET(receiver->listen_socket, &rfd)) {
			int new_socket;
			debug("New connection attempt");
			if (-1 == (new_socket = accept(receiver->listen_socket,
						       (struct sockaddr*)&clientAddress,
						       &clientAddressLen))){
				error("Could not accept new connection");
			}

			if (new_socket > maxFd)
				maxFd = new_socket;

			pthread_mutex_lock(&receiver->mutex);
			addConnectedSocket(receiver, new_socket);
			pthread_mutex_unlock(&receiver->mutex);
			debug("Established new connection");
		}

		changed_sockets = 0;
		debugf("receiver->connection_count == %i", receiver->connection_count);
		for (i = 0; i != receiver->connection_count && !changed_sockets; ++i) {
			if (FD_ISSET(receiver->connected_sockets[i], &rfd)) {
				n = recvfrom(receiver->connected_sockets[i], data, MAX_MSG_LEN, 0,
					     (struct sockaddr*)&clientAddress, &clientAddressLen);
				debugf("Received %i bytes", n);

				if (n <= 0) {
					debug("recvfrom returned with no data. Closing the connection");
					pthread_mutex_lock(&receiver->mutex);
					close(receiver->connected_sockets[i]);
					maxFd = deleteConnectedSocket(receiver,
								      &receiver->connected_sockets[i]);
					if (maxFd == -1)
						maxFd = receiver->listen_socket;
					changed_sockets = 1;
					pthread_mutex_unlock(&receiver->mutex);
					break;
				}
				
				pthread_mutex_lock(&receiver->mutex);
				PacketProcessor* pp = (PacketProcessor*)(receiver->packetProcessor);
				for (j = 0; j != receiver->processorCount; ++j){
					byte* data_iter = data;
					/* FIXME */
					while (data_iter - data < n) {
						int len = ntohs(*((uint16_t*)(data_iter+sizeof(uint16_t))));
						pp[j].processPacketCallbackFunction(pp[j].ipfixParser, data_iter, len);
						data_iter += len;
					}
					if (data_iter - data != n) {
						errorf("Theres some data remaining: %i bytes", data_iter - data);
					}
					if (data_iter - data > n) {
						errorf("More data passed than has been received: %i bytes", data_iter - data); 
					}
				}
				pthread_mutex_unlock(&receiver->mutex);
			}
		}
	}

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

