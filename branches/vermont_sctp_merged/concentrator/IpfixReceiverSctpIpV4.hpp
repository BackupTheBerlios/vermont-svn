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
#include "ipfixlolib/ipfixlolib.h"
#ifdef SUPPORT_SCTP

#ifndef _IPFIX_RECEIVER_SCTPIPV4_H_
#define _IPFIX_RECEIVER_SCTPIPV4_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

//Maximum number of simultanious connections
#define SCTP_MAX_CONNECTIONS 5

class IpfixReceiverSctpIpV4 : public IpfixReceiver {
	public:
		IpfixReceiverSctpIpV4(int port);
		virtual ~IpfixReceiverSctpIpV4();

		virtual void run();
	private:
		int listen_socket;
};

#endif

#endif /*SUPPORT_SCTP*/
