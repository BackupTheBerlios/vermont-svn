#ifndef SNDIPFIX_H
#define SNDIPFIX_H

#include <pthread.h>
#include "rcvIpfix.h"

/***** Constants ************************************************************/

/**
 * Represents an Exporter.
 * Create with @c createIpfixSender()
 */
typedef struct {
	void* ipfixExporter;     /**< underlying ipfix_exporter structure. Cast from void* to minimize header dependencies */
	uint16_t lastTemplateId; /**< Template ID of last created Template */
	char ip[128];            /**< IP of Collector we export to */
	uint16_t port;           /**< Port of Collector we export to */
	pthread_mutex_t mutex;   /**< Mutex to synchronize and/or pause sender */
	} IpfixSender;

/***** Prototypes ***********************************************************/

int initializeIpfixSenders();
int deinitializeIpfixSenders();

IpfixSender* createIpfixSender(SourceID sourceID, char* ip, uint16_t port);
void destroyIpfixSender(IpfixSender* ipfixSender);

void startIpfixSender(IpfixSender* ipfixSender);
void stopIpfixSender(IpfixSender* ipfixSender);

int sndNewDataTemplate(void* ipfixSender, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);
int sndDestroyDataTemplate(void* ipfixSender, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);
int sndDataDataRecord(void* ipfixSender, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

int sndNewTemplate(void* ipfixSender, SourceID sourceID, TemplateInfo* templateInfo);
int sndDestroyTemplate(void* ipfixSender, SourceID sourceID, TemplateInfo* templateInfo);
int sndDataRecord(void* ipfixSender, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data);

CallbackInfo getIpfixSenderCallbackInfo(IpfixSender* ipfixSender);

#endif
