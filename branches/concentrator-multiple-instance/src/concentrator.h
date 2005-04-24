#include "rcvIpfix.h"

int processDataRecord(SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data);
int processDataDataRecord(SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data);
void initializeConcentrator();
void startExporter(char* ip, uint16_t port);
void startAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime);
void startCollector(uint16_t port);
void pollAggregator();
void destroyConcentrator();

