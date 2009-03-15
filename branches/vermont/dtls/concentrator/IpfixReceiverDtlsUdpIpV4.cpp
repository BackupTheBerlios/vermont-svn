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

#ifdef SUPPORT_OPENSSL
#include "IpfixReceiverDtlsUdpIpV4.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "common/msg.h"
#include "common/OpenSSL.h"

#include <stdexcept>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif


using namespace std;

/* Parameters for Diffie-Hellman key agreement */

DH *IpfixReceiverDtlsUdpIpV4::get_dh2048() {
    static unsigned char dh2048_p[]={
	    0xF6,0x42,0x57,0xB7,0x08,0x7F,0x08,0x17,0x72,0xA2,0xBA,0xD6,
	    0xA9,0x42,0xF3,0x05,0xE8,0xF9,0x53,0x11,0x39,0x4F,0xB6,0xF1,
	    0x6E,0xB9,0x4B,0x38,0x20,0xDA,0x01,0xA7,0x56,0xA3,0x14,0xE9,
	    0x8F,0x40,0x55,0xF3,0xD0,0x07,0xC6,0xCB,0x43,0xA9,0x94,0xAD,
	    0xF7,0x4C,0x64,0x86,0x49,0xF8,0x0C,0x83,0xBD,0x65,0xE9,0x17,
	    0xD4,0xA1,0xD3,0x50,0xF8,0xF5,0x59,0x5F,0xDC,0x76,0x52,0x4F,
	    0x3D,0x3D,0x8D,0xDB,0xCE,0x99,0xE1,0x57,0x92,0x59,0xCD,0xFD,
	    0xB8,0xAE,0x74,0x4F,0xC5,0xFC,0x76,0xBC,0x83,0xC5,0x47,0x30,
	    0x61,0xCE,0x7C,0xC9,0x66,0xFF,0x15,0xF9,0xBB,0xFD,0x91,0x5E,
	    0xC7,0x01,0xAA,0xD3,0x5B,0x9E,0x8D,0xA0,0xA5,0x72,0x3A,0xD4,
	    0x1A,0xF0,0xBF,0x46,0x00,0x58,0x2B,0xE5,0xF4,0x88,0xFD,0x58,
	    0x4E,0x49,0xDB,0xCD,0x20,0xB4,0x9D,0xE4,0x91,0x07,0x36,0x6B,
	    0x33,0x6C,0x38,0x0D,0x45,0x1D,0x0F,0x7C,0x88,0xB3,0x1C,0x7C,
	    0x5B,0x2D,0x8E,0xF6,0xF3,0xC9,0x23,0xC0,0x43,0xF0,0xA5,0x5B,
	    0x18,0x8D,0x8E,0xBB,0x55,0x8C,0xB8,0x5D,0x38,0xD3,0x34,0xFD,
	    0x7C,0x17,0x57,0x43,0xA3,0x1D,0x18,0x6C,0xDE,0x33,0x21,0x2C,
	    0xB5,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,0x18,0x11,0x8D,0x7C,
	    0x84,0xA7,0x0A,0x72,0xD6,0x86,0xC4,0x03,0x19,0xC8,0x07,0x29,
	    0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,0xD0,0x0A,0x50,0x9B,
	    0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,0x41,0x9F,0x9C,0x7C,
	    0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,0xA2,0x5E,0xC3,0x55,
	    0xE9,0x32,0x0B,0x3B,
	    };
    static unsigned char dh2048_g[]={0x02};
    DH *dh;

    if ((dh=DH_new()) == NULL) return(NULL);
    dh->p=BN_bin2bn(dh2048_p,sizeof(dh2048_p),NULL);
    dh->g=BN_bin2bn(dh2048_g,sizeof(dh2048_g),NULL);
    if ((dh->p == NULL) || (dh->g == NULL))
	    { DH_free(dh); return(NULL); }
    return(dh);
}

/** 
 * Does DTLS over UDP/IPv4 specific initialization.
 * @param port Port to listen on
 * @param ipAddr IP address to bind to our socket, if equals "", no specific IP address will be bound.
 */
IpfixReceiverDtlsUdpIpV4::IpfixReceiverDtlsUdpIpV4(int port, const std::string ipAddr,
	const std::string &certificateChainFile, const std::string &privateKeyFile,
	const std::string &caFile, const std::string &caPath,
	const std::set<string> &peerFqdnsParam)
    : IpfixReceiver(port),listen_socket(-1),peerFqdns(peerFqdnsParam),
	statReceivedPackets(0),ssl_ctx(NULL),
	have_CAs(false), have_cert(false), connections(my_CompareSourceID) {
    struct sockaddr_in serverAddress;

    try {
	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
	    /* FIXME: should we use strerror_r? */
	    msg(MSG_FATAL, "Could not create socket: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsUdpIpV4, socket creation failed");
	}
	
	// if ipAddr set: Bind a specific IP address to our socket.
	// else: use wildcard address
	if(ipAddr.empty())
	    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	else
	    if ( ! inet_pton(AF_INET,ipAddr.c_str(),&serverAddress.sin_addr) ) {
		THROWEXCEPTION("IP address invalid.");
	    }
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	if(bind(listen_socket, (struct sockaddr*)&serverAddress, 
	    sizeof(struct sockaddr_in)) < 0) {
	    msg(MSG_FATAL, "Could not bind socket: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsUdpIpV4 %s:%d",ipAddr.c_str(), port );
	}

	ensure_openssl_init();

	ssl_ctx = SSL_CTX_new(DTLSv1_server_method());
	if( ! ssl_ctx) {
	    THROWEXCEPTION("Cannot create SSL_CTX");
	}
	SSL_CTX_set_read_ahead(ssl_ctx, 1);
	// set DH parameters to be used
	DH *dh = IpfixReceiverDtlsUdpIpV4::get_dh2048();
	SSL_CTX_set_tmp_dh(ssl_ctx,dh);
	DH_free(dh);
	// generate a new DH key for each handshake
	SSL_CTX_set_options(ssl_ctx,SSL_OP_SINGLE_DH_USE);
	// set default locations for trusted CA certificates
	const char *CAfile = NULL;
	const char *CApath = NULL;
	if (!caFile.empty()) CAfile = caFile.c_str();
	if (!caPath.empty()) CApath = caPath.c_str();
	if (CAfile || CApath) {
	    if ( SSL_CTX_load_verify_locations(ssl_ctx,CAfile,CApath) ) {
		have_CAs = true;
	    } else {
		msg(MSG_ERROR,"SSL_CTX_load_verify_locations() failed.");
		msg_openssl_errors();
		THROWEXCEPTION("Failed to open CA file / CA directory.");
	    }
	}
	// Load our own certificate
	if (!certificateChainFile.empty()) {
	    const char *certificate_chain_file = certificateChainFile.c_str();
	    const char *private_key_file  = certificateChainFile.c_str();
	    if (!privateKeyFile.empty())    // We expect the private key in the
					    // certificate file if no separate
					    // file for the key was specified.
		private_key_file = privateKeyFile.c_str();
	    if (!SSL_CTX_use_certificate_chain_file(ssl_ctx, certificate_chain_file)) {
		msg_openssl_errors();
		THROWEXCEPTION("Unable to load certificate chain file %s",certificate_chain_file);
	    }
	    if (!SSL_CTX_use_PrivateKey_file(ssl_ctx, private_key_file, SSL_FILETYPE_PEM)) {
		msg_openssl_errors();
		THROWEXCEPTION("Unable to load private key file %s",private_key_file);
	    }
	    if (!SSL_CTX_check_private_key(ssl_ctx)) {
		msg_openssl_errors();
		THROWEXCEPTION("Private key and certificate do not match.");
	    }
	    have_cert = true;
	} else if (!privateKeyFile.empty())
		THROWEXCEPTION("It makes no sense specifying a private key file without "
			"specifying a file that contains the corresponding certificate.");
	if (have_cert)
	    DPRINTF("We successfully loaded our certificate.");
	else
	    DPRINTF("We do NOT have a certificate. This means that we can only use "
		    "the anonymous modes of DTLS. This also implies that we can not "
		    "authenticate the client (exporter).");
	/* We leave the certificate_authorities list of the Certificate Request
	 * empty. See RFC 4346 7.4.4. Certificate request. */
#if 0
	// Load the list of CAs with which the client can authenticate itself.
	// Remember that we can only use client authentication if we (server) have
	// a certificate as well.
	if (CAfile) {
	    if (have_cert) {
		STACK_OF(X509_NAME) *cert_names;
		cert_names = SSL_load_client_CA_file(CAfile);
		if (cert_names != NULL) {
		    SSL_CTX_set_client_CA_list(ssl_ctx, cert_names);
		    have_client_CA_list = true;
		    DPRINTF("We have a list of CAs which we can "
			    "use to verify client certificates.");
		} else
		    msg_openssl_errors();
	    } else {
		msg(MSG_ERROR,"Ignoring the file containing trusted "
			"certificates. We do NOT have a certificate to "
			"authenticate ourselves that is why we can only use "
			"anonymous ciphers. Client authentication is not "
			"possible with anonymous ciphers.");
	    }
	}
#endif
	if (peerFqdns.empty()) {
	    DPRINTF("We are NOT going to verify the certificates of the exporters b/c "
		    "the peerFqdn option is NOT set.");
	} else {
	    if ( ! (have_CAs && have_cert) ) {
		msg(MSG_ERROR,"Can not verify certificates of exporters because prerequesites not met. "
			"Prerequesites are: 1. CApath or CAfile or both set, "
			"2. We have a certificate including the private key");
		THROWEXCEPTION("Cannot verify DTLS peers.");
	    } else {
		verify_peers = true;
		SSL_CTX_set_verify(ssl_ctx,SSL_VERIFY_PEER |
			SSL_VERIFY_FAIL_IF_NO_PEER_CERT,0);
		DPRINTF("We are going to request certificates from the exporters "
			"and we are going to verify those b/c "
			"the peerFqdn option is set");
	    }
	}
	if (verify_peers) {
	    SSL_CTX_set_cipher_list(ssl_ctx,"DEFAULT");
	} else {
	    SSL_CTX_set_cipher_list(ssl_ctx,"ALL"); // This includes anonymous ciphers
	}
	/* TODO: Find out what this is? */
	SensorManager::getInstance().addSensor(this, "IpfixReceiverDtlsUdpIpV4", 0);

	msg(MSG_INFO, "DTLS over UDP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								listen_socket);
    } catch(...) {
	if (listen_socket>=0) {
	    close(listen_socket);
	    listen_socket = -1;
	}
	if (ssl_ctx) {
	    SSL_CTX_free(ssl_ctx);
	    ssl_ctx = 0;
	}
	throw; // rethrow
    }
}


/**
 * Does UDP/IPv4 specific cleanup
 */
IpfixReceiverDtlsUdpIpV4::~IpfixReceiverDtlsUdpIpV4() {
    close(listen_socket);
    if (ssl_ctx) SSL_CTX_free(ssl_ctx);
}

#ifdef DEBUG
void IpfixReceiverDtlsUdpIpV4::dumpConnections() {
    struct in_addr addr;
    char ipaddr[INET_ADDRSTRLEN];
    DPRINTF("Dumping DTLS connections:");
    if (connections.empty()) {
	DPRINTF("  (none)");
	return;
    }
    connections_map::const_iterator it = connections.begin();
    for(;it!=connections.end();it++) {
	const IpfixRecord::SourceID &sourceID = it->first;
	memcpy(&addr.s_addr,sourceID.exporterAddress.ip, sourceID.exporterAddress.len);
	inet_ntop(AF_INET,&addr,ipaddr,sizeof(ipaddr));
	DPRINTF("  %s",it->second->inspect().c_str());
    }
}
#endif


const char *IpfixReceiverDtlsUdpIpV4::DtlsConnection::states[] = {
    "ACCEPTING","CONNECTED","SHUTDOWN"
};

int verify_peer_cb(void *context, const char *dnsname) {
    IpfixReceiverDtlsUdpIpV4 *receiver =
	static_cast<IpfixReceiverDtlsUdpIpV4 *>(context);
    string strdnsname(dnsname);
    transform(strdnsname.begin(),strdnsname.end(),strdnsname.begin(),
	    ::tolower);
    if (receiver->peerFqdns.find(strdnsname)!=receiver->peerFqdns.end())
	return 1;
    else
	return 0;
}

int IpfixReceiverDtlsUdpIpV4::DtlsConnection::verify_peer() {
    return verify_ssl_peer(ssl,&verify_peer_cb,&parent);
}

/**
 * UDP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverDtlsUdpIpV4::run() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen;
    clientAddressLen = sizeof(struct sockaddr_in);
    
    fd_set fd_array; //all active filedescriptors
    fd_set readfds;  //parameter for for pselect

    int ret;
    struct timespec timeOut;
    int timeout_count = 0;

    FD_ZERO(&fd_array);
    FD_SET(listen_socket, &fd_array);

    /* set a 400ms time-out on the pselect */
    timeOut.tv_sec = 0L;
    timeOut.tv_nsec = 400000000L;
    
    while(!exitFlag) {
	readfds = fd_array; // because select() changes readfds
	ret = pselect(listen_socket + 1, &readfds, NULL, NULL, &timeOut, NULL);
	if (ret == 0) {
	    /* Timeout */
	    if (++timeout_count == 10) {
		idle_processing();
		timeout_count = 0;
	    }
	    continue;
	}
	if ((ret == -1) && (errno == EINTR)) {
	    /* There was a signal... ignore */
	    continue;
	}
	if (ret < 0) {
	    msg(MSG_ERROR ,"select() returned with an error");
	    THROWEXCEPTION("IpfixReceiverDtlsUdpIpV4: terminating listener thread");
	    break;
	}

	boost::shared_array<uint8_t> secured_data(new uint8_t[MAX_MSG_LEN]);
	boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
	boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);

	ret = recvfrom(listen_socket, secured_data.get(), MAX_MSG_LEN,
		     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
	if (ret < 0) {
	    msg(MSG_FATAL, "recvfrom returned without data, terminating listener thread");
	    break;
	}
	if ( ! isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {
	    msg(MSG_FATAL, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
	    continue;
	}
	char ipaddr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET,&clientAddress.sin_addr,ipaddr,sizeof(ipaddr));
	DPRINTF("Received packet of size %d from %s:%d",ret,ipaddr,ntohs(clientAddress.sin_port));
	/* Set up sourceID */
	memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
	sourceID->exporterAddress.len = 4;
	sourceID->exporterPort = ntohs(clientAddress.sin_port);
	sourceID->protocol = IPFIX_protocolIdentifier_UDP;
	sourceID->receiverPort = receiverPort;
	sourceID->fileDescriptor = listen_socket;
	/* Search for an existing connection with same source IP and port */
	connections_map::iterator it = connections.find(*sourceID);
	DtlsConnectionPtr conn;
	if (it == connections.end()) {
	    /* create a new connection if we did not find any. */
	    DPRINTF("New connection");
	    conn = DtlsConnectionPtr( new DtlsConnection(*this,&clientAddress));
	    it = connections.insert(make_pair(*sourceID,conn)).first;
	} else {
	    /* Use existing connection */
	    DPRINTF("Found existing connection.");
	    conn = it->second;
	}
	conn->consumeDatagram(sourceID, secured_data,ret);
	dumpConnections();
#if 0
	if (conn->consumeDatagram(sourceID, secured_data,ret) == 0) {
	    DPRINTF("Removing connection.");
	    connections.erase(it);
	    dumpConnections();
	}
#endif
	    
    }
    msg(MSG_DEBUG, "IpfixReceiverDtlsUdpIpV4: Exiting");
}

/**
 * statistics function called by StatisticsManager
 */
std::string IpfixReceiverDtlsUdpIpV4::getStatisticsXML(double interval)
{
	ostringstream oss;
	
	oss << "<receivedPackets>" << statReceivedPackets << "</receivedPackets>" << endl;	

	return oss.str();
}

IpfixReceiverDtlsUdpIpV4::DtlsConnection::DtlsConnection(IpfixReceiverDtlsUdpIpV4 &_parent,struct sockaddr_in *pclientAddress) :
    parent(_parent), state(ACCEPTING), last_used(time(NULL)) {

    ssl = SSL_new(parent.ssl_ctx);
    if( ! ssl) {
	THROWEXCEPTION("Cannot create SSL object");
    }

    memcpy(&clientAddress, pclientAddress, sizeof clientAddress);

    BIO *sbio, *rbio;
    /* create output abstraction for SSL object */
    sbio = BIO_new_dgram(parent.listen_socket,BIO_NOCLOSE);

    /* create a dummy BIO that always returns EOF */
    rbio = BIO_new(BIO_s_mem());
    /* -1 means EOF */
    BIO_set_mem_eof_return(rbio,-1);
    SSL_set_bio(ssl,rbio,sbio);
    SSL_set_accept_state(ssl);

    BIO_ctrl(ssl->wbio,BIO_CTRL_DGRAM_SET_PEER,0,&clientAddress);

}

IpfixReceiverDtlsUdpIpV4::DtlsConnection::~DtlsConnection() {
    DPRINTF("Destructor of DtlsConnection.");
    if (ssl) SSL_free(ssl);
}

std::string IpfixReceiverDtlsUdpIpV4::DtlsConnection::inspect() {
    std::ostringstream o;
    char ipaddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&clientAddress.sin_addr,ipaddr,sizeof(ipaddr));
    o << ipaddr << ":" << ntohs(clientAddress.sin_port);
    o << " state:" << states[state];
    return o.str();
}

/* Accepts a DTLS connection
 * Returns values:
 * 1 Successfully connected (don't call accept() again)
 * 0 Not yet connected (call accept() again as soon as new data is available)
 * -1 Failure. Remove this connection
 */
int IpfixReceiverDtlsUdpIpV4::DtlsConnection::accept() {
    int ret, error;
    char buf[512];
    ret = SSL_accept(ssl);
    if (SSL_get_shared_ciphers(ssl,buf,sizeof buf) != NULL)
	DPRINTF("Shared ciphers:%s",buf);
    if (ret==1) {
	state = CONNECTED;
	DPRINTF("SSL_accept() succeeded.");
	const char *str=SSL_CIPHER_get_name(SSL_get_current_cipher(ssl));
	DPRINTF("CIPHER is %s",(str != NULL)?str:"(NONE)");
	if (parent.verify_peers) {
	    if (verify_peer()) {
		DPRINTF("Peer authentication successful.");
	    } else {
		msg(MSG_ERROR,"Peer authentication failed. Shutting down connection.");
		shutdown();
		return -1;
	    }
	}
	return 1;
    }
    error = SSL_get_error(ssl,ret);
    DPRINTF("SSL_accept() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
    if (ret==-1 && error == SSL_ERROR_WANT_READ) {
	DPRINTF("SSL_accept() returned SSL_ERROR_WANT_READ");
	return 0;
    }
    msg(MSG_ERROR,"SSL_accept() failed.");
    long verify_result = SSL_get_verify_result(ssl);
    if(SSL_get_verify_result(ssl)!=X509_V_OK) {
	msg(MSG_ERROR,"Last verification error: %s", X509_verify_cert_error_string(verify_result));
    }
    state = SHUTDOWN;
    msg_openssl_errors();
    return -1;
}

void IpfixReceiverDtlsUdpIpV4::DtlsConnection::shutdown() {
    int ret, error;
    ret = SSL_shutdown(ssl);
    if (ret == 0) {
	DPRINTF("Calling SSL_shutdown a second time.");
	ret = SSL_shutdown(ssl);
    }
    error = SSL_get_error(ssl,ret);
    DPRINTF("SSL_shutdown() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
    state = SHUTDOWN;
}

/* Return values:
 * 1 Success
 * 0 Failure. Remove this connection
 */

int IpfixReceiverDtlsUdpIpV4::DtlsConnection::consumeDatagram(
	boost::shared_ptr<IpfixRecord::SourceID> &sourceID,
	boost::shared_array<uint8_t> secured_data, size_t len) {

    int ret, error;

    last_used = time(NULL);

    if (state == SHUTDOWN) {
	DPRINTF("state == SHUTDOWN. Ignoring datagram");
	return 1;
    }
#ifdef DEBUG
    if ( ! BIO_eof(ssl->rbio)) {
	msg(MSG_ERROR,"EOF *not* reached on BIO. This should not happen.");
    }
#endif
    BIO_free(ssl->rbio);
    ssl->rbio = BIO_new_mem_buf(secured_data.get(),len);
    BIO_set_mem_eof_return(ssl->rbio,-1);
    if (state == ACCEPTING) {
	ret = accept();
	if (ret == 0) return 1;
	if (ret == -1) return 0;
#ifdef DEBUG
	if ( ! BIO_eof(ssl->rbio)) {
	    msg(MSG_ERROR,"EOF *not* reached on BIO. This should not happen.");
	}
#endif
	if (BIO_eof(ssl->rbio)) return 1; /* This should always be the case */
    }
    boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
    ret = SSL_read(ssl,data.get(),MAX_MSG_LEN);
    error = SSL_get_error(ssl,ret);
    DPRINTF("SSL_read() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
    if (ret<0) {
	if (error == SSL_ERROR_WANT_READ)
	    return 1;
	msg(MSG_ERROR,"SSL_read() failed. SSL_get_error() returned: %d",error);
	msg_openssl_errors();
	shutdown();
	return 0;
    } else if (ret==0) {
	if (error == SSL_ERROR_ZERO_RETURN) {
	    // remote side closed connection
	    DPRINTF("remote side closed connection.");
	} else {
	    msg(MSG_ERROR,"SSL_read() returned 0. SSL_get_error() returned: %d",error);
	    msg_openssl_errors();
	}
	shutdown();
	return 0;
    } else {
	DPRINTF("SSL_read() returned %d bytes.",ret);
    }
    parent.statReceivedPackets++;
    parent.mutex.lock();
    for (std::list<IpfixPacketProcessor*>::iterator i = parent.packetProcessors.begin();
	    i != parent.packetProcessors.end(); ++i) { 
	(*i)->processPacket(data, ret, sourceID);
    }
    parent.mutex.unlock();
    return 1;
}

void IpfixReceiverDtlsUdpIpV4::idle_processing() {
    /* Iterate over all connections and remove those that appear
     * to be unused. */
    connections_map::iterator tmp;
    connections_map::iterator it = connections.begin();
    bool changed = false;
    while(it!=connections.end()) {
	tmp = it++;
	if (tmp->second->isInactive()) {
	    DPRINTF("Removing connection %s",tmp->second->inspect().c_str());
	    connections.erase(tmp);
	    changed = true;
	}
    }
    if (changed) {
	dumpConnections();
    }
}

bool IpfixReceiverDtlsUdpIpV4::DtlsConnection::isInactive() {
    time_t diff = time(NULL) - last_used;
    switch (state) {
	case ACCEPTING:
	    if (diff > DTLS_ACCEPT_TIMEOUT) {
		DPRINTF("accept timed out on %s",inspect().c_str());
		shutdown();
		return true;
	    }
	    break;
	case CONNECTED:
	    if (diff > DTLS_IDLE_TIMEOUT) {
		DPRINTF("idle timeout on %s",inspect().c_str());
		shutdown();
		return true;
	    }
	    break;
	case SHUTDOWN:
	    if (diff > DTLS_SHUTDOWN_TIMEOUT) {
		DPRINTF("shutdown timeout on %s",inspect().c_str());
		return true;
	    }
	    break;
    }
    return false;
}
#endif /*SUPPORT_OPENSSL*/

