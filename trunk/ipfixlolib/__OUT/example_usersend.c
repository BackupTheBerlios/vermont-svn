/*
 This file is part of IPFIXLOLIB
 EXAMPLE CODE
 Published under GPL v2
 Ronny T. Lampert

 based upon the original IPFIXLOLIB
 by Jan Petranek, University of Tuebingen
 2004-11-18
 jan@petranek.de
 */
#include <stdio.h>
#include "ipfixlolib.h"

#define MY_SOURCE_ID 70538

/*
 Data we want to collect / transmit.
 NOTE: User-data must be in network byte order for interoperability
 */
typedef struct {

	uint32_t ip_src_addr;
	uint32_t ip_dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint64_t byte_count;
	uint64_t packet_count;

} meter_data;


int main(int argc, char *argv[])
{
	int ret;

	if (argc != 3 ) {
		printf("Usage: %s <server_ip_address> <server_port>\n", argv[0]);
		exit(1);
	}

	/*
	 Initialize an exporter

	 sourceID: the source ID the exported stream gets
	 exporter: an ipfix_exporter * to be initialized
	 */

	ipfix_exporter *my_exporter;
	ret = ipfix_init_exporter (MY_SOURCE_ID, &my_exporter);

        /* ipfix_init_exporter returns 0 on success */
	if (ret != 0) {
		fprintf (stderr, "ipfix_init_exporter failed!\n");
		exit (-1);
	}


	/*
	 Add a collector (the one who gets the data) to the exporting process

	 exporter: a previous init'ed exporter
	 coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
	 coll_port: port number of the collector
	 proto: transport protocol to use, TCP/UDP/SCTP

         You can add up to IPFIX_MAX_COLLECTORS collectors.
	 */

	ret = ipfix_add_collector(my_exporter,  "1.2.3.4", 4711, UDP);

	if (ret != 0) {
		fprintf (stderr, "adding collector %s:%i failed!\n", "1.2.3.4", 4711);
		exit (-1);
	}


	/*
	 You have to define a template set with the ID 12345 before the exporting process starts

	 The template shall contain the following fields:
	 # |  IPFIX name of field      |  IPFIX field ID | length of associated datatype
	 -------------------------------------------------------------------------------
	 1 |  sourceAddressV4          |   8             | 4
	 2 |  destinationAddressV4     |   12            | 4
	 3 |  transportSourcePort      |   7             | 2
	 4 |  transportDestinationPort |   11            | 2
	 5 |  deltaOctetCount          |   1             | 8
	 6 |  deltaPacketCount         |   2             | 8

	 FIXME - ???
	 As none of these fields is vendor specific, the length of the template fields is 6*4 bytes.
         FIXME
	 */

	uint16_t template_id=12345;
	uint16_t field_count=6;

	/*
	 Now start the adding of fields.

	 exporter: the exporter
	 template_id: an ID for this template
	 field_count: # of entries/fields
	 */

	ret=ipfix_start_data_template_set(my_exporter, template_id,  field_count);


	/*
	 Add fields to the exporter.

	 exporter: the exporter
	 length: sizeof() datatype
	 type: the IPFIX field ID for this entry
	 enterprise: FIXME ???
	*/

	ipfix_put_template_field(my_exporter, 4, 8, 0);
	ipfix_put_template_field(my_exporter, 4, 12, 0);
	ipfix_put_template_field(my_exporter, 2, 7, 0);
	ipfix_put_template_field(my_exporter, 2, 11, 0);
	ipfix_put_template_field(my_exporter, 8, 1, 0);
	ipfix_put_template_field(my_exporter, 8, 2, 0);


	/* Finalize the template */
	ret=ipfix_end_template_set(my_exporter);

	if (ret != 0 ) {
		fprintf (stderr, "generation of template failed!\n");
	}


	/*
	 Main exporting loop

	 1) get data
	 2) start a data set
	 3) add the fields
         4) send data set

	 */

	int exporting = 1;
	meter_data my_meter_data;

	/*
	 The total length of one data record matching the template
	 (including record header):
	 */

	uint16_t my_data_record_length = htons(4+(4+4+2+2+8+8));
	uint16_t n_template_id = htons(template_id);

	while (exporting) {


		/*
		 Get data from the metering process
		 NOTE: Already must be in network byte order
                 */
		meter_process_get_data ( &my_meter_data);

		/* Start writing a record matching our template */
		ret=ipfix_start_data_set(my_exporter, &my_data_record_length , &n_template_id);

		/* Don't forget error handling */
		if (ret != 0) {
			fprintf(stderr, "ipfix_start_data_set failed!\n");
                        exit(-1);
		}

		/* write the source address */
		ipfix_put_data_field(4, &(meter_data.ip_src_addr) );

		/* write the destination address */
		ipfix_put_data_field(4, &(meter_data.ip_dst_addr) );

		/* write the source port number */
		ipfix_put_data_field(2, &(meter_data.src_port) );

		/* write the destination port number */
		ipfix_put_data_field(2, &(meter_data.dst_port) );

		/* write the number of exported bytes */
		ipfix_put_data_field(8, &(meter_data.byte_count) );

		/* write the number of exported packets */
		ipfix_put_data_field(8, &(meter_data.packet_count) );

		/* end the data set to make it ready for sending */
		ret=ipfix_end_data_set(my_exporter);

		if (ret != 0) {
			fprintf(stderr, "ipfix_end_data_set failed!\n");
                        exit(-1);
		}

		/*
		 Send the data set

		 The template is sent first if
		 - the template has changed or is new
		 - a time-out for sending the template has occurred

                 This is automatically handled by ipfix_send.
		 */

		ret=ipfix_send(my_exporter);

		if (ret != 0) {
			fprintf (stderr, "ipfix_send failed!\n");
		}
	}


	// free resources
	ipfix_deinit_exporter (&my_exporter);

	exit(0);
}
