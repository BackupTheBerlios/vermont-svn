/*
 This file is part of IPFIXLOLIB
 TEST CODE
 Published under GPL v2
 Ronny T. Lampert, 2005-01

 based upon the original IPFIXLOLIB
 by Jan Petranek, University of Tuebingen
 2004-11-18
 jan@petranek.de
 */

#include <stdio.h>
#include "ipfixlolib.h"

#define MY_SOURCE_ID 70538
#define TEST_INIT_LOOP 1
#define TEST_TEMPLATE_LOOP 1
#define TEST_ITERATIONS 10

int print_usage(void){
	
	printf("How To Use Tests:\n\n");
	printf("\t--- Just what you are reading: \th \n");
	printf("\t--- Create SCTP collector: \tc \n");
	printf("\t--- Template creation: \t\tt \n");
	printf("\t--- Template creation with custom ID: \tT 777 \n");
	printf("\t--- Delete template with ID: \td 777 \n");
	printf("\t--- Sending: \t\t\ts \n");
	printf("\t--- Quit and deinit:\t\tq \n");
	
	return 0;
}

void ready(void){
	printf(":");	
}

int main(int argc, char *argv[])
{

	int ret;
	int sctp_exists = 0;
	ipfix_exporter *my_exporter;

	ret = ipfix_init_exporter(MY_SOURCE_ID, &my_exporter);
	if (ret != 0) {
		fprintf(stderr, "ipfix_init_exporter failed!\n");
		exit(-1);
	}

	ret=ipfix_add_collector(my_exporter, "127.0.0.1", 4711, UDP);
	if (ret != 0) {
		fprintf(stderr, "ipfix_add_collector failed!\n");
		exit(-1);
	}
	
	
	
	ready();
	
	int c;
	
	while ((c = getchar()) != 'q') {
		uint16_t my_template_id = 0;
		uint create_id = 0;
		uint delete_id = 0;
		switch (c) {
		case 'T':
			create_id = 0;
			//Test custom templates
			printf("Start testing Template creation!\n");
			scanf("%u",&create_id);
			
			ret=0;
		
			ret|=ipfix_start_template_set(my_exporter, create_id, 1);
			ret|=ipfix_put_template_field(my_exporter, create_id, 2, 8, 0);
			ret|=ipfix_end_template_set(my_exporter, create_id);
		
			if (ret != 0) {
				fprintf(stderr, "create template failed!\n");
				exit(-1);
			}
			printf("template created with ID: %u\n", create_id);
			break;
		case 't':
			//Test templates
			printf("Start testing Template creation!\n");
			my_template_id=4711;
			ret=0;
		
			ret|=ipfix_start_template_set(my_exporter, my_template_id, 1);
			ret|=ipfix_put_template_field(my_exporter, my_template_id, 2, 8, 0);
			ret|=ipfix_end_template_set(my_exporter, my_template_id);
		
			if (ret != 0) {
				fprintf(stderr, "create template failed!\n");
				exit(-1);
			}
			printf("template created with ID: %d\n", my_template_id);
			break;
		case 's':
			//Test sending
			printf("Start sending!\n");
			
			ret=ipfix_send(my_exporter);
			if (ret != 0) {
				fprintf(stderr, "ipfix_send failed!\n");
				exit(-1);
			}
			break;
		case 'c':
			// add SCTP collector
			ret=ipfix_add_collector(my_exporter, "127.0.0.1", 1500, SCTP);
			
			if (ret != 0) {
				fprintf(stderr, "ipfix_add_collector failed!\n");
				exit(-1);
			}
			sctp_exists = 1;
			break;
		case 'd':
			//delete template
			scanf("%d",&delete_id);
			printf("Start testing Template destruction ID : %d!\n", delete_id);
			
			ret=ipfix_remove_template_set(my_exporter, delete_id);
			
			if (ret != 0) {
				fprintf(stderr, "ipfix_remove_template_set failed!\n");
			}
			break;
		case 10:
			ready();
			break;
		case 'h':
			print_usage();
			break;
		default:
			print_usage();
		}
	}


	//Cleaning
	printf("Cleaning up:\n");
/*
	printf("Remove Collector!\n");
	
	ret=ipfix_remove_collector(my_exporter, "127.0.0.1", 4711);
	if (ret != 0) {
		fprintf(stderr, "ipfix_remove_collector failed!\n");
		exit(-1);
	}
	if(sctp_exists){
		
		printf("Remove  SCTP Collector!\n");
		ret=ipfix_remove_collector(my_exporter, "127.0.0.1", 1500);
		if (ret != 0) {
			fprintf(stderr, "ipfix_remove_sctp_collector failed!\n");
			exit(-1);
		}
	}
*/	
	printf("deinit exporter!\n");
	
	ipfix_deinit_exporter(my_exporter);



	printf("Done, Have a nice day!\n");
	exit(0);
}
