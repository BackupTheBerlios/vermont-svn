/*
 * This file is part of the ipfixlolib.
 * ? Publish it as documentation or part of lib?
 *
 * Example showing the sending process of the ipfixlolib version 0.0.2
 *
 * WARNING: THE LIBRARY IS NOT FUNCTIONAL YET! 
 * THIS DOCUMENT IS INFORMATIONAL ONLY!
 * THE REAL IMPLEMENTATION MAY BE SLIGHTLY DIFFERENT
 *
 * by Jan Petranek, University of Tuebingen
 * 2004-11-18
 * jan@petranek.de
 * The ipfixlolib will be published under the GPL or Library GPL
 * 
 */

#include <stdio.h>
#include "ipfixlolib.h"

#define MY_SOURCE_ID 70538

/*
 * for the sake of discussion, let's assume the user's metering process yields
 * the following struct of data in network byte order
 * Note: It's up to the user to supply the data. This is merely an example 
 */
typedef struct  
{
  uint32_t ip_src_addr;
  uint32_t ip_dst_addr;
  uint16_t src_port;
  uint16_t dst_port;
  uint64_t byte_count;
  uint64_t packet_count;
} meter_data;



/*
 * stupid test function
 */
int  meter_process_gimme_data ( meter_data* mdat) {
  (*mdat).ip_src_addr = htonl(0x0C17384E); // 12.34.56.78
  (*mdat).ip_dst_addr = htonl(0x01020304); // 1.2.3.4
  (*mdat).src_port = htons ("3333");
  (*mdat).dst_port = htons ("4444");
  (*mdat).byte_count = htonll ("1567490");
  (*mdat).packet_count = htonll ("37");
  return 0;
}

int main(int argc, char *argv[]) { // main

   /* print the usage */
   if (argc != 3 ) {
     fprintf(stderr, "Usage: %s <server_ip_address> <server_port>\n", argv[0]);
     exit(1);
   }

   // a variable this prototype uses for return values
   int ret;

   /*
    * Initialize an exporter process
    * Parameters: 
    *   sourceID The source ID, to which the exporter will be initialized to.
    *   exporter an ipfix_exporter* to be initialized
    */
   ipfix_exporter* my_exporter;
   ret = ipfix_init_exporter (MY_SOURCE_ID, &my_exporter);

   // some error handling:
   if (ret != 0) {
     fprintf (stderr, "ipfix_init_exporter failed!\n");
     exit (-1),
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
   // we simply use the ip address and destination port from the command line.
   ret = ipfix_add_collector(my_exporter,  argv[1], argv[2], UDP);  
   
   // error handling
   if (ret != 0) {
     fprintf (stderr, "adding collector %s:%i failed!\n", argv[1], argv[2]);
     exit (-1),
   }
   
   /* the user can associate multiple collectors to an exporter, as long as */
   /* the number of collectors does not exceed IPFIX_MAX_COLLECTORS */
   ret = ipfix_add_collector(my_exporter,  "1.2.3.4", 4711, UDP);  
   if (ret != 0) {
     fprintf (stderr, "adding collector %s:%i failed!\n", "1.2.3.4", 4711);
     exit (-1),
   }

   /*************************************************************************/
   /*  Define a template set with the ID 12345 before the exporting process starts */
   /*    The template shall contain the following fields: */
   /*    Number of field |  IPFIX name of field      |  IPFIX field ID | length of associated datatype */
   /*    --------------------------------------------------------------------------------------------- */
   /*    1.              |  sourceAddressV4          |   8             | 4 */
   /*    2.              |  destinationAddressV4     |   12            | 4 */
   /*    3.              |  transportSourcePort      |   7             | 2 */
   /*    4.              |  transportDestinationPort |   11            | 2 */
   /*    5.              |  deltaOctetCount          |   1             | 8 */
   /*    6.              |  deltaPacketCount         |   2             | 8  */
   /* as none of these fields is vendor specific, the length of the template fields is 6*4 bytes */
   /*************************************************************************/
   
   /* 
    * Start the template set
    */
   uint16_t my_template_length = 6*4;
   // the n indicates network byte order
   uint16_t n_template_id= htons (12345);
   uint16_t n_field_count = htons (6);

   ret =  ipfix_start_template_set(my_exporter, &my_template_length, &n_template_id,  &n_field_count);  

   /*    write the 1st template field: */
   /*    Number of field |  IPFIX name of field      |  IPFIX field ID | length of associated datatype */
   /*    --------------------------------------------------------------------------------------------- */
   /*    1.              |  sourceAddressV4          |   8             | 4 */
   

   /* first, we start with the ID */

   uint16_t template_field_id_1 = htons(8);
   
   ipfix_put_template_field(sizeof(uint16_t), (char*) &template_field_id_1);

   /* second, we tell the template, how long the data will be */
   uint16_t template_field_length_1 = htons(4);
   
   ipfix_put_template_field(sizeof(uint16_t), (char*) &template_field_length_1);   
   
   /* as ipfix_put_template_field only appends data, you might as well write */
   /* several fields into one larger buffer */

   char template_buffer[5*4]; /* the buffer */
   uint16_t* p_data = (uint16_t)* template_buffer; /* a pointer to the next free field */


   /*    write the 2nd template field: */
   /*    Number of field |  IPFIX name of field      |  IPFIX field ID | length of associated datatype */
   /*    --------------------------------------------------------------------------------------------- */
   /*    2.              |  destinationAddressV4     |   12            | 4 */   
   *p_data = htons(12); // field ID
   p_data++;
   
   *p_data = htons (4); // field length
   p_data++;
   
   /*    write the fields 3-6: */
   /*    Number of field |  IPFIX name of field      |  IPFIX field ID | length of associated datatype */
   /*    --------------------------------------------------------------------------------------------- */
   /*    3.              |  transportSourcePort      |   7             | 2 */
   /*    4.              |  transportDestinationPort |   11            | 2 */
   /*    5.              |  deltaOctetCount          |   1             | 8 */
   /*    6.              |  deltaPacketCount         |   2             | 8  */

   *p_data = htons(7);  // field ID of field #3
   p_data++;
   
   *p_data = htons (2); // field length  of field #3
   p_data++;
   
   *p_data = htons(11);  // field ID of field #4
   p_data++;
   
   *p_data = htons (2); // field length  of field #4
   p_data++;      
   
   *p_data = htons(1);  // field ID of field #5
   p_data++;
   
   *p_data = htons (8); // field length  of field #5
   p_data++;      

   *p_data = htons(2);  // field ID of field #6
   p_data++;
   
   *p_data = htons (8); // field length  of field #6
   p_data++;     

   /* our template is now complete */
   ret = ipfix_end_template_set(my_exporter);

   if (ret != 0 ) fprintf (stderr, "generation of template failed!\n");
 

   /******************************************************************/
   /* Main export & measuring loop                                   */
   /******************************************************************/

   int exporting = 1;
   meter_data my_meter_data;

   // the total length of one data record matching the template:
   uint16_t my_data_record_length = 4+4+2+2+8+8;
   
   while (exporting) {
     
     // obtain data from the metering process
     // this is entirely up to the user...
     meter_process_gimme_data ( &my_meter_data);
       
     /*************************************************************/
     /* Write a data record                                       */
     /*************************************************************/

     /* start writing a record matching our template */
     ret = ipfix_start_data_set(my_exporter, &my_data_record_length ,  n_template_id);

     // you may do some error handling here
     if (ret != 0) fprintf (stderr, "ipfix_start_data_set failed!\n");
     
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

     /* Note: the user may specify a large buffer, as in the template example */
     /* The user MAY (probably should) add more records, before he calls send */

     /* end writing the record */
     ret = ipfix_end_data_set(my_exporter);
     // you may do some error handling here
     if (ret != 0) fprintf (stderr, "ipfix_end_data_set failed!\n");

     /* emit packets, if necessary. Will send the templates first. */
     ret = ipfix_send(my_exporter);
     if (ret != 0) fprintf (stderr, "ipfix_send failed!\n");
     
   } // end of send loop
   

   // free resources
   ipfix_deinit_exporter (&my_exporter);

   exit(0);  
}
