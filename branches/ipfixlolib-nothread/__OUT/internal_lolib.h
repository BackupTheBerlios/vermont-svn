/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX
 Changes by Ronny T. Lampert

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
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
 * amount of iovec the header consumes
 */
#define HEADER_USED_IOVEC_COUNT 1

/*
 * length of a header in bytes
 */
#define IPFIX_HEADER_LENGTH 16

/*
 * maximum number of collectors at a time.
 */
#define IPFIX_MAX_COLLECTORS 16

/*
 * This macro appends data to the sendbuffer. If the sendbuffer is too small,
 * it will print an error message and set errno to -1.
 */

#define ipfix_add_data2sendbuffer(SENDBUF, POINTER, LENGTH) { \
	if ((*SENDBUF).current >= (*SENDBUF).length-2 ) { \
	fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*SENDBUF).current ); \
	errno = -1; \
	} \
	((*SENDBUF).entries[ (*SENDBUF).current ]).iov_base = POINTER; \
	((*SENDBUF).entries[ (*SENDBUF).current ]).iov_len =  LENGTH; \
	(*SENDBUF).current++; \
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
 * Each exporting process is associated with a sequence number and a source ID
 * The exporting process keeps track of the sequence number.
 */
typedef struct {
	uint32_t sequence_number;
	 uint32_t source_id;
	 // TODO: insert list of collectors
	 // insert sendbuffer for data
	 // insert sendbuffer for templates
} ipfix_exporter;

// TODO: define a collector: probably a socket + some ID



/*
 * A struct containing an array lot of io_vec
 * plus the index of the current (= last occupied) and maximum position.
 * Also has a buffer to store a header.
 * Note: The buffer is placed here, so it can be allocated once with the sendbuffer
 */
typedef struct {
	struct iovec* entries; /* an array of iovec structs, containing data and length */
	int current; /* last accessed field */
	int length; /* the length of the sendbuffer */
	char* header_store; /* memory to store the header */

} ipfix_sendbuffer;


/*******************************************************************/
/* Management of an exporter                                       */
/*******************************************************************/

/*
 Initialize an exporter process

 Parameters:
 sourceID The source ID, to which the exporter will be initialized to.
 exporter an ipfix_exporter* to be initialized
 */
int ipfix_init_exporter (uint32_t source_id, ipfix_exporter** exporter);


/*
 cleanup an exporter process
 */
int ipfix_deinit_exporter (ipfix_exporter* exporter);


/*
 Add a collector to the exporting process

 Parameters:
 exporter: The exporting process, to which the collector shall be attached
 coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
 coll_port: port number of the collector
 proto: transport protocol to use

 Returns: 0 on success or -1 on failure
 */
int ipfix_add_collector(ipfix_exporter* exporter,  char* coll_ip4_addr, int coll_port, ipfix_transport_protocol proto);


/*
 Remove a collector from the exporting process

 Parameters:
 coll_id: the ID of the collector to remove

 Returns: 0 on success, -1 on failure
 */
int ipfix_remove_collector(ipfix_exporter* exporter,   char* coll_ip4_addr, int coll_port);


/*******************************************************************/
/* Transmission                                                    */
/*******************************************************************/

/*
 Send data to collectors
 Sends all data commited via ipfix_put_data_field to this exporter.
 If necessary, sends all associated templates

 Parameters:
 exporter sending exporting process

 Return value: 0 on success, -1 on failure.
 */
int ipfix_send(ipfix_exporter* exporter);


/*******************************************************************/
/* Generation of a data set                                        */
/*******************************************************************/

/*
 Marks the beginning of a data set

 Parameters:
 exporter: exporting process to send data to
 data_length: the total length of data put into this exporter  (host byte order)
 set_id: the dataset's set ID (in network byte order)

 Note: the set ID MUST match a previously sent template ID! This is the user's responsibility, as the
 library will not perform any checks.
 */
int ipfix_start_data_set(ipfix_exporter* exporter, uint16_t* data_length, uint16_t* set_id);


/*
 Append data to the exporter's current data set

 Parameters:
 length: the length of the data
 data: data to be appended.

 Note: the data MUST be ready to be sent,
 - i.e. the data fields MUST be appended in the right order,
 - the data MUST be converted to network byte order
 - etc.
 Note: This call MUST be after a call to  ipfix_start_data_set and before a call to  ipfix_end_data_set.
 Note: This function MAY be replaced by a macro in future versions.
 */
void ipfix_put_data_field(uint16_t length, char* data);


/*
 Marks the end of a data set

 Parameters:
 exporter: exporting process to send data to
 */
int ipfix_end_data_set(ipfix_exporter* exporter);


/*
 Generation of a template set
 */

/*
 Marks the beginning of a template set

 Parameters:
 exporter: exporting process to associate the template with
 template_length: the total length of data put into this template  (host byte order)
 template_id: the template's ID (in network byte order)
 field_count: number of template fields in this template (in network byte order)
 */
int ipfix_start_template_set(ipfix_exporter* exporter, uint16_t* template_length, uint16_t* template_id,  uint16_t* field_count);


/*
 Append data to the exporter's current template set

 Parameters:
 length: the length of the data
 data: data to be appended.

 Note: the data MUST be ready to be sent,
 - i.e. the data fields MUST be appended in the right order,
 - the data MUST be converted to network byte order
 - etc.
 Note: This call MUST be after a call to  ipfix_start_template_set and before a call to  ipfix_end_template_set.
 Note: This function MAY be replaced by a macro in future versions.
 */
void ipfix_put_template_field(uint16_t length, char* data);


/*
 Marks the end of a template set

 Parameters:
 exporter: exporting process to send the template to

 Note: the generated template will be stored within the exporter
 */
int ipfix_end_template_set(ipfix_exporter* exporter);

/*
 removes a template set from the exporter

 Parameters:
 exporter: exporting process to associate the template with
 template_id: the template's ID (in network byte order)

 Returns: 0 on success, -1 on failure
 */
int ipfix_remove_template_set(ipfix_exporter* exporter, uint16_t* template_id);


/*
 Generation of an options  template set
 */


/*
 Marks the beginning of an option template set

 Parameters:
 exporter: exporting process to associate the template with
 template_length: the total length of data put into this template  (host byte order)
 template_id: the template's ID (in network byte order)

 Note: beginning with the options scope length field, all fields are user appended
 (see  ipfix_put_options_template_field)
 */
int ipfix_start_options_template_set(ipfix_exporter* exporter, uint16_t* template_length, uint16_t* template_id);


/*
 Append data to the exporter's current options template set

 Parameters:
 length: the length of the data
 data: data to be appended.

 Note: the data MUST be ready to be sent,
 - i.e. the data fields MUST be appended in the right order,
 - the data MUST be converted to network byte order
 - etc.

 Note: This call MUST be after a call to  ipfix_start_options_template_set and before a call to  ipfix_end_options_template_set.
 Note: The library does not distinguish between
 Note: This function MAY be replaced by a macro in future versions.
 */
void ipfix_put_options_template_field(uint16_t length, char* data);


/*
 Marks the end of a template set

 Parameters:
 exporter: exporting process to send the template to

 Note: the generated template will be stored within the exporter
 */
int ipfix_end_options_template_set(ipfix_exporter* exporter);

/*
 removes a template set from the exporter

 Parameters:
 exporter: exporting process to associate the template with
 template_id: the template's ID (in network byte order)

 Returns: 0 on success, -1 on failure
 */
int ipfix_remove_options_template_set(ipfix_exporter* exporter, uint16_t* template_id);

/*
 Generation of an options data record
 */
/*
 * Please use the functions available for the generation of a data record.
 * Note: the library itself does not make any difference between those two types,
 * so I found it unecessary to write these functions. If you think these functions
 * would be useful, mail me.
 */

