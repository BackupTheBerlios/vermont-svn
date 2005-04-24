#include "rcvIpfix.h"
#include "common.h"
#include "rules.h"
#include "hashing.h"
#include "config.h"
#include "sndIpfix.h"

struct concentrator {
	Rules *rules;
	IpfixSender *ipfixSender;
	IpfixReceiver *ipfixReceiver;
};

int processDataRecord(struct concentrator *c, SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data);
int processDataDataRecord(struct concentrator *c, SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data);
struct concentrator * initializeConcentrator();
int startExporter(struct concentrator *c, char* ip, uint16_t port);
int startAggregator(struct concentrator *c, char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime);
int startCollector(struct concentrator *c, uint16_t port);
void pollAggregator(struct concentrator *c);
void destroyConcentrator(struct concentrator *c);
