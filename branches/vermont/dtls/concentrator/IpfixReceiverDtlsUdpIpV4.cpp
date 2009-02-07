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
#include "common/OpenSSLInit.h"

#include <stdexcept>
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
#include <openssl/err.h>
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif


using namespace std;

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
	const std::string &caFile, const std::string &caPath)
    : IpfixReceiver(port),statReceivedPackets(0), have_client_CA_list(false),
	have_cert(false), connections(my_CompareSourceID) {
    struct sockaddr_in serverAddress;

    listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(listen_socket < 0) {
	/* ASK: should I use strerror_r? */
	msg(MSG_FATAL, "Could not create socket: %s", strerror(errno));
	/* ASK: Why not throw? */
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
	close(listen_socket);
	msg(MSG_FATAL, "Could not bind socket: %s", strerror(errno));
	THROWEXCEPTION("Cannot create IpfixReceiverDtlsUdpIpV4 %s:%d",ipAddr.c_str(), port );
    }

    ensure_openssl_init();

    ssl_ctx = SSL_CTX_new(DTLSv1_server_method());
    if( ! ssl_ctx) {
	close(listen_socket);
	THROWEXCEPTION("Cannot create SSL_CTX");
    }
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
    SSL_CTX_load_verify_locations(ssl_ctx,CAfile,CApath);
    // Load our own certificate
    if (!certificateChainFile.empty()) {
	const char *certificate_file = certificateChainFile.c_str();
	const char *PrivateKey_file = certificate_file;
	have_cert = false;
	if (!privateKeyFile.empty())
	    PrivateKey_file = privateKeyFile.c_str();
	if (!SSL_CTX_use_certificate_chain_file(ssl_ctx, certificate_file))
	    msg(MSG_ERROR,"Could not load certificate chain file");
	else if (!SSL_CTX_use_PrivateKey_file(ssl_ctx, PrivateKey_file, SSL_FILETYPE_PEM))
	    msg(MSG_ERROR,"Could not load private key file");
	else if (!SSL_CTX_check_private_key(ssl_ctx))
	    msg(MSG_ERROR,"Private key and certificate do not match.");
	else {
	    have_cert = true;
	}
	if ( ! have_cert) print_errors();
    } else if (!privateKeyFile.empty())
	    msg(MSG_ERROR,"It makes no sense specifying a private key file without "
		    "specifying a file with the corresponding certificate.");
    if (have_cert)
	DPRINTF("We successfully loaded our certificate.");
    else
	DPRINTF("We do NOT have a certificate. This means that we can only use "
		"the anonymous modes of DTLS. This also implies that we can not "
		"authenticate the client (exporter).");
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
		print_errors();
	} else {
	    msg(MSG_ERROR,"Ignoring the file containing trusted "
		    "certificates. We do NOT have a certificate to "
		    "authenticate ourselves that is why we can only use "
		    "anonymous ciphers. Client authentication is not "
		    "possible with anonymous ciphers.");
	}
    }
    // set list of ciphers to ALL (including anonymous ciphers) excluding
    // ciphers with no encryption (eNULL)
    // SSL_CTX_set_cipher_list(ssl_ctx,"ALL:!eNULL");
    SSL_CTX_set_cipher_list(ssl_ctx,"eNULL:ALL");
    SSL_CTX_set_read_ahead(ssl_ctx, 1);
    /* TODO: Find out what this is? */
    SensorManager::getInstance().addSensor(this, "IpfixReceiverDtlsUdpIpV4", 0);

    msg(MSG_INFO, "DTLS Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
							    port, 
							    listen_socket);
}


/**
 * Does UDP/IPv4 specific cleanup
 */
IpfixReceiverDtlsUdpIpV4::~IpfixReceiverDtlsUdpIpV4() {
    close(listen_socket);
    SSL_CTX_free(ssl_ctx);
}

IpfixReceiverDtlsUdpIpV4::DtlsConnection IpfixReceiverDtlsUdpIpV4::createNewConnection(struct sockaddr_in *clientAddress) {
    DtlsConnection conn;

    conn.ssl = SSL_new(ssl_ctx);
    if( ! conn.ssl) {
	THROWEXCEPTION("Cannot create SSL object");
    }

    BIO *sbio, *rbio;
    /* create output abstraction for SSL object */
    sbio = BIO_new_dgram(listen_socket,BIO_NOCLOSE);

    /* create a dummy BIO that always returns EOF */
    rbio = BIO_new(BIO_s_mem());
    /* -1 means EOF */
    BIO_set_mem_eof_return(rbio,-1);
    SSL_set_bio(conn.ssl,rbio,sbio);
    SSL_set_accept_state(conn.ssl);

    BIO_ctrl(conn.ssl->wbio,BIO_CTRL_DGRAM_SET_PEER,0,clientAddress);

    return conn;

}

/* Get errors from OpenSSL error queue and output them using msg() */
void IpfixReceiverDtlsUdpIpV4::print_errors(void) {
    char errbuf[512];
    char buf[4096];
    unsigned long e;
    const char *file, *data;
    int line, flags;

    while ((e = ERR_get_error_line_data(&file,&line,&data,&flags))) {
	ERR_error_string_n(e,errbuf,sizeof errbuf);
	snprintf(buf, sizeof buf, "%s:%s:%d:%s\n", errbuf,
                        file, line, (flags & ERR_TXT_STRING) ? data : "");
	msg(MSG_ERROR, "OpenSSL: %s",buf);
    }
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
	DPRINTF("  %s:%d",ipaddr,sourceID.exporterPort);
    }
}
#endif

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
	DtlsConnection conn;
	if (it == connections.end()) {
	    /* create a new connection if we did not find any. */
	    DPRINTF("New connection");
	    conn = createNewConnection(&clientAddress);
	    it = connections.insert(make_pair(*sourceID,conn)).first;
	} else {
	    /* Use existing connection */
	    DPRINTF("Found existing connection.");
	    conn = it->second;
	}
	dumpConnections();
	BIO_free(conn.ssl->rbio);
	conn.ssl->rbio = BIO_new_mem_buf(secured_data.get(),ret);
	BIO_set_mem_eof_return(conn.ssl->rbio,-1);
	int error;
	ret = SSL_read(conn.ssl,data.get(),MAX_MSG_LEN);
	error = SSL_get_error(conn.ssl,ret);
	DPRINTF("SSL_read() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
	bool shutdown = false;
	if (ret<0) {
	    if (error == SSL_ERROR_WANT_READ)
		continue;
	    msg(MSG_ERROR,"SSL_read() failed. SSL_get_error() returned: %d",error);
	    print_errors();
	    shutdown = true;
	} else if (ret==0) {
	    shutdown = true;

	    if (error == SSL_ERROR_ZERO_RETURN) {
		// remote side closed connection
		DPRINTF("remote side closed connection.");
	    } else {
		msg(MSG_ERROR,"SSL_read() returned 0 and SSL_get_error() returned: %d",error);
		print_errors();
	    }
	} else {
	    DPRINTF("SSL_read() returned %d bytes.",ret);
	}
	if (shutdown) {
	    ret = SSL_shutdown(conn.ssl);
	    error = SSL_get_error(conn.ssl,ret);
	    DPRINTF("SSL_shutdown() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
	    SSL_free(conn.ssl);
	    DPRINTF("Removing connection");
	    connections.erase(it);
	    dumpConnections();
	    continue;
	}

	statReceivedPackets++;
	memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
	sourceID->exporterAddress.len = 4;
	sourceID->exporterPort = ntohs(clientAddress.sin_port);
	sourceID->protocol = IPFIX_protocolIdentifier_UDP;
	sourceID->receiverPort = receiverPort;
	sourceID->fileDescriptor = listen_socket;
	mutex.lock();
	for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
	    (*i)->processPacket(data, ret, sourceID);
	}
	mutex.unlock();
	    
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
#endif /*SUPPORT_OPENSSL*/

