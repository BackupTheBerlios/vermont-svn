#ifndef IPFIXDBREADER_H
#define IPFIXDBREADER_H

#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"
#include<mysql/mysql.h>
#include <netinet/in.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define maxTables		1
#define maxCol			10
#define table_length 		16
#define column_length	25

typedef struct {
	char* tablenames[maxTables];
	int tablecount;
	char* colnames[maxCol];
	int colcount;
} DBdata;


typedef struct {
	CallbackInfo* callbackInfo;
	int callbackCount;
	DBdata* Dbdata;
} DbReader;

/**
 *	IpfixDbReader powered the communication to the database server
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
	SourceID srcid;
	DbReader* Dbreader;
} IpfixDbReader;

struct columnDB {
	char* cname;						/**column name*/
	uint16_t ipfixid;					/**IPFIX_TYPEID*/
	uint8_t length;					/**IPFIX length*/
};

	

int initializeIpfixDbReader();
int deintializeIpfixDbReader();
int destroyIpfixDbReader(IpfixDbReader* ipfixDbReader);

int startIpfixDbReader(IpfixDbReader* ipfixDbReader);
int stopIpfixDbReader(IpfixDbReader* ipfixDbReader);

IpfixDbReader* createIpfixDbReader();

int getTables(IpfixDbReader* ipfixDbReader);
int getColumns(IpfixDbReader* ipfixDbReader);

int ReadFromDB(IpfixDbReader* ipfixDbReader);

int DbReaderSendNewTemplate(IpfixDbReader* ipfixDbReader,DataTemplateInfo* dataTemplateInfo);
int DbReaderSendDataTemplate(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo, int n);

void addIpfixDbReaderCallbacks(IpfixDbReader* ipfixDbReader, CallbackInfo handles);
#ifdef __cplusplus
}
#endif

#endif

