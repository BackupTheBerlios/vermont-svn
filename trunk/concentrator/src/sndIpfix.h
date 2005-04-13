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

void startSndIpfix(IpfixSender* ipfixSender);
void stopSndIpfix(IpfixSender* ipfixSender);

void sndNewDataTemplate(DataTemplateInfo* dataTemplateInfo);
void sndDestroyDataTemplate(DataTemplateInfo* dataTemplateInfo);
void sndDataDataRecord(DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

#endif
