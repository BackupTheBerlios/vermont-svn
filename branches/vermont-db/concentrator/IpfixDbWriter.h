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
*	columnsize : Length of the string denotes the name of the single columns and datatype to store in database
*	tableNamesize : Length of the string for insert statement in depency of count columns
*	dbn_len : Length of the database name
*	maxstatem ; count of insertstatement to buffer before they store to database
*	Table_Not_Exists : Errornumber, when a table is unaware deleted
*/

#define startlen				50		
#define columnsize			30
#define tableNamesize		30
#define maxDBname			4
#define dbn_len				16
#define maxstatem			50

#define Table_Not_Exists		1146

/** 
* 	get the values of data record specified by the type.id to store them to strings an
*	 them store these strings in a buffer to store in database
*/
typedef struct {
	int count_c;						/**columns counter*/
	int statemReceived;				/**counter of insert into statements*/
	char* createDatabaseStr;			/**string to create database*/
	char* createTableStr;				/**string to create tables of database*/
	char* insertTableStr;				/**string to store one insert statement*/
	char* statemBuffer1[maxstatem];	/**buffer  of char pointers to store the insert statements*/
	char** statemBuffer2;				/**char pointer to point to buffer within single insert statements*/
} Statement;

/** 
*	handle the different database name according of the records
*/
typedef struct {
	int notDbbuffer;
	int count_dbn;					/**counter of databse names*/		
	char* dbn;						/** databasename where is activ*/
	char* dbn_tmp;					/**database name for deleted tables*/
	char* db_buffer[maxDBname]; 		/**array of database names*/
	uint64_t flowstartsseconds;		/**ipfixtype_id to identify the database name according of the data record*/
} DBname;

/**
*	IpfixDbWriter powered the communication to the database server
*	also between the other structs
*/
typedef struct {
	char* host_name ; 				 /** Hostname*/
	char* user_name ;    				/**Username (default: Standarduser) */
	char* password ;    				 /** Password (default: none) */
	unsigned int port_num; 			/** Portnumber (use default) */
	char* socket_name ;	 			 /** Socketname (use default) */
	unsigned int flags;   				 /** Connectionflags (none) */
	MYSQL* conn;                  		 	/** pointer to connection handle */	
	Statement* statement;			/**pointer to structure statement*/
	DBname* dbName;				/**pointer to structure dbName*/
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
void createDB(IpfixDbWriter* ipfixDbWriter);
void createDBTable(IpfixDbWriter* ipfixDbWriter);
void useDB(IpfixDbWriter* ipfixDbWriter);

void startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
void stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
void destroyIpfixDbWriter();

void getDbnameRec(IpfixDbWriter* ipfixDbWriter);
void lookupBufferDBname(IpfixDbWriter* ipfixDbWriter);
void getTimeRec(IpfixDbWriter* ipfixDbWriter);

int getsingleRecData(void* ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);
void writeToDb(IpfixDbWriter* ipfixDbWriter);

uint64_t getdata(IpfixDbWriter* ipfixDbWriter, FieldType type, FieldData* data);
uint64_t getIPFIXdata(IpfixDbWriter* ipfixDbWriter, FieldType type, FieldData* data);
uint64_t getIPFIXValue(FieldType type, FieldData* data);
uint32_t getdefaultIPFIXdata(IpfixDbWriter* ipfixDbWriter, int ipfixtype);

uint8_t getProtocol(FieldType type, FieldData* data);
uint16_t getTransportPort(FieldType type, FieldData* data);
uint32_t getipv4address(FieldType type, FieldData* data);

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter* ipfixDbWriter);

#ifdef __cplusplus
}
#endif

#endif


