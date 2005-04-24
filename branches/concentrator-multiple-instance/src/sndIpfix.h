#ifndef SNDIPFIX_H
#define SNDIPFIX_H

#include "rcvIpfix.h"

/***** Constants ************************************************************/

#define SND_TEMPLATE_EXPIRE_SECS  15

/**
 * Represents an Exporter.
 * Create with @c sndIpfixUdpIpv4()
 */
typedef struct {
	char ip[128];
	uint16_t port;
	} IpfixSender;

/***** Prototypes ***********************************************************/

int initializeSndIpfix();
int deinitializeSndIpfix();

IpfixSender* sndIpfixUdpIpv4(char* ip, uint16_t port);
void sndIpfixClose(IpfixSender* ipfixSender);

int startSndIpfix(IpfixSender* ipfixSender);
int stopSndIpfix(IpfixSender* ipfixSender);

int sndNewDataTemplate(DataTemplateInfo* dataTemplateInfo);
int sndDestroyDataTemplate(DataTemplateInfo* dataTemplateInfo);
int sndDataDataRecord(DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

#endif
