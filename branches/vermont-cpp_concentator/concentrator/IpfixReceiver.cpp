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

#include "IpfixReceiver.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


/**
 * Creates an IpfixReceiver. 
 * The created receiver will do type specific initialization. It's not
 * possible to change the Receiver_Type after creation time.
 * IpfixReceiver is ready to use, after a call to this function.
 * You should assign one (or more) PackeProcessor(s) for reasonable use
 * @c setPacketProcessor()
 * @param receiver_type Desired Transport/Network protocol
 * @param port Port to listen on
 * @return handle to the created instance.
 */
IpfixReceiver::IpfixReceiver(Receiver_Type receiver_type, int port) {
	this->receiver_type = receiver_type;
	
	processorCount = 0;
	
	authCount = 0;
	authHosts = NULL;
		exit = 0;
	
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		goto out1;
	}
	
	if (pthread_mutex_lock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not lock mutex");
		goto out1;
	}
	
	switch (receiver_type) {
	case UDP_IPV4:
		createUdpIpv4Receiver(port);
		break;
	case UDP_IPV6:
		msg(MSG_FATAL, "UDP over IPv6 support isn't implemented yet");
		goto out1;
	case TCP_IPV4:
		msg(MSG_FATAL, "TCP over IPv4 support is horribly broken. We won't start it");
		goto out1;
	case TCP_IPV6:
		msg(MSG_FATAL, "TCP over IPv6 support isn't implemented yet");
		goto out1;
	case SCTP_IPV4:
		msg(MSG_FATAL, "SCTP over IPv4 support isn't implemented yet");
		goto out1;
	case SCTP_IPV6:
		msg(MSG_FATAL, "SCTP over IPv6 support isn't implemented yet");
		goto out1;
	default:
		msg(MSG_FATAL, "Unknown protocol");
		goto out1;
	}


	if(pthread_create(&(thread), 0, listenerThread, this) != 0) {
		msg(MSG_FATAL, "Could not create listener thread");
		goto out1;
	}
	
	return;
out1:
	throw std::runtime_error("IpfixReceiver creation failed");
	return;
}

/**
 * Frees memory used by an IpfixReceiver.
 * Attention: Memory of the assigned PacketProcessors is NOT freed.
 * This has to be done by the calling instance itself.
 * @param ipfixReceiver Handle returned by @c createIpfixReceiver()
 */
IpfixReceiver::~IpfixReceiver() {
	/* do the connection type specific cleanup */
	switch (receiver_type) {
	case UDP_IPV4:
	case UDP_IPV6:
		destroyUdpReceiver();
		break;
	case TCP_IPV4:
	case TCP_IPV6:
	case SCTP_IPV4:
	case SCTP_IPV6:
	default:
		msg(MSG_FATAL, "Unknown protocol");
	}
	
	/* general cleanup */
	
	//FIXME: cleanup listener thread

	if (pthread_mutex_unlock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not unlock mutex");
	}

	pthread_mutex_destroy(&mutex);
}

/**
 * Starts processing messages.
 * All sockets prepared by calls to @c createIpfixReceiver() will start
 * receiving messages until @c stopIpfixReceiver() is called.
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::start() {
	if (pthread_mutex_unlock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not unlock mutex");
		return -1;
	}
	return 0;
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixReceiver() call.
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::stop() {
	if (pthread_mutex_lock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not lock mutex");
		return -1;
	}
	exit = 1;
	return 0;
}

/**
 * Assigns a list of packetProcessors to the Receiver. The list has to be managed (creation and
 * destruction) by the calling instance.
 * @param ipfixReceiver handle of receiver, the packetProcessors should be assigned to
 * @param packetProcessor List of PacketProcessors
 * @param processorCount Number of PacketProcessors in the list.
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::setPacketProcessors(std::list<IpfixPacketProcessor*> packetProcessors) {
	this->packetProcessors = packetProcessors;
	return 0;
}

/**
 * Checks if PacketProcessors where assigned to the IpfixReceiver
 * @return 0 if no PacketProcessors where assigned, > 0 otherwise
 */
int IpfixReceiver::hasPacketProcessor() {
	return processorCount;
}

/**
 * Adds a struct in_addr to the list of hosts we accept packets from
 * @param ipfixReceiver IpfixReceiver to set the callback function for
 * @param host address to add to the list
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::addAuthorizedHost(const char* host) {
	struct in_addr inaddr;

	if (inet_aton(host, &inaddr) == 0) {
		msg(MSG_ERROR, "Invalid host address: %s", host);
		return -1;
	}

	int n = ++authCount;
	authHosts = (struct in_addr*)realloc(authHosts, n * sizeof(struct in_addr));
	memcpy(&authHosts[n-1], &inaddr, sizeof(struct in_addr));
	
	return 0;
}

/**
 * Checks if a given host is a member of the list of authorized hosts
 * @param ipfixReceiver handle to an IpfixReceiver
 * @param inaddr Address of the host to check
 * @param addrlen Length of inaddr
 * @return 0 if host is NOT in list, non-zero otherwise
 */
int IpfixReceiver::isHostAuthorized(struct in_addr* inaddr, int addrlen) {
	/* if we have a list of authorized hosts, discard message if sender is not in this list */
	if (authCount > 0) {
		int i;
		for (i=0; i < authCount; i++) {
			if (memcmp(inaddr, &authHosts[i], addrlen) == 0)
				return 1;
		}
		/* isn't in list */
		return 0;
	}
	return 1;
}

/**
 * Thread function responsible for receiving packets from the network
 * @param ipfixReceiver_ handle to an IpfixReceiver created by @c createIpfixReceiver()
 * @return NULL
 */
void* IpfixReceiver::listenerThread(void* ipfixReceiver_) {
	IpfixReceiver* ipfixReceiver = (IpfixReceiver*)ipfixReceiver_;

	switch (ipfixReceiver->receiver_type) {
	case UDP_IPV4:
	case UDP_IPV6:
		ipfixReceiver->udpListener();
		break;
	case TCP_IPV4:
	case TCP_IPV6:
	case SCTP_IPV4:
	case SCTP_IPV6:
	default:
		msg(MSG_FATAL, "Unknown protocol");
	}

	return NULL;
}

/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void IpfixReceiver::stats()
{
	msg_stat("Concentrator: IpfixReceiver: %6d records received", receivedRecords);
	receivedRecords = 0;
}


/********************************************* Connection type specific functions ************************************************/


/** 
 * Does UDP/IPv4 specific initialization.
 * @param ipfixReceiver handle to an IpfixReceiver created by @createIpfixReceiver()
 * @param port Port to listen on
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::createUdpIpv4Receiver(int port) {
	struct sockaddr_in serverAddress;
	

	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
		perror("Could not create socket");
		return -1;
	}
	
	exit = 0;
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	if(bind(listen_socket, (struct sockaddr*)&serverAddress, 
		sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		return -1;
	}
	return 0;
}


/**
 * Does UDP/IPv4 specific cleanup
 * @param ipfixReceiver handle to an IpfixReceiver, created by @createIpfixReceiver()
 */
void IpfixReceiver::destroyUdpReceiver() {
	close(listen_socket);
}


/**
 * UDP specific listener function. This function is called by @c listenerThread(), when receiver_type is
 * UDP_IPV4 or UDP_IPV6.
 * @param ipfixReceiver handle to an IpfixReceiver, created by @createIpfixReceiver()
 */
void IpfixReceiver::udpListener() {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	uint8_t* data = (uint8_t*)malloc(sizeof(uint8_t)*MAX_MSG_LEN);
	SourceID *sourceID = (SourceID*)malloc(sizeof(SourceID));
	int n;
	
	while(!exit) {
		clientAddressLen = sizeof(struct sockaddr_in);
		n = recvfrom(listen_socket, data, MAX_MSG_LEN,
			     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
		if (n < 0) {
			msg(MSG_DEBUG, "recvfrom returned without data, terminating listener thread");
			break;
		}
		
		if (isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {

			uint32_t ip = ntohl(clientAddress.sin_addr.s_addr);
			memcpy(sourceID->exporterAddress.ip, &ip, 4);
			sourceID->exporterAddress.len = 4;

			pthread_mutex_lock(&mutex);
			for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
			 	pthread_mutex_lock(&(*i)->mutex);
				(*i)->ipfixParser->processMessage(data, n, sourceID);
				pthread_mutex_unlock(&(*i)->mutex);
			}
			pthread_mutex_unlock(&mutex);
		}
		else{
			msg(MSG_DEBUG, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
		}
	}
	
	free(data);
}

