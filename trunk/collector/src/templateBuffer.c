#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "templateBuffer.h"

/***** Constants ************************************************************/


/***** Data Types ************************************************************/


/***** Global Variables ******************************************************/

static BufferedTemplate* firstBufferedTemplate;

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Returns a TemplateInfo, OptionsTemplateInfo, DataTemplateInfo or NULL
 */
BufferedTemplate* getBufferedTemplate(SourceID sourceId, TemplateID templateId) {
	time_t now = time(0);
	BufferedTemplate* bt = firstBufferedTemplate;
	while (bt != 0) {
		if ((bt->sourceID == sourceId) && (bt->templateID == templateId)) {
			if ((bt->expires) && (bt->expires < now)) {
				destroyBufferedTemplate(sourceId, templateId);
				return 0;
				}
			return bt;
			}
		bt = (BufferedTemplate*)bt->next;
		}
	return 0;
	}

/**
 * Saves a TemplateInfo, OptionsTemplateInfo, DataTemplateInfo overwriting existing Templates
 */
void bufferTemplate(BufferedTemplate* bt) {
	destroyBufferedTemplate(bt->sourceID, bt->templateID);
	bt->next = (byte*)firstBufferedTemplate;
	bt->expires = 0;
	firstBufferedTemplate = bt;
	}

/**
 * Frees memory, marks Template unused.
 */
void destroyBufferedTemplate(SourceID sourceId, TemplateID templateId) {
	BufferedTemplate* predecessor = 0;
	BufferedTemplate* bt = firstBufferedTemplate;
	while (bt != 0) {
		if ((bt->sourceID == sourceId) && (bt->templateID == templateId)) break;
		predecessor = bt;
		bt = (BufferedTemplate*)bt->next;
		}
	if (bt == 0) return;
	if (predecessor != 0) {
		predecessor->next = (byte*)bt->next;
		} else {
		firstBufferedTemplate = (BufferedTemplate*)bt->next;
		}
	if (bt->setID == IPFIX_SetId_Template) {
		free(bt->templateInfo->fieldInfo);
		if (bt->templateDestructionCallbackFunction) bt->templateDestructionCallbackFunction(sourceId, bt->templateInfo);
		free(bt->templateInfo);
		} else
	if (bt->setID == IPFIX_SetId_OptionsTemplate) {
		free(bt->optionsTemplateInfo->scopeInfo);
		free(bt->optionsTemplateInfo->fieldInfo);
		if (bt->optionsTemplateDestructionCallbackFunction) bt->optionsTemplateDestructionCallbackFunction(sourceId, bt->optionsTemplateInfo);
		free(bt->optionsTemplateInfo);
		} else
	if (bt->setID == IPFIX_SetId_DataTemplate) {
		free(bt->dataTemplateInfo->fieldInfo);
		free(bt->dataTemplateInfo->dataInfo);
		free(bt->dataTemplateInfo->data);
		if (bt->dataTemplateDestructionCallbackFunction) bt->dataTemplateDestructionCallbackFunction(sourceId, bt->dataTemplateInfo);
		free(bt->dataTemplateInfo);
		} else {
		fatal("Unknown template type requested to be freed: %d", bt->setID);
		}
	free(bt);
	}

/**
 * initializes the buffer
 */
void initializeTemplateBuffer() {
	firstBufferedTemplate = 0;
	}

/**
 * Destroys all buffered templates
 */
void deinitializeTemplateBuffer() {
	while (firstBufferedTemplate != 0) {
		BufferedTemplate* bt = firstBufferedTemplate;
 		BufferedTemplate* bt2 = (BufferedTemplate*)bt->next;
 		destroyBufferedTemplate(bt->sourceID, bt->templateID);
 		firstBufferedTemplate = bt2;
 		}
	}

