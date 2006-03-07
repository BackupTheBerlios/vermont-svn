#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "rcvIpfix.h"
#include "ipfix.h"
#include<mysql/mysql.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct{
	char *host_name ; 			 /* Hostname*/
	char *user_name ;    			/* Benutzername (Standard: Anmeldename) */
	char *password ;    			 /* Kennwort (Standard: keines) */
	unsigned int port_num; 		/* Portnummer (Standardwert verwenden) */
	char *socket_name ;	 		 /* Socketname (Standardwert verwenden) */
	char *db_name ;                           /* Datenbankname (Standard: keiner) */
	unsigned int flags;   			 /* Verbindungsflags (keine) */
	MYSQL *conn;                  		 /* Zeiger auf Verbindungs-Handle */	
} IpfixDbWriter;	


typedef struct{
	char *protocol;
	int sourceIP;
	int sourcePort;
	int destinationIP;
	int destinationPort;
	int TCPcontrolBits;
	int packetdeltacount;
	int ocktetdeltacount;
	int flowstartsseconds;
	int flowendsseconds;	
} IpfixTable;



int initializeIpfixDbWriter(IpfixDbWriter *ipfixDbWriter);
int deintializeIpfixDbWriter(IpfixDbWriter *ipfixDbWriter);

IpfixDbWriter *createIpfixDbWriter();
int destroyIpfixDbWriter();

void startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
void stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);

int InsertDbFlows(void *ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);



int printDataDataRecord(void* ipfixDbWriter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter* ipfixDbWriter);

#ifdef __cplusplus
}
#endif

#endif


