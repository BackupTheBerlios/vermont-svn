/* Copyright (C) 2004  Jan Petranek */

/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
 * This file is part of the ipfixlolib.
 *
 * A low-level implementation of ipfix-functions
 *
 * by Jan Petranek, University of Tuebingen
 * 2004-11-12
 * jan@petranek.de
 * The ipfixlolib will be published under the GPL or Library GPLd
 * 
 */


#include "ipfixlolib.h"

/********************************************************************/
/*   Global, Internal variables                                     */
/********************************************************************/


// ipfix_receiving_collector collectors[IPFIX_MAX_COLLECTORS];


/********************************************************************/
/* Function definitions                                             */
/********************************************************************/

#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>

//extern "C" {

// TODO: set up sockets for TCP and SCTP (in case it will be implemented)

// declarations
int ipfix_init_sendbuffer (ipfix_sendbuffer** sendbuf, int maxelements);
int ipfix_init_collector_array(ipfix_receiving_collector** col, int col_capacity);
int ipfix_deinit_collector_array(ipfix_receiving_collector** col);
int ipfix_init_template_array(ipfix_exporter* exporter, int template_capacity);
int ipfix_deinit_template_array(ipfix_exporter* exporter);
int ipfix_deinit_sendbuffer (ipfix_sendbuffer** sendbuf);
int ipfix_init_send_socket(char* serv_ip4_addr, int serv_port, enum ipfix_transport_protocol  protocol);
int write_ipfix_message_header (ipfix_header* header, char** p_pos, char* p_end );
int ipfix_init_set_manager(ipfix_set_manager** set_manager, int max_capacity);
int ipfix_reset_set_manager (ipfix_set_manager* set_manager);
int ipfix_deinit_set_manager(ipfix_set_manager** set_manager);
int ipfix_start_template_set (ipfix_exporter* exporter, uint16_t template_id,  uint16_t field_count);
int ipfix_put_template_field(ipfix_exporter* exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_end_template_set(ipfix_exporter* exporter, uint16_t template_id );
int ipfix_init_send_socket(char* serv_ip4_addr, int serv_port, enum ipfix_transport_protocol  protocol);

/*
 * Initializes a UDP-socket to listen to. 
 * Parameters: lport the UDP-portnumber to listen to.
 * Returns: a socket to read from. -1 on failure.
 */
int init_rcv_udp_socket(int lport) {
  int s; // the socket
  struct sockaddr_in serv_addr;
  
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (lport);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   
   
   // create socket
   if (( s = socket(AF_INET, SOCK_DGRAM, 0 )) < 0 ) {
       fprintf(stderr, "error opening socket\n");
       return -1;
   }

   // bind to socket
   if (( bind (s, (struct sockaddr*) &serv_addr, sizeof(serv_addr))) < 0) {
     perror ("bind failed! \n");
     return -1;
   }
   return s;
}
 
/*
 * Initializes a UDP-socket to send data to. 
 * Parameters: 
 * char* serv_ip4_addr IP-Address of the recipient (e.g. "123.123.123.123")
 * serv_port the UDP-portnumber of the server.
 * Returns: a socket to write to. -1 on failure
 */
int init_send_udp_socket( char* serv_ip4_addr, int serv_port){
   int s; // the socket
   struct sockaddr_in serv_addr;

   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons (serv_port);
   serv_addr.sin_addr.s_addr = inet_addr(serv_ip4_addr);
   
   
   // create socket
   if (( s = socket(AF_INET, SOCK_DGRAM, 0 )) < 0 ) {
       fprintf(stderr, "error opening socket\n");
       return -1;
   }

   // connect to server
   if ( connect (s, (struct sockaddr*) &serv_addr, sizeof(serv_addr) ) < 0) {
     perror ("connect failed! \n");
     return -1;
   }
   return s;
}


/*
 * Initialize an exporter process
 * Allocates all memory necessary.
 * Parameters: 
 * sourceID The source ID, to which the exporter will be initialized to.
 * exporter an ipfix_exporter* to be initialized
 */
int ipfix_init_exporter (uint32_t source_id, ipfix_exporter** exporter) {
  // allocate memory to the exporter:
  (*exporter) = (ipfix_exporter*) malloc (sizeof (ipfix_exporter)); 
  (**exporter).source_id = source_id;
  (**exporter).sequence_number = 0;
  (**exporter).collector_max_num = 0;

  int ret;
  // TODO
  // initialize the sendbuffers
  ret =  ipfix_init_sendbuffer ( &( (**exporter).data_sendbuffer), IPFIX_MAX_SENDBUFSIZE);
  if (ret != 0) {
    fprintf (stderr, "ipfix_init_exporter : Initializing data sendbuffer failed!\n");
    return -1;
  }
  
  ret =  ipfix_init_sendbuffer ( &((**exporter).template_sendbuffer), IPFIX_MAX_SENDBUFSIZE);
  if (ret != 0) {
    fprintf (stderr, "ipfix_init_exporter : Initializing template  sendbuffer failed!\n");
    return -1;
  }

  // intialize the collectors to zero
  ret = ipfix_init_collector_array( &((**exporter).collector_arr), IPFIX_MAX_COLLECTORS);
  if (ret !=0) {
    fprintf (stderr, "ipfix_init_exporter: Initializing collectors failed!\n");
    return ret;
  }

  (**exporter).collector_max_num = IPFIX_MAX_COLLECTORS;

  // initialize an array to hold the templates.
  // TODO: own function:
  ret = ipfix_init_template_array(*exporter, IPFIX_MAX_TEMPLATES);
/*   (**exporter).ipfix_lo_template_maxsize  = IPFIX_MAX_TEMPLATES; */
/*   (**exporter).ipfix_lo_template_current_count = 0 ; */
/*   (**exporter).template_arr =  (ipfix_lo_template*) malloc (IPFIX_MAX_TEMPLATES * sizeof(ipfix_lo_template) ); */
  // we have not transmitted any templates yet!
  (**exporter).last_template_transmition_time = 0;
  (**exporter).template_transmition_timer = IPFIX_DEFAULT_TEMPLATE_TIMER;

  return 0;
}

/*
 * cleanup an exporter process
 */
int ipfix_deinit_exporter (ipfix_exporter** exporter) {
  // cleanup processes
  int ret;
  // close sockets etc.
  // (currently, nothing to do)
  
  // free all children

  // deinitialize array to hold the templates.
  ret = ipfix_deinit_template_array( *exporter);

/*   free ( (**exporter).template_arr); */
/*   (**exporter).template_arr = NULL; */

  // deinitialize the sendbuffers

  ret = ipfix_deinit_sendbuffer ( (&(**exporter).data_sendbuffer));
  ret = ipfix_deinit_sendbuffer ( (&(**exporter).template_sendbuffer));

  // deinitialize the collectors
  ret = ipfix_deinit_collector_array( &((**exporter).collector_arr) );

  // free own memory
  free (*exporter);
  (*exporter) = NULL;

  return 0;
}


/*
 * Add a collector to the exporting process
 * Parameters: 
 *  exporter: The exporting process, to which the collector shall be attached
 *  coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
 *  coll_port: port number of the collector
 *  proto: transport protocol to use
 * NOTE: this is subject to change, as a collector might use SCTP, which would require 2 connections!
 * Returns: 0 on success or -1 on failure
 */
int ipfix_add_collector(ipfix_exporter* exporter,  char* coll_ip4_addr, int coll_port, enum ipfix_transport_protocol proto){
  // check if there is a free slot at all:
  //printf (" ipfix_add_collector 0\n");
  if (( *exporter).collector_num >= (*exporter).collector_max_num ) {
    fprintf (stderr, "No more free slots for new collectors available.\n");
    fprintf (stderr, "Collector not added!\n");
    return -1;
  }
  printf (" ipfix_add_collector 1\n");
  // allocate free slot from the exporter
  int i=0;
  int searching = TRUE;
  while (searching && ( i< (*exporter).collector_max_num) ) {
    //printf (" ipfix_add_collector searching %i, i %i \n", searching, i);  
    if ( (*exporter).collector_arr[i].valid == FALSE ) {
      // we have found a free slot:
      (*exporter).collector_arr[i].ipv4address = coll_ip4_addr;
      (*exporter).collector_arr[i].port_number = coll_port;
      (*exporter).collector_arr[i].protocol = proto;

      // open the socket: call an own function.
      (*exporter).collector_arr[i].data_socket = ipfix_init_send_socket( coll_ip4_addr, coll_port, proto);
      
      // error handling, in case we were unable to open the port:
      if ( (*exporter).collector_arr[i].data_socket < 0 ) {
	fprintf (stderr, "Initializing socket for %s:%i failed!\n");
	fprintf (stderr, "Skipping this collector!\n");
	return -1;
      }
      // currently, the data socket and the template socket are the same.
      // TODO, when SCTP is added!
      (*exporter).collector_arr[i].template_socket =  (*exporter).collector_arr[i].data_socket;
      
      // now, we may set the collector to valid;
      (*exporter).collector_arr[i].valid = UNCLEAN;
      
      // increase total number of collectors.
      (*exporter).collector_num ++;
      searching = FALSE;


    }
    // prepare for next loop:
    i++;

  }

}

/*
 * Remove a collector from the exporting process
 * Parameters:
 *  coll_id: the ID of the collector to remove
 * Returns: 0 on success, -1 on failure
 */
int ipfix_remove_collector(ipfix_exporter* exporter,   char* coll_ip4_addr, int coll_port) {
  // find the collector in the exporter
  int i=0;
  int searching = TRUE;
  while (searching && ( i< (*exporter).collector_max_num) ) {
    if  (( strcmp ( (*exporter).collector_arr[i].ipv4address, coll_ip4_addr) == 0 ) 
	 && (*exporter).collector_arr[i].port_number == coll_port )  {

      // are template and data socket the same?
      if (  (*exporter).collector_arr[i].template_socket ==  (*exporter).collector_arr[i].data_socket ) {
	close ( (*exporter).collector_arr[i].data_socket );
      } else { //close both connections:
	close ( (*exporter).collector_arr[i].data_socket );
	close ( (*exporter).collector_arr[i].template_socket );
      }
      
      (*exporter).collector_arr[i].valid = FALSE;
      searching = FALSE;
    }
    i++;
  }
  if (searching) {
    fprintf (stderr, "Exporter %s not found, removing exporter failed!\n", coll_ip4_addr); 
    return -1;

  }
  return 0;
}

/************************************************************************************/
/* Template management                                                              */
/************************************************************************************/


/*
 * Add a template to the exporting process
 * Parameters: 
 *  exporter: The exporting process, to which the collector shall be attached
 *  coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
 *  coll_port: port number of the collector
 *  proto: transport protocol to use
 * Returns: 0 on success or -1 on failure
 * does not allocate memory for the template_data!
 */
/* int ipfix_add_template(ipfix_exporter* exporter,  ipfix_lo_template* template ){ */

/*   int i=0; */
/*   int found_index = -1; */
/*   int searching;   */
/*   // allocate a slot from the exporter */

/*   // do we already have a template with this ID? */
/*   // -> update it! */
/*   searching = TRUE; */
/*   while (searching && ( i< (*exporter).ipfix_lo_template_maxsize) ) { */
  
/*     if ( (*exporter).template_arr[i].valid == TRUE && (*exporter).template_arr[i].template_id == template_id) { */
/*       // remove the old template from memory! */
/*       (*exporter).template_arr[i].valid == FALSE; */
/*       free ((*exporter).template_arr[i].template_fields) ; */

/*       found_index= i; */
/*       searching = FALSE; */
/*     } */
/*     i++; */
/*   } */


/*   // allocate a new, free slot. */
/*   if (found_index <0 ) {// no update */
/*     searching = TRUE; */
    
/*     // check if there is a free slot at all: */
/*     if (( *exporter).ipfix_lo_template_current_count >= (*exporter).ipfix_lo_template_maxsize ) { */
/*       fprintf (stderr, "No more free slots for new templates available.\n"); */
/*       fprintf (stderr, "Template not added!\n"); */
      
/*       // do error handling: */
/*       found_index = -1; */
/*       searching = FALSE; */
/*       return -1; */
/*     }    */
    
/*     // search for a free slot: */
    
/*     searching = TRUE; */
/*     i = 0; */
/*     while (searching && ( i< (*exporter).ipfix_lo_template_maxsize) ) { */
  
/*       if ( (*exporter).template_arr[i].valid == FALSE ) { */
/* 	// we have found a free slot: */
	
/* 	// initialize the new slot: */
	
/* 	// set the data pointer to NULL (so realloc will alloc memory) */
/* 	(*exporter).template_arr[i].template_fields = NULL; */
	
/* 	// increase total number of templates. */
/* 	(*exporter).ipfix_lo_template_current_count ++; */
/* 	searching = FALSE; */
/* 	found_index = i; */
/*       } */

/*       // prepare for next loop: */
/*       i++; */

/*     } */


/*   } // end if allocating new slot */


/*   // test for a valid slot: */
/*   if ( (found_index > 0 ) && ( found_index < (*exporter).ipfix_lo_template_maxsize ) ) { */

/*     // now, initialize the slot we found */
/*     (*exporter).template_arr[i].template_fields = template_data; */
/*     (*exporter).template_arr[i].valid = TRUE; */
/*     (*exporter).template_arr[i].template_id = template_id; */
/*     (*exporter).template_arr[i].field_count =  field_count; */
/*     (*exporter).template_arr[i].fields_length  = template_data_length; */
/*     // we assume, the user has already set the right length */
/*     (*exporter).template_arr[i].max_fields_length  = template_data_length; */
    
/*   } else return -1; // no valid slot was found! */
/*   return 0; */

/* } */


/*
 * Helper function: Finds a template in the exporter
 * Parmeters:
 * exporter: Exporter to search for the template
 * template_id: ID of the template we search
 * cleanness: search for COMMITED or UNCLEAN templates? May even search for UNUSED templates
 * Returns: the index of the template in the exporter or -1 on failure.
 */

int ipfix_find_template(ipfix_exporter* exporter, uint16_t template_id,  enum ipfix_validity cleanness){
  // first, some safety checks:
  if (exporter == NULL) { 
    fprintf (stderr, "Exporter is NULL! Cannot search it for a template!\n");
    return -1;
  }
  if ( (*exporter).template_arr == NULL ) {
   fprintf (stderr, "Template store not initialized! Cannot search it for a template!\n");
    return -1;

  }


  int i=0;
  int found_index = -1;
  int searching;  
  // allocate a slot from the exporter

  // do we already have a template with this ID?
  // -> update it!
  searching = TRUE;
  while (searching && ( i< (*exporter).ipfix_lo_template_maxsize) ) {
  
    if ( (*exporter).template_arr[i].valid == cleanness) {
      // if we are searching for an existing template, compare the template_id too:
      if ( ( cleanness == COMMITED) || (cleanness = UNCLEAN) ) {
	if ( (*exporter).template_arr[i].template_id == template_id) {
	  return i;
	  searching = FALSE;
	}
      } else {
	// we found an unused slot; return the index:
	return i;
	searching = FALSE;
      }

    }
    i++;
  } // end while
  return -1;
}


/*
 * Remove a template from the exporting process
 * Parameters:
 * exporter: the exporter
 * template_id: ID of the template to remove
 * Returns: 0 on success, -1 on failure
 * This will free the templates data store!
 */
int ipfix_remove_template(ipfix_exporter* exporter, uint16_t template_id) {
  // find the template in the exporter
  int i=0;
  
  // TODO: maybe, we have to clean up unclean templates too:
  int found_index = ipfix_find_template(exporter,template_id, COMMITED);

 

  if (found_index >= 0) {
	(*exporter).template_arr[found_index].valid = UNUSED;
	(*exporter).ipfix_lo_template_current_count--;

	// free memory from the template
	free ( (*exporter).template_arr[found_index].template_fields);
	(*exporter).template_arr[found_index].template_fields = NULL;
  }else {
    fprintf (stderr, "Template ID %u not found, removing template failed!\n", template_id); 
    return -1;

  }
  return 0;
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
int ipfix_prepend_header (ipfix_exporter* p_exporter, int data_length, ipfix_sendbuffer* sendbuf){  

  ipfix_header header;
  time_t export_time;
  uint16_t total_length = 0;
  int ret;
  int header_length = 4* sizeof(uint32_t);

  // did the user set the data_length field?
  if (data_length != 0) {
    total_length = header_length+data_length;
  } else { // compute it on our own:
    // sum up all lengths in the iovecs:
    int i;

    // start the loop with 1, as 0 is reserved for the header!
    for (i = 1; i< (*sendbuf).current;  i++) {
      total_length += ((*sendbuf).entries[i]).iov_len;
    }
    
    // add the header lenght to the total length:
    total_length += header_length;
  }

  // write the length into the header
  header.length = total_length;

  // write version number and source ID
  header.version = IPFIX_VERSION_NUMBER;
  header.source_id = (*p_exporter).source_id;

  //  increment and write the sequence number:
  header.sequence_number =   (*p_exporter).sequence_number;
  (*p_exporter).sequence_number++;

  // get the export time:
  export_time =  time(NULL);
  if (export_time ==  ((time_t)-1) ) {
    fprintf (stderr, "time failed!\n");
    export_time=0; // just something to survive!
  }
  //  global_last_export_time = (uint32_t) export_time;
  header.export_time =   (uint32_t) export_time;


  // write the header:
  // first, get the buffer for the header
  char* p_base = (*sendbuf).header_store;
  char* p_pos = p_base;
  char* p_end = p_pos + IPFIX_HEADER_LENGTH;

  // write into that header
  ret=  write_ipfix_message_header ( &header, &p_pos, p_end );
  ((*sendbuf).entries[0]).iov_len = IPFIX_HEADER_LENGTH;
  ((*sendbuf).entries[0]).iov_base = p_base; 

  return ret;  

}



/* 
 * Write an IPFIX-Message-header
 *
 * This writes an ipfix message header to the memory between p_pos and p_end
 * Parameters: ipfix_header* header contains the data to be written
 * p_pos is the start, p_end the end of the memory, where the header will be written to
 *
 * Note: The user is supposed to call ipfix_send_array instead.
 */
int write_ipfix_message_header (ipfix_header* header, char** p_pos, char* p_end ){
  // check for available space 
  if (*p_pos + IPFIX_HEADER_LENGTH < p_end ) {
    printf ("Not enough memory allocated for header! \n");
    return -1;
  }

  // initialize to zero.
  memset(*p_pos, 0, IPFIX_HEADER_LENGTH);

  // write the version information
  write_unsigned16 (p_pos, p_end, (*header).version);

  // write the length information
  write_unsigned16(p_pos, p_end,  (*header).length);

  // write the export time
  write_unsigned32(p_pos, p_end,  (*header).export_time);

  // write the sequence_number
  write_unsigned32(p_pos, p_end,  (*header).sequence_number);

  // write the source_id
  write_unsigned32(p_pos, p_end,  (*header).source_id);  
    return 0;
}


/*
 * Initialize an ipfix_sendbuffer for at most maxelements 
 * Parameters: ipfix_sendbuffer** sendbuf pointerpointer to an ipfix-sendbuffer
 * maxelements: Maximum capacity of elements, this sendbuffer will accomodate.
 */
int ipfix_init_sendbuffer (ipfix_sendbuffer** sendbuf, int maxelements) {
  // mallocate memory for the sendbuffer
  *sendbuf = (ipfix_sendbuffer*) malloc ( sizeof (ipfix_sendbuffer) );
  
  // mallocate memory for the entries:
  (**sendbuf).entries = (struct iovec* ) malloc ( maxelements * sizeof (struct iovec*) ) ;
 
  (**sendbuf).current =  HEADER_USED_IOVEC_COUNT; // leave the 0th field blank for the header
  (**sendbuf).length = maxelements;
  (**sendbuf).commited =  HEADER_USED_IOVEC_COUNT;

  // allocate memory for the header itself
  (**sendbuf).header_store = (char*) malloc (IPFIX_HEADER_LENGTH) ;

  (**sendbuf).commited_data_length = 0;
  // initialize an ipfix_set_manager
  ipfix_init_set_manager ( (&(**sendbuf).set_manager),  IPFIX_MAX_SET_HEADER_LENGTH); 



  return 0;
}

/*
 * reset ipfix_sendbuffer
 * Resets the contents of an ipfix_sendbuffer, so the sendbuffer can again
 * be filled with data. 
 * (Present headers are also purged).
 */
int ipfix_reset_sendbuffer (ipfix_sendbuffer* sendbuf) {
  if (sendbuf == NULL ) {
    fprintf (stderr, "Sendbuffer is NULL\n");
    return -1;
  }
  (*sendbuf).current =  HEADER_USED_IOVEC_COUNT;
  (*sendbuf).commited =  HEADER_USED_IOVEC_COUNT;
  (*sendbuf).commited_data_length = 0;

  // also reset the set_manager!
   ipfix_reset_set_manager ((*sendbuf).set_manager); 
   

  return 0;
}


/*
 * Deinitialize (free) an ipfix_sendbuffer
 */
int ipfix_deinit_sendbuffer (ipfix_sendbuffer** sendbuf) {
  
  // cleanup the set manager
  ipfix_deinit_set_manager ( (&(**sendbuf).set_manager));

  // deallocate memory for the header 
  free ((**sendbuf).header_store);
  
  // free the array of iovec structs:
  free ((**sendbuf).entries );
  
  // finally, free the sendbuffer itself:
  free (*sendbuf);
  *sendbuf = NULL;
}

/*
 * initialize array of collectors
 * Allocates memory for an array of collectors
 * Parameters:
 * col: collector array to initialize
 * col_capacity: maximum amount of collectors to store in this array
 */
/* int ipfix_init_collector_array(ipfix_receiving_collector** col, int col_capacity){ */

/*   int i; */
/*   // allocate the memory for col_capacity elements: */
/*   int arr_len = (sizeof (ipfix_receiving_collector) * col_capacity); */

/*   (*col)  = (ipfix_receiving_collector*) malloc (arr_len); */
 
 
/*   for (i = 0; i< col_capacity; i++) { */
/*     ((*col)[i]).valid = FALSE; */
/*    } */
/*   return 0; */
/*  } */



/* 
 * deinitialize an array of collectors
 * Parameters:
 * col: collector array to clean up 
 */
int ipfix_deinit_collector_array(ipfix_receiving_collector** col){
  // free the memory
  free (*col);

  // set the array to NULL
  *(col) = NULL;
  return 0;
}




/*
 * Initializes a send socket 
 * Parameters: 
 * serv_ip4_addr of the recipient in dot notation
 * serv_port: port
 * protocol: transport protocol
 */
int ipfix_init_send_socket(char* serv_ip4_addr, int serv_port, enum ipfix_transport_protocol  protocol){
  int sock = -1;
  switch (protocol) {
  case UDP: {
    sock= init_send_udp_socket( serv_ip4_addr, serv_port);
    break;
  }
  case TCP: {
    fprintf (stderr, "Sorry, Transport Protocol TCP not implemented yet\n");
    break;
  }
  case SCTP: {
    fprintf (stderr, "Sorry, Transport Protocol SCTP not implemented yet\n");
    break;
  }
  default: {
    fprintf (stderr, "Transport Protocol not supported\n");
    return -1;
  }
    

  }
  return sock;
}


/*
 * initialize array of collectors
 * Allocates memory for an array of collectors
 * Parameters:
 * col: collector array to initialize
 * col_capacity: maximum amount of collectors to store in this array
 */
int ipfix_init_collector_array(ipfix_receiving_collector** col, int col_capacity){

  int i;
  // allocate the memory for col_capacity elements:
  int arr_len = (sizeof (ipfix_receiving_collector) * col_capacity);

  (*col)  = (ipfix_receiving_collector*) malloc (arr_len);
 
 
  for (i = 0; i< col_capacity; i++) {
    ((*col)[i]).valid = FALSE;
   }
  return 0;
 }



/*
 * Initializes a set manager 
 * Parameters: 
 * set_manager: set manager to initialize
 * max_capacity: maximum lenght, a header is allowed to have
 */
int ipfix_init_set_manager(ipfix_set_manager** set_manager, int max_capacity){

  // allocate memory for the set manager
  (*set_manager)  = (ipfix_set_manager*) malloc (sizeof(ipfix_set_manager));

  // allocate memory for the set header buffer & set values.
  (**set_manager).set_header_capacity = max_capacity;
  (**set_manager).set_header_length = 0;
  (**set_manager).set_header_store = (char*) malloc (max_capacity);
  (**set_manager).header_iovec = NULL;

  (**set_manager).data_length = 0;
  return 0;
} 

/*
 * reset ipfix_set_manager
 * Resets the contents of an ipfix_set_manager
 */
int ipfix_reset_set_manager (ipfix_set_manager* set_manager) {
  if (set_manager == NULL ) {
    fprintf (stderr, "Setmanager is NULL\n");
    return -1;
  }
  // reset the buffered data
  (*set_manager).set_header_length = 0;
  (*set_manager).header_iovec = NULL;
  (*set_manager).data_length = 0;


  return 0;
}


/*
 * Deinitializes a set manager 
 * Parameters: 
 * set_manager: set manager to deinitialize
 * WARNING: As long as the sendbuffer still may have data in the sendbuffer,
 * this function will refuse, unless the pointer to the header's iovec 
 * (set_manager.header_iovec) is null!
 */
int ipfix_deinit_set_manager(ipfix_set_manager** set_manager){

  // WARNING: this will blow, if the header's content is still in 
  // the sendbuffer. Call this ONLY, if the sendbuffer is
  // a) reseted
  // b) won't be send any more!

  // it will refuse to free the set manager, as long as the header_iovec 
  // is still set (i.e. is not null!)

  if ( (**set_manager).header_iovec != NULL ) {
    fprintf (stderr, "Set manager still may have data to be sent. Refusing to free it!\n");
    return -1;
  }

  // free the header buffer:
  free ( (**set_manager).set_header_store);

  // free memory from the set manager
  free (*set_manager);

  (*set_manager)  = NULL;
  return 0;
} 


/*
 * initialize array of templates
 * Allocates memory for an array of templates
 * Parameters:
 * exporter: exporter, whose template array we'll initialize
 * template_capacity: maximum amount of templates to store in this array
 */
int ipfix_init_template_array(ipfix_exporter* exporter, int template_capacity){

  int i;
  // allocate the memory for template_capacity elements:
  (*exporter).ipfix_lo_template_maxsize  = template_capacity;
  (*exporter).ipfix_lo_template_current_count = 0 ;
  (*exporter).template_arr =  (ipfix_lo_template*) malloc (template_capacity * sizeof(ipfix_lo_template) );

  for (i = 0; i< template_capacity; i++) {
    (  (*exporter).template_arr[i]).valid = UNUSED;
   }
  return 0;
 }



/* 
 * deinitialize an array of templates
 * Parameters:
 * exporter: exporter, whose template store will be purged
 */
int ipfix_deinit_template_array(ipfix_exporter* exporter){
  // free the memory
  free ( (*exporter).template_arr);
  (*exporter).template_arr = NULL;

  (*exporter).ipfix_lo_template_maxsize = 0;
  (*exporter).ipfix_lo_template_current_count = 0;

  return 0;
}

/*
 * Updates the template sendbuffer
 * will be called, after a template has been added or removed
 */
int ipfix_update_template_sendbuffer (ipfix_exporter* exporter){
  int ret;
  int i;
  int total_length = 0;
  // first, some safety checks:
  if (exporter == NULL) { 
    fprintf (stderr, "Exporter is NULL!\n");
    return -1;
  }
  if ( (*exporter).template_arr == NULL ) {
   fprintf (stderr, "Template store not initialized!\n");
    return -1;

  }

  // clean the template sendbuffer
  ret =  ipfix_reset_sendbuffer ( (*exporter).template_sendbuffer);

  // place all valid templates to the template sendbuffer
  // could be done just like put_data_field:
  
  ipfix_sendbuffer* t_sendbuf = (*exporter).template_sendbuffer;
  for (i = 0; i < (*exporter).ipfix_lo_template_maxsize; i++ )  {
    // is the current template valid?
    if ( (*exporter).template_arr[i].valid==COMMITED ) {
      // link the data to the sendbuffer:
 /*      ipfix_put_field2sendbuffer( (*exporter).template_sendbuffer ,  (*exporter).template_arr[i].template_fields, */
/* 				 (*exporter).template_arr[i].fields_length ); */

      if ( (*t_sendbuf).current >= (*t_sendbuf).length-2 ) { 
	fprintf (stderr, "Error: Template Sendbuffer too small to handle %i entries!\n", (*t_sendbuf).current ); 
	return -1;
      } 

      ((*t_sendbuf).entries[ (*t_sendbuf).current ]).iov_base = (*exporter).template_arr[i].template_fields; 
      ((*t_sendbuf).entries[ (*t_sendbuf).current ]).iov_len =  (*exporter).template_arr[i].fields_length; 
      (*t_sendbuf).current++; 
      // total_length += (*exporter).template_arr[i].fields_length; 
      (*t_sendbuf).commited_data_length +=  (*exporter).template_arr[i].fields_length; 
    }
  } // end loop over all templates
  
  // generate a header 
  // prepend a header to the sendbuffer
  // ret = ipfix_prepend_header (exporter,  (*t_sendbuf).commited_data_length, t_sendbuf);
  // done by send!
  
  // that's it!
  return 0;
}


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
int ipfix_send(ipfix_exporter* exporter){

  int ret;
  int i;
  // determine, if we need to send the template data:
  time_t time_now = time(NULL);

  // has the timer expired?
  if ( (time_now - (*exporter).last_template_transmition_time) >  (*exporter).template_transmition_timer) { 
    
    // send the template date
    
    // hope, the template sendbuffer is valid.
    
    // update the sendbuffer header, as we must set the export time & sequence number!
    ret = ipfix_prepend_header (exporter,  (*(*exporter).template_sendbuffer).commited_data_length,(*exporter).template_sendbuffer);

    (*exporter).last_template_transmition_time = time_now;
    

    // send the sendbuffer to all collectors
    for (i = 0; i < (*exporter).collector_max_num; i++) {
      // is the collector a valid target?
      if ((*exporter).collector_arr[i].valid) {
	//printf ("Sending template to exporter %s \n", (*exporter).collector_arr[i].ipv4address);
	ret = writev ((*exporter).collector_arr[i].data_socket, (*(*exporter).template_sendbuffer).entries, 
		      (*(*exporter).template_sendbuffer).current);
	// TODO: we should also check, what writev returned. NO ERROR HANDLING IMPLEMENTED YET!
	
      }
    } // end exporter loop

  } // end if export template.



  // send the current data_sendbuffer:
  int data_length =0;
  
  // is there data to send?
  if (  (*(*exporter).data_sendbuffer).commited_data_length > 0 ) {
    data_length =  (*(*exporter).data_sendbuffer).commited_data_length;

    // prepend a header to the sendbuffer
    ret = ipfix_prepend_header (exporter, data_length, (*exporter).data_sendbuffer);
    
    
    // send the sendbuffer to all collectors
    for (i = 0; i < (*exporter).collector_max_num; i++) {
      // is the collector a valid target?
      if ((*exporter).collector_arr[i].valid) {
	//printf ("Sending to exporter %s \n", (*exporter).collector_arr[i].ipv4address);

	// debugging output of data buffer:
	//printf ("Sendbuffer contains %u bytes\n",  (*(*exporter).data_sendbuffer).commited_data_length );
	//printf ("Sendbuffer contains %u fields\n",  (*(*exporter).data_sendbuffer).current );
	int tested_length = 0;
	int j;
	int k;
	for (j =0; j <  (*(*exporter).data_sendbuffer).current; j++) {
	  if ( (*(*exporter).data_sendbuffer).entries[j].iov_len > 0 ) {
	    tested_length +=  (*(*exporter).data_sendbuffer).entries[j].iov_len;
	    //printf ("Data Buffer  [%i] has %u bytes\n", j,  (*(*exporter).data_sendbuffer).entries[j].iov_len);
	    for (k =0 ; k<  (*(*exporter).data_sendbuffer).entries[j].iov_len; k++) {
	      /*printf ("Data at  buf_vector[%i] pos %i is %u \n", j,k,   *(  (char*) ( (*(*exporter).data_sendbuffer).entries[j].iov_base+k) ) );*/
	      
	   
	    }
	  }
      
	}
	//printf ("Sendbuffer really contains %u bytes ! \n", tested_length );

	ret = writev ((*exporter).collector_arr[i].data_socket, (*(*exporter).data_sendbuffer).entries, 
		      (*(*exporter).data_sendbuffer).current);
	// TODO: we should also check, what writev returned. NO ERROR HANDLING IMPLEMENTED YET!
	
      }
    } // end exporter loop
  }  // end if   

  // reset the sendbuffer
  ret =  ipfix_reset_sendbuffer ((*exporter).data_sendbuffer);
  

  // update the transmission counters.
  (*exporter).sequence_number++;

  // actually, this should return some error handling from writev.
  // TODO
  return 0;


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
int ipfix_start_data_set(ipfix_exporter* exporter, uint16_t* template_id){
  // obtain locks
  // TODO!

  // check, if there is enough space in the data set buffer
  // TODO.

  // write the template id to the data set buffer:
  // TODO: i know, copy is not that good, but I need a working verions first.
  memcpy ( (*(*(*exporter).data_sendbuffer).set_manager).set_header_store, template_id, sizeof(uint16_t) );
  (*(*(*exporter).data_sendbuffer).set_manager).set_header_length = 2;

  // save the iovec slot for the header 
  (*(*(*exporter).data_sendbuffer).set_manager).header_iovec 
    = &( (*(*exporter).data_sendbuffer).entries[(*(*exporter).data_sendbuffer).current ]   ); 

  (*(*exporter).data_sendbuffer).current++;
  //printf ("start_data_set:   (*(*exporter).data_sendbuffer).current %i\n",   (*(*exporter).data_sendbuffer).current );

  // initialize the counting of the record's data:
  (*(*(*exporter).data_sendbuffer).set_manager).data_length = 0;
  return 0;
}

// gerhard: wofr brauchen wir die L�ge in host byte order?
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
int ipfix_end_data_set(ipfix_exporter* exporter){
  // write the correct header data:

  char* p_base = (*(*(*exporter).data_sendbuffer).set_manager).set_header_store;
  char* p_pos = p_base;
  char* p_end = p_base + (*(*(*exporter).data_sendbuffer).set_manager).set_header_capacity;

  // calculate the total lenght of the record: 
  uint16_t record_length = (*(*(*exporter).data_sendbuffer).set_manager).data_length;
  record_length += IPFIX_MAX_SET_HEADER_LENGTH;

  // we already wrote the set id:
  p_pos +=  2;
  
  // write the length field:
  write_unsigned16 ( &p_pos, p_end, record_length );


  // update the sendbuffer
  (*(*exporter).data_sendbuffer).commited_data_length +=  record_length;

  // commit the header to the iovec
  (*(*(*(*exporter).data_sendbuffer).set_manager).header_iovec).iov_base = p_base;
  (*(*(*(*exporter).data_sendbuffer).set_manager).header_iovec).iov_len = IPFIX_MAX_SET_HEADER_LENGTH; 
  return 0;
}

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

// gerhard: bei Template alles in Host-Byte-Order. Hier koeen wir auf IOVecs verzichten und die
// Felder direkt hintereinander in den Buffer schreiben. Dabei wandeln wir jeweils in Network-Byte-Order 
// um.


/*
 * Will allocate memory and stuff for a new template
 * End_data_template set will add this template to the exporter
 */
int ipfix_start_template_set (ipfix_exporter* exporter, uint16_t template_id,  uint16_t field_count){
  // are we updating an existing template?
#ifdef DEBUG
  printf ("ipfix_start_template_set: start\n");
#endif

  int i;
  int searching;
  int found_index = -1;
  found_index = ipfix_find_template(exporter, template_id, COMMITED);
  
  // have we found a template?
  if (found_index >= 0) {
#ifdef DEBUG
    printf ("ipfix_start_template_set: template found at index %i\n, found_index");
#endif
    // we must overwrite the old template.
    // first, clean up the old template:
    
     // remove the old template from memory! 
    (*exporter).template_arr[found_index].valid == UNUSED;
    free ((*exporter).template_arr[found_index].template_fields) ;
    (*exporter).template_arr[found_index].template_fields= NULL;
  } else {   // allocate a new, free slot.
#ifdef DEBUG
  printf ("ipfix_start_template_set: making new template\n");
#endif
    searching = TRUE;
    
    // check if there is a free slot at all:
    if (( *exporter).ipfix_lo_template_current_count >= (*exporter).ipfix_lo_template_maxsize ) {
      fprintf (stderr, "No more free slots for new templates available.\n");
      fprintf (stderr, "Template not added!\n");
      
      // do error handling:
      found_index = -1;
      searching = FALSE;
      return -1;
    }   
    
    // search for a free slot:
#ifdef DEBUG
    printf ("ipfix_start_template_set: found_index: %i,  searching: %i, maxsize: %i \n", found_index, searching ,(*exporter).ipfix_lo_template_maxsize);
#endif

    i = 0;
    
    while (searching && ( i< (*exporter).ipfix_lo_template_maxsize) ) {
  
      if ( (*exporter).template_arr[i].valid == UNUSED ) {
	// we have found a free slot:

	// increase total number of templates.
	(*exporter).ipfix_lo_template_current_count ++;
	searching = FALSE;
	found_index = i;
	(*exporter).template_arr[i].template_fields = NULL;
	// TODO: maybe check, if this field is not null. Might only happen, when 
	// asynchronous threads change the template fields.
#ifdef DEBUG
	  printf ("ipfix_start_template_set: free slot found at %i \n", found_index);
#endif

      }

      // prepare for next loop:
      i++;

    }


  } // end if allocating new slot

  // initialize the slot:
  
   // test for a valid slot:
  if ( (found_index >= 0 ) && ( found_index < (*exporter).ipfix_lo_template_maxsize ) ) {
#ifdef DEBUG
  printf ("ipfix_start_template_set: initializing new slot\n");
#endif
    // allocate memory for the template's fields:
    // maximum length of the data: 8 bytes / field, as each field contains:
    // field type, field length (2*2bytes)
    // and may contain an Enterprise Number (4 bytes)
    // also, reserve 8 bytes space for the header!

    (*exporter).template_arr[found_index].max_fields_length = 8 * field_count + 8;
#ifdef DEBUG
    printf ("(*exporter).template_arr[found_index].max_fields_length %u \n", (*exporter).template_arr[found_index].max_fields_length);
#endif
    (*exporter).template_arr[found_index].fields_length = 8;
    
    (*exporter).template_arr[found_index].template_fields = (char*) malloc (   (*exporter).template_arr[found_index].max_fields_length );
    
    // initialize the rest:
    (*exporter).template_arr[found_index].valid = UNCLEAN;
    // TODO FIXME : only TRUE and FALSE don't seem to suffice!
    //    (*exporter).template_arr[found_index].valid = FALSE;
    (*exporter).template_arr[found_index].template_id = template_id;
    (*exporter).template_arr[found_index].field_count = field_count;

    // also, write the template header fields into the buffer (except the lenght field);
   
    //  int ret;
  char* p_pos;
  char* p_end;

  // beginning of the buffer
  p_pos =  (*exporter).template_arr[found_index].template_fields;
  // end of the buffer
  p_end = p_pos +  (*exporter).template_arr[found_index].max_fields_length;
  // add offset to the buffer's beginning: this is, where we will write to.
  //  p_pos +=  (*exporter).template_arr[found_index].fields_length;

  // set ID is 2 for a template:
  write_unsigned16 (&p_pos, p_end, 2);
  // write 0 to the lenght field; this will be overwritten with end_template
  write_unsigned16 (&p_pos, p_end, 0);
  // write the template ID:
  write_unsigned16 (&p_pos, p_end, template_id);
  // write the field count:
  write_unsigned16 (&p_pos, p_end, field_count);

  // does this work?
  // (*exporter).template_arr[found_index].fields_length += 8;
#ifdef DEBUG  
  printf ("ipfix_start_template_set: max_fields_len %u \n", (*exporter).template_arr[found_index].max_fields_length);
  printf ("ipfix_start_template_set: fieldss_len %u \n", (*exporter).template_arr[found_index].fields_length);
#endif
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
int ipfix_start_options_template_set(ipfix_exporter* exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length);

/*
 * Append field to the exporter's current template set
 * Parameters:
 *  length: length of the field or scope (in host byte order)
 *  type: field or scope type (in host byte order)
 *  enterprise: enterprise type (if zero, the enterprise field is omitted) (in host byte order)
 * Note: This function is called after ipfix_start_data_template_set or ipfix_start_option_template_set.
 * Note: This function MAY be replaced by a macro in future versions.
 */
int ipfix_put_template_field(ipfix_exporter* exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id) {

  // find the corresponding template
  int i=0;
  int found_index = -1;
  found_index = ipfix_find_template(exporter, template_id,  UNCLEAN);

 
  // test for a valid slot:
  if ( (found_index < 0 ) || ( found_index >= (*exporter).ipfix_lo_template_maxsize ) ) {
    fprintf (stderr, "Template not found. \n");
    return -1;
  }



  // set pointers to the buffer:

  int ret;
  char* p_pos;
  char* p_end;

  // beginning of the buffer
  p_pos =  (*exporter).template_arr[found_index].template_fields;
  // end of the buffer
  p_end = p_pos +  (*exporter).template_arr[found_index].max_fields_length;

#ifdef DEBUG
  printf ("ipfix_put_template_field: template found at %i\n", found_index);
  printf ("ipfix_put_template_field: A p_pos %u, p_end %u\n", p_pos, p_end);
  printf ("ipfix_put_template_field: max_fields_len %u \n", (*exporter).template_arr[found_index].max_fields_length);
  printf ("ipfix_put_template_field: fieldss_len %u \n", (*exporter).template_arr[found_index].fields_length);
#endif

  // add offset to the buffer's beginning: this is, where we will write to.
  p_pos +=  (*exporter).template_arr[found_index].fields_length;
  
#ifdef DEBUG
  printf ("ipfix_put_template_field: B p_pos %u, p_end %u\n", p_pos, p_end);
#endif

  // now write the fields to the buffer:
  char vendor_specific = NOT_VENDOR_SPECIFIC;
  if (enterprise_id != 0) vendor_specific = VENDOR_SPECIFIC;

  // write the vendor specific bit & the field type:
  ret =  write_extension_and_fieldID (&p_pos, p_end, type, vendor_specific);
  // write the field length
  ret = write_unsigned16 (&p_pos, p_end, length);

  // add the 4 bytes to the written length:
  (*exporter).template_arr[found_index].fields_length += 4;

  if (enterprise_id != 0) { // write the vendor specific id:
    ret = write_unsigned32 (&p_pos, p_end, enterprise_id);
    (*exporter).template_arr[found_index].fields_length += 4;
  }

  return 0;

}


/* 
 * Marks the end of a template set
 * Parameters:
 *   exporter: exporting process to send the template to
 * Note: the generated template will be stored within the exporter 
 */
int ipfix_end_template_set(ipfix_exporter* exporter, uint16_t template_id ) {
  int ret;
  // find the corresponding template
  int i=0;
  int found_index = -1;
  found_index = ipfix_find_template(exporter, template_id, UNCLEAN);

 
  // test for a valid slot:
  if ( (found_index < 0 ) || ( found_index >= (*exporter).ipfix_lo_template_maxsize ) ) {
    fprintf (stderr, "Template not found. \n");
    return -1;
  }


  // reallocate the memory , i.e. free superfluous memory, as we allocated enough memory to hold 
  // all possible vendor specific IDs.
  ipfix_lo_template* itemplate =   (&(*exporter).template_arr[found_index]);

  (*itemplate).template_fields = (char*) realloc ((*itemplate).template_fields , (*itemplate).fields_length ); 
  
  // write the real length field:
  // set pointers:
  char* p_pos;
  char* p_end;
  

  // beginning of the buffer
  p_pos =  (*exporter).template_arr[found_index].template_fields;
  // end of the buffer
  p_end = p_pos +  (*exporter).template_arr[found_index].max_fields_length;
  // add offset of 2 bytes to the buffer's beginning: this is, where we will write to.
  p_pos += 2;

  // write the lenght field
  write_unsigned16 (&p_pos, p_end, (*itemplate).fields_length);
  // call the template valid
  (*itemplate).valid = COMMITED;


  // commit the template buffer to the sendbuffer
  ipfix_update_template_sendbuffer (exporter);
  return 0;

}

/*
 * removes a template set from the exporter
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 * Returns: 0 on success, -1 on failure
 */ 
int ipfix_remove_template_set(ipfix_exporter* exporter, uint16_t template_id);

//}
