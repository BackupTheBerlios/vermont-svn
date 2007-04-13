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

#ifndef _IPFIX_RECEIVER_H_
#define _IPFIX_RECEIVER_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "IpfixPacketProcessor.hpp"

/**
 * Control structure for receiving process.
 */
class IpfixReceiver {
	public:
		/**
		 * Defines type of Receiver
		 */
		typedef enum {
			UDP_IPV4, UDP_IPV6, TCP_IPV4, TCP_IPV6, SCTP_IPV4, SCTP_IPV6
		} Receiver_Type;

		IpfixReceiver(Receiver_Type receiver_type, int port);
		~IpfixReceiver();

		int start();
		int stop();

		int addAuthorizedHost(const char* host);
		int isHostAuthorized(struct in_addr* inaddr, int addrlen);
		int setPacketProcessors(std::list<IpfixPacketProcessor*> packetProcessors);
		int hasPacketProcessor();

		void stats();

	protected:
		int listen_socket;

		int* connected_sockets;
		int connection_count;

		pthread_mutex_t mutex; /**< Mutex to pause receiving thread */
		pthread_t thread; /**< Thread ID for this particular instance, to sync against etc */

		int authCount; /**< Length of authHosts array */
		struct in_addr* authHosts; /**< List of authorized hosts. Only packets from hosts in this list, will be forwarded to the PacketProcessors */

		std::list<IpfixPacketProcessor*> packetProcessors; /**< Authorized incoming packets are forwarded to the packetProcessors. The list of packetProcessor must be created, managed and destroyed by an superior instance. The IpfixReceiver will only work with the given list */
		int processorCount;
		int exit; /**< exit flag to terminate thread */

		uint32_t receivedRecords; /**< Statistics: Total number of data (or dataData) records received since last statistics were polled */
	
		Receiver_Type receiver_type;

		static void* listenerThread(void* ipfixReceiver_);
		void udpListener();
		int createUdpIpv4Receiver(int port);
		void destroyUdpReceiver();
};

#endif
