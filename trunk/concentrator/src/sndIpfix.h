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
	void* ipfixExporter; /**< underlying ipfix_exporter structure. Cast from void* to minimize header dependencies */
	uint16_t lastTemplateId; /**< Template ID of last created Template */
	char ip[128]; /**< IP of Collector we export to */
	uint16_t port; /**< Port of Collector we export to */
	} IpfixSender;

/***** Prototypes ***********************************************************/

int initializeSndIpfix();
int deinitializeSndIpfix();

IpfixSender* sndIpfixUdpIpv4(SourceID sourceID, char* ip, uint16_t port);
void sndIpfixClose(IpfixSender* ipfixSender);

void startSndIpfix(IpfixSender* ipfixSender);
void stopSndIpfix(IpfixSender* ipfixSender);

int sndNewDataTemplate(void* ipfixSender, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);
int sndDestroyDataTemplate(void* ipfixSender, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);
int sndDataDataRecord(void* ipfixSender, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

CallbackInfo getSenderCallbackInfo(IpfixSender* ipfixSender);

#endif
