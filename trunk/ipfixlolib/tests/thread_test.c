/*
 This file is part of IPFIXLOLIB
 A testfile for threaded applications.
 WARNING: This is EXPERIMENTAL. As of 2005-1-23, this test is
 expected to fail!
 
 Published under GPL v2
 Ronny T. Lampert, 2005-01

 based upon the original IPFIXLOLIB
 by Jan Petranek, University of Tuebingen
 2004-11-18
 jan@petranek.de
 */

#include <stdio.h>
#include "ipfixlolib.h"

#define IPFIX_THEADED 1

#ifdef IPFIX_THREADED
#include <pthread.h>
#include <sched.h>
#endif

#define MY_SOURCE_ID 70538


#define THREAD_ITERATIONS 100000
#define MAX_TEST_COLLECTORS 3
#define MAX_TEST_EXPORTERS 1
#define MAX_TEST_TEMPLATES 3

// what cycle do we test?
#define TEST_DATA 1
#define TEST_TEMPLATE 1
#define TEST_COLLECTOR 1
#define TEST_SEND 1


//#define DEBUG 1

/* just printf for now */
#ifdef DEBUG
#define DPRINTF(fmt, args...) printf(fmt, ##args)
#else
#define DPRINTF(fmt, args...)
#endif

/*
 Manages threads related to an exporter
*/
typedef struct{
	pthread_t send_thread;
	pthread_t collector_thread;
	pthread_t template_thread;
	pthread_t data_thread;	
	int running;
	ipfix_exporter* exporter;
} exporter_manager;




/* global variable to manage the exporter */
exporter_manager my_managers[MAX_TEST_EXPORTERS];

/* using default, global values */
char *collector_ip = "127.0.0.1";
int collector_port = 4711;



/*
 * Sends data in a loop, if there is something to send
 */
void *send_loop (void* manager)
{
	int ret; 
	exporter_manager* my_manager =  ((exporter_manager*) manager); 
	DPRINTF ("SEND Thread\n");
      
	
	DPRINTF ("SEND: Address of my manager: %u \n", my_manager);
 	DPRINTF ("my_manager is running %i \n", (*my_manager).running ); 
	while ((*my_manager).running ) {
		ret = ipfix_send ( my_manager->exporter);
	}

	printf ("send thread quits \n");
	pthread_exit(NULL);
}

/*
 * Inits / deinits colletors
 */
void *collector_loop (void* manager)
{
	int ret; 
	int i,j;
	exporter_manager* my_manager =  ((exporter_manager*) manager); 
	DPRINTF ("Collector Thread\n");
      
	
 	DPRINTF ("Collector: my_manager is running %i \n", (*my_manager).running ); 
	while ((*my_manager).running ) {
		i = my_manager->exporter->source_id;
		
		// add the collectors to this exporter
		for (j=0; ( ( j< MAX_TEST_COLLECTORS) &&( (*my_manager).running))  ; j++) {
			DPRINTF ("Collector: trying to add; running: %i \n", (*my_manager).running ); 
			// use ports beginning with collector_port
			ret = ipfix_add_collector( my_manager->exporter, collector_ip, 
						   collector_port+j+MAX_TEST_COLLECTORS*i, UDP);
			if (ret < 0 ) {
				DPRINTF ("could not add collector %i to exporter %i\n", j, i);
			}
			sched_yield();

		}

		// test for exit here:
//		if (! (*my_manager).running ) goto quit;
		// remove the collectors again:
		for (j=0; ( ( j< MAX_TEST_COLLECTORS) &&( (*my_manager).running))  ; j++) {
			DPRINTF ("Collector: trying to remove; running: %i \n", (*my_manager).running ); 
			ret = ipfix_remove_collector( my_manager->exporter, collector_ip, 
						   collector_port+j+MAX_TEST_COLLECTORS*i);
			if (ret < 0 ) {
				DPRINTF ("could not remove collector %i from exporter %i\n", j, i);
			}
			sched_yield();

		}

	}

	printf ("collector thread quits \n");
	pthread_exit(NULL);
}




/*
 * Generates and removes templates to it's exporter
 */
void *template_loop (void* manager)
{
	int ret, i; 
	uint16_t my_template_id;
	// uint16_t my_n_template_id;
	exporter_manager* my_manager =  ((exporter_manager*) manager); 
	DPRINTF ("TEMPLATE Thread\n");
      

 	DPRINTF ("my_manager is running %i \n", (*my_manager).running ); 
	while ((*my_manager).running ) { // running
		// add templates
		for (i = 0; i< MAX_TEST_TEMPLATES; i++) {
			my_template_id = i + 300;
			//my_n_template_id = htons(my_template_id);	
			
			/*
			  We  define a minamal template 
			  
			  The template shall contain the following fields:
			  # |  IPFIX name of field      |  IPFIX field ID | length of associated datatype
			  -------------------------------------------------------------------------------
			  3 |  transportSourcePort      |   7             | 2			  
			*/  

			/*
			 *  Now start the adding of fields.
			 */
			ret=ipfix_start_template_set( my_manager->exporter, my_template_id, 1);
			if (ret == 0) {

				/*
				  Add fields "deltaPacket count" to the exporter.
				*/
				ret=ipfix_put_template_field( my_manager->exporter, my_template_id, 7, 2, 0);
				
				/* Finalize the template */
				ret=ipfix_end_template_set( my_manager->exporter, my_template_id);
				DPRINTF ("TEMPLATE: made template %i with success %i\n", my_template_id, ret);
			}
			sched_yield();
		} // end adding templates


		// remove the templates again:
		for (i = 0; i< MAX_TEST_TEMPLATES; i++) {
			my_template_id = i + 300;
			//my_n_template_id = htons(my_template_id);	


			/*
			 *  Now start the adding of fields.
			 */
			ret=ipfix_remove_template( my_manager->exporter, my_template_id);
			DPRINTF ("TEMPLATE: removed template %i with success %i\n", my_template_id, ret);

			sched_yield();
		} // end removing templates
	}

	printf ("template thread quits \n");
	pthread_exit(NULL);
}


/*
 * Puts data fields in a loop
 */
void *data_loop (void* manager)
{
	int ret; 
	uint16_t my_n_template_id;
	uint16_t my_data = 4711;
//	uint64_t packet_count = htonll(42);
	exporter_manager* my_manager =  ((exporter_manager*) manager); 
	DPRINTF ("DATA Thread\n");
      
	
 	DPRINTF ("DATA: my_manager is running %i \n", (*my_manager).running ); 

	my_n_template_id = htons(300);	
	while ((*my_manager).running ) {
		// TODO

			/* start a data-set */
			ret=ipfix_start_data_set(my_manager->exporter, &my_n_template_id);
			/* write fields only, if the start succeded */
			if (ret == 0) {
				
				/*
				  now fill the pre-defined data fields
				  
				  NOTE: supplied data is NOT copied and has to
				  stay valid until the ipfix_send() below!
				*/
				
				ipfix_put_data_field(my_manager->exporter, &my_data, 2);
				
				
				/* finish the data-set */
				ret=ipfix_end_data_set(my_manager->exporter);
				DPRINTF ("DATA: added data set with success %i\n",  ret);
			}

	}

	printf ("DATA thread quits \n");
	pthread_exit(NULL);
}



int main(int argc, char **argv)
{
	int ret =0;
	int i;



	if (argc != 3) {
		fprintf(stderr, "Usage: %s <server_ip_address> <server_port>\nWill use UDP.\n", argv[0]);
		exit(1);
	}

/* 	collector_ip=argv[1]; */
/*         collector_port=atoi(argv[2]); */

	DPRINTF ("alpha\n");
        /*
	 Initialize the exporters

	 sourceID: the source ID the exported stream gets
	 exporter: an ipfix_exporter * to be initialized
	 */
	
	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {

		ret=ipfix_init_exporter(i, &(my_managers[i].exporter));

		
		if (ret != 0) {
			fprintf(stderr, "ipfix_init_exporter failed!\n");
			exit(-1);
		}

		my_managers[i].running = TRUE;
	}

	DPRINTF ("BETA\n");

#ifdef TEST_SEND
	/* start the send-thread */
	
	i = 0;
	DPRINTF ("main: 	my_managers[%i].running %i \n",i, my_managers[i].running );

	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {

		ret = pthread_create( (&my_managers[i].send_thread), NULL, send_loop, &(my_managers[i]));
		DPRINTF ("created send %i thread with success %i\n",i, ret);
		

	}

#endif
	DPRINTF ("CHARLIE\n");

#ifdef TEST_COLLECTOR	
	/*
	 Add collector threads
	 */

	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {

		ret = pthread_create( (&my_managers[i].collector_thread), NULL, collector_loop, &(my_managers[i]));
		DPRINTF ("created collector %i thread with success %i\n",i, ret);
	}

#endif

#ifdef TEST_TEMPLATE
	/*
	 Add template threads
	 */
	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {

		ret = pthread_create( (&my_managers[i].template_thread), NULL, template_loop, &(my_managers[i]));
		DPRINTF ("created template %i thread with success %i\n",i, ret);
	}
#endif	

#ifdef TEST_DATA
        /*
	 Main exporting loop
	*/
	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {

		ret = pthread_create( (&my_managers[i].data_thread), NULL, data_loop, &(my_managers[i]));
		DPRINTF ("created data %i thread with success %i\n",i, ret);
	}
#endif

   


	/* if you no longer need the exporter: free resources */
	/* in the multithreaded environment, make sure, ALL trheads using the exporter are STOPPED! */

	for (i = 0; i<  THREAD_ITERATIONS; i++) {
		sched_yield();
	}

	DPRINTF ("main: STOPPING THREADS\n");
 	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {
		// tell the threads to quit:
		my_managers[i].running=FALSE;

		// wait for the send-threads to finish

#ifdef TEST_SEND
		pthread_join (my_managers[i].send_thread, NULL); 
#endif
#ifdef TEST_TEMPLATE
		pthread_join (my_managers[i].template_thread, NULL); 
#endif
#ifdef TEST_COLLECTOR
		pthread_join (my_managers[i].collector_thread, NULL); 
#endif
#ifdef TEST_DATA
 		pthread_join (my_managers[i].data_thread, NULL);  
#endif

	}


 	for (i = 0; i< MAX_TEST_EXPORTERS; i++) {
		ret=ipfix_deinit_exporter((my_managers[i].exporter));

		if (ret != 0) {
			fprintf(stderr, "ipfix_init_deexporter failed!\n");
			exit(-1);
		}
		my_managers[i].running= FALSE;
	}



	printf("bravo\n");
	exit(0);
}
