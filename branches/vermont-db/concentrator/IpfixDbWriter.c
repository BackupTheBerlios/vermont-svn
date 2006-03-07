#include <string.h>
#include<stdlib.h>
#include "IpfixDbWriter.h"
#include "msg.h"

/***** Global Variables ******************************************************/

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/
/**
 * Creates a new ipfixDbWriter. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriter *createIpfixDbWriter() {
	IpfixDbWriter* ipfixDbWriter = (IpfixDbWriter*)malloc(sizeof(IpfixDbWriter));
	initializeIpfixDbWriter(ipfixDbWriter);
	msg(MSG_INFO,"Create IpfixDbWriter success");
	return ipfixDbWriter;
}

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixDbWriter(IpfixDbWriter *ipfixDbWriter) {
																				 					     
	ipfixDbWriter->conn = mysql_init(0);
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"MySQL connection handler failure");
	}
	else
	{
		msg(MSG_FATAL,"Init Connection Handler %d",ipfixDbWriter->conn);
	}

	ipfixDbWriter->host_name="localhost" ;
	ipfixDbWriter->db_name="flows" ;    		 	 
	ipfixDbWriter->user_name=0 ;    		
	ipfixDbWriter->password=0 ;    			
	ipfixDbWriter->port_num=0; 			
	ipfixDbWriter->socket_name=0 ;	  		
	ipfixDbWriter->flags=0;   	 	
	
	
	//Connect to Database with "no name (0)"
	ipfixDbWriter->conn = mysql_real_connect(ipfixDbWriter->conn,
			ipfixDbWriter->host_name, ipfixDbWriter->user_name,ipfixDbWriter->password, 0, 
			ipfixDbWriter->port_num, ipfixDbWriter->socket_name,
			ipfixDbWriter->flags);
		
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Database connection failure");
	}
	else
	{
		msg(MSG_FATAL,"Could connect to Database %d",ipfixDbWriter->conn);
	}
	/** make string query**/
	char createDb[90];
	strcpy( createDb,"CREATE DATABASE IF NOT EXISTS ");
	strcat(createDb,ipfixDbWriter->db_name);
	
	if(mysql_query(ipfixDbWriter->conn, createDb) != 0 )
	{
		msg(MSG_FATAL,"Database creation failure %d",ipfixDbWriter->conn);
	}
	else
	{
		msg(MSG_FATAL,"Create Database flows handler %d",ipfixDbWriter->conn);
		msg(MSG_FATAL,"Create Database %s",ipfixDbWriter->db_name);
	}
	/** Real connect to database with real name **/
	ipfixDbWriter->conn = mysql_real_connect(ipfixDbWriter->conn,
			ipfixDbWriter->host_name, ipfixDbWriter->user_name,ipfixDbWriter->password,
			ipfixDbWriter->db_name, ipfixDbWriter->port_num, ipfixDbWriter->socket_name,
			ipfixDbWriter->flags);
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Database connection failure");
	}
	else 
	{
		msg(MSG_FATAL,"Could Connect to Database22");
	}

	//Creat Tables of Database
	if(mysql_query(ipfixDbWriter->conn,"CREATE TABLE IF NOT EXISTS flows (protocol VARCHAR(4) NULL)") != 0)
	{
		msg(MSG_FATAL,"Database tables creation failure");
	}
	else
	{
		msg(MSG_FATAL,"Create Database Table flows handler %d",ipfixDbWriter->conn);
		msg(MSG_FATAL,"Create Database Table %s",ipfixDbWriter->db_name);
	}

	
	return 0;

	
}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbWriter(IpfixDbWriter *ipfixDbWriter) {
	mysql_close(ipfixDbWriter->conn);
	return 0;
}

/**
 * Frees memory used by an ipfixDbWriter
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int destroyIpfixDbWriter(IpfixDbWriter *ipfixDbWriter) {
	deinitializeIpfixDbWriter(ipfixDbWriter);
	free(ipfixDbWriter);
	return 0;
}

/**
 * Starts or resumes Database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
void startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses Database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
void stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}






int InsertDbFlows(void *ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
	IpfixTable *ipfixtable = (IpfixTable *)malloc(sizeof(ipfixtable));
	int i;
	for ( i=0 ; i < dataTemplateInfo->dataCount; i++)
	{
		if( dataTemplateInfo->dataInfo[i].type.id == IPFIX_TYPEID_protocolIdentifier)
		{
			switch (*(dataTemplateInfo->data +dataTemplateInfo->dataInfo[i].offset)) {
				case IPFIX_protocolIdentifier_ICMP:
					ipfixtable->protocol = "ICMP";
					if (mysql_query(((IpfixDbWriter *)ipfixDbWriter)->conn, "INSERT INTO flows (protocol) VALUES ('ICMP')") != 0)
						msg(MSG_FATAL,"Insert not successfull");
					
					return;
				case IPFIX_protocolIdentifier_TCP:
					ipfixtable->protocol = "TCP";
					if (mysql_query(((IpfixDbWriter *)ipfixDbWriter)->conn, "INSERT INTO flows (protocol) VALUES ('TCP')") != 0)
						msg(MSG_FATAL,"Insert not successfull");
					
					return;
				case IPFIX_protocolIdentifier_UDP: 
					ipfixtable->protocol = "UDP";
					if (mysql_query(((IpfixDbWriter *)ipfixDbWriter)->conn, "INSERT INTO flows (protocol) VALUES ('UDP')") != 0)
						msg(MSG_FATAL,"Insert not successfull");

					return;
				case IPFIX_protocolIdentifier_RAW: 
					ipfixtable->protocol = "RAW";
					if (mysql_query(((IpfixDbWriter *)ipfixDbWriter)->conn, "INSERT INTO flows (protocol) VALUES ('RAW')") != 0)
						msg(MSG_FATAL,"Insert not successfull");
					
					return;
				default:
					ipfixtable->protocol = "unknown protocol";
					if (mysql_query(((IpfixDbWriter *)ipfixDbWriter)->conn, "INSERT INTO flows (protocol) VALUES ('UP')") != 0)
						msg(MSG_FATAL,"Insert not successfull");
					
					return;
			}
		}
		
	}
	return 0;
}

/**
 * Prints a DataDataRecord
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */

int printDataDataRecord(void *ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	int i;

	printf("\n-+--- DataDataRecord\n");
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `- variable data\n");
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}



CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter *ipfixDbWriter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixDbWriter;
	ci.dataDataRecordCallbackFunction = InsertDbFlows;
	return ci;
}
