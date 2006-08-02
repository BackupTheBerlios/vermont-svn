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
char *columns[] = {"srcIP", "dstIP", "srcPort", "dstPort", "proto", "dstTos","bytes","pkts","firstSwitched","lastSwitched","exporterID",0};					  
 
 /**
 *	struct to identify column names with IPFIX_TYPEID an the datatype to store the result in database
 */
struct column table [] = {		 
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
	DBname* dbnam = (DBname*)malloc(sizeof(DBname));

	ipfixDbWriter->conn = mysql_init(0);  /** get the mysl init handle*/
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Getting MySQL connecting handle failed");
	}
	/**Initialize structures*/
	ipfixDbWriter->host_name="localhost" ;
	ipfixDbWriter->user_name=0 ;    		
	ipfixDbWriter->password=0 ;    			
	ipfixDbWriter->port_num=0; 			
	ipfixDbWriter->socket_name=0 ;	  		
	ipfixDbWriter->flags=0;   	 	
	ipfixDbWriter->statement = statemen;
	statemen->count_c = 0;
	statemen->statemReceived = 0;
	statemen->statemBuffer2 = 0;
	ipfixDbWriter->dbName = dbnam;
	dbnam->count_dbn = 0;
	dbnam->flowstartsseconds = 0;
	dbnam->notDbbuffer = 0;
	int i;
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

	/**count columms*/
	for(i=0; columns[i] !=0; i++)
		statemen->count_c++;
	/**create the name of the database to store in buffer and create the databases*/
	int thirtymin = 1800;  /**is equal 30 min*/
	time_t now, t;
	now = time(&t);
	for(i=0; i < maxDBname; i++)
	{
		dbnam->flowstartsseconds = now +thirtymin*i;
		getTimeRec(ipfixDbWriter);
		dbnam->db_buffer[i] = dbnam->dbn;
		createDB(ipfixDbWriter);
	}
	return ipfixDbWriter;
}

/**
 * Frees memory used by an ipfixDbWriter
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
void destroyIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	deinitializeIpfixDbWriter();
	mysql_close(ipfixDbWriter->conn);
	free(ipfixDbWriter->dbName);
	free(ipfixDbWriter->statement);
	free(ipfixDbWriter);
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
/**
* create the database given by the name dbnam->dbn
*/
void createDB(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	Statement* stat = ipfixDbWriter->statement;
	DBname* dbnam = ipfixDbWriter->dbName;
	/** make query string to create database**/
	stat->createDatabaseStr = (char*)malloc(startlen*sizeof(char));
	strcpy(stat->createDatabaseStr,"CREATE DATABASE IF NOT EXISTS ");
	strcat(stat->createDatabaseStr,dbnam->dbn);
	/**create database*/
	if(mysql_query(ipfixDbWriter->conn, stat->createDatabaseStr) != 0 )
	{
		msg(MSG_FATAL,"Creation of database failed");
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
	}
	/** use database  db_name**/	
	if(mysql_select_db(ipfixDbWriter->conn,dbnam->dbn) !=0)
	{
		msg(MSG_FATAL,"Database not selectable");	
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));	
	}
	free(stat->createDatabaseStr);
	createDBTable(ipfixDbWriter);
}

/**
* 	Create the table of the database
*/
void createDBTable(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	Statement* statemen = ipfixDbWriter->statement;
	DBname* dbnam = ipfixDbWriter->dbName;
	int i,j;
	statemen->createTableStr = (char*)malloc(startlen+((statemen->count_c) * columnsize*sizeof(char)));
	strcpy(statemen->createTableStr , "CREATE TABLE IF NOT EXISTS ");
	strcat(statemen->createTableStr,dbnam->dbn);
	strcat(statemen->createTableStr," (");
	/**collect the names for columns and the datatypes for database in a string*/
	for(i=0; i < statemen->count_c; i++)
	{
		for(j=0; table[j].cname != "END";j++)
		{
			if( columns[i] == table[j].cname)
			{
				strcat(statemen->createTableStr,table[j].cname);
				strcat(statemen->createTableStr," ");
				strcat(statemen->createTableStr,table[j].datatype);
				if( i  != statemen->count_c-1)
				{
					strcat(statemen->createTableStr,",");
				}
			}
		}
	}
	strcat(statemen->createTableStr,")");
	/** create table*/
	if(mysql_query(ipfixDbWriter->conn,statemen->createTableStr) != 0)
	{
		msg(MSG_FATAL,"Creation of database columns failed");
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
	}
	free(statemen->createTableStr);
}

/**
*	To work with a database (insert into.. ,..) after creation
*/
void useDB(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	DBname* dbnam = ipfixDbWriter->dbName;
	/** use database*/
	if(mysql_select_db(ipfixDbWriter->conn,dbnam->dbn_tmp)  !=0)
	{
		msg(MSG_FATAL,"Database not selectable");	
		fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));	
	}
}

/** 
*	The databasename according to the time of the records when the flow is started
*	The result is given by "h_YYYYMMDD_HH_0 || 1
*	0 when the recordtime of min is  0 <= min < 30, otherwise 1
*/
void getTimeRec(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	DBname* dbnam = ipfixDbWriter->dbName;
	time_t  t = dbnam->flowstartsseconds;
	time_now = localtime(&t);
	char* dbn_str = (char*) malloc(dbn_len*sizeof(char));
	char strtmp[20];
	strcpy(dbn_str,"h_");
	sprintf(strtmp,"%u",time_now->tm_year+1900);
	strcat(dbn_str,strtmp);
	sprintf(strtmp,"%02u",time_now->tm_mon+1);
	strcat(dbn_str,strtmp);
	sprintf(strtmp,"%02u",time_now->tm_mday);
	strcat(dbn_str,strtmp);
	strcat(dbn_str,"_");
	sprintf(strtmp,"%02u",time_now->tm_hour);
	strcat(dbn_str,strtmp);
	strcat(dbn_str,"_");
	sprintf(strtmp,"%u",time_now->tm_min<30?0:1);
	strcat(dbn_str,strtmp);
	dbnam->dbn = dbn_str;
}

/**
*	Give the databasename according of flowstartsseconds
*/
void getDbnameRec(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	getTimeRec(ipfixDbWriter);
	lookupBufferDBname(ipfixDbWriter);
}

/**
*	If the databasename not in buffer, insert it and create datatabase
*	else free the databasename
*/
void lookupBufferDBname(IpfixDbWriter* ipfixDbWrite)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	DBname* dbnam = ipfixDbWriter->dbName;
	int i, cmp, notcmp;
	i = 0;
	cmp = 1;
	notcmp = 0;
	/** Is databasename in db_buffer ? */
	while((cmp) && (i < maxDBname))
	{
		if( strcmp(dbnam->dbn,dbnam->db_buffer[i]) == 0)
		{
			dbnam->notDbbuffer = 1;
			cmp = 0;
			break;			
		}
		if( i == maxDBname-1)
		{
			notcmp = 1;
		}
		i++;	
	}
	/** delete the oldest databasename and insert the new name in db_buffer*/
	if(notcmp)
	{
		free(dbnam->db_buffer[dbnam->count_dbn]);
		dbnam->db_buffer[dbnam->count_dbn] = dbnam->dbn;
		dbnam->notDbbuffer = 0;
		dbnam->count_dbn++;
		createDB(ipfixDbWriter);
		if(dbnam->count_dbn == maxDBname)
		{
			dbnam->count_dbn = 0;
		}
	}
}
/**
*	When a flow is export, get the values of the records to store according to IPFIX_TYPEID and of columns
*	Store them to buffer 
*/

int getsingleRecData(void* ipfixDbWrite, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	Statement* statemen = ipfixDbWriter->statement;
	DBname* dbnam = ipfixDbWriter->dbName;
	/** if the writeToDb process not record - drop record*/
	if(statemen->statemBuffer2 != NULL)
	{
		msg(MSG_FATAL,"Drop datarecord - still writing to database");
		return 0;
	}
	/**begin query string for insert statement*/
	char insert[startlen+(statemen->count_c * tableNamesize)];
	strcpy(insert,"INSERT INTO ");
	/**make string for the column names*/
	char tableNames[statemen->count_c * tableNamesize];
	strcpy(tableNames," (");
	/**make string the values is given by the IPFIX_TYPEID stored in the record*/ 
	char tableValues[statemen->count_c * tableNamesize]; 
	strcpy(tableValues," VALUES (");	
			
	char stringtmp[tableNamesize];
	/** loop over the IPFIX_TYPEID of the record*/
	int i ,j, k, n;
	uint64_t intdata;
	for( i=0; i < statemen->count_c; i++)
	{
		for( j=0; table[j].cname != "END"; j++)
		{ 
			if(columns[i] == table[j].cname)
			{
				strcat(tableValues,"'");
				strcat(tableNames,columns[i]);
				if( i != statemen->count_c-1)
					strcat(tableNames,",");
				if( i == statemen->count_c-1)
					strcat(tableNames,") ");
				for(k=0; k < dataTemplateInfo->fieldCount; k++)
				{
					if(dataTemplateInfo->fieldInfo[k].type.id == table[j].ipfixid)
					{
						intdata = getdata(ipfixDbWriter, dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
						sprintf(stringtmp,"%Lu",(uint64_t)intdata);
						strcat(tableValues,stringtmp);
						strcat(tableValues,"'");
						if( i != statemen->count_c-1)
							strcat(tableValues,",");
						if( i == statemen->count_c-1)
							strcat(tableValues,")");				
						break;
					}	
					else if( k == dataTemplateInfo->fieldCount-1)
					{
						for(n=0; n < dataTemplateInfo->dataCount; n++)
						{
							if(dataTemplateInfo->dataInfo[n].type.id == table[j].ipfixid)
							{
								intdata = getdata(ipfixDbWriter, dataTemplateInfo->dataInfo[n].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[n].offset));
								sprintf(stringtmp,"%Lu",(uint64_t)intdata);
								strcat(tableValues,stringtmp);
								strcat(tableValues,"'");
								if( i != statemen->count_c-1)
									strcat(tableValues,",");
								if( i == statemen->count_c-1)
									strcat(tableValues,")");					
								break;
							}
							else if(n == dataTemplateInfo->dataCount-1)
							{
								intdata = getdefaultIPFIXdata(ipfixDbWriter, table[j].ipfixid);
								sprintf(stringtmp,"%Lu",(uint64_t)intdata);
								strcat(tableValues,stringtmp);
								strcat(tableValues,"'");
								if( i != statemen->count_c-1)
									strcat(tableValues,",");
								if( i == statemen->count_c-1)
									strcat(tableValues,")");					
					   		}
						}
					}
				}
			}
		}
	}
	/**make hole query string for the insert statement*/
	getDbnameRec(ipfixDbWriter);
	strcat(insert, dbnam->dbn);  
	strcat(insert,tableNames);
	strcat(insert,tableValues);
	statemen->insertTableStr = (char*) malloc((strlen(insert)+1)*sizeof(char));
	strcpy(statemen->insertTableStr,insert);
	/** buffer the insert statements*/
	if((statemen->statemReceived) < maxstatem)
	{	
		statemen->statemBuffer1[statemen->statemReceived] = statemen->insertTableStr;
			
		if(statemen->statemReceived == maxstatem-1)
		{
			statemen->statemBuffer2 = statemen->statemBuffer1;
			writeToDb(ipfixDbWriter);
		}
		else
		{
			statemen->statemReceived++;
		}
	}
	/** when databasename already insert in lookupbuffer free it*/
	if(dbnam->notDbbuffer)
	{
		free(dbnam->dbn);
	}
	
	return 0;
}

/**
*	Write the insert statement buffer to database
*/
void writeToDb(IpfixDbWriter* ipfixDbWriter)
{
	int i;
	static char tmpnam1[16] ;
	static char tmpnam2[16] ;
	Statement* state = ipfixDbWriter->statement;
	DBname* dbnam = ipfixDbWriter->dbName;
	/**databasename of record one*/
	strncpy(tmpnam1,state->statemBuffer2[0]+12,strlen(dbnam->dbn));
	strcat(tmpnam1,"\0");
	dbnam->dbn_tmp = tmpnam1;
	
	useDB(ipfixDbWriter);

	for(i=0; i < maxstatem; i++)
	{
		/** insert buffer in database*/
		if(mysql_query(ipfixDbWriter->conn, state->statemBuffer2[i]) != 0)
		{
			msg(MSG_FATAL,"Insert of records failed");
			fprintf(stderr, "Error: %s\n", mysql_error(ipfixDbWriter->conn));
			
			/** if database deleted create a new one*/
			if(mysql_errno(ipfixDbWriter->conn) == Table_Not_Exists)
			{
				msg(MSG_FATAL,"Table not exists - deleted ?");
				strncpy(tmpnam1,state->statemBuffer2[i]+12,strlen(dbnam->dbn));
				strcat(tmpnam1,"\0");
				dbnam->dbn_tmp = tmpnam1;
				createDB(ipfixDbWriter);
				if(mysql_query(ipfixDbWriter->conn, state->statemBuffer2[i]) != 0)
				{
					msg(MSG_FATAL,"Could not create table");
				}
				else
				{
					printf("------------- record inserted-----------------\n");
				}
			}
		}
		else
			printf("------------- record inserted-----------------\n");
		/** check change databasename in the  insert statement buffer*/
		if(i < maxstatem-1)
		{
			strncpy(tmpnam1,state->statemBuffer2[i]+12,strlen(dbnam->dbn));
			strcat(tmpnam1,"\0");
		
			strncpy(tmpnam2,state->statemBuffer2[i+1]+12,strlen(dbnam->dbn));
			strcat(tmpnam2,"\0");
			if(strcmp(tmpnam1, tmpnam2) != 0)
			{
				dbnam->dbn_tmp = tmpnam2;
				useDB(ipfixDbWriter);
			}
		}
		free(state->statemBuffer1[i]);
		state->statemBuffer1[i] = 0;
	}
	state->statemReceived = 0;
	state->statemBuffer2 = NULL;
}

/**
 *	Get date of the record is given by the IPFIX_TYPEID
*/
uint64_t getdata(IpfixDbWriter* ipfixDbWriter, FieldType type, FieldData* data)
{
	if(type.id == IPFIX_TYPEID_sourceTransportPort || type.id == IPFIX_TYPEID_destinationTransportPort)
		return getTransportPort(type, data);
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
		return getipv4address(type, data);
	if(type.id == IPFIX_TYPEID_protocolIdentifier)
		return getProtocol(type, data);
	else
		return getIPFIXdata(ipfixDbWriter, type, data);
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
uint64_t getIPFIXdata(IpfixDbWriter* ipfixDbWrite,FieldType type, FieldData* data)
{ 		 
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	DBname* dbname = ipfixDbWriter->dbName;
	
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
		dbname->flowstartsseconds = Flowstartssecond;
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
uint32_t getdefaultIPFIXdata(IpfixDbWriter* ipfixDbWrite, int ipfixtype_id)
{
	IpfixDbWriter* ipfixDbWriter = ipfixDbWrite;
	DBname* dbname = ipfixDbWriter->dbName;
	
	int i;
	for( i=0; table[i].cname != "END"; i++)
	{
		if(ipfixtype_id == table[i].ipfixid)
		{
			if(ipfixtype_id == IPFIX_TYPEID_sourceTransportPort)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_destinationTransportPort)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_sourceIPv4Address)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_destinationIPv4Address)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_packetDeltaCount)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_octetDeltaCount)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_classOfServiceIPv4)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_flowStartSeconds)
			{
				dbname->flowstartsseconds = table[i].default_value;
				return table[i].default_value;
			}
			if(ipfixtype_id == IPFIX_TYPEID_flowEndSeconds)
				return table[i].default_value;
			if(ipfixtype_id == IPFIX_TYPEID_exporterIPv4Address)
				return table[i].default_value;
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
