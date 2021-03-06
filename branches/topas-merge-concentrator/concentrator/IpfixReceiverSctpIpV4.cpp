/*
 * IPFIX Concentrator Module Library - SCTP Receiver
 * Copyright (C) 2007 Alex Melnik
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

#ifdef SUPPORT_SCTP
#include "ipfixlolib/ipfixlolib.h"

#include "IpfixReceiverSctpIpV4.hpp"

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
#include <arpa/inet.h>
#include <netdb.h>

namespace VERMONT {

/** 
 * Does SCTP/IPv4 specific initialization.
 * @param port Port to listen on
 */
IpfixReceiverSctpIpV4::IpfixReceiverSctpIpV4(int port, std::string ipAddr) {
	receiverPort = port;
	
	struct sockaddr_in serverAddress;
	
	listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(listen_socket < 0) {
		perror("Could not create socket");
		THROWEXCEPTION("Cannot create IpfixReceiverSctpIpV4");
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
		THROWEXCEPTION("Cannot create IpfixReceiverSctpIpV4 %s:%d",ipAddr.c_str(), port );
	}
	if(listen(listen_socket, SCTP_MAX_CONNECTIONS) < 0 ) {
		msg(MSG_ERROR ,"Could not listen on SCTP socket %i", listen_socket);
		THROWEXCEPTION("Cannot create IpfixReceiverSctpIpV4");
	}
	msg(MSG_INFO, "SCTP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								listen_socket);
	return;
}


/**
 * Does SCTP/IPv4 specific cleanup
 */
IpfixReceiverSctpIpV4::~IpfixReceiverSctpIpV4() {
	close(listen_socket);
}


/**
 * SCTP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverSctpIpV4::run() {

	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	clientAddressLen = sizeof(struct sockaddr_in);
	
	fd_set fd_array; //all active filedescriptors
	int maxfd;
	
	FD_ZERO(&fd_array);
	FD_SET(listen_socket, &fd_array); // add listensocket
	maxfd = listen_socket;
	
	fd_set readfds;
	int ret;
	int rfd;
	
	
	while(!exit) {
		
		readfds = fd_array; // because select() changes readfds
		ret = select(maxfd + 1, &readfds, NULL, NULL, NULL); // check only for something to read
		if ((ret == -1) && (errno == EINTR)) {
			/* There was a signal... ignore */
			continue;
    		}
    		if (ret < 0) {
    			msg(MSG_ERROR ,"select() returned with an error");
			THROWEXCEPTION("IpfixReceiverSctpIpV4: terminating listener thread");
			break;
		}
		// looking for a new client to connect at listen_socket
		if (FD_ISSET(listen_socket, &readfds)){
			rfd = accept(listen_socket, (struct sockaddr*)&clientAddress, &clientAddressLen);
			
			if (rfd >= 0){
				FD_SET(rfd, &fd_array); // add new client to fd_array
				msg(MSG_DEBUG, "IpfixReceiverSctpIpV4: Client connected from %s:%d, FD=%d", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), rfd);
				if (rfd > maxfd){
					maxfd = rfd;
				}
			}else{
				msg(MSG_ERROR ,"accept() in ipfixReceiver failed");
				THROWEXCEPTION("IpfixReceiverSctpIpV4: unable to accept new connection");
			}
		}
// 		check all connected sockets for new available data
		for (rfd = listen_socket + 1; rfd <= maxfd; ++rfd) {
      			if (FD_ISSET(rfd, &readfds)) {
				boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
      				ret = recvfrom(rfd, data.get(), MAX_MSG_LEN, 0, 
      					(struct sockaddr*)&clientAddress, &clientAddressLen);
				if (ret == 0) { // shut down initiated
					FD_CLR(rfd, &fd_array); // delete dead client
					msg(MSG_DEBUG, "IpfixReceiverSctpIpV4: Client disconnected");
				}else{
					if (isHostAuthorized(&clientAddress.sin_addr,
					    sizeof(clientAddress.sin_addr))) {
						boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
		
						memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
						sourceID->exporterAddress.len = 4;
						sourceID->exporterPort = ntohs(clientAddress.sin_port);
						sourceID->protocol = IPFIX_protocolIdentifier_SCTP;
						sourceID->receiverPort = receiverPort;
						sourceID->fileDescriptor = rfd;
						pthread_mutex_lock(&mutex);
						for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
							(*i)->processPacket(data, ret, sourceID);
						}
						pthread_mutex_unlock(&mutex);
					}
					else{
						msg(MSG_DEBUG, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
					}
				}
      			}
      		}
	}
}

};

#endif /*SUPPORT_SCTP*/
