#ifndef EXP_TEMPLATEBUFFER_H
#define EXP_TEMPLATEBUFFER_H

#include "exp_rcvIpfix.h"
#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif


/***** Constants ************************************************************/

#define TEMPLATE_EXPIRE_SECS  70

/***** Data Types ************************************************************/

/**
 * Represents a single Buffered Template
 */
typedef struct {
	Exp_SourceID	sourceID;     /**< source identifier of exporter that sent this template */
	TemplateID	templateID;   /**< template# this template defines */
	uint16_t	recordLength; /**< length of one Data Record that will be transferred in Data Sets. Variable-length carry -1 */
	TemplateID	setID;        /**< should be 2,3,4 and determines the type of pointer used in the unions */
	time_t		expires;      /**< Timestamp when this Template will expire or 0 if it will never expire */
	union {
		ExpressTemplateInfo* templateInfo;
		ExpressOptionsTemplateInfo* optionsTemplateInfo;
		ExpressDataTemplateInfo* dataTemplateInfo;
	};
	void*	next;             /**< Pointer to next buffered Template */
} ExpressBufferedTemplate;

/**
 * Represents a Template Buffer
 */
typedef struct {
	ExpressBufferedTemplate* head; /**< Start of BufferedTemplate chain */
	ExpressIpfixParser* ipfixParser; /**< Pointer to the ipfixReceiver which instantiated this TemplateBuffer */
} TemplateBuffer;


/***** Prototypes ************************************************************/

TemplateBuffer* createTemplateBuffer(ExpressIpfixParser* parentIpfixParser);
void destroyTemplateBuffer(TemplateBuffer* templateBuffer);
ExpressBufferedTemplate* getBufferedTemplate(TemplateBuffer* templateBuffer, Exp_SourceID* sourceId, TemplateID templateId);
void destroyBufferedTemplate(TemplateBuffer* templateBuffer, Exp_SourceID* sourceId, TemplateID id);
void bufferTemplate(TemplateBuffer* templateBuffer, ExpressBufferedTemplate* bt);


#ifdef __cplusplus
}
#endif


#endif
