#include <string.h>
#include<stdlib.h>
#include "IpfixDbWriter.h"
#include "msg.h"



/***** Global Variables ******************************************************/
/**
* 	is needed to determine "now" time and the time of flowstartsseconds
*/
struct tm* time_now;

/**
*	column names as a array of char pointer
*/
char *columns_names[] = {"srcIP", "dstIP", "srcPort", "dstPort", "proto", "dstTos","bytes","pkts","firstSwitched","lastSwitched","exporterID",0};					  
 
 /**
 *	struct to identify column names with IPFIX_TYPEID an the datatype to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
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
	//{"firstSwitched", 22,  "INTEGER(10) UNSIGNED", 0},
	//{"lastSwitched", 21,  "INTEGER(10) UNSIGNED", 0},
	{"exporterID",ExporterID, "SMALLINT(5) UNSIGNED", 0},
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
	Table* tabl = (Table*)malloc(sizeof(Table));
	Statement* statemen = (Statement*)malloc(sizeof(Statement));
	
	ipfixDbWriter->conn = mysql_init(0);  /** get the mysl init handle*/
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Get MySQL connect handle failed");
		goto out;
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
	ipfixDbWriter->srcid = 7777;
	/**Initialize structure members Table*/	  
	ipfixDbWriter->table = tabl ;
	tabl->countbufftable = 0;
	tabl->countExpTable = 0;
	
	/**Init TableBuffer start- , endTime and name of the tables*/
	int i ;
	for(i = 0; i < maxTable; i++)
	{
		tabl->TableBuffer[i].startTableTime = 0;
		tabl->TableBuffer[i].endTableTime  = 0;
		strcpy(tabl->TableBuffer[i].TableName, "NULL");
	}
	/**count columns*/
	for(i=0; columns_names[i] !=0; i++)
		tabl->count_col++;
	
	/**Initialize structure members Statement*/	   	 	
	tabl->statement= statemen;
	statemen->statemReceived = 0;
	for( i = 0; i < maxstatement; i++)
	{
		statemen->statemBuffer[i] = "NULL";
	}
	
	/**Init struct expTable*/
	for(i = 0; i < maxExpTable; i++)
	{
		tabl->ExporterBuffer[i].Id = 0;
		tabl->ExporterBuffer[i].srcID = 0;
		tabl->ExporterBuffer[i].expIP = 0;
	}				
	
	/**Connect to Database*/
	ipfixDbWriter->conn = mysql_real_connect(ipfixDbWriter->conn,
			ipfixDbWriter->host_name, ipfixDbWriter->user_name,ipfixDbWriter->password,
			0, ipfixDbWriter->port_num, ipfixDbWriter->socket_name,
			ipfixDbWriter->flags);
	if(ipfixDbWriter->conn == 0)
	{
		msg(MSG_FATAL,"Connection to database failed");
		goto out;
	}
	else
	{
		msg(MSG_DEBUG,"Connect to database");		
	}
	/** create Database*/
	if(createDB(ipfixDbWriter) !=0)
		goto out;
	/**create table exporter*/
	if(createExporterTable(ipfixDbWriter) !=0)
		goto out;
	
	return ipfixDbWriter;
	
out : 
		destroyIpfixDbWriter(ipfixDbWriter);
		
	return NULL;	
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
	strncat(dropDb, ipfixDbWriter->db_name,strlen(ipfixDbWriter->db_name)+1);
	if(mysql_query(ipfixDbWriter->conn, dropDb) != 0 )
	{
		msg(MSG_FATAL,"Drop of exists database failed");
	}
	/** make query string to create database**/
	char  createDbStr[start_len] ;
	strcpy(createDbStr,"CREATE DATABASE IF NOT EXISTS ");
	strncat(createDbStr,ipfixDbWriter->db_name,strlen(ipfixDbWriter->db_name)+1);
	/**create database*/
	if(mysql_query(ipfixDbWriter->conn, createDbStr) != 0 )
	{
		msg(MSG_FATAL,"Creation of database %s failed", ipfixDbWriter->db_name);
	}
	else
	{
		msg(MSG_DEBUG,"Database %s created",ipfixDbWriter->db_name);
	}
	/** use database  with db_name**/	
	if(mysql_select_db(ipfixDbWriter->conn, ipfixDbWriter->db_name) !=0)
	{
		msg(MSG_FATAL,"Database %s not selectable", ipfixDbWriter->db_name);	
	}
	else
	{
		msg(MSG_DEBUG,"Database %s selected", ipfixDbWriter->db_name);
	}
	return 0;
}

int createExporterTable(IpfixDbWriter* ipfixDbWriter)
{
	char dropExporterTab[start_len];
	strcpy(dropExporterTab,"DROP TABLE IF EXISTS exporter");
	/**is there already a table with the same name - drop them*/
	if(mysql_query(ipfixDbWriter->conn, dropExporterTab) != 0)
	{
		msg(MSG_FATAL,"Drop of exists exporter table failed");
	}
	char createExporterTab[start_len+(3 * col_width)];
	strcpy(createExporterTab,"CREATE TABLE IF NOT EXISTS exporter (id SMALLINT(5) NOT NULL AUTO_INCREMENT,sourceID INTEGER(10) UNSIGNED DEFAULT NULL,srcIP INTEGER(10) UNSIGNED DEFAULT NULL,PRIMARY KEY(id))");
	/** create table exporter*/
	if(mysql_query(ipfixDbWriter->conn,createExporterTab) != 0)
	{
		msg(MSG_FATAL,"Creation of table Exporter failed");
	}
	else
	{
		msg(MSG_DEBUG,"Exporter table created");
	}
	return 0;
}

/**
* 	Create the table of the database
*/
int createDBTable(IpfixDbWriter* ipfixDbWriter,Table* table, char* tablename)
{
	int i, j ;
	char createTableStr[start_len+(table->count_col* col_width)];
	strcpy(createTableStr , "CREATE TABLE IF NOT EXISTS ");
	strncat(createTableStr,tablename,strlen(tablename)+1);
	strncat(createTableStr," (",(2*sizeof(char))+1);
	/**collect the names for columns and the datatypes for the table in a string*/
	for(i=0; i < table->count_col; i++)
	{
		for(j=0; strcmp(identify[j].cname,"END") != 0 ;j++)
		{
			/**if columns_names equal identify.cname then ...*/
			if( strcmp(columns_names[i], identify[j].cname) == 0)
			{
				strncat(createTableStr,identify[j].cname,strlen(identify[j].cname)+1);
				strncat(createTableStr," ",sizeof(char)+1);
				strncat(createTableStr,identify[j].datatype,strlen(identify[j].datatype)+1);
				if( i  != table->count_col-1)
				{
					strncat(createTableStr,",",sizeof(char)+1);
				}
			}
		}
	}
	strncat(createTableStr,")",sizeof(char)+1);
	
	/**Is there a already a table with the same name - drop them*/
	char dropTable[start_len];
	strcpy(dropTable,"DROP TABLE IF EXISTS ");
	strncat(dropTable, tablename,strlen(tablename)+1);
	if(mysql_query(ipfixDbWriter->conn, dropTable) != 0)
	{
		msg(MSG_FATAL,"Drop of exists %s table failed",tablename);
	}
	/** create table*/
	if(mysql_query(ipfixDbWriter->conn,createTableStr) != 0)
	{
		msg(MSG_FATAL,"Creation of table failed");
	}
	else
	{
		msg(MSG_DEBUG, "Table %s created ",tablename);
	}
	return 0;
}

/**
*	function receive the DataRecord or DataDataRecord when callback is started
*/
int  writeDataDataRecord(void* ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
	Table *tabl = ((IpfixDbWriter*) ipfixDbWriter)->table;
	Statement* statemen = tabl->statement;
	/** if the writeToDb process not ready - drop record*/
	if(strcmp(statemen->statemBuffer[maxstatement-1],"NULL") != 0)
	{
		msg(MSG_FATAL,"Drop datarecord - still writing to database");
		return 0;
	}
	/** sourceid null ? use default*/
	//if(sourceID == 0)
	/* overwrite sourceid if defined */
	if(((IpfixDbWriter*) ipfixDbWriter)->src != 0)
	{
		sourceID = ((IpfixDbWriter*) ipfixDbWriter)->srcid;
	}
	/** make a sql insert statement from the recors data */
	char* insertTableStr = getRecData(ipfixDbWriter, tabl, sourceID, dataTemplateInfo, length, data);
	msg(MSG_DEBUG,"Insert statement: %s",insertTableStr);	
	
	/** if statement counter lower as  max count of statement then insert record in statemenBuffer*/
	if((statemen->statemReceived) < maxstatement)
	{	
		statemen->statemBuffer[statemen->statemReceived] = insertTableStr;
		/** statemBuffer is filled ->  insert in table*/	
		if(statemen->statemReceived == maxstatement-1)
		{
			writeToDb(ipfixDbWriter, tabl, statemen);
		}
		else
		{
			statemen->statemReceived++;
		}
	}
	return 0;
}

/**
 *	function receive the  when callback is started
 */
int writeDataRecord(void* ipfixDbWriter, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data)
{
	DataTemplateInfo dataTemplateInfo;
	
	dataTemplateInfo.id = 0;
	dataTemplateInfo.preceding = 0;
	dataTemplateInfo.fieldCount = templateInfo->fieldCount;  /**< number of regular fields */
	dataTemplateInfo.fieldInfo = templateInfo->fieldInfo;   /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.dataCount = 0;   /**< number of fixed-value fields */
	dataTemplateInfo.dataInfo = NULL;    /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.data = NULL;        /**< data start pointer for fixed-value fields */
	dataTemplateInfo.userData = templateInfo->userData;    /**< pointer to a field that can be used by higher-level modules */
	
	msg(MSG_DEBUG,"receiveRec calls receiveDataRec");	

	return writeDataDataRecord(ipfixDbWriter, sourceID, &dataTemplateInfo, length, data);
}

/**
*	loop over the DataTemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in database
*/
char* getRecData(IpfixDbWriter* ipfixDbWriter,Table* table,SourceID sourceID, DataTemplateInfo* dataTemplateInfo,uint16_t length, FieldData* data)
{
	int i ,j, k, n;
	uint64_t intdata = 0;
	uint32_t flowstartsec = 0;
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
		for( j=0; strcmp(identify[j].cname,"END") != 0; j++)
		{ 
			
			if(columns_names[i] == identify[j].cname)
			{	
				int notfound = 1;
				strncat(ColValues,"'",sizeof(char)+1);
				strncat(ColNames,columns_names[i],strlen(columns_names[i])+1);
				if( i != table->count_col-1)
					strncat(ColNames,",",sizeof(char)+1);
				if( i == table->count_col-1)
					strncat(ColNames,") ",(2*sizeof(char))+1);
				if(dataTemplateInfo->fieldCount > 0)
				{
					for(k=0; k < dataTemplateInfo->fieldCount; k++)
					{	
						if(dataTemplateInfo->fieldInfo[k].type.id == identify[j].ipfixid)
						{
							notfound = 0;						
							intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
							sprintf(stringtmp,"%Lu",(uint64_t)intdata);
							strncat(ColValues,stringtmp,strlen(stringtmp)+1);
							strncat(ColValues,"'",sizeof(char)+1);
							if(identify[j].ipfixid == IPFIX_TYPEID_flowStartSeconds)
							{
								flowstartsec = intdata;			
							}
							if( i !=  table->count_col-1)
								strncat(ColValues,",",sizeof(char)+1);
							if( i == table->count_col-1)
								strncat(ColValues,")",sizeof(char)+1);	
							break;
						}
					}	
				}
				if( dataTemplateInfo->dataCount > 0 && notfound)
				{
					for(n=0; n < dataTemplateInfo->dataCount; n++)
					{
						if(dataTemplateInfo->dataInfo[n].type.id == identify[j].ipfixid)
						{
							notfound = 0;
							intdata = getdata(dataTemplateInfo->dataInfo[n].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[n].offset));
							sprintf(stringtmp,"%Lu",(uint64_t)intdata);
							strncat(ColValues,stringtmp,strlen(stringtmp)+1);
							strncat(ColValues,"'",sizeof(char)+1);
							if( i != table->count_col-1)
								strncat(ColValues,",",sizeof(char)+1);
							if( i == table->count_col-1)
								strncat(ColValues,")",sizeof(char)+1);					
							break;
						}
					}
				}
				if(notfound)
				{	
					if(identify[j].ipfixid == ExporterID)
					{
						/**lookup exporter buffer to get exporterID from sourcID and expIP**/
						intdata = getExporterID(ipfixDbWriter, table, sourceID,0);
						sprintf(stringtmp,"%Lu",(uint64_t)intdata);
						strncat(ColValues,stringtmp,strlen(stringtmp)+1);
						strncat(ColValues,"'",sizeof(char)+1);
					}
					else
					{
						intdata = getdefaultIPFIXdata(identify[j].ipfixid);
						sprintf(stringtmp,"%Lu",(uint64_t)intdata);
						strncat(ColValues,stringtmp,strlen(stringtmp)+1);
						strncat(ColValues,"'",sizeof(char)+1);
					}
					if( i != table->count_col-1)
						strncat(ColValues,",",sizeof(char)+1);
					if( i == table->count_col-1)
						strncat(ColValues,")",sizeof(char)+1);					
				}
			}
		}
	}
		
	/**make hole query string for the insert statement*/
	char tablename[table_len] ;
	char* tablen = getTableName(ipfixDbWriter, table, flowstartsec);
	strcpy(tablename, tablen);
	/** Insert statement = INSERT INTO + tablename +  Columnsname + Values of record*/
	strncat(insert, tablename,strlen(tablename)+1);  
	strncat(insert,ColNames,strlen(ColNames)+1);
	strncat(insert,ColValues, strlen(ColValues)+1);
	
	
	char* insertTableStr = (char*) malloc((strlen(insert)+1)*sizeof(char));
	strcpy(insertTableStr,insert);
		
	return insertTableStr;
}
/**
*	Function writes the content of the statemBuffer to database
*	statemBuffer consist of single insert statements
*/
int writeToDb(IpfixDbWriter* ipfixDbWriter,Table* table, Statement* statement)
{
	int i ;
	
	char LockTables[start_len * maxTable] ; 
	strcpy(LockTables,"LOCK TABLES ");
	/**Look all tables in the buffer to store the insert statements*/
	for(i=0; i < maxTable; i++)
	{
		if(strcmp(table->TableBuffer[i].TableName,"NULL") != 0)
		{
			strncat(LockTables, table->TableBuffer[i].TableName,strlen(table->TableBuffer[i].TableName)+1);
			strncat(LockTables," WRITE",(6*sizeof(char))+1);
		}
		if(strcmp(table->TableBuffer[i+1].TableName,"NULL") != 0 && (i+1) < maxTable)
			strncat(LockTables,",",sizeof(char)+1);
	}
	if(mysql_query(ipfixDbWriter->conn, LockTables) != 0)
	{
		msg(MSG_FATAL,"Lock of table failed");
	}
	/**Write the insert statement to database*/
	for(i=0; i < maxstatement; i++)
	{
		if(mysql_query(ipfixDbWriter->conn, statement->statemBuffer[i]) != 0)
		{
			msg(MSG_FATAL,"Insert of records failed");
		}
		else
		{
			msg(MSG_DEBUG,"Record inserted");
		}
		free(statement->statemBuffer[i]);
		statement->statemBuffer[i] = "NULL";
	}
	
	char UnLockTable[start_len] = "UNLOCK TABLES";
	if(mysql_query(ipfixDbWriter->conn, UnLockTable) != 0)
	{
		msg(MSG_FATAL,"Unlock of tables failed");
	}
	statement->statemReceived = 0;
	msg(MSG_DEBUG,"Write to database is complete");
	return 0;
}

/**
*	Returns the tablename of a record according flowstartsec
*/
char* getTableName(IpfixDbWriter* ipfixDbWriter,Table*  table , uint64_t flowstartsec)
{
	int i;
	msg(MSG_DEBUG,"Content of TableBuffer :");
	for(i = 0; i < maxTable; i++)
	{
		msg(MSG_DEBUG,"TableStartTime : %Lu TableEndTime : %Lu TableName : %s", table->TableBuffer[i].startTableTime, table->TableBuffer[i].endTableTime, table->TableBuffer[i].TableName);	
	}
	/** Is  flowstartsec in intervall of tablecreationtime in buffer ?*/
	for(i = 0; i < maxTable; i++)
	{
		/**Is flowstartsec between  the range of tablecreattime and tablecreattime+30 min*/
		if(table->TableBuffer[i].startTableTime <= flowstartsec && flowstartsec < table->TableBuffer[i].endTableTime)
		{
			msg(MSG_DEBUG,"Table: %s is in TableBuffer",  table->TableBuffer[i].TableName);
			return table->TableBuffer[i].TableName;
		}
	}
	/**Tablename is not in TableBuffer*/	
	char tabNam[table_len];
	getTableNamDependTime(tabNam, flowstartsec);
	
	uint64_t startTime = getTableStartTime(flowstartsec);
	uint64_t endTime = getTableEndTime(startTime);

	table->TableBuffer[table->countbufftable].startTableTime = startTime;
	table->TableBuffer[table->countbufftable].endTableTime = endTime;
	strcpy(table->TableBuffer[table->countbufftable].TableName, tabNam);

	/** createTable when not in buffer*/
	if(createDBTable(ipfixDbWriter, table, table->TableBuffer[table->countbufftable].TableName) != 0)
	{	
		msg(MSG_DEBUG,"Struct bufentry clean up after failure");
		table->TableBuffer[table->countbufftable].startTableTime = 0;
		table->TableBuffer[table->countbufftable].endTableTime = 0;
		strcpy(table->TableBuffer[table->countbufftable].TableName,"NULL");
		return 0; 
	}
	/** If end of tablebuffer reached ?  Begin from  the start*/  
	if(table->countbufftable < maxTable-1)
	{
		table->countbufftable++;
	}
	else
	{
		table->countbufftable = 0;
	}
	/** countbufftable is null, when the last entry of tablebuffer is reached*/
	if(table->countbufftable == 0)
		return table->TableBuffer[maxTable-1].TableName;
	else 
		return table->TableBuffer[table->countbufftable-1].TableName;
}

/** 
 *	The tablename according to the time of the records when the flow is started
 *	The result is given by "h_YYYYMMDD_HH_0 || 1"
 *	0, when the recordtime of min is  0 <= min < 30, otherwise 1
 */
char* getTableNamDependTime(char* tablename, uint64_t flowstartsec)
{
	char strtmp[table_len];
	/** according to flowstartsec make the tablename*/
	time_t  t = flowstartsec;
	/**time in Coordinated Universal Time - UTC, it was formerly Greenwich Mean Time - GMT*/
	/** for use local time, change expression gmtime() to localtime()*/
	time_now = gmtime(&t);
	strcpy(tablename,"h_");
	sprintf(strtmp,"%u",time_now->tm_year+1900);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	sprintf(strtmp,"%02u",time_now->tm_mon+1);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	sprintf(strtmp,"%02u",time_now->tm_mday);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	strncat(tablename,"_",sizeof(char)+1);
	sprintf(strtmp,"%02u",time_now->tm_hour);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	strncat(tablename,"_",sizeof(char)+1);
	sprintf(strtmp,"%u",time_now->tm_min<30?0:1);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	
	return tablename;
}		

/**
*	Calculates the time of the tables according flowstartsec
*	It determines in which table the record must be store
*/
uint64_t getTableStartTime(uint64_t flowstartsec)
{
	uint64_t startTime;
	time_t  t = flowstartsec;
	time_now = localtime(&t);
	
	if(time_now->tm_min< 30)
	{
		time_now->tm_min = 0;
		time_now->tm_sec = 0;
		startTime = mktime(time_now);
		return startTime;
	}
	else
	{
		time_now->tm_min = 30;
		time_now->tm_sec = 0;
		startTime = mktime(time_now);
		return startTime;
	}
	return 0;
}
/** 
*	Tableendtime is the time that past since tablestarttime plus the time for the duration time
*	for tables to store
* 	1800 sec is equal for 30 min tables
*/
uint64_t getTableEndTime(uint64_t startTime)
{
	uint64_t endTime = startTime + 1800;
	return endTime;
}

/**
*	Returns the exporterID 
*  	For every different sourcID and expIP a unique ExporterID will be generated from the database
* 	First lookup for the ExporterID in the ExporterBuffer according sourceID and expIP, is there nothing
*  	lookup in the ExporterTable, is there also nothing insert sourceID and expIP an return the generated
*      ExporterID
*/
int getExporterID(IpfixDbWriter* ipfixDbWriter,Table* table, SourceID sourceID, uint64_t expIP)
{
	int i;
	msg(MSG_DEBUG,"Content of ExporterBuffer");
	for(i = 0; i < maxExpTable; i++)
	{
		msg(MSG_DEBUG,"exporterID:%d	   sourceID:%Lu	   expIP:%Lu",table->ExporterBuffer[i].Id, table->ExporterBuffer[i].srcID,table->ExporterBuffer[i].expIP);
	}
	/** Is the exporterID in ExporterBuffer*/
	for(i = 0; i < maxExpTable; i++)
	{
		if(table->ExporterBuffer[i].srcID==sourceID && table->ExporterBuffer[i].expIP== expIP  && table->ExporterBuffer[i].Id > 0)
		{
			msg(MSG_DEBUG,"Exporter sourceID/IP with ID %d is in the ExporterBuffer",table->ExporterBuffer[i].Id);
			return table->ExporterBuffer[i].Id;
		}
	}
	/**exporterID is not in ExporterBuffer*/
	MYSQL_RES* dbResult;
	MYSQL_ROW dbRow;
	int exporterID = 0;

	char selectStr[70] = "SELECT id FROM exporter WHERE sourceID=";
	char stringtmp[10];
	sprintf(stringtmp,"%u",sourceID);
	strncat(selectStr, stringtmp,strlen(stringtmp)+1);
	strncat(selectStr," AND srcIP=",(11*sizeof(char))+1);
	sprintf(stringtmp,"%Lu",expIP);
	strncat(selectStr, stringtmp,strlen(stringtmp)+1);
		
	if(mysql_query(ipfixDbWriter->conn, selectStr) != 0)
	{
		msg(MSG_DEBUG,"Select on exporter table failed");
		return 0;// If a failure occurs, return exporterID = 0
	}
	else
	{
		dbResult = mysql_store_result(ipfixDbWriter->conn);
		/** is the exporterID in the exporter table ?*/
		if(( dbRow = mysql_fetch_row(dbResult))) 
		{
			exporterID = atoi(dbRow[0]);
			mysql_free_result(dbResult);
			msg(MSG_DEBUG,"ExporterID %d is in exporter table",exporterID);
			/**Write new exporter in the ExporterBuffer*/
			table->ExporterBuffer[table->countExpTable].Id = exporterID;
			table->ExporterBuffer[table->countExpTable].srcID = sourceID;
			table->ExporterBuffer[table->countExpTable].expIP = expIP;
			
			if(table->countExpTable < maxExpTable-1)
			{
				table->countExpTable++;
			}
			else
			{
				table->countExpTable = 0;					
			}
			return exporterID;
		}
		else
		{
			/**ExporterID is not in exporter table - insert expID and expIP and return the exporterID*/
			char LockExporter[start_len] = "LOCK TABLES exporter WRITE";
			char UnLockExporter[start_len] = "UNLOCK TABLES";
			char insertStr[70] = "INSERT INTO exporter (ID,sourceID,srcIP) VALUES('NULL','";
			sprintf(stringtmp,"%u",sourceID);
			strncat(insertStr,stringtmp,strlen(stringtmp)+1);
			strncat(insertStr,"','",(3*sizeof(char))+1);
			sprintf(stringtmp,"%Lu",expIP);
			strncat(insertStr, stringtmp,strlen(stringtmp)+1);
			strncat(insertStr,"')",(2*sizeof(char))+1);	
			
			mysql_free_result(dbResult);
			if(mysql_query(ipfixDbWriter->conn, LockExporter) != 0)
			{
				msg(MSG_FATAL,"Lock of exporter table failed");
			}
			
			if(mysql_query(ipfixDbWriter->conn, insertStr) != 0)
			{
				msg(MSG_DEBUG,"Insert in exporter table failed");
				/**Unlock the table when a failure occur*/
				if(mysql_query(ipfixDbWriter->conn, UnLockExporter) != 0)
				{
					msg(MSG_FATAL,"UnLock of exporter table failed");
				}
			}
			else
			{
				exporterID = mysql_insert_id(ipfixDbWriter->conn); 
				msg(MSG_DEBUG,"ExporterID %d inserted in exporter table", exporterID);
				/**Write new exporter in the ExporterBuffer*/
				table->ExporterBuffer[table->countExpTable].Id = exporterID;
				table->ExporterBuffer[table->countExpTable].srcID = sourceID;
				table->ExporterBuffer[table->countExpTable].expIP = expIP;
			
				if(table->countExpTable < maxExpTable-1)
				{
					table->countExpTable++;
				}
				else
				{
					table->countExpTable = 0;					
				}
				
				if(mysql_query(ipfixDbWriter->conn, UnLockExporter) != 0)
				{
					msg(MSG_FATAL,"UnLock of exporter table failed");
				}
			}
			return exporterID;
		}
	}
}
/**
 *	Get data of the record is given by the IPFIX_TYPEID
*/
uint64_t getdata(FieldType type, FieldData* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
		return getipv4address(type, data);
	else
		return getIPFIXValue(type, data);
}
/**
 *	determine the ipv4address of the data record
 */
uint32_t getipv4address( FieldType type, FieldData* data)
{

	if (type.length > 5) 
	{
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return 0;
	}

	if ((type.length == 5) && ( type.id == IPFIX_TYPEID_sourceIPv4Address || IPFIX_TYPEID_destinationIPv4Address )) /*&& (imask != 0)*/ 
	{
		msg(MSG_DEBUG,"imask drop from ipaddress");
		type.length = 4;
	}
	
	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask == 0)*/ 
	{
		return getIPFIXValue(type, data);
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
	for( i=0; strcmp(identify[i].cname, "END") != 0; i++)
	{
		if(ipfixtype_id == identify[i].ipfixid)
		{
			return identify[i].default_value;
		}
	}
	return 0;
}




CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter *ipfixDbWriter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixDbWriter;
	ci.dataRecordCallbackFunction = writeDataRecord;
	ci.dataDataRecordCallbackFunction = writeDataDataRecord;
	
	return ci;
}
