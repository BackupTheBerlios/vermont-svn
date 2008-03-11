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

#include "IpfixReceiverUdpIpV4.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "common/msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace VERMONT {

/** 
 * Does UDP/IPv4 specific initialization.
 * @param port Port to listen on
 */
IpfixReceiverUdpIpV4::IpfixReceiverUdpIpV4(int port, std::string ipAddr) {
	receiverPort = port;

	struct sockaddr_in serverAddress;

	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
		perror("Could not create socket");
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4");
	}
	
	exit = 0;

	// if ipAddr set: listen on a specific interface 
	// else: listen on all interfaces
	if(ipAddr == "")
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		serverAddress.sin_addr.s_addr = inet_addr(ipAddr.c_str());
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	if(bind(listen_socket, (struct sockaddr*)&serverAddress, 
		sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4 %s:%d",ipAddr.c_str(), port );
	}
	msg(MSG_INFO, "UDP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								listen_socket);
	return;
}


/**
 * Does UDP/IPv4 specific cleanup
 */
IpfixReceiverUdpIpV4::~IpfixReceiverUdpIpV4() {
	close(listen_socket);
}


/**
 * UDP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverUdpIpV4::run() {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;

	fd_set fdread;
	int ret;
	struct timeval timeout;

	while(!exit) {
		boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
		boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
		int n;
		
		// use select so that we do not block in recvfrom, when no
		// data is coming and TOPAS is shut down
		do {
			FD_ZERO(&fdread);
			FD_SET(listen_socket, &fdread);
			timeout.tv_usec = 0;
			timeout.tv_sec = 1;
			if (-1 == ret) {
				msg(MSG_FATAL, "Error in select: %s", strerror(errno));
				break;
			} else {
				if (exit) { 
					return;
				}
			}
		} while (0 >= (ret = select(listen_socket + 1, &fdread, NULL, NULL, &timeout)));
		clientAddressLen = sizeof(struct sockaddr_in);
		n = recvfrom(listen_socket, data.get(), MAX_MSG_LEN,
			     0, (struct sockaddr*)&clientAddress, &clientAddressLen);

		if (n < 0) {
			msg(MSG_DEBUG, "recvfrom returned without data, terminating listener thread");
			break;
		}
		
		if (isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {
// 			uint32_t ip = clientAddress.sin_addr.s_addr;
			memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
			sourceID->exporterAddress.len = 4;
			sourceID->exporterPort = ntohs(clientAddress.sin_port);
			sourceID->protocol = IPFIX_protocolIdentifier_UDP;
			sourceID->receiverPort = receiverPort;
			sourceID->fileDescriptor = listen_socket;
			pthread_mutex_lock(&mutex);
			for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
				(*i)->processPacket(data, n, sourceID);
			}
			pthread_mutex_unlock(&mutex);
		}
		else{
			msg(MSG_DEBUG, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
		}
	}
}

};

