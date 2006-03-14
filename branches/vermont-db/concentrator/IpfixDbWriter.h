#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"
#include<mysql/mysql.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
	char* host_name ; 			 /* Hostname*/
	char* user_name ;    			/* Benutzername (Standard: Anmeldename) */
	char* password ;    			 /* Kennwort (Standard: keines) */
	unsigned int port_num; 		/* Portnummer (Standardwert verwenden) */
	char* socket_name ;	 		 /* Socketname (Standardwert verwenden) */
	char* db_name ;                           /* Datenbankname (Standard: keiner) */
	unsigned int flags;   			 /* Verbindungsflags (keine) */
	MYSQL* conn;                  		 /* Zeiger auf Verbindungs-Handle */	
} IpfixDbWriter;	

typedef struct{
	char* protocol;
	uint16_t sourcePort;
	char* sourceIP;
	uint16_t destinationPort;
	char* destinationIP;
	uint16_t TCPcontrolBits;
	uint16_t packetdeltacount;
	uint octetdeltacount;
	long long flowstartsseconds;
	long long flowendsseconds;	
} IpfixTable;

/*

typedef struct{
	int length;
} fieldtype;

typedef struct{
	fieldtype* colons;
	uint16_t colonscount;
	char* colonsname;
} columns;
*/


			


int initializeIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int deintializeIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);

IpfixDbWriter* createIpfixDbWriter();
int destroyIpfixDbWriter();

void startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
void stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);

int InsertDbFlows(void *ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

void getProtocol(IpfixTable *ipfixtable, FieldType type, FieldData* data);
uint16_t getTransportPort(FieldType type, FieldData* data);
void getipv4address(IpfixTable* ipfixtable, FieldType type, FieldData* data);
long long getdata(FieldType type, FieldData* data);
long long getdataValue(FieldType type, FieldData* data);

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter* ipfixDbWriter);

#ifdef __cplusplus
}
#endif

#endif


