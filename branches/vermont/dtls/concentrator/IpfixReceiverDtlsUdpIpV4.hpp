/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
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

#ifndef _IPFIX_RECEIVER_DTLSUDPIPV4_H_
#define _IPFIX_RECEIVER_DTLSUDPIPV4_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>
#include <map>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

#ifdef SUPPORT_OPENSSL

#include <openssl/ssl.h>
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif

#endif /* SUPPORT_OPENSSL */

class IpfixReceiverDtlsUdpIpV4 : public IpfixReceiver, Sensor {
#ifdef SUPPORT_OPENSSL
    public:
	IpfixReceiverDtlsUdpIpV4(int port, const std::string ipAddr = "",
	    const std::string &certificateChainFile = "", const std::string &privateKeyFile = "",
	    const std::string &caFile = "", const std::string &caPath = "",
	    const std::vector<string> &peerFqdns = std::vector<string>() );
	virtual ~IpfixReceiverDtlsUdpIpV4();

	virtual void run();
	virtual std::string getStatisticsXML(double interval);
		
    private:
	int listen_socket;
	uint32_t statReceivedPackets;  /**< number of received packets */ 
	SSL_CTX *ssl_ctx;
	static DH *get_dh2048();

	/* Several conditions have to be met before I can authenticate my peer
	 * (exporter):
	 *  - CAfile has to be set since I have to send a Certificate
	 *    Request to my peer and I need to know the names of the CAs to
	 *    include in this request. (have_client_CA_list)
	 *  - At least one of CAfile and CApath have to be set in order
	 *    to be able to verify my peer's certificate. This is somehow
	 *    related to the previous one. (have_CAs)
	 *  - I need to have a valid certificate including the matching
	 *    private key. (have_cert)
	 */
	bool have_client_CA_list;
	bool have_CAs;
	bool have_cert;
	bool verify_peers; /* Do we authenticate our peer by verifying its
			      certificate? */
	struct DtlsConnection {
	    SSL* ssl;
	};
	typedef bool (*CompareSourceID)(const IpfixRecord::SourceID&, const IpfixRecord::SourceID&);
	// compare based on source address and source port
	static bool my_CompareSourceID(const IpfixRecord::SourceID& lhs, const IpfixRecord::SourceID& rhs) {
	    int result;
	    if (lhs.exporterAddress.len < rhs.exporterAddress.len) return true;
	    if (lhs.exporterAddress.len > rhs.exporterAddress.len) return false;
	    result = memcmp(lhs.exporterAddress.ip, rhs.exporterAddress.ip, lhs.exporterAddress.len);
	    if (result < 0) return true;
	    if (result > 0) return false;
	    if (lhs.exporterPort < rhs.exporterPort) return true;
	    return false;
	}
	typedef std::map<IpfixRecord::SourceID,DtlsConnection,CompareSourceID> connections_map;
	connections_map connections;

	DtlsConnection createNewConnection(struct sockaddr_in *);
	void print_errors(void);
#ifdef DEBUG
	void dumpConnections(void);
#else
	inline void dumpConnections(void) {}
#endif

#else

    public:
	IpfixReceiverDtlsUdpIpV4(int port, const std::string ipAddr = "",
	    const std::string &certificateChainFile = "", const std::string &privateKeyFile = "",
	    const std::string &caFile = "", const std::string &caPath = "",
	    const std::vector<string> &peerFqdns = std::vector<string>() ) {
	    THROWEXCEPTION("DTLS over UDP not supported!");
	}

	virtual ~IpfixReceiverDtlsUdpIpV4() {}

	virtual void run() {}
	virtual std::string getStatisticsXML(double interval) { return ""; }

#endif /* SUPPORT_OPENSSL */
};

#endif
