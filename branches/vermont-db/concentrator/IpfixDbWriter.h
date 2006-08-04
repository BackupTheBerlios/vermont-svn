#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"
#include<mysql/mysql.h>
#include <netinet/in.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
*	startlen : Length of statement for INSERT IN.., CREATE TABL.., CREATE DATA..
*	col_width : Length of the string denotes the name of the single columns and datatype to store in table
*	ins_width : Length of the string for insert statement in depency of count columns
* 	maxTable : count of tablenames 
*	maxstatem ; count of insertstatement to buffer before they store to database
*	Table_Not_Exists : Errornumber, when a table is unaware deleted
*/

#define start_len				50		
#define col_width				40
#define ins_width                        25
#define maxTable			4
#define maxstatem			10

#define Table_Not_Exists		1146

/** 
* 	get the values of data record specified by the type.id to store them to strings an
*	 them store these strings in a buffer to store in database
*/
typedef struct {
	int statemReceived;				/**counter of insert into statements*/
	char* statemBuffer1[maxstatem];	/**buffer  of char pointers to store the insert statements*/
	char** statemBuffer2;				/**char pointer to point to buffer within  insert statements*/
} Statement;

/** 
*	handle the different database name according of the records
*/
typedef struct {
	int countbufftable;				/**counter of buffered table names*/		
	char* tablebuffer[maxTable]; 		/**array of table names*/
} Table;

/**
*	IpfixDbWriter powered the communication to the database server
*	also between the other structs
*/
typedef struct {
	char* host_name ; 				 /** Hostname*/
	char* db_name;					/**Name of the database*/
	char* user_name ;    				/**Username (default: Standarduser) */
	char* password ;    				 /** Password (default: none) */
	unsigned int port_num; 			/** Portnumber (use default) */
	char* socket_name ;	 			 /** Socketname (use default) */
	unsigned int flags;   				 /** Connectionflags (none) */
	MYSQL* conn; 					/** pointer to connection handle */	
	Statement* statement;
	Table* table;
} IpfixDbWriter;

/**
*	Identify the depency between columns names and IPFIX_TYPEID working with a char pointer array
*	in this array there is also st	anding  the defaultvalue of  the IPFIX_TYPEID and the datatype to store in database
*/
struct column{
	char* cname;						/**column name*/
	int ipfixid;						/**IPFIX_TYPEID*/
	char* datatype;					/**which datatype to store in database*/
	int default_value;					/**when no IPFIX_TYPEID is stored in the record, use defaultvalue to store in database*/
};

int initializeIpfixDbWriter();
int deintializeIpfixDbWriter();

IpfixDbWriter* createIpfixDbWriter();
int createDB(IpfixDbWriter* ipfixDbWriter);
int  createDBTable(IpfixDbWriter* ipfixDbWriter, char* TableName);

int startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int destroyIpfixDbWriter();

int countColumns();
char* getTableName(IpfixDbWriter* ipfixDbWriter,Table* table, uint64_t flowstartsec);

int getsingleRecData(void* ipfixDbWriter,SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);
int writeToDb(IpfixDbWriter* ipfixDbWriter, Statement* statement);

uint64_t getdata(FieldType type, FieldData* data);
uint64_t getIPFIXdata(FieldType type, FieldData* data);
uint64_t getIPFIXValue(FieldType type, FieldData* data);
uint32_t getdefaultIPFIXdata(int ipfixtype);

uint8_t getProtocol(FieldType type, FieldData* data);
uint16_t getTransportPort(FieldType type, FieldData* data);
uint32_t getipv4address(FieldType type, FieldData* data);

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter* ipfixDbWriter);

#ifdef __cplusplus
}
#endif

#endif


