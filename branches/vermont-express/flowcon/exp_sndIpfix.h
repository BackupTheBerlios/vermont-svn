#ifndef EXP_SNDIPFIX_H
#define EXP_SNDIPFIX_H

#include "exp_rcvIpfix.h"
#include "ipfixlolib/ipfixlolib.h"

/***** Constants ************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Represents an Exporter.
 * Create with @c createIpfixSender()
 */
typedef struct {
	ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. Cast from void* to minimize header dependencies */
	uint16_t lastTemplateId; /**< Template ID of last created Template */
	char ip[128]; /**< IP of Collector we export to */
	uint16_t port; /**< Port of Collector we export to */
	uint32_t sentRecords; /**< Statistics: Total number of records sent since last statistics were polled */
} ExpressIpfixSender;

/***** Prototypes ***********************************************************/

int ExpressinitializeIpfixSenders();
int ExpressdeinitializeIpfixSenders();

ExpressIpfixSender* ExpresscreateIpfixSender(uint16_t observationDomainId, const char* ip, uint16_t port);
void ExpressdestroyIpfixSender(ExpressIpfixSender* ipfixSender);

void ExpressstartIpfixSender(ExpressIpfixSender* ipfixSender);
void ExpressstopIpfixSender(ExpressIpfixSender* ipfixSender);

int ipfixSenderAddCollector(ExpressIpfixSender *ips, const char *ip, uint16_t port);

int ExpresssndNewDataTemplate(void* ipfixSender, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo);
int ExpresssndDestroyDataTemplate(void* ipfixSender, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo);
int ExpresssndDataDataRecord(void* ipfixSender, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

ExpressCallbackInfo ExpressgetIpfixSenderCallbackInfo(ExpressIpfixSender* ipfixSender);

void statsIpfixSender(void* ipfixSender);

#ifdef __cplusplus
}
#endif

#endif

