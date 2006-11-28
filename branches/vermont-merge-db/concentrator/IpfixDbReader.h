#ifndef IPFIXDBREADER_H
#define IPFIXDBREADER_H

#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

#include <mysql/mysql.h>

#ifdef __cplusplus
extern "C" {
#endif

#define maxTables        1     // count of tables that will be send
#define maxCol          10     // max count of columns in the table
#define tableLength    16     // tablename length
#define columnLength   25     // columnname legth

typedef struct {
	char* tableNames[maxTables];
	int tableCount;
	char* colNames[maxCol];
	int colCount;
} DbData;


typedef struct {
	CallbackInfo* callbackInfo;
	int callbackCount;
	DbData* dbData;
} DbReader;

/**
 *	IpfixDbReader powered the communication to the database server
 *	also between the other structs
 */
typedef struct {
	char* hostName;         /** Hostname*/
	char* dbName;           /**Name of the database*/
	char* userName ;    	 /**Username (default: Standarduser) */
	char* password ;    	 /** Password (default: none) */
	unsigned int portNum; 	 /** Portnumber (use default) */
	char* socketName ;      /** Socketname (use default) */
	unsigned int flags;      /** Connectionflags (none) */
	MYSQL* conn;             /** pointer to connection handle */	
	SourceID srcId;
	DbReader* dbReader;
	pthread_mutex_t mutex;   /** start/stop mutex for db replaying process */
	pthread_t thread;
} IpfixDbReader;

	

int initializeIpfixDbReaders();
int deinitializeIpfixDbReaders();
int destroyIpfixDbReader(IpfixDbReader* ipfixDbReader);

int startIpfixDbReader(IpfixDbReader* ipfixDbReader);
int stopIpfixDbReader(IpfixDbReader* ipfixDbReader);

IpfixDbReader* createIpfixDbReader();

void addIpfixDbReaderCallbacks(IpfixDbReader* ipfixDbReader, CallbackInfo handles);


#ifdef __cplusplus
}
#endif

#endif

