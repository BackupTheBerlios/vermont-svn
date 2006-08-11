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
		msg(MSG_FATAL,"Get MySQL connec handle failed");
		mysql_close(ipfixDbWriter->conn);
		free(tabl);
		free(statemen);
		free(ipfixDbWriter);
	}
	else
	{
		msg(MSG_DEBUG,"Get MySQL init handler");
	}
	/**Initialize structure members IpfixDbWriter*/
	ipfixDbWriter->host_name = "localhost" ;
	ipfixDbWriter->db_name = "flows";
	ipfixDbWriter->user_name = 0 ;    		
	ipfixDbWriter->password = 0 ;    			
	ipfixDbWriter->port_num = 0; 			
	ipfixDbWriter->socket_name = 0 ;	  		
	ipfixDbWriter->flags = 0;
	/**Initialize structure members Table*/	  
	ipfixDbWriter->table = tabl ;
	tabl->countbufftable = 0;
	
	/**Init 2-dim buffer of table creationtime and tablename*/
	int i, j;
	for(i = 0; i < maxTable; i++)
	{
		for(j = 0; j < 2; j++)
		{
			tabl->tablebuffer[i][j] = "NULL";
		}
	}
	/**count columns*/
	for(i=0; columns_names[i] !=0; i++)
		tabl->count_col++;
	
	/**Initialize structure members Statement*/	   	 	
	tabl->statement= statemen;
	statemen->statemReceived = 0;
	statemen->statemBuffer1 = (char**)malloc(maxstatement*sizeof(char*));
	statemen->statemBuffer2 = 0;



	/**Connect to Database*/
	ipfixDbWriter->conn = mysql_real_connect(ipfixDbWriter->conn,
			ipfixDbWriter->host_name, ipfixDbWriter->user_name,ipfixDbWriter->password,
			0, ipfixDbWriter->port_num, ipfixDbWriter->socket_name,
			ipfixDbWriter->flags);
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Connection to database failed");
		mysql_close(ipfixDbWriter->conn);
		free(tabl);
		free(statemen);
		free(ipfixDbWriter);
	
	}
	else
	{
		msg(MSG_DEBUG,"Connect to database");		
	}
	/** create Database*/
	if(createDB(ipfixDbWriter) == 0)
	{
		msg(MSG_DEBUG,"Create database function success");
	}
	/**create table exporter*/
	if(createTabExporter(ipfixDbWriter) == 0)
	{
		msg(MSG_DEBUG,"Create table exporter success");
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
	free(ipfixDbWriter->table->statement);
	free(ipfixDbWriter->table);
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
int createDB(IpfixDbWriter* ipfixDbWriter)
{
	/**Is there a already a database with the same name - drop  them*/
	char dropDb[start_len];
	strcpy(dropDb, "DROP DATABASE IF EXISTS ");
	strcat(dropDb, ipfixDbWriter->db_name);
	if(mysql_query(ipfixDbWriter->conn, dropDb) != 0 )
	{
		msg(MSG_FATAL,"Drop of exists database failed");
	}
	/** make query string to create database**/
	char  createDbStr[start_len] ;
	strcpy(createDbStr,"CREATE DATABASE IF NOT EXISTS ");
	strcat(createDbStr,ipfixDbWriter->db_name);
	/**create database*/
	if(mysql_query(ipfixDbWriter->conn, createDbStr) != 0 )
	{
		msg(MSG_FATAL,"Creation of database %s failed", ipfixDbWriter->db_name);
		mysql_close(ipfixDbWriter->conn);
		free(ipfixDbWriter->table->statement);
		free(ipfixDbWriter->table);
		free(ipfixDbWriter);
	}
	else
	{
		msg(MSG_DEBUG,"Database %s created",ipfixDbWriter->db_name);
	}
	/** use database  with db_name**/	
	if(mysql_select_db(ipfixDbWriter->conn, ipfixDbWriter->db_name) !=0)
	{
		msg(MSG_FATAL,"Database %s not selectable", ipfixDbWriter->db_name);	
		mysql_close(ipfixDbWriter->conn);
		free(ipfixDbWriter->table->statement);
		free(ipfixDbWriter->table);
		free(ipfixDbWriter);
	}
	else
	{
		msg(MSG_DEBUG,"Database %s selected", ipfixDbWriter->db_name);
	}
	return 0;
}

int createTabExporter(IpfixDbWriter* ipfixDbWriter)
{
	char dropExporterTable[start_len];
	strcpy(dropExporterTable,"DROP TABLE IF EXISTS exporter");
	/**is there already a table with the same name - drop them*/
	if(mysql_query(ipfixDbWriter->conn, dropExporterTable) != 0)
	{
		msg(MSG_FATAL,"Drop of exists exporter table failed");
	}
	char createExporterTable[start_len+(3 * col_width)];
	strcpy(createExporterTable,"CREATE TABLE IF NOT EXISTS exporter (id SMALLINT(6) NOT NULL AUTO_INCREMENT,sourceID INTEGER(10) UNSIGNED DEFAULT NULL,srcIP INTEGER(10) UNSIGNED DEFAULT NULL,PRIMARY KEY(id))");
	/** create table exporter*/
	if(mysql_query(ipfixDbWriter->conn,createExporterTable) != 0)
	{
		msg(MSG_FATAL,"Creation of table Exporter failed");
	}
	return 0;
}

/**
* 	Create the table of the database
*/
int createDBTable(IpfixDbWriter* ipfixDbWriter,Table* table, char* TableName)
{
	int i, j ;
	char createTableStr[start_len+(table->count_col* col_width)];
	strcpy(createTableStr , "CREATE TABLE IF NOT EXISTS ");
	strcat(createTableStr,TableName);
	strcat(createTableStr," (");
	/**collect the names for columns and the datatypes for the table in a string*/
	for(i=0; i < table->count_col; i++)
	{
		for(j=0; strcmp(identify[j].cname,"END") != 0 ;j++)
		{
			/**if columns_names equal identify.cname then ...*/
			if( strcmp(columns_names[i], identify[j].cname) == 0)
			{
				strcat(createTableStr,identify[j].cname);
				strcat(createTableStr," ");
				strcat(createTableStr,identify[j].datatype);
				if( i  != table->count_col-1)
				{
					strcat(createTableStr,",");
				}
			}
		}
	}
	strcat(createTableStr,")");
	
	/**Is there a already a table with the same name - drop them*/
	char dropTable[start_len];
	strcpy(dropTable,"DROP TABLE IF EXISTS ");
	strcat(dropTable, TableName);
	if(mysql_query(ipfixDbWriter->conn, dropTable) != 0)
	{
		msg(MSG_FATAL,"Drop of exists %s table failed",TableName);
	}
	
	/** create table*/
	if(mysql_query(ipfixDbWriter->conn,createTableStr) != 0)
	{
		msg(MSG_FATAL,"Creation of table failed");
	}
	else
	{
		msg(MSG_DEBUG, "Creation of table %s successful ",TableName);
	}
	return 0;
}

/**
*	function receive the datadatarecord when callback is started
*/
int receiveDataRec(void* ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
	int i;
	Table *tabl = ((IpfixDbWriter*) ipfixDbWriter)->table;
	Statement* statemen = tabl->statement;
	
	/** if the writeToDb process not ready - drop record*/
	if((statemen->statemBuffer2 != NULL) && (statemen->statemReceived == maxstatement-1))
	{
		msg(MSG_FATAL,"Drop datarecord - still writing to database");
		return 0;
	}
	/** make a sql insert statement from the data of the record*/
	char* insertTableStr = getRecData(ipfixDbWriter, tabl, sourceID, dataTemplateInfo, length, data);
	msg(MSG_DEBUG,"Insert statement : %s\n",insertTableStr);	
	
	/** if statement counter lower as  max count of statement then insert record in statemenBuffer*/
	if((statemen->statemReceived) < maxstatement)
	{	
		statemen->statemBuffer1[statemen->statemReceived] = insertTableStr;
		/** statemBuffer is filled >  insert in table*/	
		if(statemen->statemReceived == maxstatement-1)
		{
				statemen->statemBuffer2 = (char**)malloc(maxstatement*sizeof(char*));
				memcpy(statemen->statemBuffer2,statemen->statemBuffer1,maxstatement * sizeof(char*));	
				free(statemen->statemBuffer1);
				statemen->statemBuffer1 =  NULL;
				statemen->statemBuffer1 = (char**)malloc(maxstatement*sizeof(char*));
				
			if(writeToDb(ipfixDbWriter, statemen) != 0)
			{
				for(i = 0;i < maxstatement; i++)
					free(statemen->statemBuffer2[i]) ;
				
				free(statemen->statemBuffer2);
				statemen->statemBuffer2 = 0;
			}
		}
		else
		{
			statemen->statemReceived++;
		}
	}
	return 0;
}

/**
*	loop over the DataTemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in db
*/
char* getRecData(IpfixDbWriter* ipfixDbWriter,Table* table,SourceID sourceID, DataTemplateInfo* dataTemplateInfo,uint16_t length, FieldData* data)
{
	int i ,j, k, n;
	uint64_t intdata = 0;
	uint64_t flowstartsec = 0;
	/**begin query string for insert statement*/
	char insert[start_len+(table->count_col * ins_width)];
	strcpy(insert,"INSERT INTO ");
	/**make string for the column names*/
	char ColNames[table->count_col * ins_width];
	strcpy(ColNames," (");
	/**make string for the values  given by the IPFIX_TYPEID stored in the record*/ 
	char ColValues[table->count_col * ins_width]; 
	strcpy(ColValues," VALUES (");	
	
	char stringtmp[ins_width];// needed to cast from char to string
			
	/**loop over the columname and loop over the IPFIX_TYPEID of the record  to get the corresponding */
	/**data to store and make insert statement*/
	for( i=0; i < table->count_col; i++)
	{
		for( j=0; identify[j].cname != "END"; j++)
		{ 
			if(columns_names[i] == identify[j].cname)
			{
				strcat(ColValues,"'");
				strcat(ColNames,columns_names[i]);
				if( i != table->count_col-1)
					strcat(ColNames,",");
				if( i == table->count_col-1)
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
						}
						if( i !=  table->count_col-1)
							strcat(ColValues,",");
						if( i == table->count_col-1)
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
								if( i != table->count_col-1)
									strcat(ColValues,",");
								if( i == table->count_col-1)
									strcat(ColValues,")");					
								break;
							}
							else if(n == dataTemplateInfo->dataCount-1)
							{
								intdata = getdefaultIPFIXdata(identify[j].ipfixid);
								sprintf(stringtmp,"%Lu",(uint64_t)intdata);
								strcat(ColValues,stringtmp);
								strcat(ColValues,"'");
								if( i != table->count_col-1)
									strcat(ColValues,",");
								if( i == table->count_col-1)
									strcat(ColValues,")");					
							}
						}
					}
				}
			}
		}
	}
	/**make hole query string for the insert statement*/
	
	char* tablename = getTableName(ipfixDbWriter, table, flowstartsec);
	/** tablename +  Columnsname + Values of record*/
	strcat(insert, tablename);  
	strcat(insert,ColNames);
	strcat(insert,ColValues);
	
	char* insertTableStr = (char*) malloc((strlen(insert)+1)*sizeof(char));
	strcpy(insertTableStr,insert);
	
	return insertTableStr;

}

char* getTableName(IpfixDbWriter* ipfixDbWriter,Table*  table,  uint64_t flowstartsec)
{
	int i = 0;
	/** Is  flowstartsec in intervall of tablecreationtime in buffer ?*/
	for(i = 0; i < maxTable; i++)
	{
		/**Is flowstartsec between  the range of tablecreattime and tablecreattime+30 min*/
		if(atoi(table->tablebuffer[i][0]) <= flowstartsec &&  flowstartsec< (atoi(table->tablebuffer[i][0]) + 1800)) 
		{
			msg(MSG_DEBUG,"Table creation time : %s Tablename : %s in Buffer", table->tablebuffer[i][0], table->tablebuffer[i][1]);
			return table->tablebuffer[i][1];
		}
	}
	/**Tablename is not in tablebuffer*/	
	char tabNam[table_len];
	getTableByTime(tabNam, flowstartsec);
	char* TableName = (char*)malloc((strlen(tabNam)+1)*sizeof(char));
	strcpy(TableName, tabNam);
	
	char tmp[table_len] ;
	uint64_t startTime = getTabStartTime(flowstartsec);
	sprintf(tmp,"%Lu",startTime);
	char* TableTime = (char*)malloc((strlen(tmp)+1)*sizeof(char));
	strcpy(TableTime, tmp);
 	
	/**there are no pointer in first bufferentries to be freed*/
	if(table->tablebuffer[table->countbufftable][0] != "NULL")
	{
		msg(MSG_DEBUG,"free table in tablebuffer");
		free(table->tablebuffer[table->countbufftable][0]);
		free(table->tablebuffer[table->countbufftable][1]);
	}
	table->tablebuffer[table->countbufftable][0] = TableTime;
	table->tablebuffer[table->countbufftable][1] = TableName;
	
	/** createTable when not in buffer*/
	if(createDBTable(ipfixDbWriter, table, TableName) != 0)
	{	
		msg(MSG_DEBUG,"Error in function createDBTable");
		free(TableTime);
		free(TableName);
		table->tablebuffer[table->countbufftable][0] = "NULL";
		table->tablebuffer[table->countbufftable][1] = "NULL";
		
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
		msg(MSG_DEBUG,"Table creation time : %s Tablename : %s in Buffer", table->tablebuffer[i][0], table->tablebuffer[i][1]);
	}
	return TableName;
}


uint64_t getTabStartTime(uint64_t flowstartsec)
{
	uint64_t startTimeStamp;
	time_t  t = flowstartsec;
	time_now = localtime(&t);
	
	if(time_now->tm_min< 30)
	{
		time_now->tm_min = 0;
		time_now->tm_sec = 0;
		startTimeStamp = mktime(time_now);
		return startTimeStamp;
	}
	else
	{
		time_now->tm_min = 30;
		time_now->tm_sec = 0;
		startTimeStamp =mktime(time_now);
		return startTimeStamp;
	}
	return 0;
}


/** 
 *	The tablename according to the time of the records when the flow is started
 *	The result is given by "h_YYYYMMDD_HH_0 || 1"
 *	0, when the recordtime of min is  0 <= min < 30, otherwise 1
 */
char* getTableByTime(char* tableName, uint64_t flowstartsec)
{
	char strtmp[table_len];
	/** according to flowstartsec make the tablename*/
	time_t  t = flowstartsec;
	time_now = localtime(&t);
	strcpy(tableName,"h_");
	sprintf(strtmp,"%u",time_now->tm_year+1900);
	strcat(tableName,strtmp);
	sprintf(strtmp,"%02u",time_now->tm_mon+1);
	strcat(tableName,strtmp);
	sprintf(strtmp,"%02u",time_now->tm_mday);
	strcat(tableName,strtmp);
	strcat(tableName,"_");
	sprintf(strtmp,"%02u",time_now->tm_hour);
	strcat(tableName,strtmp);
	strcat(tableName,"_");
	sprintf(strtmp,"%u",time_now->tm_min<30?0:1);
	strcat(tableName,strtmp);
	
	return tableName;
}		

int writeToDb(IpfixDbWriter* ipfixDbWriter, Statement* statement)
{
	int i;
	for(i=0; i < maxstatement; i++)
	{
		if(mysql_query(ipfixDbWriter->conn, statement->statemBuffer2[i]) != 0)
		{
			msg(MSG_FATAL,"Insert of records failed");
		}
		else
			printf("------------- records inserted-----------------\n");

		
		
		free(statement->statemBuffer2[i]);
		statement->statemBuffer2[i] = 0;
	}
	free(statement->statemBuffer2);
	statement->statemReceived = 0;
	/** if buffer 2 == Null  writeToDB ends and records can store again*/
	statement->statemBuffer2 = NULL;
	msg(MSG_DEBUG,"Write to database is completed");
	return 0;
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
	ci.dataDataRecordCallbackFunction = receiveDataRec;
	
	return ci;
}
