#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ExporterID        0	

/**
 * startlen  : Length of statement for INSERT IN.., CREATE TABL.., CREATE DATA..
 * col_width : Length of the string denotes the name of the single columns
 *             and datatype to store in table
 * ins_width : Length of the string for insert statement in depency of count columns
 * maxTable  : count of tablenames 
 * maxstatem : count of insertstatement to buffer before they store to database
 * table_len : Length of table name string
 */
#define start_len         50		
#define col_width         40
#define ins_width         25
#define maxTable           3
#define maxExpTable        3
#define maxstatement	  10
#define table_len         16


/**
 * Struct stores for each bufentry TableBuffer[maxTable]
 *  start-, endtime and tablename for the different tables
 */
typedef struct {
	uint64_t startTableTime;
	uint64_t endTableTime;				
	char TableName[table_len];
} bufentry;

/**
 * Store for each expTable ExporterBuffer[maxExpTable]
 * exporterID,srcID and expIP for the different exporters
 */
typedef struct {
	int Id;          /** Id entry of sourcID and expIP in the ExporterTable */
	uint64_t srcID;	 /** SourceID of  the exporter monitor */
	uint64_t  expIP; /** IP of the exporter */
} expTable;

/** 
 * Store the single statements for insert in a buffer until statemReceived is equal maxstatemt	 
 */
typedef struct {
	int statemReceived;               /**counter of insert into statements*/
	char* statemBuffer[maxstatement]; /**buffer  of char pointers to store the insert statements*/
} Statement;

/** 
*	makes a buffer for the different tables and the different exporters
*/
typedef struct {
	int count_col;                         /**counter of columns*/
	int countbufftable;		       /**counter of buffered table names*/
	bufentry TableBuffer[maxTable];	       /**buffer to store struct bufentry*/		
	int countExpTable;                     /**counter of buffered exporter*/
	expTable ExporterBuffer[maxExpTable];  /**buffer to store struct expTable*/
	Statement* statement;                  /**pointer to struct Statement*/
} Table;	

/**
 * IpfixDbWriter powered the communication to the database server
 * also between the other structs
 */
typedef struct {
	char* host_name;        /** Hostname*/
	char* db_name;          /**Name of the database*/
	char* user_name;      	/**Username (default: Standarduser) */
	char* password ;        /** Password (default: none) */
	unsigned int port_num;  /** Portnumber (use default) */
	char* socket_name ;     /** Socketname (use default) */
	unsigned int flags;     /** Connectionflags (none) */
	MYSQL* conn;            /** pointer to connection handle */	
	Table* table;           /**pointer to struct Table*/
	SourceID srcid;         /**Exporter default SourceID */
} IpfixDbWriter;

/**
 * Identify the depency between columns names and 
 * IPFIX_TYPEID working with a char pointer array
 * in this array there is also standing  the defaultvalue
 * of the IPFIX_TYPEID and the datatype to store in database
*/
struct column{
	char* cname;       /** column name */
	int ipfixid;       /** IPFIX_TYPEID */
	char* datatype;    /** which datatype to store in database */
	int default_value; /** when no IPFIX_TYPEID is stored in the record,
			    *  use defaultvalue to store in database
			    */
};

int initializeIpfixDbWriters();
int deinitializeIpfixDbWriters();

int startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int destroyIpfixDbWriter(IpfixDbWriter*  ipfixDbWriter);

IpfixDbWriter* createIpfixDbWriter();

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter* ipfixDbWriter);

#ifdef __cplusplus
}
#endif

#endif


