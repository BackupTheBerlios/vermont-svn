#ifndef EXP_PRINTIPFIX_H
#define EXP_PRINTIPFIX_H

#include "exp_rcvIpfix.h"

#ifdef __cplusplus
extern "C" {
#endif


/***** Constants ************************************************************/

/**
 * Represents an IpfixPrinter.
 * Create with @c createIpfixPrinter()
 */
typedef struct {
	int dummy;
} ExpressIpfixPrinter;

/***** Prototypes ***********************************************************/

int ExpressinitializeIpfixPrinters();
int ExpressdeinitializeIpfixPrinters();

ExpressIpfixPrinter* ExpresscreateIpfixPrinter();
void ExpressdestroyIpfixPrinter(ExpressIpfixPrinter* ipfixPrinter);

void ExpressstartIpfixPrinter(ExpressIpfixPrinter* ipfixPrinter);
void ExpressstopIpfixPrinter(ExpressIpfixPrinter* ipfixPrinter);

int ExpressprintDataTemplate(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo);
int ExpressprintDataDataRecord(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);
int ExpressprintDataTemplateDestruction(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo);

int ExpressprintOptionsTemplate(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressOptionsTemplateInfo* optionsTemplateInfo);
int ExpressprintOptionsRecord(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressOptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data);
int ExpressprintOptionsTemplateDestruction(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressOptionsTemplateInfo* optionsTemplateInfo);

int ExpressprintTemplate(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressTemplateInfo* templateInfo);
int ExpressprintDataRecord(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressTemplateInfo* templateInfo, uint16_t length, FieldData* data);
int ExpressprintTemplateDestruction(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressTemplateInfo* templateInfo);

ExpressCallbackInfo ExpressgetIpfixPrinterCallbackInfo(ExpressIpfixPrinter* ipfixPrinter);

#ifdef __cplusplus
}
#endif

#endif
