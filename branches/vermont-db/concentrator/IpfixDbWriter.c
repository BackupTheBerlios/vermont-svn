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
IpfixDbWriter* createIpfixDbWriter() {
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
int initializeIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
																				 					     
	ipfixDbWriter->conn = mysql_init(0);
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"MySQL connection handler failure");
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
	/** make string query to create database**/
	char createDb[90];
	strcpy( createDb,"CREATE DATABASE IF NOT EXISTS ");
	strcat(createDb,ipfixDbWriter->db_name);
	
	if(mysql_query(ipfixDbWriter->conn, createDb) != 0 )
	{
		msg(MSG_FATAL,"Database creation failure %d",ipfixDbWriter->conn);
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
	msg(MSG_FATAL,"Database handler in connect%d",ipfixDbWriter->conn);
	//printf("connection handler in connect %d\n",ipfixDbWriter->conn);
	//Creat tables of database
	if(mysql_query(ipfixDbWriter->conn,"CREATE TABLE IF NOT EXISTS flows (protocol VARCHAR(4) NULL )") != 0)//, srcPort SMALLINT UNSIGNED NULL, srcIP VARCHAR(18) NULL, destPort SMALLINT UNSIGNED NULL, destIP VARCHAR(18) NULL, tcpctrlbits SMALLINT UNSIGNED NULL, packetdeltacount SMALLINT UNSIGNED NULL,octetdeltacount INT UNSIGNED NULL, flowstartsseconds BIGINT UNSIGNED NULL, flowendsseconds BIGINT UNSIGNED NULL
	{
		msg(MSG_FATAL,"Creation of database tables failure");
	}
	return 0;
}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	mysql_close(ipfixDbWriter->conn);
	return 0;
}

/**
 * Frees memory used by an ipfixDbWriter
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int destroyIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	deinitializeIpfixDbWriter(ipfixDbWriter);
	free(ipfixDbWriter);
	return 0;
}

/**
 * Starts or resumes database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
void startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
void stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

int InsertDbFlows(void* ipfixDbWrite, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
	IpfixTable* ipfixtable = (IpfixTable*)malloc(sizeof(IpfixTable)) ;
	IpfixDbWriter * ipfixDbWriter = ipfixDbWrite;
	int i =0;
	for ( i=0 ; i < dataTemplateInfo->dataCount; i++){
		if( dataTemplateInfo->dataInfo[i].type.id == IPFIX_TYPEID_protocolIdentifier){
			getProtocol(ipfixtable, dataTemplateInfo->fieldInfo[i].type,dataTemplateInfo->data+dataTemplateInfo->dataInfo[i].offset);
		}
	}
	if(ipfixtable->protocol == "ICMP"){
		ipfixtable->sourcePort = 0;
		ipfixtable->sourceIP = 0;
		ipfixtable->destinationPort = 0;
		ipfixtable->destinationIP = 0;
		ipfixtable->TCPcontrolBits = 0;

		for (i=0; i < dataTemplateInfo->fieldCount; i++){
			printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
			printf("\n");
			
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_packetDeltaCount){
				ipfixtable->packetdeltacount = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}			
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_octetDeltaCount){
				ipfixtable->octetdeltacount = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}						
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_flowStartSeconds){
				ipfixtable->flowstartsseconds = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}									
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_flowStartSeconds){
				ipfixtable->flowendsseconds = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}												
		}
		printf("protocol : %s\n", ipfixtable->protocol);
		printf("Packetdeltacount : %d\n", ipfixtable->packetdeltacount);
		printf("Oktetdeltacount : %d\n", ipfixtable->octetdeltacount);
		printf("flowstart  : %Lu\n", ipfixtable->flowstartsseconds);
		printf("flowend  : %Lu\n", ipfixtable->flowendsseconds);
	}
	
	if(ipfixtable->protocol == "TCP"){
		for (i=0; i < dataTemplateInfo->fieldCount; i++){
			printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
			printf("\n");

			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_sourceTransportPort ){
				ipfixtable->sourcePort = getTransportPort(dataTemplateInfo->fieldInfo[i].type,(data+dataTemplateInfo->fieldInfo[i].offset));
			}
			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_sourceIPv4Address){
				getipv4address(ipfixtable, dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}		
			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_destinationTransportPort ){
				ipfixtable->destinationPort = getTransportPort(dataTemplateInfo->fieldInfo[i].type,(data+dataTemplateInfo->fieldInfo[i].offset));
			}
			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_destinationIPv4Address){
				getipv4address(ipfixtable,dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}	
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_tcpControlBits){
				ipfixtable->TCPcontrolBits = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_packetDeltaCount){
				ipfixtable->packetdeltacount = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}			
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_octetDeltaCount){
				ipfixtable->octetdeltacount = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}						
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_flowStartSeconds){
				ipfixtable->flowstartsseconds = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}									
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_flowStartSeconds){
				ipfixtable->flowendsseconds = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}												
		}
		//make query string
		if(mysql_query( ipfixDbWriter->conn,"INSERT INTO flows (protocol) VALUES ('TCP')")  != 0)
		{
			msg(MSG_FATAL,"Insert of record failure");
		}
		printf("protocol : %s\n", ipfixtable->protocol);
		printf("source Port : %d\n", ipfixtable->sourcePort);
		printf("source Address  :%s\n",ipfixtable->sourceIP);
		printf("desti Port : %d\n", ipfixtable->destinationPort);
		printf("desti Address : %s\n", ipfixtable->destinationIP);
		printf("TCP controlBits : %d\n", ipfixtable->TCPcontrolBits);
		printf("Packetdeltacount : %d\n", ipfixtable->packetdeltacount);
		printf("Oktetdeltacount : %d\n", ipfixtable->octetdeltacount);
		printf("flowstart  : %Lu\n", ipfixtable->flowstartsseconds);
		printf("flowend  : %Lu\n", ipfixtable->flowendsseconds);
	}	

	if(ipfixtable->protocol == "UDP"){
		ipfixtable->TCPcontrolBits = 0;
		for (i=0; i < dataTemplateInfo->fieldCount; i++){
			
			printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
			printf("\n");

			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_sourceTransportPort ){
				ipfixtable->sourcePort = getTransportPort(dataTemplateInfo->fieldInfo[i].type,(data+dataTemplateInfo->fieldInfo[i].offset));
			}
			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_sourceIPv4Address){
				getipv4address(ipfixtable, dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}		
			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_destinationTransportPort ){
				ipfixtable->destinationPort = getTransportPort(dataTemplateInfo->fieldInfo[i].type,(data+dataTemplateInfo->fieldInfo[i].offset));
			}
			if(dataTemplateInfo->fieldInfo[i].type.id == IPFIX_TYPEID_destinationIPv4Address){
				getipv4address(ipfixtable,dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}	
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_packetDeltaCount){
				ipfixtable->packetdeltacount = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}			
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_octetDeltaCount){
				ipfixtable->octetdeltacount = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}						
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_flowStartSeconds){
				ipfixtable->flowstartsseconds = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}									
			if(dataTemplateInfo->fieldInfo[i].type.id ==  IPFIX_TYPEID_flowStartSeconds){
				ipfixtable->flowendsseconds = getdata(dataTemplateInfo->fieldInfo[i].type, (data+dataTemplateInfo->fieldInfo[i].offset));
			}												
		}
		printf("protocol : %s\n", ipfixtable->protocol);
		printf("source Port : %d\n", ipfixtable->sourcePort);
		printf("source Address  :%s\n",ipfixtable->sourceIP);
		printf("desti Port : %d\n", ipfixtable->destinationPort);
		printf("desti Address : %s\n", ipfixtable->destinationIP);
		printf("Packetdeltacount : %d\n", ipfixtable->packetdeltacount);
		printf("Oktetdeltacount : %d\n", ipfixtable->octetdeltacount);
		printf("flowstart  : %Lu\n", ipfixtable->flowstartsseconds);
		printf("flowend  : %Lu\n", ipfixtable->flowendsseconds);
	}	
	
	if(ipfixtable->protocol == "RAW"){
		ipfixtable->sourcePort = 0;
		ipfixtable->sourceIP = 0;
		ipfixtable->destinationPort = 0;
		ipfixtable->destinationIP = 0;
		ipfixtable->TCPcontrolBits = 0;
		ipfixtable->packetdeltacount = 0;
		ipfixtable->octetdeltacount = 0;
		ipfixtable->flowstartsseconds = 0;
		ipfixtable->flowendsseconds= 0;
	}
	if(ipfixtable->protocol == "UP"){
		ipfixtable->sourcePort = 0;
		ipfixtable->sourceIP = 0;
		ipfixtable->destinationPort = 0;
		ipfixtable->destinationIP = 0;
		ipfixtable->TCPcontrolBits = 0;
		ipfixtable->packetdeltacount = 0;
		ipfixtable->octetdeltacount = 0;
		ipfixtable->flowstartsseconds = 0;
		ipfixtable->flowendsseconds= 0;
	}
	
	free(ipfixtable);
	return 0;
}

void getProtocol(IpfixTable* ipfixtable,FieldType type, FieldData* data)
{
	//char *protoc = (char*)malloc(5*sizeof(char));
	static char buffer[5];
	char* protoc = buffer; 
	switch (data[0]) {
		case IPFIX_protocolIdentifier_ICMP:
			protoc = "ICMP";
			printf("ICMP\n");
			ipfixtable->protocol = protoc;
			break;
		case IPFIX_protocolIdentifier_TCP:
			protoc = "TCP";
			printf("TCP\n");
			ipfixtable->protocol = protoc;
			break;
		case IPFIX_protocolIdentifier_UDP:
			protoc = "UDP";
			printf("UDP\n");
			ipfixtable->protocol = protoc; 
			break;
		case IPFIX_protocolIdentifier_RAW:
			protoc = "RAW";
			printf("RAW\n");
			ipfixtable->protocol = protoc;
			break;
		default:
			protoc = "UP";
			printf("UP\n");
			ipfixtable->protocol = protoc;
			break;
	}
}
 
uint16_t getTransportPort(FieldType type, FieldData* data)
{
	if (type.length == 0) {
		printf("zero-length Port");
		return 0;
	}
	if (type.length == 2) {
		uint16_t port = ((uint16_t)data[0] << 8)+data[1];

		if(type.id == IPFIX_TYPEID_sourceTransportPort){
			return port;
		}
		if(type.id == IPFIX_TYPEID_destinationTransportPort){
			return port;
		}
	}
	printf("Port with length %d unparseable", type.length);
	return 0;
}

void getipv4address(IpfixTable* ipfixtable, FieldType type, FieldData* data)
{
	//char *octetstring = (char *)malloc(25*sizeof(char));	
	static char buffer[25];
	char* octetstring = buffer;
	int octet1 = 0;
	int octet2 = 0;
	int octet3 = 0;
	int octet4 = 0;
	int imask = 0;
	if (type.length >= 1) octet1 = data[0];
	if (type.length >= 2) octet2 = data[1];
	if (type.length >= 3) octet3 = data[2];
	if (type.length >= 4) octet4 = data[3];
	if (type.length >= 5) imask = data[4];
	
	if (type.length > 5) {
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return;
	}
	/**Create octetstring from integers**/
	if ((type.length == 5) && ( type.id == IPFIX_TYPEID_sourceIPv4Address)) /*&& (imask != 0)*/ {
		sprintf(octetstring,"%d.%d.%d.%d/%d",octet1,octet2,octet3,octet4,32-imask);
		ipfixtable->sourceIP = octetstring;
		return;
	}
	if ((type.length == 5) && (type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask != 0)*/ {
		sprintf(octetstring,"%d.%d.%d.%d/%d",octet1,octet2,octet3,octet4,32-imask);
		ipfixtable->destinationIP = octetstring;
		return;
	}
	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_sourceIPv4Address)) /*&& (imask == 0)*/ {
		sprintf(octetstring,"%d.%d.%d.%d",octet1,octet2,octet3,octet4);
		ipfixtable->destinationIP = octetstring;
		return ;
	}	
	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask == 0)*/ {
		sprintf(octetstring,"%d.%d.%d.%d",octet1,octet2,octet3,octet4);
		ipfixtable->sourceIP = octetstring;
		return;
	}
	return; 
}

long long getdata(FieldType type, FieldData* data)
{ 		 
	if(type.id ==  IPFIX_TYPEID_tcpControlBits){
		int controlBits = getdataValue(type, data);
		return controlBits;
	}
	if(type.id ==  IPFIX_TYPEID_packetDeltaCount){
		uint16_t packetdeltacount = getdataValue(type, data);
		return packetdeltacount;
	}
	if(type.id ==  IPFIX_TYPEID_octetDeltaCount){
		uint octetdeltacount = getdataValue(type, data);
		return octetdeltacount;
	}		
	if(type.id ==  IPFIX_TYPEID_flowStartSeconds){
		long long flowstartssecond = getdataValue(type, data);
		return flowstartssecond;
	}				
	if(type.id ==  IPFIX_TYPEID_flowStartSeconds){
		long long flowendssecond = getdataValue(type, data);
		return flowendssecond;
	}						
	return 0;
}

long long getdataValue(FieldType type, FieldData* data)
{
	switch (type.length) {
		case 1:
			return  (*(uint8_t*)data);
		case 2:
			return ntohs(*(uint16_t*)data);
		case 4:
			return ntohl(*(uint32_t*)data);
		case 8:
			return ntohll(*(uint64_t*)data);
		default:
			printf("Uint with length %d unparseable\n", type.length);
			return 0;
	}
}

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter *ipfixDbWriter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixDbWriter;
	ci.dataDataRecordCallbackFunction = InsertDbFlows;
	return ci;
}
