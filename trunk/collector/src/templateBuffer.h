/** \file
 * Template Buffer.
 * Used by rcvIpfix to store Templates of all kinds
 */

#ifndef TEMPLATEBUFFER_H
#define TEMPLATEBUFFER_H

#include "rcvIpfix.h"
#include "tools.h"
#include "time.h"

/***** Constants ************************************************************/


/***** Data Types ************************************************************/

/**
 * FIXME
 */
typedef struct {
	SourceID	sourceID;		/**< source identifier of exporter that sent this template */
	TemplateID	templateID;		/**< template# this template defines */
	uint16		recordLength;	/**< length of one record in the data set. Variable-length carry -1 */
	TemplateID	setID;			/**< should be 2,3,4 and determines the type of pointer used in the unions */
	time_t		expires;	/**< Timestamp when this Template will expire or 0 if it will never expire */
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
	byte*       next;           /**< Pointer to next buffered Template */
	} BufferedTemplate;

/***** Prototypes ************************************************************/

void initializeTemplateBuffer();
void deinitializeTemplateBuffer();
BufferedTemplate* getBufferedTemplate(SourceID sourceId, TemplateID templateId);
void destroyBufferedTemplate(SourceID sourceId, TemplateID id);
void bufferTemplate(BufferedTemplate* bt);

#endif
