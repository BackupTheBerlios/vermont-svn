#include <string.h>
#include<stdlib.h>
#include "IpfixDbReader.h"
#include "msg.h"



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

/***** Exported Functions ****************************************************/


/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixDbReader() {
	return 0;
}
																				 					     
/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbReader() {
	return 0;
}																				 					     


/**
 * Starts or resumes database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int  startIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int stopIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
int destroyIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	deinitializeIpfixDbReader();
	mysql_close(ipfixDbReader->conn);
	free(ipfixDbReader->Dbreader->Dbdata);
	free(ipfixDbReader->Dbreader);
	free(ipfixDbReader);

	return 0;
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReader* createIpfixDbReader()
{
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)malloc(sizeof(IpfixDbReader));
	DbReader* dbreader = (DbReader*)malloc(sizeof(DbReader));
	DBdata* dbdata = (DBdata*)malloc(sizeof(DBdata));
	
	ipfixDbReader->Dbreader = dbreader;
	dbreader->Dbdata= dbdata;
	/** get the mysl init handle*/
	ipfixDbReader->conn = mysql_init(0); 
	if(ipfixDbReader->conn == 0)
	{
		msg(MSG_FATAL,"Get MySQL connect handle failed");
		goto out;
	}
	else
	{
		msg(MSG_DEBUG,"Get MySQL init handler");
	}
	/**Initialize structure members IpfixDbWriter*/
	ipfixDbReader->host_name = "localhost" ;
	ipfixDbReader->db_name = "nasty";
	ipfixDbReader->user_name = 0 ;    		
	ipfixDbReader->password = 0 ;    			
	ipfixDbReader->port_num = 0; 			
	ipfixDbReader->socket_name = 0 ;	  		
	ipfixDbReader->flags = 0;
	ipfixDbReader->srcid = 8888;
	/**Initialize structure members DbReader*/
	dbreader->callbackInfo = NULL;
	dbreader->callbackCount = 0;
	/**Initialize structure members DBdata*/
	dbdata->colcount = 0;
	dbdata->tablecount = 0;
	
	/**Connect to Database*/
	ipfixDbReader->conn = mysql_real_connect(ipfixDbReader->conn,
			ipfixDbReader->host_name, ipfixDbReader->user_name,ipfixDbReader->password,
			0, ipfixDbReader->port_num, ipfixDbReader->socket_name,
			ipfixDbReader->flags);
	if(ipfixDbReader->conn == 0)
	{
		msg(MSG_FATAL,"Connection to database failed");
		goto out;
	}
	else
	{
		msg(MSG_DEBUG,"Connect to database");		
	}
	
	/** use database  with db_name**/	
	if(mysql_select_db(ipfixDbReader->conn, ipfixDbReader->db_name) !=0)
	{
		msg(MSG_FATAL,"Database %s not selectable", ipfixDbReader->db_name);	
		goto out;
	}
	else
	{
		msg(MSG_DEBUG,"Database %s selected", ipfixDbReader->db_name);
	}
	/** get tablenames of the database*/
	if(getTables(ipfixDbReader) != 0)
	{
		msg(MSG_DEBUG,"Error in function getTables");
		goto out;
	}
	/**get columnsname of one table*/
	if(getColumns(ipfixDbReader) != 0)
	{
		msg(MSG_DEBUG,"Error in function getColumns");
		goto out;
	}
	
	return ipfixDbReader;
	
out:
		destroyIpfixDbReader(ipfixDbReader);
		
	return NULL;
}

/**
*	First send a a new template , then send the dataTemplates for the count of tables
*/
int ReadFromDB(IpfixDbReader* ipfixDbReader)
{
	int i;
	DataTemplateInfo* dataTemplateInfo = (DataTemplateInfo*)malloc(sizeof(DataTemplateInfo));
	DBdata* dbdat = ipfixDbReader->Dbreader->Dbdata;
	DbReaderSendNewTemplate(ipfixDbReader, dataTemplateInfo);
	for(i = 0; i < dbdat->tablecount && i < maxTables; i++)
	{
		DbReaderSendDataTemplate(ipfixDbReader, dataTemplateInfo,i);
	}
	msg(MSG_DEBUG,"sending from database is done");
	return 0;
}
/**
*  	First  send a new template before sending a dataTemplate, because the collector must be announced
* 	what for types and lengths of datas are following
*/
int DbReaderSendNewTemplate(IpfixDbReader* ipfixDbReader,DataTemplateInfo* dataTemplateInfo)
{
	int i,n;
	int fieldLength  = 0;

	DbReader* dbreader = ipfixDbReader->Dbreader;
	DBdata* dbdat = dbreader->Dbdata;
	
	dataTemplateInfo->id =0;
	dataTemplateInfo->preceding= 0;	
	dataTemplateInfo->fieldCount = 0;
	dataTemplateInfo->fieldInfo = NULL;
	dataTemplateInfo->dataCount = 0;
	dataTemplateInfo->dataInfo = NULL;
	dataTemplateInfo->data = NULL;
	dataTemplateInfo->userData = NULL;
		
	for(i = 0; i < dbdat->colcount; i++)
	{
		for(n = 0; strcmp(tabs[n].cname,"END") != 0; n++)
		{
			if(strcmp(dbdat->colnames[i],tabs[n].cname) == 0)
			{
				dataTemplateInfo->fieldCount++;
				dataTemplateInfo->fieldInfo = realloc(dataTemplateInfo->fieldInfo, sizeof(FieldInfo)*dataTemplateInfo->fieldCount);
				FieldInfo* fi = &dataTemplateInfo->fieldInfo[dataTemplateInfo->fieldCount - 1];	
				fi->type.id = tabs[n].ipfixid;
				fi->type.length = tabs[n].length;
				fi->type.eid = 0;
				fi->offset = fieldLength;
				fieldLength = fieldLength + fi->type.length; 
			}
		}
	}
	for (n = 0; n < dbreader->callbackCount; n++)
	{
		CallbackInfo* ci = &dbreader->callbackInfo[n];
		if (ci->dataTemplateCallbackFunction)
		{
			ci->dataTemplateCallbackFunction(ci->handle, ipfixDbReader->srcid,dataTemplateInfo);
			msg(MSG_DEBUG,"DBReader send new template");
		}
	}
	return 0;
}
/**
*	Select a given table and get the values by reading the database
* 	The Typs of the values from database are strings, therefore they must change into IPFIX format 
*/
int DbReaderSendDataTemplate(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo, int n)
{
	DbReader* dbreader = ipfixDbReader->Dbreader;
	DBdata* dbdat = dbreader->Dbdata;
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
	strncat(select, dbdat->tablenames[n],table_length+1);
	strcat(select," ORDER BY firstSwitched");
	/** get all data from database*/
	if(mysql_query(ipfixDbReader->conn, select) != 0)
	{
		msg(MSG_DEBUG,"Select on table faild");
		return 1;
	}
	else
	{
		dbResult = mysql_store_result(ipfixDbReader->conn);
		while((dbRow = mysql_fetch_row(dbResult)))
		{
			for(i = 0; i < dbdat->colcount; i++)
			{
				for(j = 0; j < dbdat->colcount; j++)
				{
					if(strcmp(dbdat->colnames[i], tabs[j].cname) == 0)
					{
						if((tabs[j].ipfixid ==  IPFIX_TYPEID_octetDeltaCount)  || (tabs[j].ipfixid == IPFIX_TYPEID_packetDeltaCount))
						{		
							uint64_t dbentry = htonll(atoll(dbRow[i]));
							uint64_t* pdat = &dbentry;
							dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
							memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat, dataTemplateInfo->fieldInfo[i].type.length);
							break;
						}
						if((tabs[j].ipfixid == IPFIX_TYPEID_destinationIPv4Address)  || (tabs[j].ipfixid == IPFIX_TYPEID_sourceIPv4Address))
						{								
							uint32_t dbentry = htonl(atoll(dbRow[i]));
							uint32_t* pdat = &dbentry;
							dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
							memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
							break;
						}	
						// TODO:
                                                // - exportzeitpunkte anhand von lastswitched (nicht firstswitched)
						// - tut das unabhaengig von Reihenfolge der beiden zeitstempel in der tabelle
						if( tabs[j].ipfixid == IPFIX_TYPEID_flowStartSeconds )
						{	
							/** set current flowstattime to flows*/
							if(firstflowstart == 0)
							{
								firstflowstart = atoll(dbRow[i]);
								time_t t;
								flowstartref = time(&t);
								flowstart_change = flowstartref;
								flowstartHBO = flowstartref;
								flowstart = htonl(flowstartref);	
							}
							else
							{	
								flowstartHBO = 	flowstartref+atoll(dbRow[i])-firstflowstart;
								flowstart =htonl(flowstartHBO);
							}							
							uint32_t* pdat = &flowstart;
							dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
							memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
							break;
						}
						if(tabs[j].ipfixid == IPFIX_TYPEID_flowEndSeconds )
						{		
							/** set current flowendtime to flows*/
							flowend = htonl(flowstartref+atoll(dbRow[i])-firstflowstart);				
							uint32_t* pdat = &flowend;
							dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
							memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
							break;
						}			
						if((tabs[j].ipfixid == IPFIX_TYPEID_sourceTransportPort)  || (tabs[j].ipfixid == IPFIX_TYPEID_destinationTransportPort))
						{	
							uint16_t dbentry = htons(atol(dbRow[i]));
							uint16_t* pdat = &dbentry;
							dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
							memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
							break;
						}
						if((tabs[j].ipfixid == IPFIX_TYPEID_protocolIdentifier) || (tabs[j].ipfixid ==IPFIX_TYPEID_classOfServiceIPv4))
						{								
							uint8_t dbentry = atoi(dbRow[i]);
							uint8_t* pdat = &dbentry;
							dataLength = dataLength + dataTemplateInfo->fieldInfo[i].type.length;
							memcpy(data+dataTemplateInfo->fieldInfo[i].offset,pdat,dataTemplateInfo->fieldInfo[i].type.length);
							break;
						}
					}
				}
			}
			/** according to flowstarttime wait for sending the template*/
			if(flowstart_change != flowstartHBO)
			{
				sleep(flowstartHBO - flowstart_change);
				flowstart_change = flowstartHBO;
			}
			for (k = 0; k < dbreader->callbackCount; k++)
			{
				CallbackInfo* ci = &dbreader->callbackInfo[k];
				if (ci->dataDataRecordCallbackFunction) 
				{
					ci->dataDataRecordCallbackFunction(ci->handle,ipfixDbReader->srcid, dataTemplateInfo,dataLength,data);
					msg(MSG_DEBUG,"DBReader send template");
				}
			}	
		}
		mysql_free_result(dbResult);
	}
	return 0;
}

/**
*	get all tablenames in database that matches with the wildcard "h%"
**/
int getTables(IpfixDbReader* ipfixDbReader)
{
	DBdata* dbdata = ipfixDbReader->Dbreader->Dbdata;
	int i = 0;
	char* wild = "h%";
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	dbResult = mysql_list_tables(ipfixDbReader->conn, wild);
	if(dbResult == 0)
	{
		msg(MSG_FATAL,"There are no tables in database %s", ipfixDbReader->db_name);	
		return 1;
	}
	if(mysql_num_rows(dbResult) < maxTables)
	{
		msg(MSG_FATAL,"There are not so much tables in database as defined in maxTable");	
		return 1;
	}
	
	while(( dbRow = mysql_fetch_row(dbResult)) && i < maxTables)
	{
		char *table = (char*)malloc(sizeof(char) * (table_length+1));
		strcpy(table,dbRow[0]);
		dbdata->tablenames[i] = table;
		dbdata->tablecount++;
		i++;
	}
	mysql_free_result(dbResult);
	
	return 0;
}		
/**
*	Get the names of columns 
**/
int getColumns(IpfixDbReader* ipfixDbReader)
{
	DBdata* dbdata = ipfixDbReader->Dbreader->Dbdata;
	int j = 0;
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	char showcolStr[50] = "SHOW COLUMNS FROM ";
	strncat(showcolStr, dbdata->tablenames[0],strlen(dbdata->tablenames[0])+1);
	if(mysql_query(ipfixDbReader->conn, showcolStr) != 0)
	{	
		msg(MSG_DEBUG,"Show columns on table %s failed");
		return 1;
	}
	else
	{
		dbResult = mysql_store_result(ipfixDbReader->conn);
		
		if(dbResult == 0)
		{
			msg(MSG_FATAL,"There are no Columns in the table");	
			return 1;
		}
		
		while((dbRow = mysql_fetch_row(dbResult)))
		{
			if(strcmp(dbRow[0],"exporterID") != 0)
			{
				char* column = (char*)malloc(sizeof(char)*column_length);
				strcpy(column, dbRow[0]);
				dbdata->colnames[j]=column;
				dbdata->colcount++;
				j++;
			}
		}
	}
	mysql_free_result(dbResult);
	if(dbdata->colcount > maxCol)
	{
		msg(MSG_DEBUG,"The Count of Columns differ from define");
		return 1;
	}
	return 0;
}

/**
 * Adds a set of callback functions to the list of functions to call when Message from database is read
 * @param ipfixDbReader IpfixDbReader to set the callback function for
 * @param handles set of callback functions
 */
void addIpfixDbReaderCallbacks(IpfixDbReader* ipfixDbReader, CallbackInfo handles) 
{
	DbReader* dbreader = ipfixDbReader->Dbreader;
	int n = ++dbreader->callbackCount;
	dbreader->callbackInfo = (CallbackInfo*)realloc(dbreader->callbackInfo, n * sizeof(CallbackInfo));
	memcpy(&dbreader->callbackInfo[n-1], &handles, sizeof(CallbackInfo));
}







