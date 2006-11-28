#include <string.h>
#include <stdlib.h>
#include "IpfixDbReader.h"
#include "msg.h"


/**** Internal types *********************************************************/

struct columnDB {
	char* cname;       /**column name*/
	uint16_t ipfixId;  /**IPFIX_TYPEID*/
	uint8_t length;    /**IPFIX length*/
};


/***** Global Variables ******************************************************/

struct columnDB tabs[] = {		 
	{"dstIP", IPFIX_TYPEID_destinationIPv4Address,4},
	{"srcIP", IPFIX_TYPEID_sourceIPv4Address, 4},	
	{"srcPort", IPFIX_TYPEID_sourceTransportPort, 2},	
	{"dstPort", IPFIX_TYPEID_destinationTransportPort, 2},
	{"proto",IPFIX_TYPEID_protocolIdentifier , 1},
	{"dstTos", IPFIX_TYPEID_classOfServiceIPv4, 1},
	{"bytes", IPFIX_TYPEID_octetDeltaCount,  8},
	{"pkts", IPFIX_TYPEID_packetDeltaCount, 8},
	{"firstSwitched", IPFIX_TYPEID_flowStartSeconds, 4},
	{"lastSwitched", IPFIX_TYPEID_flowEndSeconds, 4},
	{"END"}
} ;


/***** Internal Functions ****************************************************/

int getTables(IpfixDbReader* ipfixDbReader);
int getColumns(IpfixDbReader* ipfixDbReader);

void* readFromDB(void* ipfixDbReader);

int DbReaderSendNewTemplate(IpfixDbReader* ipfixDbReader,DataTemplateInfo* dataTemplateInfo);
int DbReaderSendDataTemplate(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo, int n);

int connectToDb(IpfixDbReader* ipfixDbReader,
		const char* hostName, const char* dbName, 
		const char* username, const char* password,
		unsigned int port, SourceID sourceId);


/**
 * First send a a new template , then send the dataTemplates for the
 * count of tables
 */
// TODO: make this run in a thread!!!!
void* readFromDB(void* ipfixDbReader_)
{
	int i;
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)ipfixDbReader_;

	DataTemplateInfo* dataTemplateInfo = (DataTemplateInfo*)malloc(sizeof(DataTemplateInfo));
	DbData* dbData = ipfixDbReader->dbReader->dbData;

	// TODO: make IpfixDbReader exit if exit was requested!
	for(i = 0; i < dbData->tableCount && i < maxTables; i++) {
		pthread_mutex_lock(&ipfixDbReader->mutex);
		DbReaderSendNewTemplate(ipfixDbReader, dataTemplateInfo);
		DbReaderSendDataTemplate(ipfixDbReader, dataTemplateInfo,i);
		pthread_mutex_unlock(&ipfixDbReader->mutex);
	}

	msg(MSG_DEBUG,"sending from database is done");
	return 0;
}
/**
 * First  send a new template before sending a dataTemplate, because
 * the collector must be announced what for types and length of
 * data are following
*/
int DbReaderSendNewTemplate(IpfixDbReader* ipfixDbReader,DataTemplateInfo* dataTemplateInfo)
{
	int i,n;
	int fieldLength  = 0;

	DbReader* dbReader = ipfixDbReader->dbReader;
	DbData* dbData = dbReader->dbData;
	
	dataTemplateInfo->id =0;
	dataTemplateInfo->preceding= 0;	
	dataTemplateInfo->fieldCount = 0;
	dataTemplateInfo->fieldInfo = NULL;
	dataTemplateInfo->dataCount = 0;
	dataTemplateInfo->dataInfo = NULL;
	dataTemplateInfo->data = NULL;
	dataTemplateInfo->userData = NULL;
		
	for(i = 0; i < dbData->colCount; i++) {
		for(n = 0; strcmp(tabs[n].cname,"END") != 0; n++) {
			if(strcmp(dbData->colNames[i],tabs[n].cname) != 0) {
				continue;
			}
			
			dataTemplateInfo->fieldCount++;
			dataTemplateInfo->fieldInfo = realloc(dataTemplateInfo->fieldInfo,
							      sizeof(FieldInfo)*dataTemplateInfo->fieldCount);
			FieldInfo* fi = &dataTemplateInfo->fieldInfo[dataTemplateInfo->fieldCount - 1];	
			fi->type.id = tabs[n].ipfixId;
			fi->type.length = tabs[n].length;
			fi->type.eid = 0;
			fi->offset = fieldLength;
			fieldLength = fieldLength + fi->type.length; 
		}
	}

	for (n = 0; n != dbReader->callbackCount; n++) {
		CallbackInfo* ci = &dbReader->callbackInfo[n];
		if (ci->dataTemplateCallbackFunction) {
			ci->dataTemplateCallbackFunction(ci->handle, ipfixDbReader->srcId,
							 dataTemplateInfo);
			msg(MSG_DEBUG,"DbReader send new template");
		}
	}
	return 0;
}

/**
 * Select a given table and get the values by reading
 * the database. The Typs of the values from database are
 * strings, therefore they must change into IPFIX format 
*/

/* TODO: rename function */
int DbReaderSendDataTemplate(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo, int n)
{
	DbReader* dbReader = ipfixDbReader->dbReader;
	DbData* dbData = dbReader->dbData;
	int i,  j,k;
	int dataLength = 0;
	uint32_t flowstart = 0;
	uint32_t flowstartref = 0;				//current time 
	uint32_t firstflowstart = 0;				//old time of first flow starts
	uint32_t flowstart_change = 0;			//occupied when the starttime of the flows change in relation of current time
	uint32_t flowstartHBO = 0;				//flowstarttime in host-byte-order
	uint32_t flowend = 0;
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;

	// TODO:	
	FieldData* data = (FieldData*)malloc(sizeof(FieldInfo)*dataTemplateInfo->fieldCount);
	
	char selectStr[20] = "SELECT * FROM ";
	char select[50];
	strcpy(select,selectStr);
	strncat(select, dbData->tableNames[n],tableLength+1);
	// TODO: by lastSwitched
	strcat(select," ORDER BY firstSwitched");
	/** get all data from database*/
	if(mysql_query(ipfixDbReader->conn, select) != 0) {
		msg(MSG_DEBUG,"Select on table failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	}

	dbResult = mysql_store_result(ipfixDbReader->conn);
	while((dbRow = mysql_fetch_row(dbResult))) {
		for(i = 0; i < dbData->colCount; i++) {
			for(j = 0; j < dbData->colCount; j++) {
				if(strcmp(dbData->colNames[i], tabs[j].cname) != 0) {
					// we don't whant this colname
					continue;
				}
				if((tabs[j].ipfixId ==  IPFIX_TYPEID_octetDeltaCount) ||
				   (tabs[j].ipfixId == IPFIX_TYPEID_packetDeltaCount)) {	
					uint64_t dbentry = htonll(atoll(dbRow[i]));
					uint64_t* pdat = &dbentry;
					dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
					memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat, dataTemplateInfo->fieldInfo[i].type.length);
					break;
				}
				if((tabs[j].ipfixId == IPFIX_TYPEID_destinationIPv4Address)
				   || (tabs[j].ipfixId == IPFIX_TYPEID_sourceIPv4Address)) {
					uint32_t dbentry = htonl(atoll(dbRow[i]));
					uint32_t* pdat = &dbentry;
					dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
					memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
					break;
				}	
				// TODO:
				// - exportzeitpunkte anhand von lastswitched (nicht firstswitched)
				// - tut das unabhaengig von Reihenfolge der beiden zeitstempel in der tabelle --> zeitstempelumrechnung nach iteration
				if( tabs[j].ipfixId == IPFIX_TYPEID_flowStartSeconds ) {	
					/** set current flowstattime to flows*/
					if(firstflowstart == 0){
						firstflowstart = atoll(dbRow[i]);
						time_t t;
						flowstartref = time(&t);
						flowstart_change = flowstartref;
						flowstartHBO = flowstartref;
						flowstart = htonl(flowstartref);	
					}else {	
						// TODO: nur einmal berechnen
						flowstartHBO = 	flowstartref+atoll(dbRow[i])-firstflowstart;
						flowstart =htonl(flowstartHBO);
					}							
					uint32_t* pdat = &flowstart;
					dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
					memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
					break;
				}
				if(tabs[j].ipfixId == IPFIX_TYPEID_flowEndSeconds ) {		
					/** set current flowendtime to flows*/
					flowend = htonl(flowstartref+atoll(dbRow[i])-firstflowstart);				
					uint32_t* pdat = &flowend;
					dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
					memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
					break;
				}			
				if((tabs[j].ipfixId == IPFIX_TYPEID_sourceTransportPort) ||
				   (tabs[j].ipfixId == IPFIX_TYPEID_destinationTransportPort)) {
					uint16_t dbentry = htons(atol(dbRow[i]));
					uint16_t* pdat = &dbentry;
					dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
					memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
					break;
				}
				if((tabs[j].ipfixId == IPFIX_TYPEID_protocolIdentifier) ||
				   (tabs[j].ipfixId ==IPFIX_TYPEID_classOfServiceIPv4))	{
					uint8_t dbentry = atoi(dbRow[i]);
					uint8_t* pdat = &dbentry;
					dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
					memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
					break;
				}
			}
		}
		/** according to flowstarttime wait for sending the record*/
		if(flowstart_change != flowstartHBO) {
			sleep(flowstartHBO - flowstart_change);
			flowstart_change = flowstartHBO;
		}
		for (k = 0; k < dbReader->callbackCount; k++) {
			CallbackInfo* ci = &dbReader->callbackInfo[k];
			if (ci->dataDataRecordCallbackFunction) {
				ci->dataDataRecordCallbackFunction(ci->handle,
								   ipfixDbReader->srcId,
								   dataTemplateInfo,dataLength,data);
				msg(MSG_DEBUG,"DbReader send template");
			}
		}	
	}
	mysql_free_result(dbResult);
	
	return 0;
}

/**
 * get all tableNames in database that matches with the wildcard "h%"
 **/
int getTables(IpfixDbReader* ipfixDbReader)
{
	DbData* dbData = ipfixDbReader->dbReader->dbData;
	int i = 0;
	char* wild = "h%";
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	dbResult = mysql_list_tables(ipfixDbReader->conn, wild);
	if(dbResult == 0) {
		msg(MSG_FATAL,"There are no tables in database %s", ipfixDbReader->dbName);	
		return 1;
	}
	if(mysql_num_rows(dbResult) < maxTables) {
		msg(MSG_FATAL,"There are not so much tables in database as defined in maxTable");	
		return 1;
	}
	
	while(( dbRow = mysql_fetch_row(dbResult)) && i < maxTables) {
		char *table = (char*)malloc(sizeof(char) * (tableLength+1));
		strcpy(table,dbRow[0]);
		dbData->tableNames[i] = table;
		dbData->tableCount++;
		i++;
	}
	mysql_free_result(dbResult);
	
	return 0;
}

/**
 * Get the names of columns 
 */
int getColumns(IpfixDbReader* ipfixDbReader)
{
	DbData* dbData = ipfixDbReader->dbReader->dbData;
	int j = 0;
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	char showcolStr[50] = "SHOW COLUMNS FROM ";
	strncat(showcolStr, dbData->tableNames[0],strlen(dbData->tableNames[0])+1);
	if(mysql_query(ipfixDbReader->conn, showcolStr) != 0) {	
		msg(MSG_DEBUG,"Show columns on table %s failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	} else {
		dbResult = mysql_store_result(ipfixDbReader->conn);
		
		if(dbResult == 0) {
			msg(MSG_FATAL,"There are no Columns in the table");	
			return 1;
		}
		
		while((dbRow = mysql_fetch_row(dbResult))) {
			if(strcmp(dbRow[0],"exporterID") != 0) {
				char* column = (char*)malloc(sizeof(char)*columnLength);
				strcpy(column, dbRow[0]);
				dbData->colNames[j]=column;
				dbData->colCount++;
				j++;
			}
		}
	}
	mysql_free_result(dbResult);
	if(dbData->colCount > maxCol) {
		msg(MSG_DEBUG,"The Count of Columns differ from define");
		return 1;
	}
	return 0;
}


int connectToDb(IpfixDbReader* ipfixDbReader,
		const char* hostName, const char* dbName, 
		const char* userName, const char* password,
		unsigned int port, SourceID sourceId)

{
	/** get the mysl init handle*/
	ipfixDbReader->conn = mysql_init(0); 
	if(ipfixDbReader->conn == 0) {
		msg(MSG_FATAL,"Get MySQL connect handle failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"mysql init successfull");
	}
	/**Initialize structure members IpfixDbWriter*/
	ipfixDbReader->hostName = hostName;
	ipfixDbReader->dbName = dbName;
	ipfixDbReader->userName = userName;
	ipfixDbReader->password = password;
	ipfixDbReader->portNum = port;
	ipfixDbReader->socketName = 0;	  		
	ipfixDbReader->flags = 0;
	ipfixDbReader->srcId = sourceId;
	/**Initialize structure members DbReader*/
	ipfixDbReader->dbReader->callbackInfo = NULL;
	ipfixDbReader->dbReader->callbackCount = 0;
	/**Initialize structure members DbData*/
	ipfixDbReader->dbReader->dbData->colCount = 0;
	ipfixDbReader->dbReader->dbData->tableCount = 0;
	
	/**Connect to Database*/
	if (!mysql_real_connect(ipfixDbReader->conn,
				ipfixDbReader->hostName,
				ipfixDbReader->userName,ipfixDbReader->password,
				0, ipfixDbReader->portNum, ipfixDbReader->socketName,
				ipfixDbReader->flags)) {
		msg(MSG_FATAL,"Connection to database failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	}

	return 0;
}

/***** Exported Functions ****************************************************/


/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixDbReaders() {
	return 0;
}
																				 					     
/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbReaders() {
	return 0;
}																				 					     


/**
 * Starts or resumes database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int startIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	pthread_mutex_unlock(&ipfixDbReader->mutex);
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int stopIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	pthread_mutex_lock(&ipfixDbReader->mutex);
	return 0;
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
int destroyIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	mysql_close(ipfixDbReader->conn);
	if (!pthread_mutex_destroy(&ipfixDbReader->mutex)) {
		msg(MSG_ERROR, "Could not destroy mutex");
	}
	free(ipfixDbReader->dbReader->dbData);
	free(ipfixDbReader->dbReader);
	free(ipfixDbReader);

	return 0;
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReader* createIpfixDbReader(const char* hostName, const char* dbName, 
				   const char* userName, const char* password,
				   unsigned int port, SourceID sourceId)
{
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)malloc(sizeof(IpfixDbReader));
	if (!ipfixDbReader) {
		msg(MSG_ERROR, "Could not allocate IpfixDbReader");
		goto out0;
	}

	if (pthread_mutex_init(&ipfixDbReader->mutex, NULL)) {
		msg(MSG_FATAL, "Could not init mutex");
		goto out1;
	}

        if (pthread_mutex_lock(&ipfixDbReader->mutex)) {
                msg(MSG_FATAL, "Could not lock mutex");
                goto out1;
        }

	DbReader* dbReader = (DbReader*)malloc(sizeof(DbReader));
	if (!ipfixDbReader) {
		msg(MSG_ERROR, "Could not allocate DbReader");
		goto out1;
	}

	DbData* dbData = (DbData*)malloc(sizeof(DbData));
	if (!dbData) {
		msg(MSG_ERROR, "Could not allocate dbData");
		goto out2;
	}
	
	ipfixDbReader->dbReader = dbReader;
	dbReader->dbData = dbData;
	if (connectToDb(ipfixDbReader, hostName, dbName, userName,
			password, port, sourceId)) {
		goto out3;
	}
	msg(MSG_DEBUG,"Connected to database");
	
	/** use database  with db_name**/	
	if(mysql_select_db(ipfixDbReader->conn, ipfixDbReader->dbName) !=0) {
		msg(MSG_FATAL,"Database %s not selectable", ipfixDbReader->dbName);	
		goto out3;
	} else {
		msg(MSG_DEBUG,"Database %s selected", ipfixDbReader->dbName);
	}
	/** get tableNames of the database*/
	if(getTables(ipfixDbReader) != 0) {
		msg(MSG_DEBUG,"Error in function getTables");
		goto out3;
	}
	/**get columnsname of one table*/
	if(getColumns(ipfixDbReader) != 0) {
		msg(MSG_DEBUG,"Error in function getColumns");
		goto out3;
	}

	if (pthread_create(&ipfixDbReader->thread, 0, readFromDB, ipfixDbReader)) {
		msg(MSG_FATAL, "Could not create dbRead thread");
                goto out3;
	}
	
	return ipfixDbReader;

out3:
	free(dbData);
out2:
	free(dbReader);
out1:
	free(ipfixDbReader);
out0:	
	return NULL;
}


/**
 * Adds a set of callback functions to the list of functions to call
 * when Message from database is read.
 * @param ipfixDbReader IpfixDbReader to set the callback function for
 * @param handles set of callback functions
 */
void addIpfixDbReaderCallbacks(IpfixDbReader* ipfixDbReader, CallbackInfo handles) 
{
	DbReader* dbReader = ipfixDbReader->dbReader;
	int n = ++dbReader->callbackCount;
	dbReader->callbackInfo = (CallbackInfo*)realloc(dbReader->callbackInfo, 
							n * sizeof(CallbackInfo));
	memcpy(&dbReader->callbackInfo[n-1], &handles, sizeof(CallbackInfo));
}
