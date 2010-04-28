/*
 This file is part of IPFIXLOLIB
 EXAMPLE CODE
 Published under GPL v2
 Ronny T. Lampert, 2005-01
 Daniel Mentz, 2010-04

 based upon the original IPFIXLOLIB
 by Jan Petranek, University of Tuebingen
 2004-11-18
 jan@petranek.de
 */

#include <stdio.h>
#include <getopt.h>
#include "ipfixlolib/ipfixlolib.h"

#define MY_OBSERVATION_DOMAIN_ID 70538

struct config {
	const char *coll_ip4_addr;
	int coll_port;
	enum ipfix_transport_protocol transport_protocol;
	uint16_t mtu;
	const char *peer_fqdn;
	const char *certificate_chain_file;
	const char *private_key_file;
	const char *ca_file;
	const char *ca_path;
};

struct config myconfig = {
	.coll_ip4_addr = 0,
	.coll_port = 0,
	.transport_protocol = UDP,
	.mtu = 0,
	.peer_fqdn = 0,
	.certificate_chain_file = 0,
	.private_key_file = 0,
	.ca_file = 0,
	.ca_path = 0
};

/*
 Data we want to transmit.
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

int parse_command_line_arguments(int argc, char **argv) {
	int transport = UDP;
	enum opts { mtu=1, peer_fqdn,cert,key,CAfile,CApath,collector,port };
	struct option long_options[] = {
		{"udp", no_argument, &transport, UDP},
		{"sctp", no_argument, &transport, SCTP},
		{"dtls_over_udp", no_argument, &transport, DTLS_OVER_UDP},
		{"dtls_over_sctp", no_argument, &transport, DTLS_OVER_SCTP},
		{"mtu",required_argument,0,mtu},
		{"peer_fqdn",required_argument,0,peer_fqdn},
		{"cert",required_argument,0,cert},
		{"key",required_argument,0,key},
		{"CAfile",required_argument,0,CAfile},
		{"CApath",required_argument,0,CApath},
		{"collector",required_argument,0,collector},
		{"port",required_argument,0,port},
		{0, 0, 0, 0}
	};
	while (1) {
		int c;
		int option_index = 0;
		long l;
		char *endptr;
		struct in_addr tmpaddr;

		c = getopt_long (argc, argv, "", long_options, &option_index);
		if (c == -1) break;
		switch (c) {
			case 0:
				break;
			case mtu:
				printf("mtu: %s\n",optarg);
				break;
			case peer_fqdn:
				myconfig.peer_fqdn = optarg;
				break;
			case cert:
				myconfig.certificate_chain_file = optarg;
				break;
			case key:
				myconfig.private_key_file = optarg;
				break;
			case CAfile:
				myconfig.ca_file = optarg;
				break;
			case CApath:
				myconfig.ca_path = optarg;
				break;
			case collector:
				if (inet_pton(AF_INET,optarg,&tmpaddr) != 1) {
					fprintf(stderr,"bad IP address\n");
					return -1;
				}
				myconfig.coll_ip4_addr = optarg;
				break;
			case port:
				l = strtol(optarg,&endptr,0);
				if (*endptr == '\0' || l < 0 || l > UINT16_MAX) {
					fprintf(stderr,"bad port number\n");
					return -1;
				}
				myconfig.coll_port = l;
				break;
		}
	}
	myconfig.transport_protocol = transport;
	if (myconfig.coll_port == 0) {
		if (myconfig.transport_protocol == DTLS_OVER_UDP ||
				myconfig.transport_protocol == DTLS_OVER_SCTP)
			myconfig.coll_port = 4740; /* default port for secure connections */
		else
			myconfig.coll_port = 4739; /* default port */
	}
	if (optind != argc) {
		fprintf(stderr,"invalid arguments\n");
		return -1;
	}
	if (!myconfig.coll_ip4_addr) {
		fprintf(stderr,"Please specify IPv4 address of collector.\n");
		return -1;
	}

	return 0;
}

/*
 You can add up to IPFIX_MAX_COLLECTORS collectors.
*/
int add_collector(ipfix_exporter *exporter) {
	int ret;

	void *aux_config = NULL;
	ipfix_aux_config_udp acu = {
		.mtu = myconfig.mtu
	};
	ipfix_aux_config_dtls_over_udp acdou = {
		.udp = { .mtu = myconfig.mtu},
		.dtls = { .peer_fqdn = myconfig.peer_fqdn}
	};
	ipfix_aux_config_dtls_over_sctp acdos = {
		.dtls = { .peer_fqdn = myconfig.peer_fqdn}
	};
	if (myconfig.transport_protocol == UDP) {
		aux_config = &acu;
	} else if (myconfig.transport_protocol == DTLS_OVER_UDP) {
		aux_config = &acdou;
	} else if (myconfig.transport_protocol == DTLS_OVER_SCTP) {
		aux_config = &acdos;
	}
	if ((ret = ipfix_add_collector(exporter, myconfig.coll_ip4_addr,
			myconfig.coll_port, myconfig.transport_protocol, aux_config))) {
		fprintf(stderr, "ipfix_add_collector() failed.\n");
		return -1;
	}
	printf("ipfix_add_collector returned %i\n", ret);
	while(ipfix_beat(exporter))
		usleep(10000);
	return 0;

}


/* generate constant data for testing / example purposes */
int get_sample_data1(meter_data *mdat)
{
	mdat->ip_src_addr = htonl(0x01020304);	// 1.2.3.4
	mdat->ip_dst_addr = htonl(0x02040608);	// 2.4.6.8
	mdat->src_port = htons(12);
	mdat->dst_port = htons(13);
	mdat->byte_count = htonll(1567490);
	mdat->packet_count = htonll(42);

	return 0;
}

/* generate some other data */
int get_sample_data2(meter_data *mdat)
{
	mdat->ip_src_addr = htonl(0x01020304);	// 1.2.3.4
	mdat->ip_dst_addr = htonl(0x02040608);	// 2.4.6.8
	mdat->src_port = htons(22);
	mdat->dst_port = htons(4713);
	mdat->byte_count = htonll(3);
	mdat->packet_count = htonll(2);

	return 0;
}


int main(int argc, char **argv)
{
	int ret =0;

	if (parse_command_line_arguments(argc,argv)) {
		exit(EXIT_FAILURE);
	}

	/* Initialize the exporter.
	 * MY_OBSERVATION_ID is the Observation ID that will be sent
	 * with every IPFIX Message. */
	ipfix_exporter *my_exporter;
	ret=ipfix_init_exporter(MY_OBSERVATION_DOMAIN_ID, &my_exporter);

	if (ret) {
		fprintf(stderr, "ipfix_init_exporter failed!\n");
		exit(EXIT_FAILURE);
	}
	
	if (add_collector(my_exporter)) exit(EXIT_FAILURE);

	/*
	 * Prior to sending Data Records we have to define a Template that describes the 
	 * encoding of these Data Records.
	 * We choose 12345 as the Template ID.

	 The template shall contain the following fields:
	 # |  IPFIX name of field      |  IPFIX field ID | length of associated datatype
	 -------------------------------------------------------------------------------
	 1 |  sourceAddressV4          |   8             | 4
	 2 |  destinationAddressV4     |   12            | 4
	 3 |  transportSourcePort      |   7             | 2
	 4 |  transportDestinationPort |   11            | 2
	 5 |  deltaOctetCount          |   1             | 8
	 6 |  deltaPacketCount         |   2             | 8

	 */  
	uint16_t my_template_id = 12345;
	uint16_t my_n_template_id = htons(my_template_id); /* Template ID in network byte order */

	/*
	 Now start the adding of fields.

	 exporter: the exporter
	 template_id: an ID for this template
	 field_count: # of entries/fields
	 */
	ret = ipfix_start_template_set(my_exporter, my_template_id, 6);

	/*
	 Add fields to the exporter.

	 exporter: the exporter
	 template_id: the template ID chosen beforehand
	 type: the IPFIX field ID for this entry
	 length: sizeof() datatype
	 enterprise: FIXME ???
	 */
	ret = ipfix_put_template_field(my_exporter, my_template_id, 8,  4, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id, 12, 4, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id, 7,  2, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id, 11, 2, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id, 1,  8, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id, 2,  8, 0);

        /* Finalize the template */
	ret = ipfix_end_template_set(my_exporter, my_template_id);


	/*
	 * We decide to define another Template with ID 6789.
	 The template shall contain the following fields:
	 # |  IPFIX name of field      |  IPFIX field ID | length of associated datatype
	 -------------------------------------------------------------------------------
	 1 |  sourceAddressV4          |   8             | 4
	 2 |  destinationAddressV4     |   12            | 4
	 3 |  transportSourcePort      |   7             | 2
	 4 |  transportDestinationPort |   11            | 2
	 */

	uint16_t my_template_id2 = 6789;
	uint16_t my_n_template_id2 = htons(my_template_id2); /* Same Template ID in network byte order */

	/*
	 Now start the adding of fields.

	 exporter: the exporter
	 template_id: an ID for this template
	 field_count: # of entries/fields
	 */
	ret = ipfix_start_template_set(my_exporter, my_template_id2, 4);

	/*
	 Add fields to the exporter.

	 exporter: the exporter
	 template_id: the template ID chosen beforehand
	 type: the IPFIX field ID for this entry
	 length: sizeof() datatype
	 enterprise: FIXME ???
	 */
	ret = ipfix_put_template_field(my_exporter, my_template_id2, 8,  4, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id2, 12, 4, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id2, 7,  2, 0);
	ret = ipfix_put_template_field(my_exporter, my_template_id2, 11, 2, 0);

        /* Finalize the template */
	ret = ipfix_end_template_set(my_exporter, my_template_id2);

        /*
	 Main exporting loop

	 What you basically do is

	 1) get data
	 2) start a data set
	 3) add the fields
	 4) finish data set
         5) send data
	 */
	
	/* let's specify the number of IPFIX Messages */
	int exporting = 2;

	while(exporting) {

		int i;
		meter_data my_meter_data[5];

                /* you may loop and add one or more data-sets */
		for (i = 0; i < 2; i++) {

			/* start a data-set */
			ret=ipfix_start_data_set(my_exporter, my_n_template_id);
			
			if (ret != 0 ) {
				// do not try use ipfix_put_data_field or  ipfix_put_end_field,
				// if  ret=ipfix_start_data_set has failed!

				fprintf(stderr, "ipfix_start_data_set failed!\n");	
			} else {
				/* get data - must be in Network Byte Order for interoperability */
				get_sample_data1(&my_meter_data[i*2]);

				/*
				  now fill the pre-defined data fields

				  NOTE: supplied data is NOT copied and has to
				  stay valid until the ipfix_send() below!

				  NOTE: It's the user's responsability to ensure that
				  the added data is conform to the indicated template.
				*/
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].ip_src_addr), 4);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].ip_dst_addr), 4);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].src_port), 2);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].dst_port), 2);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].byte_count), 8);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].packet_count), 8);

				/* more than one record can be added to a data set, so let's add another one */
				/* NOTE: we need another my_meter_data here, because the data of my_meter_data 
				   has to remain valid till ipfix_send() is called */
				get_sample_data2(&my_meter_data[i*2+1]);

				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2+1].ip_src_addr), 4);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2+1].ip_dst_addr), 4);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2+1].src_port), 2);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2+1].dst_port), 2);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2+1].byte_count), 8);
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2+1].packet_count), 8);

				/* finish the data-set 
				   remark: the main task of ipfix_end_data_set is to calculate the length of the data set */
				ret=ipfix_end_data_set(my_exporter,2);

				if (ret != 0)
					fprintf(stderr, "ipfix_end_data_set failed!\n");
			}
		}

		/* start a data-set for second template*/
		ret=ipfix_start_data_set(my_exporter, my_n_template_id2);

		if (ret != 0 ) {
		    // do not try use ipfix_put_data_field or  ipfix_put_end_field,
		    // if  ret=ipfix_start_data_set has failed!

		    fprintf(stderr, "ipfix_start_data_set failed!\n");	
		} else {
		    /* get data - must be in Network Byte Order for interoperability */
		    get_sample_data1(&my_meter_data[4]);

		    /*
		       now fill the pre-defined data fields
		     */
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].ip_src_addr), 4);
		    /* We changed our mind and want to stop, so call ipfix_cancel_data_set 
		       instead of ipfix_end_data_set */
		    ret=ipfix_cancel_data_set(my_exporter);

		    if (ret != 0)
			fprintf(stderr, "ipfix_end_data_set failed!\n");
		}

		
		/* start again for second template*/
		ret=ipfix_start_data_set(my_exporter, my_n_template_id2);

		if (ret != 0 ) {
		    // do not try use ipfix_put_data_field or  ipfix_put_end_field,
		    // if  ret=ipfix_start_data_set has failed!

		    fprintf(stderr, "ipfix_start_data_set failed!\n");	
		} else {
		    /* get data - must be in Network Byte Order for interoperability */
		    get_sample_data1(&my_meter_data[4]);

		    /*
		       now fill the pre-defined data fields
		     */
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].ip_src_addr), 4);
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].ip_dst_addr), 4);

		    /* Set marker in order to go back */
		    ipfix_set_data_field_marker(my_exporter);

		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].src_port), 2);

		    /* Go back to the marker */
		    ipfix_delete_data_fields_upto_marker(my_exporter);

		    /* It was just a joke, so let's put the data field again */
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].src_port), 2);
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].dst_port), 2);


		    /* finish the data set 
			remark: the main task of ipfix_end_data_set is to
			calculate the length of the data set */
		    ret=ipfix_end_data_set(my_exporter,1);

		    if (ret != 0)
			fprintf(stderr, "ipfix_end_data_set failed!\n");
		}

		/*
		 send the data-set(s)
		 template sending is handled entirely by the library, too.

		 NOTE: ALL DATA added via ipfix_put_data_field has to
		 stay valid until ipfix_send() returns.
		 */
		ret=ipfix_send(my_exporter);
		if (ret != 0)
			fprintf(stderr, "ipfix_send failed!\n");

		exporting--;
	}

	/* if you no longer need the exporter: free resources */
	ret=ipfix_remove_collector(my_exporter, myconfig.coll_ip4_addr, myconfig.coll_port);
	ipfix_deinit_exporter(my_exporter);

	printf("Done.\n");
	exit(EXIT_SUCCESS);
}
