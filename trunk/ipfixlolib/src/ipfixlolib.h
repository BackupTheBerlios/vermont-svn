/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX
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

#include "encoding.h"

/*
 * version number of the ipfix-protocol
 */
#define IPFIX_VERSION_NUMBER 0x000a

/*
 * amount of iovec, the header consumes
 * change only, if you change the internal code!
 */
#define HEADER_USED_IOVEC_COUNT 1

/*
 * length of a header in bytes
 */
#define IPFIX_HEADER_LENGTH 16

/*
 * maximum number of collectors at a time.
 * can be specified by user
 */
#define IPFIX_MAX_COLLECTORS 16

/*
 * maximum number of templates at a time
 * can be specified by user
 */
#define IPFIX_MAX_TEMPLATES 16

/*
 * Default time, until templates are re-sent again:
 * 30 seconds:
 * can be specified by user
 */
#define IPFIX_DEFAULT_TEMPLATE_TIMER 30

#define TRUE 1
#define FALSE 0

/*
 * length of a set header, i.e. the set ID and the length field
 * i.e. 4 bytes
 */
#define IPFIX_MAX_SET_HEADER_LENGTH 4

/*
 * maximum size of a sendbuffer
 * TODO: This value is delibaretely chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SENDBUFSIZE 4048

/*
 * This macro appends data to the sendbuffer. If the sendbuffer is too small,
 * it will print an error message and set errno to -1.
 */

/* #define ipfix_add_data2sendbuffer(SENDBUF, POINTER, LENGTH) { \ */
/*   if ((*SENDBUF).current >= (*SENDBUF).length-2 ) { \ */
/*     fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*SENDBUF).current ); \ */
/*     errno = -1; \ */
/*   } \ */
/*   ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_base = POINTER; \ */
/*   ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_len =  LENGTH; \ */
/*   (*SENDBUF).current++; \ */
/* }   */

#define ipfix_put_field2sendbuffer(SENDBUF, POINTER, LENGTH) { \
  if ((*SENDBUF).current >= (*SENDBUF).length-2 ) { \
    fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*SENDBUF).current ); \
    errno = -1; \
  } \
  ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_base = POINTER; \
  ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_len =  LENGTH; \
  (*SENDBUF).current++; \
  (*(*SENDBUF).set_manager).data_length+= LENGTH; \
}


#define ipfix_put_data_field(EXPORTER, POINTER, LENGTH) { \
  if ((*(*EXPORTER).data_sendbuffer).current >= (*(*EXPORTER).data_sendbuffer).length-2 ) { \
    fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*(*EXPORTER).data_sendbuffer).current ); \
    errno = -1; \
  } \
  ((*(*EXPORTER).data_sendbuffer).entries[ (*(*EXPORTER).data_sendbuffer).current ]).iov_base = POINTER; \
  ((*(*EXPORTER).data_sendbuffer).entries[ (*(*EXPORTER).data_sendbuffer).current ]).iov_len =  LENGTH; \
  (*(*EXPORTER).data_sendbuffer).current++; \
  (*(*(*EXPORTER).data_sendbuffer).set_manager).data_length+= LENGTH; \
}

/* Struct containing an ipfix-header */
/*     Header Format (see draft-ietf-ipfix-protocol-03.txt) */

/*     0                   1                   2                   3 */
/*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 */

/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |       Version Number          |            Length             | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |                           Export Time                         | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |                       Sequence Number                         | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |                          Source ID                            | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    Message Header Field Descriptions */

/*    Version */
/*            Version of Flow Record format exported in this message. The */
/*            value of this field is 0x000a for the current version. */

/*    Length */
/*            Total Length is the length of the IPFIX message, measured in */
/*            octets, including message Header and FlowSet(s). */

/*    Export Time */
/*            Time in seconds since 0000 UTC 1970, at which the Export */
/*            Packet leaves the Exporter. */

/*    Sequence Number */
/*            Incremental sequence counter of all IPFIX Messages sent from */
/*            the current Observation Domain by the Exporting Process. */
/*            This value MUST SHOULD be used by the Collecting Process to */
/*            identify whether any IPFIX Messages have been missed. */

/*    Source ID */
/*            A 32-bit value that identifies the Exporter Process */
/*            Observation Domain. Collecting Process SHOULD use the */
/*            combination of the source IP address and the Source ID field */
/*            to separate different export streams originating from the */
/*            same Exporting Process. */

typedef struct {
	uint16_t version;
	uint16_t length;
	uint32_t export_time;
	uint32_t sequence_number;
	uint32_t source_id;
} ipfix_header;


enum ipfix_transport_protocol {UDP, TCP, SCTP};

/*
 * These indicate, if a field is commited (i.e. can be used)
 * unused or unclean (i.e. data is not complete yet)
 *
 */
enum ipfix_validity {UNUSED, UNCLEAN, COMMITED};

/*
 * Manages a record set
 * stores the record set header
 *
 */
typedef struct{
	struct iovec *header_iovec;

	/* buffer to store the header */
	char *set_header_store;

	/* total capacity of the header */
	int set_header_capacity;

	/* used length of the header store */
	int set_header_length;

	int data_length;
        /* do we need this?? */
	/* uint16_t* set_id;
         */
} ipfix_set_manager;


/*
 * A struct containing an array lot of io_vec
 * plus the index of the current (= last occupied) and maximum position.
 * Also has a buffer to store a header.
 * Note: The buffer is placed here, so it can be allocated once with the sendbuffer
 */
typedef struct {
	struct iovec *entries; /* an array of iovec structs, containing data and length */
	int current; /* last accessed field */
	int commited; /* last commited field (i.e. end data set has been called) */
	int length; /* the length of the sendbuffer */
	char *header_store; /* memory to store the header */
	int commited_data_length; /* length of the contained data (in bytes) */
	//  int uncommited_data_length; /* length of data not yet commited */
	ipfix_set_manager *set_manager;
} ipfix_sendbuffer;


/*
 * A collector receiving messages from this exporter
 */
typedef struct {
	int valid; // indicates, wheter this collector is valid. .
	char ipv4address[16];
	int port_number;
	enum ipfix_transport_protocol protocol;
	// warning! To use SCTP, we will need several ports!
	int data_socket; // socket data is sent to
	int template_socket; // socket, templates are sent to
} ipfix_receiving_collector;

/*
 * A template, mostly in binary form
 */
typedef struct{
	enum ipfix_validity valid; // indicates, wheter this template is valid.
	uint16_t template_id;
	uint16_t field_count;
	int fields_length;
	int max_fields_length;
	char *template_fields;
} ipfix_lo_template;

/*
 * Each exporting process is associated with a sequence number and a source ID
 * The exporting process keeps track of the sequence number.
 */
typedef struct {
	uint32_t sequence_number;
	uint32_t source_id;
	ipfix_sendbuffer *template_sendbuffer;
	ipfix_sendbuffer *data_sendbuffer;
	int collector_num; // number of currently listening collectors
	int collector_max_num; // maximum available collector
	ipfix_receiving_collector *collector_arr; // array of collectors

	// we also need some timer / counter to indicate,
	// if we should send the templates too.
	uint32_t last_template_transmition_time;
	// time, after templates are transmitted again
	uint32_t template_transmition_timer;

	int ipfix_lo_template_maxsize;
	int ipfix_lo_template_current_count;
	ipfix_lo_template *template_arr;

} ipfix_exporter;





/*******************************************************************/
/* Management of an exporter                                       */
/*******************************************************************/

/*
 * Initialize an exporter process
 * Parameters:
 * sourceID The source ID, to which the exporter will be initialized to.
 * exporter an ipfix_exporter* to be initialized
 */
int ipfix_init_exporter (uint32_t source_id, ipfix_exporter **exporter);

/*
 * cleanup an exporter process
 */
int ipfix_deinit_exporter(ipfix_exporter *exporter);

// gerhard: exporter als referenz übergeben, um ihn auf NULL setzen zu können!

/*
 * Add a collector to the exporting process
 * Parameters:
 *  exporter: The exporting process, to which the collector shall be attached
 *  coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
 *  coll_port: port number of the collector
 *  proto: transport protocol to use
 * Returns: 0 on success or -1 on failure
 */
int ipfix_add_collector(ipfix_exporter *exporter,  char *coll_ip4_addr, int coll_port, enum ipfix_transport_protocol proto);

/*
 * Remove a collector from the exporting process
 * Parameters:
 *  coll_id: the ID of the collector to remove
 * Returns: 0 on success, -1 on failure
 */
int ipfix_remove_collector(ipfix_exporter *exporter,   char *coll_ip4_addr, int coll_port);


/*******************************************************************/
/* Transmission                                                    */
/*******************************************************************/

/*
 * Send data to collectors
 * Sends all data commited via ipfix_put_data_field to this exporter.
 * If necessary, sends all associated templates
 * Parameters:
 *  exporter sending exporting process
 * Return value: 0 on success, -1 on failure.
 */
int ipfix_send(ipfix_exporter *exporter);

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
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t *template_id);

// gerhard: wofür brauchen wir die Länge in host byte order?
// if we want to calculate the entire packet length based on the data set's length, we will need the length
// here in host byte order.

/*
 * Append data to the exporter's current data set
 * Parameters:
 *  exporter: exporting process to send data to
 *  length: the length of the data
 *  data: data to be appended.
 * Note: the data MUST be ready to be sent,
 *  - i.e. the data fields MUST be appended in the right order,
 *  - the data MUST be converted to network byte order
 *  - etc.
 * Note: This call MUST be after a call to  ipfix_start_data_set and before a call to  ipfix_end_data_set.
 * Note: This function MAY be replaced by a macro in future versions.
 */
//void ipfix_put_data_field(ipfix_exporter* exporter, uint16_t length, char* data);



/*
 * Marks the end of a data set
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_end_data_set(ipfix_exporter *exporter);

/*******************************************************************/
/* Generation of a data and option template set                    */
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

//int ipfix_start_template_set (ipfix_exporter* exporter,
//	ipfix_lo_template** template, uint16_t template_id,  uint16_t field_count);

int ipfix_start_template_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t field_count);

//int ipfix_start_data_template_set(ipfix_exporter* exporter, uint16_t template_id,  uint16_t field_count);

// gerhard: bei Template alles in Host-Byte-Order. Hier koennen wir auf IOVecs verzichten und die
// Felder direkt hintereinander in den Buffer schreiben. Dabei wandeln wir jeweils in Network-Byte-Order
// um.

/*
 * Marks the beginning of an option template set
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 *  scope_length: the option scope length (in host byte oder)
 *  option_length: the option scope length (in host byte oder)
 */
int ipfix_start_options_template_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length);

/*
 * Append field to the exporter's current template set
 * Parameters:
 *  length: length of the field or scope (in host byte order)
 *  type: field or scope type (in host byte order)
 *  enterprise: enterprise type (if zero, the enterprise field is omitted) (in host byte order)
 * Note: This function is called after ipfix_start_data_template_set or ipfix_start_option_template_set.
 * Note: This function MAY be replaced by a macro in future versions.
 */
//void ipfix_put_template_field(ipfix_exporter* exporter, uint16_t length, uint16_t type, uint32_t enterprise);
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
/* int ipfix_put_template_field(ipfix_exporter* exporter, ipfix_lo_template* template, uint16_t length, uint16_t type, uint32_t enterprise); */


/*
 * Marks the end of a template set
 * Parameters:
 *   exporter: exporting process to send the template to
 * Note: the generated template will be stored within the exporter
 */
//int ipfix_end_template_set(ipfix_exporter* exporter);
int ipfix_end_template_set(ipfix_exporter *exporter, uint16_t template_id );

/*
 * removes a template set from the exporter
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 * Returns: 0 on success, -1 on failure
 */
int ipfix_remove_template_set(ipfix_exporter *exporter, uint16_t template_id);


