#ifndef TEMPLATEBUFFER_H
#define TEMPLATEBUFFER_H

#include "rcvIpfix.h"
#include "time.h"

/***** Constants ************************************************************/

#define TEMPLATE_EXPIRE_SECS  15

/***** Data Types ************************************************************/

/**
 * FIXME: add description
 */
typedef struct {
	SourceID	sourceID;     /**< source identifier of exporter that sent this template */
	TemplateID	templateID;   /**< template# this template defines */
	uint16_t	recordLength; /**< length of one record in the data set. Variable-length carry -1 */
	TemplateID	setID;        /**< should be 2,3,4 and determines the type of pointer used in the unions */
	time_t		expires;      /**< Timestamp when this Template will expire or 0 if it will never expire */
	union {
		TemplateInfo* templateInfo;
		OptionsTemplateInfo* optionsTemplateInfo;
		DataTemplateInfo* dataTemplateInfo;
		};
	union {
		TemplateDestructionCallbackFunction* templateDestructionCallbackFunction;
		DataTemplateDestructionCallbackFunction* dataTemplateDestructionCallbackFunction;
		OptionsTemplateDestructionCallbackFunction* optionsTemplateDestructionCallbackFunction;
		};
	void*	next;             /**< Pointer to next buffered Template */
	} BufferedTemplate;

/**
 * FIXME: add description
 */	
typedef struct {
	BufferedTemplate* head;
	} TemplateBuffer;


/***** Prototypes ************************************************************/

TemplateBuffer* createTemplateBuffer();
void destroyTemplateBuffer(TemplateBuffer* templateBuffer);
BufferedTemplate* getBufferedTemplate(TemplateBuffer* templateBuffer, SourceID sourceId, TemplateID templateId);
void destroyBufferedTemplate(TemplateBuffer* templateBuffer, SourceID sourceId, TemplateID id);
void bufferTemplate(TemplateBuffer* templateBuffer, BufferedTemplate* bt);

#endif
