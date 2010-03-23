/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */

#ifndef IPFIXLOLIB_H
#define IPFIXLOLIB_H
/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX

 Changes by Gerhard Münz, 2006-02-01
   Changed and debugged sendbuffer structure and Co
   Added new function for canceling data sets and deleting fields

 Changes by Christoph Sommer, 2005-04-14
   Modified ipfixlolib-nothread Rev. 80
   Added support for DataTemplates (SetID 4)

 Changes by Ronny T. Lampert, 2005-01

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef SUPPORT_SCTP 
#include <netinet/sctp.h>
#endif
#ifdef SUPPORT_DTLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include "common/OpenSSL.h"
#endif

#if defined(SUPPORT_OPENSSL) && defined(SUPPORT_SCTP)
#define SUPPORT_DTLS_OVER_SCTP
#endif

#include "encoding.h"
#include "ipfix_names.h"
#include "common/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * version number of the ipfix-protocol
 */
#define IPFIX_VERSION_NUMBER 0x000a

/*
 * Amount of iovec, the header consumes.
 * Change only, if you change the internal code!
 */
#define HEADER_USED_IOVEC_COUNT 1

/*
 * maximum number of collectors at a time.
 * can be specified by user
 */
#define IPFIX_MAX_COLLECTORS 16

/*
 * maximum number of templates at a time;
 * can be specified by user
 */
#define IPFIX_MAX_TEMPLATES 16

/*
 * Default time, until templates are re-sent again:
 * 30 seconds:
 * can be specified by user
 */
#define IPFIX_DEFAULT_TEMPLATE_TIMER 20
/* 
 * Default time, until a new SCTP retransmission attempt 
 * takes place
 * 5 minutes = 400 seconds
 * can be specified by user
*/
#define IPFIX_DEFAULT_SCTP_RECONNECT_TIMER 300
/*
 * Default reliability for sending IPFIX-data-records
 * 0 = reliable
 * can be specified by user
 */
#define IPFIX_DEFAULT_SCTP_DATA_LIFETIME 0

#define TRUE 1
#define FALSE 0

/*
 * maximum number of sets per IPFIX packet
 * TODO: This value is delibaretely chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SETS_PER_PACKET 4

/*
 * maximum size of a sendbuffer
 * TODO: This value is delibaretely chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SENDBUFSIZE (32 * 1024)

/*
 * maximum size of an IPFIX packet
 */
#define IPFIX_MAX_PACKETSIZE (1<<16)

/* MTU considerations apply to UDP and DTLS over UDP only. */

/* The MTU is set by the user. Path MTU discovery is turned off. */
#define IPFIX_MTU_FIXED 0
/* Path MTU discovery is turned on. */
#define IPFIX_MTU_DISCOVER 1

/*
 * Stevens: The maximum size of an IPv4 datagram is 65535 bytes, including
 * the IPv4 header. This is because of the 16-bit total length field.
 */
#define IPFIX_MTU_MAX UINT16_MAX
/* Use a very conservative default MTU so that it even works with IPSec over PPPoE */
#define IPFIX_MTU_CONSERVATIVE_DEFAULT 1400

#ifdef IP_MTU_DISCOVER
/* basically Linux */
#define IPFIX_MTU_DEFAULT IPFIX_MTU_MAX
#define IPFIX_MTU_MODE_DEFAULT IPFIX_MTU_DISCOVER
#else
/* non-Linux, mostly FreeBSD */
#define IPFIX_MTU_DEFAULT IPFIX_MTU_CONSERVATIVE_DEFAULT
#define IPFIX_MTU_MODE_DEFAULT IPFIX_MTU_FIXED
#endif


/* Struct containing an ipfix-header */
/*     Header Format (see RFC 5101)

3.1. Message Header Format


   The format of the IPFIX Message Header is shown in Figure F.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Version Number          |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Export Time                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Sequence Number                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Observation Domain ID                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Figure F: IPFIX Message Header Format

   Message Header Field Descriptions:

   Version

      Version of Flow Record format exported in this message.  The value
      of this field is 0x000a for the current version, incrementing by
      one the version used in the NetFlow services export version 9
      [RFC3954].

   Length

      Total length of the IPFIX Message, measured in octets, including
      Message Header and Set(s).

   Export Time

      Time, in seconds, since 0000 UTC Jan 1, 1970, at which the IPFIX
      Message Header leaves the Exporter.

   Sequence Number

      Incremental sequence counter modulo 2^32 of all IPFIX Data Records
      sent on this PR-SCTP stream from the current Observation Domain by
      the Exporting Process.  Check the specific meaning of this field
      in the subsections of Section 10 when UDP or TCP is selected as
      the transport protocol.  This value SHOULD be used by the
      Collecting Process to identify whether any IPFIX Data Records have
      been missed.  Template and Options Template Records do not
      increase the Sequence Number.

   Observation Domain ID

      A 32-bit identifier of the Observation Domain that is locally
      unique to the Exporting Process.  The Exporting Process uses the
      Observation Domain ID to uniquely identify to the Collecting
      Process the Observation Domain that metered the Flows.  It is
      RECOMMENDED that this identifier also be unique per IPFIX Device.
      Collecting Processes SHOULD use the Transport Session and the
      Observation Domain ID field to separate different export streams
      originating from the same Exporting Process.  The Observation
      Domain ID SHOULD be 0 when no specific Observation Domain ID is
      relevant for the entire IPFIX Message, for example, when exporting
      the Exporting Process Statistics, or in case of a hierarchy of
      Collectors when aggregated Data Records are exported.
      */

typedef struct {
	uint16_t version;
	uint16_t length;
	uint32_t export_time;
	uint32_t sequence_number;
	uint32_t observation_domain_id;
} ipfix_header;

/*  Set Header:
    
      0                   1                   2                   3 
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
     |          Set ID               |          Length               | 
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 
*/
/* Note that this ipfix_set_header struct is only used for data sets.
 * The header of template sets is built up in a char array.
 * (See ipfix_start_datatemplate_set)
 */

typedef struct {
    uint16_t set_id;
    uint16_t length;
} ipfix_set_header;

#define IPFIX_OVERHEAD_PER_SET 4


enum ipfix_transport_protocol {
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT 
	RAWDIR, 
#endif 
	SCTP, UDP, TCP, DTLS_OVER_UDP, DTLS_OVER_SCTP
	};

typedef struct {
    uint16_t mtu; /* Maximum transmission unit.
		     If 0, PMTU discovery will be used.
		     Applies to UDP and DTLS only. */
} ipfix_aux_config_udp;

typedef struct {
    const char *peer_fqdn;
} ipfix_aux_config_dtls;

typedef struct {
    ipfix_aux_config_dtls dtls;
    ipfix_aux_config_udp udp;
} ipfix_aux_config_dtls_over_udp;

typedef struct {
    ipfix_aux_config_dtls dtls;
} ipfix_aux_config_dtls_over_sctp;

/*
 * These indicate, if a field is committed (i.e. can be used)
 * unused or unclean (i.e. data is not complete yet)
 * T_SENT (Template was sent) and T_WITHDRAWN (Template destroyed) 
 * are used with SCTP, since Templates are sent only once
 * T_TOBEDELETED templates will be deleted the next time when the buffer is updated
 */
enum template_state {T_UNUSED, T_UNCLEAN, T_COMMITED, T_SENT, T_WITHDRAWN, T_TOBEDELETED};


/*
 * Indicates the state of the collector. After collector is added
 * state changes to C_NEW, successful call of the socket connect() function
 * sets the state to C_CONNECTED. If connection is lost and the socket closed
 * state changes to C_DISCONNECTED and reconnection attempts can take place
*/
/* The lifecycles of connections of type DTLS over UDP
 * and plain UDP are as follows:
 *
 * DTLS over UDP:
 *  - state == C_UNUSED
 *  - Successful calls to socket() and connect()
 *  - state <= C_NEW
 *  - DTLS handshake is taking place
 *  - DTLS handshake succeeded.
 *  - Templates are sent
 *  - state <= C_CONNECTED
 *
 * UDP:
 *  - state == C_UNUSED
 *  - Successful calls to socket() and connect()
 *  - state <= C_NEW
 *  - Templates are sent
 *  - state <= C_CONNECTED
 */
enum collector_state {C_UNUSED, C_NEW, C_DISCONNECTED, C_CONNECTED};


/*
 * Manages a record set
 * stores the record set header
 *
 * Please note that this struct is only used for data sets.
 *
 */
typedef struct{
	/* number of the current set. */
	/* The maximum is IPFIX_MAX_SETS_PER_PACKET.
	 * set_counter also serves as an index into set_header_store. */
	unsigned set_counter;

	/* buffer to store set headers */
	ipfix_set_header set_header_store[IPFIX_MAX_SETS_PER_PACKET];

	/* set length = sum of field length */
	/* This refers to the current data set only */
	unsigned data_length;
} ipfix_set_manager;


/*
 * A struct buffering data of an IPFIX Message
 */
typedef struct {
	struct iovec entries[IPFIX_MAX_SENDBUFSIZE]; /* an array of iovec structs, containing data and length */
	/* usage of entries:
	   - the first HEADER_USED_IOVEC_COUNT=1 entries are reserved for the ipfix header
	   - the remaining entries are used for
	     * set headers
	     * individual fields of the sets/records
	 */
	unsigned current; /* last accessed entry in entries */
	unsigned committed; /* last committed entry in entries, i.e. when end_data_set was called for the last time */
	unsigned marker; /* marker that allows to delete recently added entries */
	unsigned committed_data_length; /* length of the contained data (in bytes)
					 * not including the IPFIX message header. */
	ipfix_header packet_header; /* A misnomer in my (Daniel Mentz's)
				       opinion. Should be message_header
				       since it's the header of an
				       IPFIX Message. */
	ipfix_set_manager set_manager; /* Only relevant when sendbuffer used
					  for data. Not relevant if used for
					  template sets. */
} ipfix_sendbuffer;

#ifdef SUPPORT_DTLS
typedef struct {
	int socket;
	uint16_t mtu;
	SSL *ssl;
	/* int want_read; */
	time_t last_reconnect_attempt_time;
} ipfix_dtls_connection;
#endif

/*
 * A collector receiving messages from this exporter
 */
typedef struct {
	char ipv4address[16];
	int port_number;
	enum ipfix_transport_protocol protocol;
	int data_socket; // socket data and templates are sent to
	/* data_socket is NOT used for DTLS connections */
	struct sockaddr_in addr;
	uint32_t last_reconnect_attempt_time; // applies only to SCTP and DTLS at the moment
	enum collector_state state;
	int mtu_mode; /* Either IPFIX_MTU_FIXED or IPFIX_MTU_DISCOVER */
	uint16_t mtu; /* Maximum transmission unit.
			 Applies to UDP and DTLS over UDP only. */
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	char* packet_directory_path; /**< if protocol==RAWDIR: path to a directory to store packets in. Ignored otherwise. */
	int packets_written; /**< if protcol==RAWDIR: number of packets written to packet_directory_path. Ignored otherwise. */
#endif
#ifdef SUPPORT_DTLS
	/* Time in seconds after which a DTLS connection
	 * will be replaced by a new one. */
	unsigned dtls_max_connection_age;
	unsigned dtls_connect_timeout;
	ipfix_dtls_connection dtls_main;
	ipfix_dtls_connection dtls_replacement;
	time_t connect_time; /* point in time when the connection setup
				succeeded. We need this to calculate the
				age of a connection. If DTLS is used,
				a connection rollover is performed when
				a connection reaches a certain age.*/
	const char *peer_fqdn;
#endif
} ipfix_receiving_collector;

/*
 * A template, mostly in binary form
 */
typedef struct{
	enum template_state state; // indicates, whether this template is valid.
	uint16_t template_id;
	uint16_t field_count;	// the number of fields the user announced
				// when calling ipfix_start_template_set
	uint16_t fields_added;	// make sure the user does not add more
				// fields than he told us to add in the
				// first place.
				// Make sure fields_added <= field_count
	int fields_length;	// This also includes the length of the Set Header
				// It's basically the number of bytes written
				// into template_fields so far.

	int max_fields_length;	// size of the malloced char array
				// template_fields points to.

	char *template_fields;	// This includes the Set Header and the
				// Template Record Header as it goes out on the wire.
				// Note that the type ipfix_set_header is *not* used
				// to build Set Headers for template sets.
} ipfix_lo_template;

/*
 * Each exporting process is associated with a sequence number and a source ID
 * The exporting process keeps track of the sequence number.
 */
typedef struct {
	uint32_t sequence_number; // total number of data records 
	uint32_t sn_increment; // to be added to sequence number before sending data records
	uint32_t observation_domain_id;
	uint16_t max_message_size; /* Maximum size of an IPFIX message.
		       * This is the maximum size that all collectors allow.
		       * If a new collector is added that only allows
		       * smaller IPFIX messages, this value has to be
		       * updated.
		       * Only observed when sending messages
		       * containing data sets. IPFIX messages
		       * containing template sets might get
		       * longer than that. That's a TODO */
	ipfix_sendbuffer *template_sendbuffer;
	ipfix_sendbuffer *sctp_template_sendbuffer;
	ipfix_sendbuffer *data_sendbuffer;
	int collector_max_num; // maximum available collector
	ipfix_receiving_collector *collector_arr; // array of (collector_max_num) collectors

	// we also need some timer / counter to indicate,
	// if we should send the templates too.
	// This applies only to UDP and DTLS over UDP.
	// It contains the return value from time(NULL) which is the time since
	// the  Epoch (00:00:00 UTC, January 1, 1970), measured in seconds.
	uint32_t last_template_transmission_time;
	
	// force template send next time packets are sent (to include new template ids)
	uint32_t force_template_send;
	
	// time, after templates are transmitted again
	uint32_t template_transmission_timer;
	// lifetime of an SCTP data packet
	uint32_t sctp_lifetime;
	// time, after new sctp reconnection will be initiated (default = 5 min)
	// (0 ==> no reconnection -> destroy collector)
	uint32_t sctp_reconnect_timer;
	int ipfix_lo_template_maxsize;
	ipfix_lo_template *template_arr;
#ifdef SUPPORT_DTLS
	SSL_CTX *ssl_ctx;
	const char *certificate_chain_file;
	const char *private_key_file;
	const char *ca_file;
	const char *ca_path;
#endif
} ipfix_exporter;


void ipfix_beat(ipfix_exporter *exporter); /* enables ipfixlolib to continue non-blocking connection setup */
/* generated by genproto */
int ipfix_init_exporter(uint32_t observation_domain_id, ipfix_exporter **exporter);
int ipfix_deinit_exporter(ipfix_exporter *exporter);

int ipfix_add_collector(ipfix_exporter *exporter, const char *coll_ip4_addr, int coll_port, enum ipfix_transport_protocol proto, void *aux_config);
int ipfix_remove_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port);

int ipfix_start_template_set(ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count);
int ipfix_start_optionstemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length);
int ipfix_start_datatemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t preceding, uint16_t field_count, uint16_t fixedfield_count);
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_end_template_set(ipfix_exporter *exporter, uint16_t template_id );
/* gerhard: use ipfix_remove_template_set
int ipfix_remove_template(ipfix_exporter *exporter, uint16_t template_id);
*/
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t template_id);
uint16_t ipfix_get_remaining_space(ipfix_exporter *exporter);
int ipfix_put_data_field(ipfix_exporter *exporter,void *data, unsigned length);
int ipfix_end_data_set(ipfix_exporter *exporter, uint16_t number_of_records);
int ipfix_cancel_data_set(ipfix_exporter *exporter);
int ipfix_set_data_field_marker(ipfix_exporter *exporter);
int ipfix_delete_data_fields_upto_marker(ipfix_exporter *exporter);
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length);
int ipfix_remove_template_set(ipfix_exporter *exporter, uint16_t template_id);
int ipfix_send(ipfix_exporter *exporter);
int ipfix_enterprise_flag_set(uint16_t id);
// Set up time after that Templates are going to be resent
int ipfix_set_template_transmission_timer(ipfix_exporter *exporter, uint32_t timer); 	 
// Sets a packet lifetime for SCTP data packets (lifetime > 0 : unreliable packets) 	 
int ipfix_set_sctp_lifetime(ipfix_exporter *exporter, uint32_t lifetime);
// Set up SCTP reconnect timer, time after that a reconnection attempt is made, 
// if connection to the collector was lost.
int ipfix_set_sctp_reconnect_timer(ipfix_exporter *exporter, uint32_t timer);

int ipfix_set_dtls_certificate(ipfix_exporter *exporter, const char *certificate_chain_file, const char *private_key_file);
int ipfix_set_ca_locations(ipfix_exporter *exporter, const char *ca_file, const char *ca_path);

#ifdef __cplusplus
}
#endif

#endif
