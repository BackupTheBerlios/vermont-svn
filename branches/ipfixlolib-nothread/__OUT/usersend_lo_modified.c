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
   uint16_t template_id= 12345;
   uint16_t field_count = 6;

   ret =  ipfix_start_data_template_set(my_exporter, n_template_id,  n_field_count);  

   /* add fields */
   ipfix_put_template_field(my_exporter, 4, 8, 0);
   ipfix_put_template_field(my_exporter, 4, 12, 0);
   ipfix_put_template_field(my_exporter, 2, 7, 0);
   ipfix_put_template_field(my_exporter, 2, 11, 0);
   ipfix_put_template_field(my_exporter, 8, 1, 0);
   ipfix_put_template_field(my_exporter, 8, 2, 0);


   /* our template is now complete */
   ret = ipfix_end_template_set(my_exporter);

   if (ret != 0 ) fprintf (stderr, "generation of template failed!\n");
 

   /******************************************************************/
   /* Main export & measuring loop                                   */
   /******************************************************************/

   int exporting = 1;
   meter_data my_meter_data;

   // the total length of one data record matching the template (including record header):
   uint16_t my_data_record_length = hton(4+(4+4+2+2+8+8));
   uint16_t n_template_id = hton(template_id);
   
   while (exporting) {
     
     // obtain data from the metering process
     // this is entirely up to the user...
     // we suppose that data is already in network byte order
     meter_process_gimme_data ( &my_meter_data);
       
     /*************************************************************/
     /* Write a data record                                       */
     /*************************************************************/

     /* start writing a record matching our template */
     ret = ipfix_start_data_set(my_exporter, &my_data_record_length , &n_template_id);

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
