/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX

 Changes by Daniel Mentz, 2009-01
   Added support for DTLS over UDP
   Note that this work is still ongoing
 TODO:
  * Call SSL_read() from time to time to receive alerts from remote end

 Changes by Gerhard Muenz, 2008-03
   non-blocking SCTP socket
 
 Changes by Alex Melnik, 2007-12
   Added SCTP support
   Corrected sequence number calculation
 
 Changes by Gerhard Muenz, 2006-02-01
   Changed and debugged sendbuffer structure and Co
   Added new function for canceling data sets and deleting fields

 Changes by Christoph Sommer, 2005-04-14
 Modified ipfixlolib-nothread Rev. 80
 Added support for DataTemplates (SetID 4)

 Changes by Ronny T. Lampert, 2005-01
 Changed 03-2005: Had to add a lot of casts for malloc() and friends
 because of stricter C++ checking

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */
#include "ipfixlolib.h"
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define bit_set(data, bits) ((data & bits) == bits)

#ifdef SUPPORT_OPENSSL
static int ensure_exporter_set_up_for_dtls(ipfix_exporter *exporter);
static void deinit_openssl_ctx(ipfix_exporter *exporter);
static int setup_dtls_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col, ipfix_dtls_connection *con);
static int dtls_send(ipfix_exporter *exporter, ipfix_receiving_collector *col, const struct iovec *iov, int iovcnt);
static int dtls_receive_if_necessary(ipfix_dtls_connection *con);
static int dtls_connect(ipfix_receiving_collector *col, ipfix_dtls_connection *con);
static void dtls_shutdown_and_cleanup(ipfix_dtls_connection *con);
static void dtls_fail_connection(ipfix_dtls_connection *con);
#endif
#ifdef SUPPORT_SCTP
static int init_send_sctp_socket(struct sockaddr_in serv_addr);
#endif
static int init_send_udp_socket(struct sockaddr_in serv_addr);
static int ipfix_find_template(ipfix_exporter *exporter, uint16_t template_id, enum template_state cleanness);
static void ipfix_prepend_header(ipfix_exporter *p_exporter, int data_length, ipfix_sendbuffer *sendbuf);
static int ipfix_init_sendbuffer(ipfix_sendbuffer **sendbufn);
static int ipfix_reset_sendbuffer(ipfix_sendbuffer *sendbuf);
static int ipfix_deinit_sendbuffer(ipfix_sendbuffer **sendbuf);
static int ipfix_init_collector_array(ipfix_receiving_collector **col, int col_capacity);
static void remove_collector(ipfix_receiving_collector *collector);
static int ipfix_deinit_collector_array(ipfix_receiving_collector **col);
static int ipfix_init_send_socket(struct sockaddr_in serv_addr , enum ipfix_transport_protocol protocol);
static int ipfix_init_template_array(ipfix_exporter *exporter, int template_capacity);
static int ipfix_deinit_template_array(ipfix_exporter *exporter);
static int ipfix_update_template_sendbuffer(ipfix_exporter *exporter);
static int ipfix_send_templates(ipfix_exporter* exporter);
static int ipfix_send_data(ipfix_exporter* exporter);

#ifdef SUPPORT_OPENSSL

#define SSL_ERR(c) {c,#c}

static struct sslerror {
    int code;
    char *str;
} sslerrors[] = {
    SSL_ERR(SSL_ERROR_NONE),
    SSL_ERR(SSL_ERROR_ZERO_RETURN),
    SSL_ERR(SSL_ERROR_WANT_READ),
    SSL_ERR(SSL_ERROR_WANT_WRITE),
    SSL_ERR(SSL_ERROR_WANT_ACCEPT),
    SSL_ERR(SSL_ERROR_WANT_CONNECT),
    SSL_ERR(SSL_ERROR_WANT_X509_LOOKUP),
    SSL_ERR(SSL_ERROR_SYSCALL),
    SSL_ERR(SSL_ERROR_SSL),
};
static const char *get_ssl_error_string(int ret) {
    unsigned int i;
    static char unknown[] = "Unknown error code: %d";
    static char s[sizeof(unknown) + 20];
    for(i=0;i < sizeof(sslerrors) / sizeof(struct sslerror);i++) {
        if (sslerrors[i].code==ret) {
            return sslerrors[i].str;
        }
    }
    snprintf(s, sizeof(s), unknown, ret);
    return s;
}
#endif /* #ifdef SUPPORT_OPENSSL */


#ifdef SUPPORT_OPENSSL
/* A separate SSL_CTX object is created for every ipfix_exporter.
 * Returns 0 on success, -1 on error
 * */
static int ensure_exporter_set_up_for_dtls(ipfix_exporter *e) {
    ensure_openssl_init();

    if (e->ssl_ctx) return 0;

    /* This SSL_CTX object will be freed in deinit_openssl_ctx() */
    if ( ! (e->ssl_ctx=SSL_CTX_new(DTLSv1_client_method())) ) {
	msg(MSG_FATAL, "Failed to create SSL context");
	msg_openssl_errors();
	return -1;
    }
    SSL_CTX_set_read_ahead(e->ssl_ctx,1);

    if ( (e->ca_file || e->ca_path) &&
	! SSL_CTX_load_verify_locations(e->ssl_ctx,e->ca_file,e->ca_path) ) {
	msg(MSG_FATAL,"SSL_CTX_load_verify_locations() failed.");
	msg_openssl_errors();
	return -1;
    }
    /* Load our own certificate */
    if (e->certificate_chain_file) {
	if (!SSL_CTX_use_certificate_chain_file(e->ssl_ctx, e->certificate_chain_file)) {
	    msg(MSG_FATAL,"Unable to load certificate chain file %s",e->certificate_chain_file);
	    msg_openssl_errors();
	    return -1;
	}
	if (!SSL_CTX_use_PrivateKey_file(e->ssl_ctx, e->private_key_file, SSL_FILETYPE_PEM)) {
	    msg(MSG_FATAL,"Unable to load private key file %s",e->private_key_file);
	    msg_openssl_errors();
	    return -1;
	}
	if (!SSL_CTX_check_private_key(e->ssl_ctx)) {
	    msg(MSG_FATAL,"Private key and certificate do not match");
	    msg_openssl_errors();
	    return -1;
	}
	DPRINTF("We successfully loaded our certificate.");
    } else {
	DPRINTF("We do NOT have a certificate. This means that we can only use "
		"the anonymous modes of DTLS. This also implies that we can not "
		"authenticate the client (exporter).");
    }
    /* We leave the certificate_authorities list of the Certificate Request
     * empty. See RFC 4346 7.4.4. Certificate request. */
    return 0;
}

static void deinit_openssl_ctx(ipfix_exporter *e) {
	if (e->ssl_ctx) SSL_CTX_free(e->ssl_ctx);
	e->ssl_ctx = NULL;
}

/* Return values:
 * -1 failure
 *  0 We expected a packet but did not receive any
 *  1 We received a packet or there was no need to receive a packet.
 */
static int dtls_receive_if_necessary(ipfix_dtls_connection *con) {
    int len;
    /* return immediately if there's no need to read data. */
    if ( ! con->want_read) return 1;

    len = read(con->socket,con->recvbuf,sizeof(con->recvbuf));
    if (len == 0) {
	msg(MSG_FATAL, "IPFIX: EOF detected on UDP socket for DTLS.");
	return -1;
    } else if (len<0) {
	if (errno == EAGAIN) {
	    /* Didn't receive a packet */
	    DPRINTF("Tried to receive a packet for OpenSSL but none was there.");
	    return 0;
	} else {
	    msg(MSG_FATAL, "IPFIX: read failed on UDP socket: %s\n", strerror(errno));
	    return -1;
	}
    }
    DPRINTF("read %d bytes of data which I now pass to OpenSSL.",len);
#ifdef DEBUG
    if (!BIO_eof(con->ssl->rbio)) {
	msg(MSG_FATAL,"BIO did not reach EOF yet! This should not happend!");
    }
#endif
    /* Free existing BIO */
    BIO_free(con->ssl->rbio);
    /* Create new BIO */
    con->ssl->rbio = BIO_new_mem_buf(con->recvbuf,len);
    BIO_set_mem_eof_return(con->ssl->rbio,-1);
    con->want_read = 0;
    return 1;
}

static int dtls_verify_peer_cb(void *context, const char* dnsname) {
    const ipfix_receiving_collector *col =
	(const ipfix_receiving_collector *) context;
    return strcasecmp(col->peer_fqdn,dnsname) ? 0 : 1;
}

static int dtls_get_replacement_connection_ready(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {
    int ret;
    if (!col->dtls_replacement.ssl) {
	/* No SSL object has been created yet. Let's open a socket and
	 * setup a new SSL object. */
	DPRINTF("Setting up replacement connection.");
	if (setup_dtls_connection(exporter,col,&col->dtls_replacement)) {
	    return -1;
	}
    }
    ret = dtls_connect(col,&col->dtls_replacement);
    if (ret == 1) {
	DPRINTF("Replacement connection setup successful.");
	return 1; /* SUCCESS */
    }
    if (ret == 0) {
	if (exporter->dtls_connect_timeout && 
		(time(NULL) - col->dtls_replacement.last_reconnect_attempt_time > exporter->dtls_connect_timeout)) {
	    msg(MSG_ERROR,"DTLS replacement connection setup taking too long.");
	    dtls_fail_connection(&col->dtls_replacement);
	} else {
	    DPRINTF("Replacement connection setup still ongoing.");
	    return 0;
	}
    }
    return -1;
}

static int dtls_send_templates(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {

    ipfix_prepend_header(exporter,
	exporter->template_sendbuffer->committed_data_length,
	exporter->template_sendbuffer);
    DPRINTF("Sending templates.");
    return dtls_send(exporter,col,
	exporter->template_sendbuffer->entries,
	exporter->template_sendbuffer->current);
}

/* returns 0 on success and -1 on error.
 * Note that success does not mean that we are connected. You still have
 * to check the state member of ipfix_receiving_collector to determine
 * if we are connected. */
static int dtls_manage_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col) {
    int ret;

    if (col->state == C_CONNECTED) {
	if( exporter->dtls_max_connection_age == 0 ||
		time(NULL) - col->connect_time < exporter->dtls_max_connection_age) 
	return 0;
	/* Alright, the connection is already very old and needs to be
	 * replaced. Let's get the replacement / backup connection ready. */
	ret = dtls_get_replacement_connection_ready(exporter, col);
	if (ret == 1) { /* Connection setup completed */
	    DPRINTF("Shutting down old DTLS connection.");
	    dtls_shutdown_and_cleanup(&col->dtls_main);
	    DPRINTF("Swapping in new DTLS connection.");
	    memcpy(&col->dtls_main,&col->dtls_replacement,sizeof(col->dtls_main));
	    col->connect_time = time(NULL);
	    ret = dtls_send_templates(exporter, col);
	}
	/* We ignore all other return values of dtls_get_replacement_connection_ready() */
    }
    if (col->state == C_NEW) {
	/* Connection setup is still ongoing. Let's push it forward. */
	ret = dtls_connect(col,&col->dtls_main);
	if (ret == 1) {
	    /* SUCCESS */
	    col->state = C_CONNECTED;
	    col->connect_time = time(NULL);
	    ret = dtls_send_templates(exporter, col);
	    /* dtls_send (inside dtls_send_templates) calls
	     * dtls_fail_connection() and sets col->state
	     * in case of failure. */
	    if (ret >= 0) return 0; else return -1;
	} else if (ret == -1) {
	    /* Failure
	     * dtls_connect() cleaned up SSL object already.
	     * Remember that the socket is now part of the DTLS connection
	     * abstraction. dtls_connect() closed the socket as well. */
	    col->state = C_DISCONNECTED;
	    return -1;
	}
	/* Ok. We get to this point if dtls_connect() returned 0.
	 * In this case the connection setup is still ongoing.
	 * But let's check if it's not ongoing for too long. */
	if (exporter->dtls_connect_timeout && 
		(time(NULL) - col->dtls_main.last_reconnect_attempt_time > exporter->dtls_connect_timeout)) {
	    msg(MSG_ERROR,"DTLS connection setup taking too long.");
	    dtls_fail_connection(&col->dtls_main);
	    col->state = C_DISCONNECTED;
	}
    }
    if (col->state == C_DISCONNECTED) {
	if (setup_dtls_connection(exporter,col,&col->dtls_main)) {
	    /* col->state stays in C_DISCONNECTED in this case
	     * setup_dtls_connection() does not alter it. */
	    return -1;
	}
	col->state = C_NEW;
    }
    return 0;
}

/* Return values:
 * -1 failure
 *  0 no failure but not yet connected. You need to call dtls_connect again
 *        next time
 *  1 yes. now we're connected. Don't call dtls_connect again. */
static int dtls_connect(ipfix_receiving_collector *col, ipfix_dtls_connection *con) {
    int ret, error;

    for (;;) {
	ret = dtls_receive_if_necessary(con);
	switch (ret) {
	    case -1: /* failure */
		dtls_fail_connection(con);
		return -1;
	    case 0: return 0; /* A packet was expected but we did not
				 receive any packet. Therefore it makes
				 no sense to call OpenSSL because OpenSSL
				 needs a packet to proceed with its
				 handshake. */
	    case 1: break;  /* Alright we received a packet or we did
			       not need one. Let's proceed. */
	}
	ret = SSL_connect(con->ssl);
	error = SSL_get_error(con->ssl,ret);
	DPRINTF("SSL_connect returned: ret: %d, SSL_get_error: %s\n",ret,get_ssl_error_string(error));
	if (error == SSL_ERROR_NONE) {
	    DPRINTF("DTLS handshake succeeded. We are now connected.");
	    if (col->peer_fqdn) { /* We need to verify the identity of our peer */
		if (verify_ssl_peer(con->ssl,&dtls_verify_peer_cb,col)) {
		    DPRINTF("Peer authentication successful.");
		} else {
		    msg(MSG_ERROR,"Peer authentication failed. Shutting down connection.");
		    dtls_fail_connection(con);
		    return -1;
		}
	    }
	    return 1;
	} else if (error == SSL_ERROR_WANT_READ) {
	    con->want_read = 1;
	    /* Proceed with next iteration of the endless loop. */
	} else {
	    msg(MSG_FATAL, "IPFIX: SSL_connect failed with %s. Errors:",get_ssl_error_string(error));
	    msg_openssl_errors();
	    dtls_fail_connection(con);
	    return -1;
	}
    }
}

#if 0
static int dtls_receiver(ipfix_dtls_connection *con) {
    int len, error;
    char buf[IPFIX_MAX_PACKETSIZE]; /* general purpose buffer */
    /* Try to receive a packet on the UDP socket. Receiving a packet in
     * dtls_send may sound weird but that's the only way we can find out
     * whether the remote end closed the connection. */
    if (BIO_eof(con->ssl->rbio)) {
	len = read(con->data_socket,col->dtls.recvbuf,sizeof(col->dtls.recvbuf));
	if (len == 0) {
	    msg(MSG_FATAL, "IPFIX: EOF detected on UDP socket for DTLS.");
	    dtls_fail_connection(col);
	    return -1;
	} else if (len<0 && errno != EAGAIN) {
	    msg(MSG_FATAL, "IPFIX: read failed on UDP socket: %s\n", strerror(errno));
	    dtls_fail_connection(col);
	    return -1;
	} else if (len>0) {
	    DPRINTF("read %d bytes of data which I now pass to OpenSSL.",len);
	    /* Free existing BIO */
	    BIO_free(col->dtls.ssl->rbio);
	    /* Create new BIO */
	    col->dtls.ssl->rbio = BIO_new_mem_buf(col->dtls.recvbuf,len);
	    BIO_set_mem_eof_return(col->dtls.ssl->rbio,-1);
	}
    }
    /* If there's a packet (UDP datagram) waiting in the memory BIO
     * then read it */
    if ( ! BIO_eof(col->dtls.ssl->rbio)) {
	len = SSL_read(col->dtls.ssl, buf, sizeof(buf));
	error = SSL_get_error(col->dtls.ssl,len);
	if (len >  0) {
	    msg(MSG_ERROR,"Received unexpected data on DTLS channel.");
	} else if (len == -1 && error == SSL_ERROR_ZERO_RETURN) {
	    msg(MSG_ERROR,"Remote end shut down connection.");
	    dtls_fail_connection(col);
	    return -1;
	} else if (error == SSL_ERROR_WANT_READ) {
	    /* do nothing */
	} else {
	    msg(MSG_ERROR,"SSL_read() failed: len: %d, SSL_get_error: %s\n",len,get_ssl_error_string(error));
	    dtls_fail_connection(col);
	    return -1;
	}
    }
}
#endif

/* Return values:
 * n>0: sent n bytes
 * 0: Could not send due to OpenSSL returning SSL_ERROR_WANT_READ
 * -1: Recoverable error
 * -2: Error. Shutdown connection if you get this.
 */

static int dtls_send_helper(ipfix_exporter *exporter,
	ipfix_dtls_connection *con,
	const struct iovec *iov, int iovcnt) {
    int len, error, i, ret;
    char sendbuf[IPFIX_MAX_PACKETSIZE];
    char *sendbufcur = sendbuf;
    int maxsendbuflen = sizeof(sendbuf);
    /* Collect data form iovecs */
    for (i=0;i<iovcnt;i++) {
	if (sendbufcur + iov[i].iov_len > sendbuf + maxsendbuflen) {
	    msg(MSG_FATAL, "IPFIX: sendbuffer for dtls_send too small.");
	    return -1;
	}
	memcpy(sendbufcur,iov[i].iov_base,iov[i].iov_len);
	sendbufcur+=iov[i].iov_len;
    }

    for(;;) {
	len = SSL_write(con->ssl, sendbuf, sendbufcur - sendbuf);
	error = SSL_get_error(con->ssl,len);
	DPRINTF("SSL_write() returned: len: %d, SSL_get_error: %s\n",len,get_ssl_error_string(error));
	switch (error) {
	    case SSL_ERROR_NONE:
		if (len!=sendbufcur - sendbuf) {
		    msg(MSG_FATAL, "IPFIX: len!=sendbuflen when calling SSL_write()");
		    return -1;
		}
		return sendbufcur - sendbuf; /* SUCCESS */
	    case SSL_ERROR_WANT_READ:
		con->want_read = 1;
		/* Continue in loop */
	    default:
		msg(MSG_FATAL, "IPFIX: SSL_write failed.");
		dtls_fail_connection(con);
		return -2;
	}
	ret = dtls_receive_if_necessary(con);
	switch (ret) {
	    case -1: /* failure */
		dtls_fail_connection(con);
		return -2;
	    case 0: return 0; /* A packet was expected but we did not
				 receive any packet. Therefore it makes
				 no sense to call OpenSSL because OpenSSL
				 needs a packet to proceed with its
				 handshake. */
	    case 1: break;  /* Alright we received a packet or we did
			       not need one. Let's proceed. */
	}
    }
}

/* Return values:
 * -1 error
 *  0 could not write because OpenSSL returned SSL_ERROR_WANT_READ
 *  n>0 number of bytes written
 */
static int dtls_send(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	const struct iovec *iov, int iovcnt) {

    int len;

    /* DTLS negotiation has to be finished before we can send data.
     * Drop out of this function if we are not yet connected. */
    if (col->state != C_CONNECTED) {
	return -1;
    }

    len = dtls_send_helper(exporter, &col->dtls_main, iov, iovcnt);
    if (len == -2) {
	col->state = C_DISCONNECTED;
	return -1;
    }
    return len;
}

/* returns 0 on success and -1 on failure */
static int setup_dtls_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col, ipfix_dtls_connection *con) {
    int flags;
    BIO *rbio, *sbio;
/* Resources allocated in this function. Those need to be freed in case of failure:
 * - socket
 * - SSL object
 * - dgram BIO
 * - memory BIO
 */

#ifdef DEBUG
    if (con->socket!=-1) {
	msg(MSG_FATAL,"socket != -1");
	close(con->socket);
    }
#endif

    /* Create socket */
    con->socket = init_send_udp_socket(col->addr);

    /* set socket to non-blocking i/o */
    flags = fcntl(con->socket,F_GETFL,0);
    flags |= O_NONBLOCK;
    if (fcntl(con->socket,F_SETFL,flags)<0) {
	msg(MSG_FATAL, "IPFIX: Failed to set socket to non-blocking i/o");
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* ensure a SSL_CTX object is set up */
    if (ensure_exporter_set_up_for_dtls(exporter)) {
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* create SSL object */
    if ( ! (con->ssl = SSL_new(exporter->ssl_ctx))) {
	msg(MSG_FATAL, "Failed to create SSL object.");
	msg_openssl_errors();
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* Set verification parameters and cipherlist */
    if (!col->peer_fqdn) {
	SSL_set_cipher_list(con->ssl,"ALL"); // This includes anonymous ciphers
	DPRINTF("We are NOT going to verify the certificates of the collectors b/c "
		"the peerFqdn option is NOT set.");
    } else {
	if ( ! ((exporter->ca_file || exporter->ca_path) &&
		    exporter->certificate_chain_file) ) {
	    msg(MSG_ERROR,"Can not verify certificates of collectors because prerequesites not met. "
		    "Prerequesites are: 1. CApath or CAfile or both set, "
		    "2. We have a certificate including the private key");
	    SSL_free(con->ssl);con->ssl = NULL;
	    close(con->socket);con->socket = -1;
	    return -1;
	} else {
	    SSL_set_cipher_list(con->ssl,"DEFAULT");
	    SSL_set_verify(con->ssl,SSL_VERIFY_PEER |
		    SSL_VERIFY_FAIL_IF_NO_PEER_CERT,0);
	    DPRINTF("We are going to request certificates from the collectors "
		    "and we are going to verify those b/c "
		    "the peerFqdn option is set");
	}
    }
    /* create output abstraction for SSL object */
    if ( ! (sbio = BIO_new_dgram(con->socket,BIO_NOCLOSE))) {
	msg(MSG_FATAL,"Failed to create datagram BIO.");
	msg_openssl_errors();
	SSL_free(con->ssl);con->ssl = NULL;
	close(con->socket);con->socket = -1;
	return -1;
    }

    BIO_ctrl_set_connected(sbio,1,&col->addr); /* TODO: Explain, why are we doing this? */
    /* create a dummy BIO that always returns EOF */
    if ( ! (rbio = BIO_new(BIO_s_mem()))) {
	msg(MSG_FATAL,"Failed to create memory BIO");
	msg_openssl_errors();
	SSL_free(con->ssl);con->ssl = NULL;
	close(con->socket);con->socket = -1;
	BIO_free(sbio);
	return -1;
    }
    /* -1 means EOF */
    BIO_set_mem_eof_return(rbio,-1);
    SSL_set_bio(con->ssl,rbio,sbio);
    DPRINTF("Set up SSL object.");

    con->want_read = 0;
    con->last_reconnect_attempt_time = time(NULL);

    return 0;
}

static void dtls_shutdown_and_cleanup(ipfix_dtls_connection *con) {
    int ret,error;
    if (!con->ssl) return;
    DPRINTF("Shutting down SSL connection.");
    ret = SSL_shutdown(con->ssl);
#ifdef DEBUG
    error = SSL_get_error(con->ssl,ret);
    DPRINTF("SSL_shutdown returned: ret: %d, SSL_get_error: %s\n",ret,get_ssl_error_string(error));
#endif
    /* Note: SSL_free() also frees associated sending and receiving BIOs */
    SSL_free(con->ssl);
    con->ssl = NULL;
    con->want_read = 0;
    /* Close socket */
    if ( con->socket != -1) {
	DPRINTF("Closing socket");
	close(con->socket);
	con->socket = -1;
    }
}

static void dtls_fail_connection(ipfix_dtls_connection *con) {
    DPRINTF("Failing DTLS connection.");
    dtls_shutdown_and_cleanup(con);
}

#endif /* SUPPORT_OPENSSL */

/*
 * Initializes a UDP-socket to send data to.
 * Parameters:
 * char* serv_ip4_addr IP-Address of the recipient (e.g. "123.123.123.123")
 * serv_port the UDP-portnumber of the server.
 * Returns: a socket to write to. -1 on failure
 */
static int init_send_udp_socket(struct sockaddr_in serv_addr){

        int s;
        // create socket
        if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                msg(MSG_FATAL, "IPFIX: error opening socket, %s", strerror(errno));
                return -1;
        }

        // connect to server
        if(connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr) ) < 0) {
                msg(MSG_FATAL, "IPFIX: connect failed, %s", strerror(errno));
                /* clean up */
                close(s);
                return -1;
        }

	return s;
}


#ifdef SUPPORT_SCTP
/********************************************************************
** SCTP Extension Code:
*********************************************************************/
/*
 * Initializes a SCTP-socket to send data to.
 * Parameters:
 * char* serv_ip4_addr IP-Address of the recipient (e.g. "123.123.123.123")
 * serv_port the SCTP-portnumber of the server.
 * Returns: a socket to write to. -1 on failure
 */
static int init_send_sctp_socket(struct sockaddr_in serv_addr){
	
	int s;
	
	//create socket:
	DPRINTFL(MSG_VDEBUG, "Creating SCTP Socket ...");
	if((s = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0 ) {
                msg(MSG_FATAL, "IPFIX: error opening SCTP socket, %s", strerror(errno));
                return -1;
        }
	// set non.blocking
	int flags;
	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK;
	if(fcntl(s, F_SETFL, flags) == -1) {
                msg(MSG_FATAL, "IPFIX: could not set socket non-blocking");
		close(s);
                return -1;
	}
	// connect (non-blocking, i.e. handshake is initiated, not terminated)
        if((connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr) ) == -1) && (errno != EINPROGRESS)) {
		msg(MSG_ERROR, "IPFIX: SCTP connect failed, %s", strerror(errno));
		close(s);
                return -1;
        }
	DPRINTFL(MSG_VDEBUG, "SCTP Socket created");

	return s;
}

/*
 * modification of the original sctp_sendmsg to handle iovec structs
 * Parameters:
 * s 		: socket
 * *vector 	: iovec struct containing the buffer to send
 * v_len 	: lenght of the buffer
 * *to 		: address where data is going to be sent
 * tolen 	: length of the address
 * ppid, flags, stream_no, timetolive, context : sctp parameters
 */
int sctp_sendmsgv(int s, struct iovec *vector, int v_len, struct sockaddr *to,
		socklen_t tolen, uint32_t ppid, uint32_t flags,
	     	uint16_t stream_no, uint32_t timetolive, uint32_t context){

	struct msghdr outmsg;
	char outcmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
	struct cmsghdr *cmsg;
	struct sctp_sndrcvinfo *sinfo;

	outmsg.msg_name = to;
	outmsg.msg_namelen = tolen;
	outmsg.msg_iov = vector;
	outmsg.msg_iovlen = v_len;

	outmsg.msg_control = outcmsg;
	outmsg.msg_controllen = sizeof(outcmsg);
	outmsg.msg_flags = 0;

	cmsg = CMSG_FIRSTHDR(&outmsg);
	cmsg->cmsg_level = IPPROTO_SCTP;
	cmsg->cmsg_type = SCTP_SNDRCV;
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

	outmsg.msg_controllen = cmsg->cmsg_len;
	sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
	memset(sinfo, 0, sizeof(struct sctp_sndrcvinfo));
	sinfo->sinfo_ppid = ppid;
	sinfo->sinfo_flags = flags;
	sinfo->sinfo_stream = stream_no;
	sinfo->sinfo_timetolive = timetolive;
	sinfo->sinfo_context = context;

	return sendmsg(s, &outmsg, 0);
}
#endif /*SUPPORT_SCTP*/

/********************************************************************
//END of SCTP Extension Code:
*********************************************************************/

/*
 * Initialize an exporter process
 * Allocates all memory necessary.
 * Parameters:
 * sourceID The source ID, to which the exporter will be initialized to.
 * exporter an ipfix_exporter* to be initialized
 */
int ipfix_init_exporter(uint32_t source_id, ipfix_exporter **exporter)
{
        ipfix_exporter *tmp;
        int ret;

        if(!(tmp=(ipfix_exporter *)malloc(sizeof(ipfix_exporter)))) {
                goto out;
        }

        tmp->source_id=source_id;
        tmp->sequence_number = 0;
        tmp->sn_increment = 0;
        tmp->collector_max_num = 0;
#ifdef SUPPORT_OPENSSL
	tmp->ssl_ctx = NULL;
	tmp->certificate_chain_file = NULL;
	tmp->private_key_file = NULL;
	tmp->ca_file = NULL;
	tmp->ca_path = NULL;
	tmp->dtls_connect_timeout = 30;
	tmp->dtls_max_connection_age = 120;
#endif

        // initialize the sendbuffers
        ret=ipfix_init_sendbuffer(&(tmp->data_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "IPFIX: initializing data sendbuffer failed");
                goto out1;
        }

        ret=ipfix_init_sendbuffer(&(tmp->template_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "IPFIX: initializing template sendbuffer failed");
                goto out2;
        }
	
	ret=ipfix_init_sendbuffer(&(tmp->sctp_template_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "IPFIX: initializing sctp template sendbuffer failed");
                goto out5;
        }
	
        // intialize the collectors to zero
        ret=ipfix_init_collector_array( &(tmp->collector_arr), IPFIX_MAX_COLLECTORS);
        if (ret !=0) {
                msg(MSG_FATAL, "IPFIX: initializing collectors failed");
                goto out3;
        }

        tmp->collector_max_num = IPFIX_MAX_COLLECTORS;

        // initialize an array to hold the templates.
        if(ipfix_init_template_array(tmp, IPFIX_MAX_TEMPLATES)) {
                goto out4;
        }
	
        // we have not transmitted any templates yet!
        tmp->last_template_transmission_time=0;
        tmp->template_transmission_timer=IPFIX_DEFAULT_TEMPLATE_TIMER;
	tmp->sctp_reconnect_timer=IPFIX_DEFAULT_SCTP_RECONNECT_TIMER;
	tmp->sctp_lifetime=IPFIX_DEFAULT_SCTP_DATA_LIFETIME;
	
        /* finally attach new exporter to the pointer we were given */
        *exporter=tmp;

        return 0;

out5:
        ipfix_deinit_sendbuffer(&(tmp->sctp_template_sendbuffer));
out4:
        ipfix_deinit_collector_array(&(tmp->collector_arr));
out3:
        ipfix_deinit_sendbuffer(&(tmp->data_sendbuffer));
out2:
        ipfix_deinit_sendbuffer(&(tmp->template_sendbuffer));
out1:
        free(tmp);
out:
        /* we have nothing to free */
        return -1;
}


/*
 * cleanup an exporter process
 */
int ipfix_deinit_exporter(ipfix_exporter *exporter) {
        // cleanup processes
        int ret;
        // close sockets etc.
        // (currently, nothing to do)

        // free all children

        // deinitialize array to hold the templates.
        ret=ipfix_deinit_template_array(exporter);

        /*   free ( (**exporter).template_arr); */
        /*   (**exporter).template_arr = NULL; */

        // deinitialize the sendbuffers
        ret=ipfix_deinit_sendbuffer(&(exporter->data_sendbuffer));
        ret=ipfix_deinit_sendbuffer(&(exporter->template_sendbuffer));
	ret=ipfix_deinit_sendbuffer(&(exporter->sctp_template_sendbuffer));

        // find the collector in the exporter
        int i=0;
	for(i=0;i<exporter->collector_max_num;i++) {
	    if (exporter->collector_arr[i].state != C_UNUSED)
		remove_collector(&exporter->collector_arr[i]);
        }
        // deinitialize the collectors
        ret=ipfix_deinit_collector_array(&(exporter->collector_arr));

#ifdef SUPPORT_OPENSSL
	deinit_openssl_ctx(exporter);
	free( (void *) exporter->certificate_chain_file);
	free( (void *) exporter->private_key_file);
	free( (void *) exporter->ca_file);
	free( (void *) exporter->ca_path);
#endif

        // free own memory
        free(exporter);
        exporter=NULL;

        return 0;
}

static ipfix_receiving_collector *get_free_collector_slot(ipfix_exporter *exporter) {
    ipfix_receiving_collector *collector;
    int i;
    for(i=0;i<exporter->collector_max_num;i++) {
	collector = &exporter->collector_arr[i];
	if(collector->state == C_UNUSED)
	    return collector;
    }
    return NULL;
}

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
int add_collector_rawdir(ipfix_receiving_collector *collector,char *path) {
    collector->ipv4address[0] = '\0';
    collector->port_number = 0;
    collector->data_socket = -1;
    memset(&(collector->addr), 0, sizeof(collector->addr));
    collector->last_reconnect_attempt_time = 0;


    collector->packet_directory_path = strdup(coll_ip4_addr);
    collector->packets_written = 0;
    collector->state = C_NEW;
    return 0;
}
#endif

#ifdef SUPPORT_OPENSSL
int add_collector_dtls(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	ipfix_aux_config_dtls *aux_config) {

    col->dtls_replacement.socket = -1;
    col->dtls_replacement.ssl = NULL;
    col->dtls_replacement.want_read = 0;

    ipfix_aux_config_dtls *aux_config_dtls = (ipfix_aux_config_dtls *) aux_config;
    if (aux_config && aux_config_dtls->peer_fqdn)
	col->peer_fqdn = strdup(aux_config_dtls->peer_fqdn);

    if (setup_dtls_connection(exporter,col,&col->dtls_main)) {
	free( (void *) col->peer_fqdn);
	col->peer_fqdn = NULL;
	return -1;
    }
    col->state = C_NEW; /* By setting the state to C_NEW we are
				 basically allocation the slot. */
    /* Initiate connection setup */
    dtls_manage_connection(exporter, col);
    return 0;
}
#endif

int add_collector_remaining_protocols(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {
    // call a separate function for opening the socket
    col->data_socket = ipfix_init_send_socket(col->addr, col->protocol);
    // error handling, in case we were unable to open the port:
    if(col->data_socket < 0 ) {
	msg(MSG_ERROR, "IPFIX: add collector, initializing socket failed");
	return -1;
    }
    // now, we may set the collector to valid;
    col->state = C_NEW; /* By setting the state to C_NEW we are
				 basically allocation the slot. */
    col->last_reconnect_attempt_time = time(NULL);

    return 0;
}

/*
 * Add a collector to the exporting process
 * Parameters:
 *  exporter: The exporting process, to which the collector shall be attached
 *  coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
 *  coll_port: port number of the collector
 *  proto: transport protocol to use
 * Returns: 0 on success or -1 on failure
 */
int ipfix_add_collector(ipfix_exporter *exporter, const char *coll_ip4_addr,
	int coll_port, enum ipfix_transport_protocol proto, void *aux_config)
{
    // check, if exporter is valid
    if(exporter == NULL) {
	msg(MSG_FATAL, "IPFIX: add_collector, exporter is NULL");
	return -1;
    }

    // get free slot
    ipfix_receiving_collector *collector = get_free_collector_slot(exporter);
    if( ! collector) {
	msg(MSG_FATAL, "IPFIX: no more free slots for new collectors available, limit %d reached",
		exporter->collector_max_num
	   );
	return -1;
    }
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
    /* It is the duty of add_collector_rawdir to set collector->state */
    if (proto==RAWDIR) return add_collector_rawdir(collector,coll_ip4_addr);
#endif
    /*
    FIXME: only a quick fix to make that work
    Must be copied, else pointered data must be around forever
    Better use binary/u_int32_t representation
    */
    strncpy(collector->ipv4address, coll_ip4_addr, sizeof(collector->ipv4address));
    /* strncpy does not null terminate the destination char array if the
     * length of the source string is equal or greater then the maximum length
     * (third parameter) */
    collector->ipv4address[sizeof(collector->ipv4address)-1] = '\0';
    collector->port_number = coll_port;
    collector->protocol = proto;

    memset(&(collector->addr), 0, sizeof(collector->addr));
    collector->addr.sin_family = AF_INET;
    collector->addr.sin_port = htons(coll_port);
    collector->addr.sin_addr.s_addr = inet_addr(coll_ip4_addr);

#ifdef SUPPORT_OPENSSL
    /* It is the duty of add_collector_dtls to set collector->state */
    if (proto ==  DTLS_OVER_UDP)
	return add_collector_dtls(exporter, collector,
		(ipfix_aux_config_dtls *) aux_config);
#endif
    return add_collector_remaining_protocols(exporter, collector);
}

static void remove_collector(ipfix_receiving_collector *collector) {
#ifdef SUPPORT_OPENSSL
    /* Shutdown DTLS connection */
    if (collector->protocol == DTLS_OVER_UDP) {
	dtls_shutdown_and_cleanup(&collector->dtls_main);
	dtls_shutdown_and_cleanup(&collector->dtls_replacement);
	free( (void *) collector->peer_fqdn);
    }
    collector->peer_fqdn = NULL;
#endif
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
    if (collector->protocol != RAWDIR) {
#endif
    if ( collector->data_socket != -1) {
	close ( collector->data_socket );
    }
    collector->data_socket = -1;
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
    }
    if (collector->protocol == RAWDIR) {
	free(collector->packet_directory_path);
    }
#endif
    collector->state = C_UNUSED;
}

/*
 * Remove a collector from the exporting process
 * Parameters:
 * Returns: 0 on success, -1 on failure
 */
int ipfix_remove_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port) {
    int i;
    for(i=0;i<exporter->collector_max_num;i++) {
	ipfix_receiving_collector *collector = &exporter->collector_arr[i];
	if( ( strcmp( collector->ipv4address, coll_ip4_addr) == 0 )
		&& collector->port_number == coll_port) {
	    remove_collector(collector);
	    return 0;
	}
    }
    msg(MSG_ERROR, "IPFIX: remove_collector, exporter %s not found", coll_ip4_addr);
    return -1;
}

/************************************************************************************/
/* Template management                                                              */
/************************************************************************************/



/*
 * Helper function: Finds a template in the exporter
 * Parmeters:
 * exporter: Exporter to search for the template
 * template_id: ID of the template we search
 * cleanness: search for T_UNUSED templates or for existing by ID
 * Returns: the index of the template in the exporter or -1 on failure.
 */

static int ipfix_find_template(ipfix_exporter *exporter, uint16_t template_id, enum template_state cleanness)
{
	DPRINTFL(MSG_VDEBUG, "IPFIX: ipfix_find_template with ID: %d",template_id);

        int i=0;
        int searching;

        // first, some safety checks:
        if(exporter == NULL) {
                msg(MSG_ERROR, "IPFIX: find_template, NULL exporter given");
                return -1;
        }
        if(exporter->template_arr == NULL) {
                msg(MSG_ERROR, "IPFIX: find_template, template array not initialized, cannot search for %d", template_id);
                return -1;
        }

        // do we already have a template with this ID?
        // -> update it!
        searching = TRUE;
       if (cleanness == T_UNUSED) {
		while(searching && ( i< exporter->ipfix_lo_template_maxsize) ) {
			if( exporter->template_arr[i].state == cleanness) {
					// we found an unused slot; return the index:
					return i;
			}
			i++;
		}
	}else{
		while(searching && ( i< exporter->ipfix_lo_template_maxsize) ) {
				// we are searching for an existing template, compare the template_id:
				if(exporter->template_arr[i].template_id == template_id) {
					DPRINTFL(MSG_VDEBUG, "IPFIX: ipfix_find_template with ID: %d, validity %d found at %d", template_id, exporter->template_arr[i].state, i);
					return i;
					searching = FALSE;
				}
			i++;
		}
	}
        return -1;
}


/*
 * Remove a template from the exporting process but create a withdrawal message first
 * Parameters:
 * exporter: the exporter
 * template_id: ID of the template to remove
 * Returns: 0 on success, -1 on failure
 * This will free the templates data store!
 */
int ipfix_remove_template_set(ipfix_exporter *exporter, uint16_t template_id)
{
        int ret = 0;
// argument T_SENT is ignored in ipfix_find_template
        int found_index = ipfix_find_template(exporter,template_id, T_SENT);
	if (found_index >= 0) {
		if(exporter->template_arr[found_index].state == T_SENT){
			DPRINTFL(MSG_VDEBUG, "IPFIX: ipfix_remove_template_set: creating withdrawal msg for ID: %d, validity %d", template_id, exporter->template_arr[found_index].state);
			char *p_pos;
			char *p_end;
	
			// write the withdrawal message fields into the buffer
			// beginning of the buffer
			p_pos = exporter->template_arr[found_index].template_fields;
			// end of the buffer since the WITHDRAWAL message for one template is always 8 byte
			p_end = p_pos + 8;
	
			// set ID is 2 for a template, 4 for a template with fixed fields:
			// for withdrawal masseges we keep the template set ID
			p_pos +=  2;
			// write 8 to the lenght field
			write_unsigned16 (&p_pos, p_end, 8);
			// keep the template ID:
			p_pos +=  2;
			// write 0 for the field count, since it indicates that this is a withdrawal message
			write_unsigned16 (&p_pos, p_end, 0);
			exporter->template_arr[found_index].fields_length = 8;
			exporter->template_arr[found_index].field_count = 0;
			exporter->template_arr[found_index].state = T_WITHDRAWN;
			DPRINTFL(MSG_VDEBUG, "IPFIX: ipfix_remove_template_set: ... Withdrawn");
       		}
       		if(exporter->template_arr[found_index].state == T_COMMITED) {
			ipfix_deinit_template_set(exporter, &(exporter->template_arr[found_index]) );
		}
        }else{
		msg(MSG_ERROR, "IPFIX: remove_template ID %u not found", template_id);
		return -1;
        }
        return ret;
}

/************************************************************************************/
/* End of Template management                                                       */
/************************************************************************************/



/*
 * Prepends an ipfix message header to the sendbuffer
 *
 * The ipfix message header is set according to:
 * - the exporter ( Source ID and sequence number)
 * - the length of the contained data
 * - the current system time
 * - the ipfix version number
 *
 * Note: the first HEADER_USED_IOVEC_COUNT  iovec struct are reserved for the header! These will be overwritten!
 */
static void ipfix_prepend_header(ipfix_exporter *p_exporter, int data_length, ipfix_sendbuffer *sendbuf)
{

        time_t export_time;
        uint16_t total_length = 0;

        // did the user set the data_length field?
        if (data_length != 0) {
                total_length = data_length + sizeof(ipfix_header);
        } else {
                // compute it on our own:
                // sum up all lengths in the iovecs:
                int i;

                // start the loop with 1, as 0 is reserved for the header!
                for (i = 1; i< sendbuf->current;  i++) {
                        total_length += sendbuf->entries[i].iov_len;
                }

                // add the header lenght to the total length:
                total_length += sizeof(ipfix_header);
        }

        // write the length into the header
        (sendbuf->packet_header).length = htons(total_length);

        // write version number and source ID and sequence number
        (sendbuf->packet_header).version = htons(IPFIX_VERSION_NUMBER);
        (sendbuf->packet_header).source_id = htonl(p_exporter->source_id);
        (sendbuf->packet_header).sequence_number = htonl(p_exporter->sequence_number);

        // get the export time:
        export_time = time(NULL);
        if(export_time == (time_t)-1) {
                // survive
                export_time=0;
                msg(MSG_ERROR,"IPFIX: prepend_header, time() failed, using %d", export_time);
        }
        //  global_last_export_time = (uint32_t) export_time;
        (sendbuf->packet_header).export_time = htonl((uint32_t)export_time);
}



/*
 * Create and initialize an ipfix_sendbuffer for at most maxelements
 * Parameters: ipfix_sendbuffer** sendbuf pointerpointer to an ipfix-sendbuffer
 */
static int ipfix_init_sendbuffer(ipfix_sendbuffer **sendbuf)
{
        ipfix_sendbuffer *tmp;

        // mallocate memory for the sendbuffer
        if(!(tmp=(ipfix_sendbuffer *)malloc(sizeof(ipfix_sendbuffer)))) {
                goto out;
        }

        tmp->current = HEADER_USED_IOVEC_COUNT; // leave the 0th field blank for the header
        tmp->committed = HEADER_USED_IOVEC_COUNT;
        tmp->marker = HEADER_USED_IOVEC_COUNT;
        tmp->committed_data_length = 0;

        // init and link packet header
        memset(&(tmp->packet_header), 0, sizeof(ipfix_header));
        tmp->entries[0].iov_len = sizeof(ipfix_header);
        tmp->entries[0].iov_base = &(tmp->packet_header);

        // initialize an ipfix_set_manager
	(tmp->set_manager).set_counter = 0;
        memset(&(tmp->set_manager).set_header_store, 0, sizeof((tmp->set_manager).set_header_store));
        (tmp->set_manager).data_length = 0;

        *sendbuf=tmp;
        return 0;

//out1:
        free(tmp);
out:
        return -1;
}

/*
 * reset ipfix_sendbuffer
 * Resets the contents of an ipfix_sendbuffer, so the sendbuffer can again
 * be filled with data.
 * (Present headers are also purged).
 */
static int ipfix_reset_sendbuffer(ipfix_sendbuffer *sendbuf)
{
        if(sendbuf == NULL ) {
                DPRINTFL(MSG_VDEBUG, "IPFIX: trying to reset NULL sendbuf");
                return -1;
        }

        sendbuf->current = HEADER_USED_IOVEC_COUNT;
        sendbuf->committed = HEADER_USED_IOVEC_COUNT;
        sendbuf->marker = HEADER_USED_IOVEC_COUNT;
        sendbuf->committed_data_length = 0;

        memset(&(sendbuf->packet_header), 0, sizeof(ipfix_header));

        // also reset the set_manager!
	(sendbuf->set_manager).set_counter = 0;
        memset(&(sendbuf->set_manager).set_header_store, 0, sizeof((sendbuf->set_manager).set_header_store));
        (sendbuf->set_manager).data_length = 0;

        return 0;
}


/*
 * Deinitialize (free) an ipfix_sendbuffer
 */
static int ipfix_deinit_sendbuffer(ipfix_sendbuffer **sendbuf)
{
        // free the sendbuffer itself:
        free(*sendbuf);
        *sendbuf = NULL;

        return 0;
}


/*
 * initialize array of collectors
 * Allocates memory for an array of collectors
 * Parameters:
 * col: collector array to initialize
 * col_capacity: maximum amount of collectors to store in this array
 */
static int ipfix_init_collector_array(ipfix_receiving_collector **col, int col_capacity)
{
        int i;
        ipfix_receiving_collector *tmp;

        tmp=(ipfix_receiving_collector *)malloc((sizeof(ipfix_receiving_collector) * col_capacity));
        if(!tmp) {
                return -1;
        }

        for (i = 0; i< col_capacity; i++) {
		ipfix_receiving_collector *c = &tmp[i];
                c->state = C_UNUSED;
		c->ipv4address[0] = '\0';
		c->port_number = 0;
		c->protocol = 0;
		c->data_socket = -1;
		c->last_reconnect_attempt_time = 0;
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
		c->packet_directory_path = NULL:
		c->packets_written = 0;
#endif
#ifdef SUPPORT_OPENSSL
		c->dtls_main.socket = c->dtls_replacement.socket = -1;
		c->dtls_main.ssl = c->dtls_replacement.ssl = NULL;
		c->dtls_main.want_read = c->dtls_replacement.want_read = 0;
		c->peer_fqdn = NULL;
#endif
#
        }

        *col=tmp;
        return 0;
}


/*
 * deinitialize an array of collectors
 * Parameters:
 * col: collector array to clean up
 */
static int ipfix_deinit_collector_array(ipfix_receiving_collector **col)
{
        free(*col);
        *col=NULL;

        return 0;
}


/*
 * Initializes a send socket
 * Parameters:
 * serv_ip4_addr of the recipient in dot notation
 * serv_port: port
 * protocol: transport protocol
 */
static int ipfix_init_send_socket(struct sockaddr_in serv_addr, enum ipfix_transport_protocol protocol)
{
    int sock = -1;

    switch(protocol) {
	case UDP:
	    sock= init_send_udp_socket( serv_addr );
	    break;

	case TCP:
	    msg(MSG_FATAL, "IPFIX: Transport Protocol TCP not implemented");
	    break;
	case SCTP:
#ifdef SUPPORT_SCTP
	    sock= init_send_sctp_socket( serv_addr );
	    break;
#else
	    msg(MSG_FATAL, "IPFIX: Library compiled without SCTP support.");
	    break;
#endif
	default:
	    msg(MSG_FATAL, "IPFIX: Transport Protocol not supported");
    }

    return sock;
}

/*
 * initialize array of templates
 * Allocates memory for an array of templates
 * Parameters:
 * exporter: exporter, whose template array we'll initialize
 * template_capacity: maximum amount of templates to store in this array
 */
static int ipfix_init_template_array(ipfix_exporter *exporter, int template_capacity)
{
        int i;

        DPRINTFL(MSG_VDEBUG, "IPFIX - ipfix_init_template_array with elem %d", template_capacity);
        // allocate the memory for template_capacity elements:
        exporter->ipfix_lo_template_maxsize  = template_capacity;
        exporter->ipfix_lo_template_current_count = 0 ;
        exporter->template_arr =  (ipfix_lo_template*) malloc (template_capacity * sizeof(ipfix_lo_template) );

        for(i = 0; i< template_capacity; i++) {
                exporter->template_arr[i].state = T_UNUSED;
        }

        return 0;
}


/*
 * deinitialize an array of templates
 * Parameters:
 * exporter: exporter, whose template store will be purged
 */
static int ipfix_deinit_template_array(ipfix_exporter *exporter)
{
        /* FIXME: free all templates in the array!
         This was our memory leak.
         JanP, 2005-21-1
         */
        int i=0;
        int ret = 0;
        
        for(i=0; i< exporter->ipfix_lo_template_maxsize; i++) {
                // if template was sent we need a withdrawal message first
                if (exporter->template_arr[i].state == T_SENT){
                 	ret = ipfix_remove_template_set(exporter, exporter->template_arr[i].template_id );
                }
        }
        // send all created withdrawal messages
        ipfix_send_templates(exporter);
        
	for(i=0; i< exporter->ipfix_lo_template_maxsize; i++) {
                // try to free all templates:
                ret = ipfix_deinit_template_set(exporter, &(exporter->template_arr[i]) );
                // for debugging:
                DPRINTFL(MSG_VDEBUG, "ipfix_deinit_template_array deinitialized template %i with success %i ", i, ret);
                // end debugging
        }
        free(exporter->template_arr);

        exporter->template_arr = NULL;
        exporter->ipfix_lo_template_maxsize = 0;
        exporter->ipfix_lo_template_current_count = 0;

        return 0;
}


/*
 * Updates the template sendbuffers
 * will be called, after a template has been added or removed
 */
static int ipfix_update_template_sendbuffer (ipfix_exporter *exporter)
{
        int ret;
        int i;

        // first, some safety checks:
        if (exporter == NULL) {
                DPRINTFL(MSG_VDEBUG, "IPFIX: trying to update NULL template sendbuffer");
                return -1;
        }
        if (exporter->template_arr == NULL) {
                DPRINTFL(MSG_VDEBUG,  "IPFIX: update_template_sendbuffer, template store not initialized");
                return -1;

        }

        ipfix_sendbuffer* t_sendbuf = exporter->template_sendbuffer;
	ipfix_sendbuffer* sctp_sendbuf = exporter->sctp_template_sendbuffer;

        // clean the template sendbuffers
        ret=ipfix_reset_sendbuffer(t_sendbuf);
	ret=ipfix_reset_sendbuffer(sctp_sendbuf);

        // place all valid templates into the template sendbuffer
        // could be done just like put_data_field:

        for (i = 0; i < exporter->ipfix_lo_template_maxsize; i++ )  {
                switch (exporter->template_arr[i].state) {
                	case (T_TOBEDELETED):
				// free memory and mark T_UNUSED
				ipfix_deinit_template_set(exporter, &(exporter->template_arr[i]) );
				break;
			case (T_COMMITED): // send to SCTP and UDP collectors and mark as T_SENT
				if (sctp_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "IPFIX: SCTP template sendbuffer too small to handle more than %i entries", sctp_sendbuf->current);
					return -1;
                        	}
                        	if (t_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "IPFIX: UDP template sendbuffer too small to handle more than %i entries", t_sendbuf->current);
					return -1;
                        	}
				sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
                        	sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
                        	sctp_sendbuf->current++;
                        	sctp_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				
				t_sendbuf->entries[ t_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
				t_sendbuf->entries[ t_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
				t_sendbuf->current++;
				t_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
                        	
                        	exporter->template_arr[i].state = T_SENT;
                        	break;
                	case (T_SENT): // only to UDP collectors
                		if (t_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "IPFIX: UDP template sendbuffer too small to handle more than %i entries", t_sendbuf->current);
					return -1;
                        	}
                		t_sendbuf->entries[ t_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
				t_sendbuf->entries[ t_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
				t_sendbuf->current++;
				t_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				break;
                	case (T_WITHDRAWN): // put the SCTP withdrawal message and mark T_TOBEDELETED
                		if (sctp_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "IPFIX: SCTP template sendbuffer too small to handle more than %i entries", sctp_sendbuf->current);
					return -1;
				}
				sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
				sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
				sctp_sendbuf->current++;
				sctp_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				
				exporter->template_arr[i].state = T_TOBEDELETED;
				DPRINTFL(MSG_VDEBUG, "IPFIX: ipfix_update_template_sendbuffer: Withdrawal for template ID: %d added to sctp_sendbuffer", exporter->template_arr[i].template_id);
				break;
			default : // Do nothing : T_UNUSED or T_UNCLEAN
				break;
                }
        } // end loop over all templates

        // that's it!
        return 0;
}

#ifdef SUPPORT_SCTP
/*
 * function used by SCTP to reconnect to a collector, if connection
 * was lost. After succesful reconnection resend all active templates.
 * i: index of the collector in the exporters collector_arr
 */
int ipfix_sctp_reconnect(ipfix_exporter *exporter , int i){
	int bytes_sent;
	time_t time_now = time(NULL);
	exporter->collector_arr[i].last_reconnect_attempt_time = time_now;
	// error occured while being connected?
	if(exporter->collector_arr[i].state == C_CONNECTED) {
		// the socket has not yet been closed
		close(exporter->collector_arr[i].data_socket);
		exporter->collector_arr[i].data_socket = -1;
	}
	    
	// create new socket if not yet done
	if(exporter->collector_arr[i].data_socket < 0) {
		exporter->collector_arr[i].data_socket = init_send_sctp_socket( exporter->collector_arr[i].addr );
		if( exporter->collector_arr[i].data_socket < 0) {
		msg(MSG_ERROR, "ipfix_sctp_reconnect(): SCTP socket creation in reconnect failed, %s", strerror(errno));
		exporter->collector_arr[i].state = C_DISCONNECTED;
		return -1;
		}
	}

	// connect (non-blocking)
	// this is the second call of connect (it was already called in init_send_sctp_socket)
	if(connect(exporter->collector_arr[i].data_socket, (struct sockaddr*)&(exporter->collector_arr[i].addr), sizeof(exporter->collector_arr[i].addr)) == -1) {
		switch(errno) {
			case EISCONN:
				// connected
				break;
			case EALREADY: // is returned if connection establishment has not yet finished
				// connection attempt not yet finished
				msg(MSG_DEBUG, "ipfix_sctp_reconnect(): still connecting...");
				exporter->collector_arr[i].state = C_NEW;
				return -1;
			case EINPROGRESS: // is returned only if connect was called for the first time
				// ==> connect() called in init_send_sctp_socket must have failed
				msg(MSG_ERROR, "ipfix_sctp_reconnect(): SCTP connection could not be established");
				close(exporter->collector_arr[i].data_socket);
				exporter->collector_arr[i].data_socket = -1;
				exporter->collector_arr[i].state = C_DISCONNECTED;
				return -1;
			default:
				// error or timeout
				msg(MSG_ERROR, "ipfix_sctp_reconnect(): SCTP (re)connect failed, %s", strerror(errno));
				close(exporter->collector_arr[i].data_socket);
				exporter->collector_arr[i].data_socket = -1;
				exporter->collector_arr[i].state = C_DISCONNECTED;
				return -1;
		}
	}
	
	msg(MSG_INFO, "ipfix_sctp_reconnect(): successfully (re)connected.");

	//reconnected -> resend all active templates
	ipfix_prepend_header(exporter,
		exporter->template_sendbuffer->committed_data_length,
		exporter->template_sendbuffer);

	if((bytes_sent = sctp_sendmsgv(exporter->collector_arr[i].data_socket,
		exporter->template_sendbuffer->entries,
		exporter->template_sendbuffer->current,
		(struct sockaddr*)&(exporter->collector_arr[i].addr),
		sizeof(exporter->collector_arr[i].addr),
		0,0,
		0,//Stream Number
			0,//packet lifetime in ms (0 = reliable, do not change for templates)
		0
			)) == -1) {
			msg(MSG_ERROR, "ipfix_sctp_reconnect(): SCTP sending templates after reconnection failed, %s", strerror(errno));
			close(exporter->collector_arr[i].data_socket);
		exporter->collector_arr[i].data_socket = -1;
			exporter->collector_arr[i].state = C_DISCONNECTED;
			return -1;
	}
	msg(MSG_VDEBUG, "ipfix_sctp_reconnect(): %d template bytes sent to SCTP collector",bytes_sent);

	// we are done
	exporter->collector_arr[i].state = C_CONNECTED;
	return 0;
}
#endif /*SUPPORT_SCTP*/

/*******************************************************************/
/* Transmission                                                    */
/*******************************************************************/
/*
 * If necessary, sends all associated templates
 * Parameters:
 *  exporter sending exporting process
 * Return value: 1 on success, -1 on failure, 0 on no need to send.
 */
static int ipfix_send_templates(ipfix_exporter* exporter)
{
	int i;
	int bytes_sent;
	int expired;
	// determine, if we need to send the template data:
	time_t time_now = time(NULL);

        // has the timer expired? (for UDP)
	// Remember: This is a global timer for all collectors associated with a given exporter
        expired = ( (time_now - exporter->last_template_transmission_time) >  exporter->template_transmission_timer);
                
        // update the sendbuffers
	ipfix_update_template_sendbuffer(exporter);

	// send the sendbuffer to all collectors depending on their protocol
	for (i = 0; i < exporter->collector_max_num; i++) {
		// is the collector a valid target?
		if ((*exporter).collector_arr[i].state) {
			DPRINTFL(MSG_VDEBUG, "Sending template to exporter %s:%d Proto: %d",
				exporter->collector_arr[i].ipv4address,
				exporter->collector_arr[i].port_number,
				exporter->collector_arr[i].protocol
			);
			switch(exporter->collector_arr[i].protocol){
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
			char* packet_directory_path;
#endif
#ifdef SUPPORT_OPENSSL
			case DTLS_OVER_UDP:
				/* ensure that we are connected i.e. DTLS handshake has been finished.
				 * This function does no harm if we are already connected. */
				if (dtls_manage_connection(exporter,&exporter->collector_arr[i])) {
					/* break out of switch statement if dtls_manage_connection failed */
					break;
				}
				/* dtls_manage_connection() might return success even if we're not yet connected.
				 * This might happen if OpenSSL is still waiting for data from the
				 * remote end and therefore returned SSL_ERROR_WANT_READ. */
				if ( exporter->collector_arr[i].state != C_CONNECTED ) {
				    DPRINTF("We are not yet connected so we can't send templates.");
				    break;
				}
				/* Fall through */
#else
				msg(MSG_FATAL, "IPFIX: Library compiled without DTLS support.");
				return -1;
#endif
			case UDP:
				if (expired && (exporter->template_sendbuffer->committed_data_length > 0)){
					//Timer only used for UDP and DTLS over UDP
					exporter->last_template_transmission_time = time_now;
					// update the sendbuffer header, as we must set the export time & sequence number!
					ipfix_prepend_header(exporter,
						exporter->template_sendbuffer->committed_data_length,
						exporter->template_sendbuffer);
#ifdef SUPPORT_OPENSSL
					if (exporter->collector_arr[i].protocol == DTLS_OVER_UDP) {
						dtls_send(exporter,&exporter->collector_arr[i],
							exporter->template_sendbuffer->entries,
							exporter->template_sendbuffer->current);
					} else {
#endif
					if((bytes_sent = writev(exporter->collector_arr[i].data_socket,
						exporter->template_sendbuffer->entries,
						exporter->template_sendbuffer->current
						))  == -1){
						msg(MSG_ERROR, "ipfix_send_templates(): could not send to %s:%d errno: %s  (UDP)... socket not opened at the collector?",exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number, strerror(errno));
					}else{
						msg(MSG_VDEBUG, "ipfix_send_templates(): %d Template Bytes sent to UDP collector %s:%d",
							bytes_sent, exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
					}
#ifdef SUPPORT_OPENSSL
					}
#endif
				}
			break;

			case TCP:
				msg(MSG_FATAL, "IPFIX: Transport Protocol TCP not implemented");
				return -1;
			case SCTP:
#ifdef SUPPORT_SCTP
				switch (exporter->collector_arr[i].state){
				
				case C_NEW:	// try to connect to the new collector once per second
					if (time_now > exporter->collector_arr[i].last_reconnect_attempt_time) {
						ipfix_sctp_reconnect(exporter, i);
					}
					break;
				case C_DISCONNECTED: //reconnect attempt if reconnection time reached
					if(exporter->sctp_reconnect_timer == 0) { // 0 = no more reconnection attempts
						msg(MSG_ERROR, "ipfix_send_templates(): reconnect failed, removing collector %s:%d (SCTP)", exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
						remove_collector(&exporter->collector_arr[i]);
					} else if ((time_now - exporter->collector_arr[i].last_reconnect_attempt_time) >  exporter->sctp_reconnect_timer) {
						ipfix_sctp_reconnect(exporter, i);
					}
					break;
				case C_CONNECTED:
					if (exporter->sctp_template_sendbuffer->committed_data_length > 0) {
						// update the sendbuffer header, as we must set the export time & sequence number!
						ipfix_prepend_header(exporter,
							exporter->sctp_template_sendbuffer->committed_data_length,
							exporter->sctp_template_sendbuffer);
						if((bytes_sent = sctp_sendmsgv(exporter->collector_arr[i].data_socket,
							exporter->sctp_template_sendbuffer->entries,
							exporter->sctp_template_sendbuffer->current,
							(struct sockaddr*)&(exporter->collector_arr[i].addr),
							sizeof(exporter->collector_arr[i].addr),
							0,0,
							0,//Stream Number
							0,//packet lifetime in ms (0 = reliable, do not change for tamplates)
							0
							)) == -1) {
							// send failed
							msg(MSG_ERROR, "ipfix_send_templates(): could not send to %s:%d errno: %s  (SCTP)",exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number, strerror(errno));
							ipfix_sctp_reconnect(exporter, i); //1st reconnect attempt 
							// if result is C_DISCONNECTED and sctp_reconnect_timer == 0, collector will 
							// be removed on the next call of ipfix_send_templates()
						} else {
							// send was successful
							msg(MSG_VDEBUG, "ipfix_send_templates(): %d template bytes sent to SCTP collector %s:%d",
								bytes_sent, exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
						}
					}
					break;	
				default:
				msg(MSG_FATAL, "IPFIX: Unknown collector socket state");
				return -1;
				}
			break;
#else
			msg(MSG_FATAL, "IPFIX: Library compiled without SCTP support.");
			return -1;
#endif

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
			case RAWDIR:
				ipfix_prepend_header(exporter,
					    exporter->template_sendbuffer->committed_data_length,
					    exporter->template_sendbuffer);
				packet_directory_path = exporter->collector_arr[i].packet_directory_path;
				char fnamebuf[1024];
				sprintf(fnamebuf, "%s/%08d", packet_directory_path, exporter->collector_arr[i].packets_written++);
				int f = creat(fnamebuf, S_IRWXU | S_IRWXG);
				if(f<0)
				    msg(MSG_ERROR, "IPFIX: could not open RAWDIR file %s", fnamebuf);
				else if(writev(f, exporter->template_sendbuffer->entries, exporter->template_sendbuffer->current)<0)
				    msg(MSG_ERROR, "IPFIX: could not write to RAWDIR file %s", fnamebuf);
				close(f);
			break;
#endif	
			default:
				msg(MSG_FATAL, "IPFIX: Transport Protocol not supported");
				return -1;	
			}
		}
	} // end exporter loop

	return 1;
}

/*
 Send data to collectors
 Sends all data committed via ipfix_put_data_field to this exporter.
 Parameters:
 exporter sending exporting process

 Return value:
 on success: 0
 on failure: -1
 */
static int ipfix_send_data(ipfix_exporter* exporter)
{
        int i;
	int bytes_sent;
        // send the current data_sendbuffer:
        int data_length=0;
        
#ifdef SUPPORT_SCTP
	//time_t time_now = time(NULL);
#endif
        
        // is there data to send?
        if (exporter->data_sendbuffer->committed_data_length > 0 ) {
		// increment sequence number
		//exporter->sequence_number += exporter->sn_increment;
		//exporter->sn_increment = 0;

                data_length = exporter->data_sendbuffer->committed_data_length;

                // prepend a header to the sendbuffer
                ipfix_prepend_header(exporter, data_length, exporter->data_sendbuffer);

                // send the sendbuffer to all collectors
                for (i = 0; i < exporter->collector_max_num; i++) {
                        // is the collector a valid target?
                        if(exporter->collector_arr[i].state != C_UNUSED) {
#ifdef DEBUG
                                DPRINTFL(MSG_VDEBUG, "IPFIX: Sending to exporter %s", exporter->collector_arr[i].ipv4address);

                                // debugging output of data buffer:
                                DPRINTFL(MSG_VDEBUG, "Sendbuffer contains %u bytes",  exporter->data_sendbuffer->committed_data_length );
                                DPRINTFL(MSG_VDEBUG, "Sendbuffer contains %u fields",  exporter->data_sendbuffer->committed );
                                int tested_length = 0;
                                int j;
                                /*int k;*/
                                for (j =0; j <  exporter->data_sendbuffer->committed; j++) {
                                        if(exporter->data_sendbuffer->entries[j].iov_len > 0 ) {
                                                tested_length += exporter->data_sendbuffer->entries[j].iov_len;
                                                DPRINTFL(MSG_VDEBUG, "Data Buffer [%i] has %u bytes", j, exporter->data_sendbuffer->entries[j].iov_len);

                                                /*for (k=0; k < exporter->data_sendbuffer->entries[j].iov_len; k++) {
                                                        DPRINTFL (MSG_VDEBUG, "Data at  buf_vector[%i] pos %i is 0x%hx", j,k,   *(  (char*) ( (*(*exporter).data_sendbuffer).entries[j].iov_base+k) ) );
                                                }*/
                                        }
                                }
                                DPRINTFL(MSG_VDEBUG, "IPFIX: Sendbuffer really contains %u bytes!", tested_length );
#endif
				switch(exporter->collector_arr[i].protocol){
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
				char* packet_directory_path;
#endif
				case UDP:
					if((bytes_sent=writev( exporter->collector_arr[i].data_socket,
						exporter->data_sendbuffer->entries,
						exporter->data_sendbuffer->committed
							     )) == -1){
						msg(MSG_VDEBUG, "ipfix_send_data(): could not send to %s:%d errno: %s  (UDP)... socket not opened at the collector?",exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number, strerror(errno));
					}else{

						msg(MSG_VDEBUG, "ipfix_send_data(): %d data bytes sent to UDP collector %s:%d",
								bytes_sent, exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
					}
					break;

				case TCP:
					msg(MSG_FATAL, "IPFIX: Transport Protocol TCP not implemented");
					return -1;
				case SCTP:
#ifdef SUPPORT_SCTP			
					if(exporter->collector_arr[i].state == C_CONNECTED){
						if((bytes_sent = sctp_sendmsgv(exporter->collector_arr[i].data_socket,
							exporter->data_sendbuffer->entries,
							exporter->data_sendbuffer->committed,
							(struct sockaddr*)&(exporter->collector_arr[i].addr),
							sizeof(exporter->collector_arr[i].addr),
							0,0,
							0,//Stream Number
							exporter->sctp_lifetime,//packet lifetime in ms(0 = reliable )
							0
							)) == -1) {
							// send failed
							msg(MSG_ERROR, "ipfix_send_data() could not send to %s:%d errno: %s  (SCTP)",exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number, strerror(errno));
							// drop data and call ipfix_sctp_reconnect
							ipfix_sctp_reconnect(exporter, i);
							// if result is C_DISCONNECTED and sctp_reconnect_timer == 0, collector will 
							// be removed on the next call of ipfix_send_templates()
								}
						msg(MSG_VDEBUG, "ipfix_send_data(): %d data bytes sent to SCTP collector %s:%d",
							bytes_sent, exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
					}
					break;
#else
					msg(MSG_FATAL, "IPFIX: Library compiled without SCTP support.");
					return -1;
#endif
#ifdef SUPPORT_OPENSSL
				case DTLS_OVER_UDP:
					if((bytes_sent=dtls_send( exporter, &exporter->collector_arr[i],
						exporter->data_sendbuffer->entries,
						exporter->data_sendbuffer->committed
							     )) == -1){
						msg(MSG_VDEBUG, "ipfix_send_data(): could not send to %s:%d (DTLS over UDP)",exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
					}else{

						msg(MSG_VDEBUG, "ipfix_send_data(): %d data bytes sent to DTLS over UDP collector %s:%d",
								bytes_sent, exporter->collector_arr[i].ipv4address, exporter->collector_arr[i].port_number);
					}
					break;
#else
					msg(MSG_FATAL, "IPFIX: Library compiled without DTLS support.");
					return -1;
#endif

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
				case RAWDIR:
					ipfix_prepend_header(exporter,
						    exporter->template_sendbuffer->committed_data_length,
						    exporter->template_sendbuffer);
					packet_directory_path = exporter->collector_arr[i].packet_directory_path;
					char fnamebuf[1024];
					sprintf(fnamebuf, "%s/%08d", packet_directory_path, exporter->collector_arr[i].packets_written++);
					int f = creat(fnamebuf, S_IRWXU | S_IRWXG);
					if(f<0)
					    msg(MSG_ERROR, "IPFIX: could not open RAWDIR file %s", fnamebuf);
					else if(writev(f, exporter->data_sendbuffer->entries, exporter->data_sendbuffer->current)<0)
					    msg(MSG_ERROR, "IPFIX: could not write to RAWDIR file %s", fnamebuf);
					close(f);
					break;
#endif
				default:
					msg(MSG_FATAL, "IPFIX: Transport Protocol not supported");
					return -1;	
                        	}
                        }
                } // end exporter loop
		// increment sequence number
		exporter->sequence_number += exporter->sn_increment;
		exporter->sn_increment = 0;
        }  // end if

        // reset the sendbuffer
        ipfix_reset_sendbuffer(exporter->data_sendbuffer);
        return 0;
}


/*
 Send data to collectors
 Sends all data committed via ipfix_put_data_field to this exporter.
 If necessary, sends all associated templates.
 Increment sequence number(sequence_number) only here.

 Parameters:
 exporter sending exporting process
 Return value: 0 on success, -1 on failure.
 */
int ipfix_send(ipfix_exporter *exporter)
{
        int ret = 0;

        if(ipfix_send_templates(exporter) < 0) {
                msg(MSG_ERROR, "IPFIX: sending templates failed");
                ret = -1;
        }
        if(ipfix_send_data(exporter) < 0) {
                msg(MSG_ERROR, "IPFIX: sending data failed");
                ret = -1;
        }

        return ret;
}

/*******************************************************************/
/* Generation of a data set                                        */
/*******************************************************************/

/*
 * Marks the beginning of a data set
 * Parameters:
 *  exporter: exporting process to send data to
 *   data_length: total length of data put into this set  (network byte order)
 *  template_id: ID of the used template (in network byte order)
 * Note: the set ID MUST match a previously sent template ID! This is the user's responsibility, as the
 * library will not perform any checks.
 */
// parameter data_length will be deprecated soon!!!
// calculate via put datafield.
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t template_id)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	
	// security check
	if(exporter->data_sendbuffer->current != exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: start_data_set called twice.");
                goto out;
        }
    
        // check, if there is enough space in the data set buffer
        // the -1 is because, we expect, we want to add at least one data field.
        if(exporter->data_sendbuffer->current >= IPFIX_MAX_SENDBUFSIZE-1 ) {
                msg(MSG_ERROR, "IPFIX: start_data_set sendbuffer too small to handle more than %i entries",
                    exporter->data_sendbuffer->current
                   );
                goto out;
        }

	// check if we do have space for another set header
        if((current + 1) >= IPFIX_MAX_SETS_PER_PACKET ) {
                msg(MSG_ERROR, "IPFIX: start_data_set set_header_store too small to handle more than %i entries",
                    current + 1
                   );
                goto out;
        }

        // write the set id (=template id) to the data set buffer (length will be added by ipfix_end_data_set):
	manager->set_header_store[current].set_id = template_id;

        // link current set header in entries
        exporter->data_sendbuffer->entries[exporter->data_sendbuffer->current].iov_base = &(manager->set_header_store[current]);
        exporter->data_sendbuffer->entries[exporter->data_sendbuffer->current].iov_len = sizeof(ipfix_set_header);

        exporter->data_sendbuffer->current++;
	exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;

        // initialize the counting of the record's data:
        manager->data_length = 0;

        return 0;

out:
        return -1;
}


/*
 * Marks the end of a data set
 * Parameters:
 *   exporter: exporting process to send data to
 *   number_of_records: number of data records in this set (used to calculate the sequence number)
 */
int ipfix_end_data_set(ipfix_exporter *exporter, uint16_t number_of_records)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	uint16_t record_length;

	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: ipfix_end_data_set called but there is no started set to end.");
                return -1;
        }

	// add number of data records to sequence number increment
	exporter->sn_increment += number_of_records;

        // calculate and store the total length of the set:
        record_length = manager->data_length + sizeof(ipfix_set_header);
	manager->set_header_store[current].length = htons(record_length);

        // update the sendbuffer
        exporter->data_sendbuffer->committed_data_length += record_length;

	// now as we are finished with this set, increase set_counter
	manager->set_counter++;
	
	// update committed 
	exporter->data_sendbuffer->committed = exporter->data_sendbuffer->current;
	exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;
	    
        return 0;
}


/*
 * Cancel a previously started data set
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_cancel_data_set(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	int i;

	// security check
	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: cancel_data_set called but there is no set to cancel.");
                goto out;
        }
    
        // clean set id and length:
	manager->set_header_store[current].set_id = 0;
        manager->data_length = 0;

        // clean up entries
	for(i=exporter->data_sendbuffer->committed; i<exporter->data_sendbuffer->current; i++) {
	    exporter->data_sendbuffer->entries[i].iov_base = NULL;
	    exporter->data_sendbuffer->entries[i].iov_len = 0;
	}

        exporter->data_sendbuffer->current = exporter->data_sendbuffer->committed;
        exporter->data_sendbuffer->marker = exporter->data_sendbuffer->committed;

        return 0;

out:
        return -1;
}

/*
 * Sets the data field marker to the current position in order to allow deletion of newly added fields
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_set_data_field_marker(ipfix_exporter *exporter)
{
    exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;
    return 0;
}

/*
 * Delete recently added fields up to the marker
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_delete_data_fields_upto_marker(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	int i;

	// security check
	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: delete_data_fields_upto_marker called but there is no set.");
                goto out;
        }

	// if marker is before current, clean the entries and set current back 
	if(exporter->data_sendbuffer->marker < exporter->data_sendbuffer->current) {
	    for(i=exporter->data_sendbuffer->marker; i<exporter->data_sendbuffer->current; i++) {
		// alse decrease data_length
		manager->data_length -= exporter->data_sendbuffer->entries[i].iov_len;
		exporter->data_sendbuffer->entries[i].iov_base = NULL;
		exporter->data_sendbuffer->entries[i].iov_len = 0;
	    }
	    exporter->data_sendbuffer->current = exporter->data_sendbuffer->marker;
	}

        return 0;

out:
        return -1;
}

/*******************************************************************/
/* Generation of a data template set and option template set       */
/*******************************************************************/

/*
 * Marks the beginning of a data template set
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 *  field_count: number of template fields in this template (in host byte order)
 */
// length not ommited; need it to allocate buffer for template
// template length changes depending on vendor specific stuff.

// gerhard: bei Template alles in Host-Byte-Order. Hier koeen wir auf IOVecs verzichten und die
// Felder direkt hintereinander in den Buffer schreiben. Dabei wandeln wir jeweils in Network-Byte-Order
// um.


/*
 * Will allocate memory and stuff for a new template
 * End_data_template set will add this template to the exporter
 */
int ipfix_start_datatemplate_set (ipfix_exporter *exporter, uint16_t template_id, uint16_t preceding, uint16_t field_count, uint16_t fixedfield_count)
{
        // are we updating an existing template?
        int i;
        int searching;
        int found_index = -1;
	int datatemplate=(fixedfield_count || preceding) ? 1 : 0;

	/* Make sure that template_id is > 255 */
	if ( ! template_id > 255 ) {
	    msg(MSG_ERROR, "IPFIX: start_datatemplate_set: Template id has to be > 255. Start of template cancelled.");
	    return -1;
	}
        found_index = ipfix_find_template(exporter, template_id, T_SENT);

        // have we found a template?
        if(found_index >= 0) {
                // we must overwrite the old template.
                // first, clean up the old template:
		switch (exporter->template_arr[found_index].state){
			case T_SENT:
				// create a withdrawal message first
				ipfix_remove_template_set(exporter, exporter->template_arr[found_index].template_id);
			case T_WITHDRAWN:
				// send withdrawal messages
				ipfix_send_templates(exporter);
			case T_COMMITED:
			case T_UNCLEAN:
			case T_TOBEDELETED:
				// nothing to do, template can be deleted
				ipfix_deinit_template_set(exporter, &(exporter->template_arr[found_index]));
				break;
			default:
				DPRINTFL(MSG_VDEBUG, "template valid flag is T_UNUSED or wrong\n");	
				break;
		}
        } else {
                /* allocate a new, free slot */

                searching = TRUE;

                // check if there is a free slot at all:
                if(exporter->ipfix_lo_template_current_count >= exporter->ipfix_lo_template_maxsize ) {
                        msg(MSG_ERROR, "IPFIX: start_template_set has no more free slots for new templates available, not added");
                        // do error handling:
                        found_index = -1;
                        searching = FALSE;
                        return -1;
                }

                i = 0;

                // search for a free slot:
                while(searching && i < exporter->ipfix_lo_template_maxsize) {

                        if(exporter->template_arr[i].state == T_UNUSED ) {
                                // we have found a free slot:

                                // increase total number of templates.
                                exporter->ipfix_lo_template_current_count ++;
                                searching = FALSE;
                                found_index = i;
                                exporter->template_arr[i].template_fields = NULL;
                                // TODO: maybe check, if this field is not null. Might only happen, when
                                // asynchronous threads change the template fields.
                        }
                        i++;
                }
        }

        /*
         initialize the slot
         test for a valid slot
         */
        if( (found_index >= 0 ) && ( found_index < exporter->ipfix_lo_template_maxsize ) ) {
                //  int ret;
                char *p_pos;
                char *p_end;

                // allocate memory for the template's fields:
                // maximum length of the data: 8 bytes for each field, as one field contains:
                // field type, field length (2*2bytes)
                // and may contain an Enterprise Number (4 bytes)
                // also, reserve 8 bytes space for the header!

                exporter->template_arr[found_index].max_fields_length = 8 * (field_count + fixedfield_count) + (datatemplate ? 12 : 8);
		exporter->template_arr[found_index].fields_length = (datatemplate ? 12 : 8);

                exporter->template_arr[found_index].template_fields = (char*)malloc(exporter->template_arr[found_index].max_fields_length );

                // initialize the rest:
                exporter->template_arr[found_index].state = T_UNCLEAN;
                exporter->template_arr[found_index].template_id = template_id;
                exporter->template_arr[found_index].field_count = field_count;

                // also, write the template header fields into the buffer (except the lenght field);

                // beginning of the buffer
                p_pos = exporter->template_arr[found_index].template_fields;
                // end of the buffer
                p_end = p_pos + exporter->template_arr[found_index].max_fields_length;
                // add offset to the buffer's beginning: this is, where we will write to.
                //  p_pos +=  (*exporter).template_arr[found_index].fields_length;

		// set ID is 2 for a template set, 4 for a template with fixed fields:
		// see RFC 5101: 3.3.2 Set Header Format
		write_unsigned16 (&p_pos, p_end, datatemplate ? 4 : 2);
                // write 0 to the lenght field; this will be overwritten with end_template
                write_unsigned16 (&p_pos, p_end, 0);
                // write the template ID: (has to be > 255)
                write_unsigned16 (&p_pos, p_end, template_id); 
                // write the field count:
                write_unsigned16 (&p_pos, p_end, field_count);

                if (datatemplate) {
                        // write the fixedfield count:
                        write_unsigned16 (&p_pos, p_end, fixedfield_count);
                        // write the preceding:
                        write_unsigned16 (&p_pos, p_end, preceding);
                }

                // does this work?
                // (*exporter).template_arr[found_index].fields_length += 8;
        } else return -1;

        return 0;
}
/*
 * Marks the beginning of an option template set
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 *  scope_length: the option scope length (in host byte oder)
 *  option_length: the option scope length (in host byte oder)
 */
int ipfix_start_optionstemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length)
{
        msg(MSG_FATAL, "IPFIX: start_optionstemplate_set() not implemented");
        return -1;
}

/*
 * Append field to the exporter's current template set
 * Parameters:
 *  template_id: the id specified at ipfix_start_template_set()
 *  type: field or scope type (in host byte order)
 *        Note: The enterprise id will only be used, if type has the enterprise bit set.
 *  length: length of the field or scope (in host byte order)
 *  enterprise: enterprise type (in host byte order)
 * Note: This function is called after ipfix_start_data_template_set or ipfix_start_option_template_set.
 * Note: This function MAY be replaced by a macro in future versions.
 */
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id)
{
        int found_index;
        /* set pointers to the buffer */
        int ret;
        char *p_pos;
        char *p_end;
	int enterprise_bit_set = ipfix_enterprise_flag_set(type);

        found_index=ipfix_find_template(exporter, template_id,  T_UNCLEAN);

        /* test for a valid slot */
        if( found_index < 0 || found_index >= exporter->ipfix_lo_template_maxsize ) {
                msg(MSG_VDEBUG, "IPFIX: put_template_field,  template ID %d not found", template_id);
                return -1;
        }

        /* beginning of the buffer */
        p_pos = exporter->template_arr[found_index].template_fields;
        // end of the buffer
        p_end = p_pos + exporter->template_arr[found_index].max_fields_length;

        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_field: template found at %d", found_index);
        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_field: A p_pos %p, p_end %p", p_pos, p_end);
        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_field: max_fields_len %d", exporter->template_arr[found_index].max_fields_length);
        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_field: fieldss_len %d", exporter->template_arr[found_index].fields_length);

        // add offset to the buffer's beginning: this is, where we will write to.
        p_pos += exporter->template_arr[found_index].fields_length;

        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_field: B p_pos %p, p_end %p", p_pos, p_end);

        if(enterprise_bit_set) {
                DPRINTFL(MSG_VDEBUG, "Notice: using enterprise ID %d with data %d", template_id, enterprise_id);
        }

        // now write the field to the buffer:
        ret = write_extension_and_fieldID(&p_pos, p_end, type);
        // write the field length
        ret = write_unsigned16(&p_pos, p_end, length);

        // add the 4 bytes to the written length:
        exporter->template_arr[found_index].fields_length += 4;

        // write the vendor specific id
        if (enterprise_bit_set) {
                ret = write_unsigned32(&p_pos, p_end, enterprise_id);
                exporter->template_arr[found_index].fields_length += 4;
        }

        return 0;
}


/*
   ipfix_start_template_set
   Starts a new template, see ipfix_start_datatemplate_set
*/
int ipfix_start_template_set (ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count) {
        return ipfix_start_datatemplate_set(exporter, template_id, 0, field_count, 0);
}


/*
   ipfix_put_template_fixedfield
   Append fixed-value data type field to the exporter's current data template set, see ipfix_put_template_field
*/
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id) {
        return ipfix_put_template_field(exporter, template_id, type, length, enterprise_id);
}


/*
   ipfix_put_template_data
   Append fixed-value data to the exporter's current data template set
*/
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length) {
        int found_index;
        /* set pointers to the buffer */
        int ret;
        char *p_pos;
        char *p_end;
	
        int i;

        found_index=ipfix_find_template(exporter, template_id,  T_UNCLEAN);

        /* test for a valid slot */
        if ( found_index < 0 || found_index >= exporter->ipfix_lo_template_maxsize ) {
                fprintf (stderr, "Template not found. ");
                return -1;
        }

        ipfix_lo_template *templ=(&(*exporter).template_arr[found_index]);
        templ->max_fields_length += data_length;
        templ->template_fields=(char *)realloc(templ->template_fields, templ->max_fields_length);

        /* beginning of the buffer */
        p_pos =  (*exporter).template_arr[found_index].template_fields;
        // end of the buffer
        p_end = p_pos +  (*exporter).template_arr[found_index].max_fields_length;

        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_data: template found at %i", found_index);
        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_data: A p_pos %p, p_end %p", p_pos, p_end);
        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_data: max_fields_len %u ", (*exporter).template_arr[found_index].max_fields_length);
        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_data: fieldss_len %u ", (*exporter).template_arr[found_index].fields_length);

        // add offset to the buffer's beginning: this is, where we will write to.
        p_pos += (*exporter).template_arr[found_index].fields_length;

        DPRINTFL(MSG_VDEBUG, "ipfix_put_template_data: B p_pos %p, p_end %p", p_pos, p_end);

	for(i = 0; i < data_length; i++) {
		ret = write_octet(&p_pos, p_end, *(((uint8_t*)data)+i) );
	}

        // add to the written length:
        (*exporter).template_arr[found_index].fields_length += data_length;

        return 0;
}

/*
 * Marks the end of a template set
 * Parameters:
 *   exporter: exporting process to send the template to
 * Note: the generated template will be stored within the exporter
 */
int ipfix_end_template_set(ipfix_exporter *exporter, uint16_t template_id)
{
        int found_index;
        char *p_pos;
        char *p_end;

        found_index=ipfix_find_template(exporter, template_id, T_UNCLEAN);

        // test for a valid slot:
        if ( (found_index < 0 ) || ( found_index >= exporter->ipfix_lo_template_maxsize ) ) {
                msg(MSG_ERROR, "IPFIX: template %d not found", template_id);
                return -1;
        }

        // reallocate the memory , i.e. free superfluous memory, as we allocated enough memory to hold
        // all possible vendor specific IDs.
        ipfix_lo_template *templ=(&exporter->template_arr[found_index]);
	/* FIXME: Remove instance of strong profanity from comment below or
	 * apply the Parental Advisory sticker to source code */
        /* sometime I'll fuck C++ with a serious chainsaw - casting malloc() et al is DUMB */
        templ->template_fields=(char *)realloc(templ->template_fields, templ->fields_length);
	// FIXME: Shouldn't max_fields_length be reset at this point?

        /*
         write the real length field:
         set pointers:
         beginning of the buffer
         */
        p_pos = exporter->template_arr[found_index].template_fields;
        // end of the buffer
        p_end = p_pos + exporter->template_arr[found_index].max_fields_length; // FIXME: max_fields_length is obsolete due to the realloc above
        // add offset of 2 bytes to the buffer's beginning: this is, where we will write to.
        p_pos += 2;

        // write the lenght field
        write_unsigned16 (&p_pos, p_end, templ->fields_length);
        // call the template valid
        templ->state = T_COMMITED;

        return 0;
}

/*
 * removes a template set from the exporter
 * Checks, if the template is in use, before trying to free it.
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template* : pointer to the template to be freed
 * Returns: 0  on success, -1 on failure
 * This is an internal function.
 */
int ipfix_deinit_template_set(ipfix_exporter *exporter, ipfix_lo_template *templ)
{
        // note: ipfix_deinit_template_array tries to free all possible templates, many of them
        // won't be initialized. So you'll get a lot of warning messages, which are just fine...

        if(templ == NULL) {
                return -1;
        }

        // FIXME: make sure, we get a mutex lock on the templates, or else an other process
        // might try to write to an unclean template!!!

        // first test, if we can free this template
        if (templ->state == T_UNUSED) {
                return -1;
        } else {
        	DPRINTFL(MSG_VDEBUG, "IPFIX: ipfix_deinit_template_set: deleting Template ID: %d validity: %d", templ->template_id, templ->state);
		templ->state = T_UNUSED;
		free(templ->template_fields);
                exporter->ipfix_lo_template_current_count--;
	}

        return 0;
}
// Set up time after that Templates are going to be resent
int ipfix_set_template_transmission_timer(ipfix_exporter *exporter, uint32_t timer){
	
    exporter->template_transmission_timer = timer;
    return 0;
}

// Set up SCTP packet lifetime
int ipfix_set_sctp_lifetime(ipfix_exporter *exporter, uint32_t lifetime) {
    exporter->sctp_lifetime = lifetime;
    return 0;
}
// Set up SCTP reconnect timer, time after that a reconnection attempt is made, 
// if connection to the collector was lost.
int ipfix_set_sctp_reconnect_timer(ipfix_exporter *exporter, uint32_t timer) {
    exporter->sctp_reconnect_timer = timer;
    return 0;
}

// Set the the names of the files in which the X.509 certificate and the
// matching private key can be found. If private_key_file is NULL the
// certificate chain file will be searched for the private key.
// See OpenSSL man pages for more details
int ipfix_set_dtls_certificate(ipfix_exporter *exporter,
	const char *certificate_chain_file, const char *private_key_file) {
#ifdef SUPPORT_OPENSSL
    if (exporter->ssl_ctx) {
	msg(MSG_ERROR, "Too late to set certificate. SSL context already created.");
	return -1;
    }
    if (exporter->certificate_chain_file) {
	msg(MSG_ERROR, "Certificate can not be reset.");
	return -1;
    }
    if ( ! certificate_chain_file) {
	msg(MSG_ERROR, "ipfix_set_dtls_certificate called with bad parameters.");
	return -1;
    }
    exporter->certificate_chain_file = strdup(certificate_chain_file);
    if (private_key_file) {
	exporter->private_key_file = strdup(private_key_file);
    }
    return 0;
#else /* SUPPORT_OPENSSL */
    msg(MSG_FATAL, "IPFIX: Library compiled without OPENSSL support.");
    return -1;
#endif /* SUPPORT_OPENSSL */
}

// Set the locations of the CA certificates. See OpenSSL man pages for more details.
int ipfix_set_ca_locations(ipfix_exporter *exporter, const char *ca_file, const char *ca_path) {
#ifdef SUPPORT_OPENSSL
    if (exporter->ssl_ctx) {
	msg(MSG_ERROR, "Too late to set CA locations. SSL context already created.");
	return -1;
    }
    if (exporter->ca_file || exporter->ca_path) {
	msg(MSG_ERROR, "CA locations can not be reset.");
	return -1;
    }
    if (ca_file) exporter->ca_file = strdup(ca_file);
    if (ca_path) exporter->ca_path = strdup(ca_path);
    return 0;
#else /* SUPPORT_OPENSSL */
    msg(MSG_FATAL, "IPFIX: Library compiled without OPENSSL support.");
    return -1;
#endif
}

/* check if the enterprise bit in an ID is set */
int ipfix_enterprise_flag_set(uint16_t id)
{
        return bit_set(id, IPFIX_ENTERPRISE_FLAG);
}


#ifdef __cplusplus
}
#endif