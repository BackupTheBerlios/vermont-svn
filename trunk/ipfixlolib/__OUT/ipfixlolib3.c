
/*
 * This file is part of the ipfixlolib.
 *
 * A low-level implementation of ipfix-functions
 *
 * by Jan Petranek, University of Tuebingen
 * 2004-11-12
 * jan@petranek.de
 * The ipfixlolib will be published under the GPL or Library GPL
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



// TODO: set up sockets for TCP and SCTP (in case it will be implemented)

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
  // TODO: write this to an own function; 
  ret = ipfix_init_collector_array( &((**exporter).collector_arr), IPFIX_MAX_COLLECTORS);
  if (ret !=0) {
    fprintf (stderr, "ipfix_init_exporter: Initializing collectors failed!\n");
    return ret;
  }

  (**exporter).collector_max_num = IPFIX_MAX_COLLECTORS;

  return 0;
}

/*
 * cleanup an exporter process
 */
int ipfix_deinit_exporter (ipfix_exporter** exporter) {
  // cleanup processes

  // close sockets etc.
  // (currently, nothing to do)
  
  // free all children

  // deinitialize the sendbuffers
  int ret;
  ret = ipfix_deinit_sendbuffer ( (&(**exporter).data_sendbuffer));
  ret = ipfix_deinit_sendbuffer ( (&(**exporter).template_sendbuffer));

  // deinitialize the collectors
  ret = ipfix_deinit_collector_array( &((**exporter).collector_arr) );

  // free own memory
  free (*exporter);

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
  if (( *exporter).collector_num >= (*exporter).collector_max_num ) {
    fprintf (stderr, "No more free slots for new collectors available.\n");
    fprintf (stderr, "Collector not added!\n");
    return -1;
  }

  // allocate free slot from the exporter
  int i=0;
  int searching = TRUE;
  while (searching && ( i< (*exporter).collector_max_num) ) {
  
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
      (*exporter).collector_arr[i].valid = TRUE;
      
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
  return 0;
}


/*
 * Deinitialize (free) an ipfix_sendbuffer
 */
int ipfix_deinit_sendbuffer (ipfix_sendbuffer** sendbuf) {
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
  // determine, if we need to send the template data:
  // TODO!
  int ret;
  int i;
  

  // send the current data_sendbuffer:
  int data_length =0;


  // prepend a header to the sendbuffer
  ret = ipfix_prepend_header (exporter, data_length, (*exporter).data_sendbuffer);


  // send the sendbuffer to all collectors
  for (i = 0; i < (*exporter).collector_max_num; i++) {
    // is the collector a valid target?
    if ((*exporter).collector_arr[i].valid) {
      printf ("Sending to exporter %s \n", (*exporter).collector_arr[i].ipv4address);
      ret = writev ((*exporter).collector_arr[i].data_socket, (*(*exporter).data_sendbuffer).entries, 
	     (*(*exporter).data_sendbuffer).current);
      // TODO: we should also check, what writev returned. NO ERROR HANDLING IMPLEMENTED YET!

    }
  }

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
  // set the template id:
  ipfix_put_data_field( (*exporter).data_sendbuffer, template_id, 2);
  (*(*exporter).data_sendbuffer).current++;
  
  // reserve a slot for the record size 
  (*(*exporter).data_sendbuffer).record_header_index =  (*(*exporter).data_sendbuffer).current;
  (*(*exporter).data_sendbuffer).current++;

  // initialize the counting of the record's data:
  // first, we add the size of the header:
  (*(*exporter).data_sendbuffer).uncommited_data_length= 0;

  // obtain locks
  // TODO!

}

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
int ipfix_end_data_set(ipfix_exporter* exporter){
  // write the correct header data:

  // write the length field:
  uint16_t* p_dat_len = (uint16_t*) malloc (sizeof uint16_t);
  *p_dat_len = htons ( (*(*exporter).data_sendbuffer).uncommited_data_length); 

  (*(*(*exporter).data_sendbuffer).record_header_index).iov_base =0;
  (*(*(*exporter).data_sendbuffer).record_header_index).iov_len = (sizeof uint16_t);

  

  // commit the data:
  (*(*exporter).data_sendbuffer).commited =  (*(*exporter).data_sendbuffer).current;
  (*(*exporter).data_sendbuffer).commited_data_length +=  (*(*exporter).data_sendbuffer).uncommited_data_length;
}

