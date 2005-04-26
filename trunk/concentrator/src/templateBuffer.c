/** \file
 * Template Buffer for rcvIpfix.
 *
 * Used by rcvIpfix to store Templates of all kinds
 *
 */

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "templateBuffer.h"
#include "common.h"

/***** Constants ************************************************************/


/***** Data Types ************************************************************/


/***** Global Variables ******************************************************/


/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Returns a TemplateInfo, OptionsTemplateInfo, DataTemplateInfo or NULL
 */
BufferedTemplate* getBufferedTemplate(TemplateBuffer* templateBuffer, SourceID sourceId, TemplateID templateId) {
	time_t now = time(0);
	BufferedTemplate* bt = templateBuffer->head;
	while (bt != 0) {
		if ((bt->sourceID == sourceId) && (bt->templateID == templateId)) {
			if ((bt->expires) && (bt->expires < now)) {
				destroyBufferedTemplate(templateBuffer, sourceId, templateId);
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
void bufferTemplate(TemplateBuffer* templateBuffer, BufferedTemplate* bt) {
	destroyBufferedTemplate(templateBuffer, bt->sourceID, bt->templateID);
	bt->next = templateBuffer->head;
	bt->expires = 0;
	templateBuffer->head = bt;
	}

/**
 * Frees memory, marks Template unused.
 */
void destroyBufferedTemplate(TemplateBuffer* templateBuffer, SourceID sourceId, TemplateID templateId) {
	BufferedTemplate* predecessor = 0;
	BufferedTemplate* bt = templateBuffer->head;
	while (bt != 0) {
		if ((bt->sourceID == sourceId) && (bt->templateID == templateId)) break;
		predecessor = bt;
		bt = (BufferedTemplate*)bt->next;
		}
	if (bt == 0) return;
	if (predecessor != 0) {
		predecessor->next = bt->next;
		} else {
		templateBuffer->head = (BufferedTemplate*)bt->next;
		}
	if (bt->setID == IPFIX_SetId_Template) {
		free(bt->templateInfo->fieldInfo);
		if (templateBuffer->templateDestructionCallbackFunction) templateBuffer->templateDestructionCallbackFunction(templateBuffer->ipfixAggregator, sourceId, bt->templateInfo);
		free(bt->templateInfo);
		} else
	if (bt->setID == IPFIX_SetId_OptionsTemplate) {
		free(bt->optionsTemplateInfo->scopeInfo);
		free(bt->optionsTemplateInfo->fieldInfo);
		if (templateBuffer->optionsTemplateDestructionCallbackFunction) templateBuffer->optionsTemplateDestructionCallbackFunction(templateBuffer->ipfixAggregator, sourceId, bt->optionsTemplateInfo);
		free(bt->optionsTemplateInfo);
		} else
	if (bt->setID == IPFIX_SetId_DataTemplate) {
		free(bt->dataTemplateInfo->fieldInfo);
		free(bt->dataTemplateInfo->dataInfo);
		free(bt->dataTemplateInfo->data);
		if (templateBuffer->dataTemplateDestructionCallbackFunction) templateBuffer->dataTemplateDestructionCallbackFunction(templateBuffer->ipfixAggregator, sourceId, bt->dataTemplateInfo);
		free(bt->dataTemplateInfo);
		} else {
		fatalf("Unknown template type requested to be freed: %d", bt->setID);
		}
	free(bt);
	}

/**
 * initializes the buffer
 */
TemplateBuffer* createTemplateBuffer() {
	TemplateBuffer* templateBuffer = (TemplateBuffer*)malloc(sizeof(TemplateBuffer));
	
	templateBuffer->head = 0;

	templateBuffer->templateDestructionCallbackFunction = 0;
	templateBuffer->dataTemplateDestructionCallbackFunction = 0;
	templateBuffer->optionsTemplateDestructionCallbackFunction = 0;
	
	return templateBuffer;
	}

/**
 * Destroys all buffered templates
 */
void destroyTemplateBuffer(TemplateBuffer* templateBuffer) {
	while (templateBuffer->head != 0) {
		BufferedTemplate* bt = templateBuffer->head;
 		BufferedTemplate* bt2 = (BufferedTemplate*)bt->next;
 		destroyBufferedTemplate(templateBuffer, bt->sourceID, bt->templateID);
 		templateBuffer->head = bt2;
 		}
	free(templateBuffer);
	}

