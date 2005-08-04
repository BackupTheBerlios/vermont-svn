#include "ipfixReceiver.h"

#include "rcvIpfix.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/******************************************* Forward declaration *********************************/

static int createUdpIpv4Receiver(IpfixReceiver* ipfixReceiver, int port);
static void* listenerThread(void* ipfixReceiver_);
static void destroyUdpReceiver(IpfixReceiver* ipfixReceiver);
static void udpListener(IpfixReceiver* ipfixReceiver);

/******************************************* Implementation *************************************/

int initializeIpfixReceivers() {
	return 0;
}

int deinitializeIpfixReceivers() {
	return 0;
}

IpfixReceiver* createIpfixReceiver(Receiver_Type receiver_type, int port) {
	IpfixReceiver* ipfixReceiver;
	
	if(!(ipfixReceiver=(IpfixReceiver*)malloc(sizeof(IpfixReceiver)))) {
		fatal("Ran out of memory");
		goto out0;
	}
	
	ipfixReceiver->receiver_type = receiver_type;
	
	ipfixReceiver->processorCount = 0;
	ipfixReceiver->packetProcessor = NULL;
	
	ipfixReceiver->authCount = 0;
	ipfixReceiver->authHosts = NULL;
	
	if (pthread_mutex_init(&ipfixReceiver->mutex, NULL) != 0) {
		fatal("Could not init mutex");
		goto out1;
	}
	
	if (pthread_mutex_lock(&ipfixReceiver->mutex) != 0) {
		fatal("Could not lock mutex");
		goto out1;
	}
	
	switch (receiver_type) {
	case UDP_IPV4:
		createUdpIpv4Receiver(ipfixReceiver, port);
		break;
	case UDP_IPV6:
		error("UDP over IPv6 support isn't implemented yet");
		goto out1;
	case TCP_IPV4:
		error("TCP over IPv4 support is horribly broken. We won't start it");
		goto out1;
	case TCP_IPV6:
		error("TCP over IPv6 support isn't implemented yet");
		goto out1;
	case SCTP_IPV4:
		error("SCTP over IPv4 support isn't implemented yet");
		goto out1;
	case SCTP_IPV6:
		error("SCTP over IPv6 support isn't implemented yet");
		goto out1;
	default:
		error("Unknown protocol");
		goto out1;
	}


	if(pthread_create(&(ipfixReceiver->thread), 0, listenerThread, ipfixReceiver) != 0) {
		fatal("Could not create listener thread");
		goto out1;
	}
	
	return ipfixReceiver;
out1:
	destroyIpfixReceiver(ipfixReceiver);
out0:
	return NULL;
}


void destroyIpfixReceiver(IpfixReceiver* ipfixReceiver) {
	/* do the connection type specific cleanup */
	switch (ipfixReceiver->receiver_type) {
	case UDP_IPV4:
	case UDP_IPV6:
		destroyUdpReceiver(ipfixReceiver);
		break;
	case TCP_IPV4:
	case TCP_IPV6:
	case SCTP_IPV4:
	case SCTP_IPV6:
	default:
		error("Unknown protocol");
	}
	
	/* general cleanup */
	
	if (pthread_mutex_unlock(&ipfixReceiver->mutex) != 0) {
		error("Could not unlock mutex");
	}

	pthread_mutex_destroy(&ipfixReceiver->mutex);
       
	free(ipfixReceiver);
}


int startIpfixReceiver(IpfixReceiver* ipfixReceiver) {
	if (pthread_mutex_unlock(&ipfixReceiver->mutex) != 0) {
		fatal("Could not unlock mutex");
		return -1;
	}
	
	return 0;
}


int stopIpfixReceiver(IpfixReceiver* ipfixReceiver) {
	if (pthread_mutex_lock(&ipfixReceiver->mutex) != 0) {
		fatal("Could not lock mutex");
		return -1;
	}
	
	return 0;
}


int setPacketProcessor(IpfixReceiver* ipfixReceiver, void* packetProcessor, int processorCount) {
	ipfixReceiver->packetProcessor = packetProcessor;
	ipfixReceiver->processorCount = processorCount;

	return 0;
}

int hasPacketProcessor(IpfixReceiver* ipfixReceiver) {
	return ipfixReceiver->processorCount;
}


int addAuthorizedHost(IpfixReceiver* ipfixReceiver, const char* host) {
	struct in_addr inaddr;

	if (inet_aton(host, &inaddr) == 0) {
		errorf("Invalid host address: %s", host);
		return -1;
	}

	int n = ++ipfixReceiver->authCount;
	ipfixReceiver->authHosts = (struct in_addr*)realloc(ipfixReceiver->authHosts, n * sizeof(struct in_addr));
	memcpy(&ipfixReceiver->authHosts[n-1], &inaddr, sizeof(struct in_addr));
	
	return 0;
}

int isHostAuthorized(IpfixReceiver* ipfixReceiver, struct in_addr* inaddr, int addrlen) {
	/* if we have a list of authorized hosts, discard message if sender is not in this list */
	if (ipfixReceiver->authCount > 0) {
		int i;
		for (i=0; i < ipfixReceiver->authCount; i++) {
			if (memcmp(inaddr, &ipfixReceiver->authHosts[i], addrlen) == 0)
				return 1;
		}
		/* isn't in list */
		return 0;
	}
	return 1;
}



static void* listenerThread(void* ipfixReceiver_) {
	IpfixReceiver* ipfixReceiver = (IpfixReceiver*)ipfixReceiver_;

	switch (ipfixReceiver->receiver_type) {
	case UDP_IPV4:
	case UDP_IPV6:
		udpListener(ipfixReceiver);
		break;
	case TCP_IPV4:
	case TCP_IPV6:
	case SCTP_IPV4:
	case SCTP_IPV6:
	default:
		error("Unknown protocol");
	}

	return NULL;
}

/********************************************* Connection type specific functions ************************************************/


static int createUdpIpv4Receiver(IpfixReceiver* ipfixReceiver, int port) {
	struct sockaddr_in serverAddress;
	

	ipfixReceiver->listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ipfixReceiver->listen_socket < 0) {
		perror("Could not create socket");
		return -1;
	}
	
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	if(bind(ipfixReceiver->listen_socket, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		return -1;
	}
	return 0;
}

static void destroyUdpReceiver(IpfixReceiver* ipfixReceiver) {
	close(ipfixReceiver->listen_socket);
}

static void udpListener(IpfixReceiver* ipfixReceiver) {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	byte* data = (byte*)malloc(sizeof(byte)*MAX_MSG_LEN);
	int n, i;
	
	while(1) {
	
		//static uint16_t packets = 0;
		//if (packets++ >= 10000) break;
		
		clientAddressLen = sizeof(struct sockaddr_in);
		n = recvfrom(ipfixReceiver->listen_socket, data, MAX_MSG_LEN, 0, (struct sockaddr*)&clientAddress, &clientAddressLen);
		if (n < 0) {
			debug("recvfrom returned without data, terminating listener thread");
			break;
		}
		
		if (isHostAuthorized(ipfixReceiver, &clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {
			pthread_mutex_lock(&ipfixReceiver->mutex);
			PacketProcessor* pp = (PacketProcessor*)(ipfixReceiver->packetProcessor);
			for (i = 0; i != ipfixReceiver->processorCount; ++i) 
				pp[i].processPacketCallbackFunction(pp[i].ipfixParser, data, n);
			
			pthread_mutex_unlock(&ipfixReceiver->mutex);
		}
		else{
			debugf("packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
		}
	}
	
	free(data);
}
