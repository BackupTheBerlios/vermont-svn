#include <string.h>
#include<stdlib.h>
#include "IpfixDbWriter.h"
#include "msg.h"



/***** Global Variables ******************************************************/
/**
* 	is needed to determine the realtime and the time of flowstartsseconds
*/
struct tm* time_now;



/**
*	column names as a array of char pointer
*/
char *columns_names[] = {"srcIP", "dstIP", "srcPort", "dstPort", "proto", "dstTos","bytes","pkts","firstSwitched","lastSwitched","exporterID",0};					  
 
 /**
 *	struct to identify column names with IPFIX_TYPEID an the datatype to store the result in database
 */
struct column identify [] = {		 
	{"dstIP", IPFIX_TYPEID_destinationIPv4Address, "INTEGER(10) UNSIGNED",0},
	{"srcIP", IPFIX_TYPEID_sourceIPv4Address, "INTEGER(10) UNSIGNED", 0},	
	{"srcPort", IPFIX_TYPEID_sourceTransportPort,  "SMALLINT(5) UNSIGNED", 0},	
	{"dstPort", IPFIX_TYPEID_destinationTransportPort,  "SMALLINT(5) UNSIGNED",0},
	{"proto",IPFIX_TYPEID_protocolIdentifier , "TINYINT(3) UNSIGNED", 0},
	{"dstTos", IPFIX_TYPEID_classOfServiceIPv4, "TINYINT(3) UNSIGNED", 0},
	{"bytes", IPFIX_TYPEID_octetDeltaCount,  "BIGINT(20) UNSIGNED", 0},
	{"pkts", IPFIX_TYPEID_packetDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{"firstSwitched", IPFIX_TYPEID_flowStartSeconds,  "INTEGER(10) UNSIGNED", 0},
	{"lastSwitched", IPFIX_TYPEID_flowEndSeconds,  "INTEGER(10) UNSIGNED", 0},
	{"exporterID",IPFIX_TYPEID_exporterIPv4Address, "SMALLINT(5) UNSIGNED", 0},
	{"END"}
} ;
	

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixDbWriter() {
	return 0;
}
																				 					     
/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbWriter() {
	return 0;
}																				 					     
												 					     
/**
 * Creates a new ipfixDbWriter. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriter* createIpfixDbWriter() 
{	
	IpfixDbWriter* ipfixDbWriter = (IpfixDbWriter*)malloc(sizeof(IpfixDbWriter));
	Statement* statemen = (Statement*)malloc(sizeof(Statement));
	Table* tabl = (Table*)malloc(sizeof(Table));
	ipfixDbWriter->conn = mysql_init(0);  /** get the mysl init handle*/
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Getting MySQL connecting handle failed");
	}
	else
	{
		msg(MSG_DEBUG,"Getting MySQL init handler");
	}
	/**Initialize structure members IpfixDbWriter*/
	ipfixDbWriter->host_name = "localhost" ;
	ipfixDbWriter->db_name = "flows";
	ipfixDbWriter->user_name = 0 ;    		
	ipfixDbWriter->password = 0 ;    			
	ipfixDbWriter->port_num = 0; 			
	ipfixDbWriter->socket_name = 0 ;	  		
	ipfixDbWriter->flags = 0;
	/**Initialize structure members Statement*/	   	 	
	ipfixDbWriter->statement = statemen;
	statemen->statemReceived = 0;
	statemen->statemBuffer2 = 0;
	/**Initialize structure members Table*/	  
	ipfixDbWriter->table = tabl ;
	tabl->countbufftable = 0;
	/**Init the buffer of the table names*/
	int i;
	for(i = 0; i < maxTable; i++)
		tabl->tablebuffer[i] = "NULL";
	

	/**Connect to Database*/
	ipfixDbWriter->conn = mysql_real_connect(ipfixDbWriter->conn,
			ipfixDbWriter->host_name, ipfixDbWriter->user_name,ipfixDbWriter->password,
			0, ipfixDbWriter->port_num, ipfixDbWriter->socket_name,
			ipfixDbWriter->flags);
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Connection to database failed");
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
	}
	else
	{
		msg(MSG_DEBUG,"Connect to database");		
	}
	
	
	/** create Database*/
	if(createDB(ipfixDbWriter) !=0)
	{
		msg(MSG_DEBUG,"Create database function success");
	}
	return ipfixDbWriter;
}

/**
 * Frees memory used by an ipfixDbWriter
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int destroyIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	deinitializeIpfixDbWriter();
	mysql_close(ipfixDbWriter->conn);
	free(ipfixDbWriter);
	return 0;
}

/**
 * Starts or resumes database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int  startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}
/**
* create the database given by the name dbnam->dbn
*/
int createDB(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	
	/** make query string to create database**/
	char  createDbStr[start_len] ;
	strcpy(createDbStr,"CREATE DATABASE IF NOT EXISTS ");
	strcat(createDbStr,ipfixDbWriter->db_name);
	/**create database*/
	if(mysql_query(ipfixDbWriter->conn, createDbStr) != 0 )
	{
		msg(MSG_FATAL,"Creation of database %s failed", ipfixDbWriter->db_name);
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
	}
	else
	{
		msg(MSG_DEBUG,"Database %s created",ipfixDbWriter->db_name);
	}
	/** use database  with db_name**/	
	if(mysql_select_db(ipfixDbWriter->conn, ipfixDbWriter->db_name) !=0)
	{
		msg(MSG_FATAL,"Database %s not selectable", ipfixDbWriter->db_name);	
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));	
	}
	else
	{
		msg(MSG_DEBUG,"Database %s selected", ipfixDbWriter->db_name);
	}
	return 0;
}

/**
* 	Create the table of the database
*/
int createDBTable(IpfixDbWriter* ipfixDbWrite, char* TableName)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	int i, j, count_col ;
	count_col = countColumns();
	char createTableStr[start_len+(count_col* col_width)];
	strcpy(createTableStr , "CREATE TABLE IF NOT EXISTS ");

	printf("TableName %s\n",TableName);
	strcat(createTableStr,TableName);
	strcat(createTableStr," (");
	/**collect the names for columns and the datatypes for the table in a string*/
	for(i=0; i < count_col; i++)
	{
		for(j=0; identify[j].cname != "END";j++)
		{
			if( columns_names[i] == identify[j].cname)
			{
				strcat(createTableStr,identify[j].cname);
				strcat(createTableStr," ");
				strcat(createTableStr,identify[j].datatype);
				if( i  != count_col-1)
				{
					strcat(createTableStr,",");
				}
			}
		}
	}
	strcat(createTableStr,")");
	/** create table*/
	if(mysql_query(ipfixDbWriter->conn,createTableStr) != 0)
	{
		msg(MSG_FATAL,"Creation of table failed");
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
	}
	else
	{
		msg(MSG_DEBUG, "Creation of table success");
	}
	return 0;
}

/** 
 *	The tablename according to the time of the records when the flow is started
 *	The result is given by "h_YYYYMMDD_HH_0 || 1"
 *	0, when the recordtime of min is  0 <= min < 30, otherwise 1
 */
 char* getTableName(IpfixDbWriter* ipfixDbWrite,Table*  tabl,  uint64_t flowstartsec)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	Table* table = tabl;
	int i = 0;
	static char tablename[16];
	char strtmp[5];
	time_t  t = flowstartsec;
	time_now = localtime(&t);
	strcpy(tablename,"h_");
	sprintf(strtmp,"%u",time_now->tm_year+1900);
	strcat(tablename,strtmp);
	sprintf(strtmp,"%02u",time_now->tm_mon+1);
	strcat(tablename,strtmp);
	sprintf(strtmp,"%02u",time_now->tm_mday);
	strcat(tablename,strtmp);
	strcat(tablename,"_");
	sprintf(strtmp,"%02u",time_now->tm_hour);
	strcat(tablename,strtmp);
	strcat(tablename,"_");
	sprintf(strtmp,"%u",time_now->tm_min<30?0:1);
	strcat(tablename,strtmp);
	printf("tablename %s\n",tablename);
		
	/** Is tablename in tablebuffer ?*/
	while( i < maxTable)
	{
		if(strcmp(table->tablebuffer[i], tablename) == 0)
		{
			msg(MSG_DEBUG,"Table %s is in tablebuffer", table->tablebuffer[i]);
			return table->tablebuffer[i];
		}
		i++;
	}
	/**Tablename is not in tablebuffer*/	
	msg(MSG_DEBUG,"Table %s is in not in tablebuffer", tablename);
	char* tableN = (char*)malloc((strlen(tablename)+1)*sizeof(char));
	strcpy(tableN, tablename);
	/**there are no pointer in first bufferentries to be freed*/
	if(table->tablebuffer[table->countbufftable] != "NULL")
	{
		free(table->tablebuffer[table->countbufftable]);
		msg(MSG_DEBUG,"free table buffer %d ",table->countbufftable);
	}
	table->tablebuffer[table->countbufftable] = tableN;
	/** createTable when not in buffer*/
	if(createDBTable(ipfixDbWriter, tableN) != 0)
	{	
		msg(MSG_DEBUG,"Table creation function success");
	}
	
	
	/** If end of buffer reached ?  Begin from  the start*/  
	if(table->countbufftable < maxTable-1)
	{
		table->countbufftable++;
	}
	else
	{
		table->countbufftable = 0;
	}
	
	for(i = 0; i < maxTable; i++)
	{
		msg(MSG_DEBUG,"Tables in the TableBuffer : %s", table->tablebuffer[i]);
	}
		
	return tableN;
}

int getsingleRecData(void* ipfixDbWrite, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	Table *tabl = ipfixDbWriter->table;
	Statement* statemen = ipfixDbWriter->statement;
	int i ,j, k, n;
	uint64_t intdata = 0;
	uint64_t flowstartsec = 0;
	int count_col = 0;
	count_col = countColumns();
	/** if the writeToDb process not record - drop record*/
	if(statemen->statemBuffer2 != NULL)
	{
		msg(MSG_FATAL,"Drop datarecord - still writing to database");
		return 0;
	}
	
	/**begin query string for insert statement*/
	char insert[start_len+(count_col * ins_width)];
	strcpy(insert,"INSERT INTO ");
	/**make string for the column names*/
	char ColNames[count_col * ins_width];
	strcpy(ColNames," (");
	/**make string for the values  given by the IPFIX_TYPEID stored in the record*/ 
	char ColValues[count_col * ins_width]; 
	strcpy(ColValues," VALUES (");	
	
	char stringtmp[ins_width];// needed to cast from char to string
			
	/** loop over the IPFIX_TYPEID of the record  to get data to store*/
	for( i=0; i < count_col; i++)
	{
		for( j=0; identify[j].cname != "END"; j++)
		{ 
			if(columns_names[i] == identify[j].cname)
			{
				strcat(ColValues,"'");
				strcat(ColNames,columns_names[i]);
				if( i != count_col-1)
					strcat(ColNames,",");
				if( i == count_col-1)
					strcat(ColNames,") ");
				for(k=0; k < dataTemplateInfo->fieldCount; k++)
				{	
					if(dataTemplateInfo->fieldInfo[k].type.id == identify[j].ipfixid)
					{
													
						intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
						sprintf(stringtmp,"%Lu",(uint64_t)intdata);
						strcat(ColValues,stringtmp);
						strcat(ColValues,"'");
						if(identify[j].ipfixid == IPFIX_TYPEID_flowStartSeconds)
						{
							flowstartsec = intdata;			
							printf("flowstartsec  %Lu\n",flowstartsec);
						}
						if( i != count_col-1)
							strcat(ColValues,",");
						if( i ==count_col-1)
							strcat(ColValues,")");	
						break;
					}	
					else if( k == dataTemplateInfo->fieldCount-1)
					{
						for(n=0; n < dataTemplateInfo->dataCount; n++)
						{
							if(dataTemplateInfo->dataInfo[n].type.id == identify[j].ipfixid)
							{
								intdata = getdata(dataTemplateInfo->dataInfo[n].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[n].offset));
								sprintf(stringtmp,"%Lu",(uint64_t)intdata);
								strcat(ColValues,stringtmp);
								strcat(ColValues,"'");
								if( i != count_col-1)
									strcat(ColValues,",");
								if( i == count_col-1)
									strcat(ColValues,")");					
								break;
							}
							else if(n == dataTemplateInfo->dataCount-1)
							{
								intdata = getdefaultIPFIXdata(identify[j].ipfixid);
								sprintf(stringtmp,"%Lu",(uint64_t)intdata);
								strcat(ColValues,stringtmp);
								strcat(ColValues,"'");
								if( i != count_col-1)
									strcat(ColValues,",");
								if( i == count_col-1)
									strcat(ColValues,")");					
							}
						}
					}
				}
			}
		}
	}
	/**make hole query string for the insert statement*/
	char* tablename = getTableName(ipfixDbWriter, tabl, flowstartsec);
	/** tablename +  Columnsname + Values of record*/
	strcat(insert, tablename);  
	strcat(insert,ColNames);
	strcat(insert,ColValues);
	
	char* insertTableStr = (char*) malloc((strlen(insert)+1)*sizeof(char));
	strcpy(insertTableStr,insert);


	
	/** if statement counter lower as  max count of statement then insert record in statemenBuffer*/
	if((statemen->statemReceived) < maxstatem)
	{	
		statemen->statemBuffer1[statemen->statemReceived] = insertTableStr;
		/** statemBuffer is filled >  insert in table*/	
		if(statemen->statemReceived == maxstatem-1)
		{
			/**pointer of buffer1 = buffer 2*/
			statemen->statemBuffer2 = statemen->statemBuffer1;
			writeToDb(ipfixDbWriter, statemen);
		}
		else
		{
			statemen->statemReceived++;
		}
	}
	return 0;
}


int writeToDb(IpfixDbWriter* ipfixDbWrite, Statement* statemen)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	Statement* state = statemen;
	int i;
	for(i=0; i < maxstatem; i++)
	{
		if(mysql_query(ipfixDbWriter->conn, state->statemBuffer2[i]) != 0)
		{
			msg(MSG_FATAL,"Insert of records failed");
			fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
		}
		else
			printf("------------- records inserted-----------------\n");

		
		
		free(state->statemBuffer1[i]);
		state->statemBuffer1[i] = 0;
	}
	state->statemReceived = 0;
	/** if buffer 2 = Null  writeToDB is end and recors can store again*/
	state->statemBuffer2 = NULL;
	msg(MSG_DEBUG,"statement pointer to buffer2 is free - now data can store in buffer1");
	return 0;
}

/**
*	Count the columns of the tables
*/
int countColumns()
{
	/**count columms*/
	int i;
	int count_col = 0;
	for(i=0; columns_names[i] !=0; i++)
		count_col++;
	return count_col;			
}

/**
 *	Get data of the record is given by the IPFIX_TYPEID
*/
uint64_t getdata(FieldType type, FieldData* data)
{
	if(type.id == IPFIX_TYPEID_sourceTransportPort || type.id == IPFIX_TYPEID_destinationTransportPort)
		return getTransportPort(type, data);
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
		return getipv4address(type, data);
	if(type.id == IPFIX_TYPEID_protocolIdentifier)
		return getProtocol(type, data);
	else
		return getIPFIXdata(type, data);
}

/**
*	determine the protocol of the data record
*/
uint8_t getProtocol(FieldType type, FieldData* data)
{
	switch (data[0]) 
	{
		case IPFIX_protocolIdentifier_ICMP:
			return IPFIX_protocolIdentifier_ICMP;
		case IPFIX_protocolIdentifier_TCP:
			return IPFIX_protocolIdentifier_TCP;
		case IPFIX_protocolIdentifier_UDP:
			return IPFIX_protocolIdentifier_UDP;
		case IPFIX_protocolIdentifier_RAW:
			return IPFIX_protocolIdentifier_RAW;
		default:
			msg(MSG_FATAL,"No protocolidentifier");
			return 0;
	}
}

/**
*	determine the transportport of the data record
*/
uint16_t getTransportPort(FieldType type, FieldData* data)
{
	uint16_t port = 0;
	if (type.length == 0) 
	{
		printf("zero-length Port");
		if(type.id == IPFIX_TYPEID_sourceTransportPort)
			return port;
		if(type.id == IPFIX_TYPEID_destinationTransportPort)
			return port;
	}
	if (type.length == 2) 
	{
		port = ((uint16_t)data[0] << 8)+data[1];
		if(type.id == IPFIX_TYPEID_sourceTransportPort)
			return port;
		if(type.id == IPFIX_TYPEID_destinationTransportPort)
			return port;
	}
	else
	{
		printf("Port with length %d unparseable", type.length);
		if(type.id == IPFIX_TYPEID_sourceTransportPort)
			return port;
		if(type.id == IPFIX_TYPEID_destinationTransportPort)
			return port;
	}
	return 0;
}

/**
 *	determine the ipv4address of the data record
 */
uint32_t getipv4address( FieldType type, FieldData* data)
{
	uint32_t octets = 0;
	char octetstring[10];
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
	
	if (type.length > 5) 
	{
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return 0;
	}
	/**Create octetstring from integers**/
	if ((type.length == 5) && ( type.id == IPFIX_TYPEID_sourceIPv4Address)) /*&& (imask != 0)*/ 
	{
		sprintf(octetstring,"%d%d%d%d%d",octet1,octet2,octet3,octet4,32-imask);
		octets = atoi(octetstring);
		return octets;
	}
	if ((type.length == 5) && (type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask != 0)*/ 
	{
		sprintf(octetstring,"%d%d%d%d%d",octet1,octet2,octet3,octet4,32-imask);
		octets = atoi(octetstring);
		return octets;
	}
	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_sourceIPv4Address)) /*&& (imask == 0)*/ 
	{
		sprintf(octetstring,"%d%d%d%d",octet1,octet2,octet3,octet4);
		octets = atoi(octetstring);
		return octets;
	}	
	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask == 0)*/ 
	{
		sprintf(octetstring,"%d%d%d%d",octet1,octet2,octet3,octet4);
		octets = atoi(octetstring);
		return octets;
	}	
	return 0;
}

/**
 *	getdata of the data record according to IPFIX_TYPEID
 */
uint64_t getIPFIXdata(FieldType type, FieldData* data)
{ 		 

	
	if(type.id ==  IPFIX_TYPEID_classOfServiceIPv4)
	{
		uint16_t dstTos = getIPFIXValue(type, data);
		return dstTos;
	}
	if(type.id ==  IPFIX_TYPEID_packetDeltaCount)
	{
		uint16_t Packetdeltacount = getIPFIXValue(type, data);
		return Packetdeltacount;
	}
	if(type.id ==  IPFIX_TYPEID_octetDeltaCount)
	{
		uint Octetdeltacount = getIPFIXValue(type, data);
		return Octetdeltacount;
	}		
	if(type.id ==  IPFIX_TYPEID_flowStartSeconds)
	{
		uint64_t Flowstartssecond = getIPFIXValue(type, data);
		return Flowstartssecond;
	}				
	if(type.id ==  IPFIX_TYPEID_flowEndSeconds)
	{
		uint64_t Flowendssecond = getIPFIXValue(type, data);
		return Flowendssecond;
	}
	else 
	{
		msg(MSG_FATAL,"No IPFIX_TYPEID specified");	
	}					
	return 0;
}

/**
*	get the IPFIX value 
*/
uint64_t getIPFIXValue(FieldType type, FieldData* data)
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

/**
*	if there no IPFIX_TYPEID in the given data record 
* 	get the default value to store in the database columns
*/
uint32_t getdefaultIPFIXdata(int ipfixtype_id)
{
	int i;
	for( i=0; identify[i].cname != "END"; i++)
	{
		if(ipfixtype_id == identify[i].ipfixid)
		{
			if(ipfixtype_id == IPFIX_TYPEID_sourceTransportPort)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_destinationTransportPort)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_sourceIPv4Address)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_destinationIPv4Address)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_packetDeltaCount)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_octetDeltaCount)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_classOfServiceIPv4)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_flowStartSeconds)
			{
				return identify[i].default_value;
			}
			if(ipfixtype_id == IPFIX_TYPEID_flowEndSeconds)
				return identify[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_exporterIPv4Address)
				return identify[i].default_value;
		}
	}
	return 0;
}


CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter *ipfixDbWriter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixDbWriter;
	ci.dataDataRecordCallbackFunction = getsingleRecData;
	
	return ci;
}
