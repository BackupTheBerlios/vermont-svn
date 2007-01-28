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
#include "exp_templateBuffer.h"
#include "exp_ipfix.h"

#include "msg.h"
/***** Constants ************************************************************/

#define NetflowV9_SetId_Template  0


/***** Data Types ************************************************************/


/***** Global Variables ******************************************************/


/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Returns a TemplateInfo, OptionsTemplateInfo, DataTemplateInfo or NULL
 */
ExpressBufferedTemplate* getBufferedTemplate(TemplateBuffer* templateBuffer, Exp_SourceID* sourceId, TemplateID templateId) 
{
	time_t now = time(0);
	ExpressBufferedTemplate* bt = templateBuffer->head;
	while (bt != 0) {
		if ((bt->sourceID.observationDomainId == sourceId->observationDomainId) && (bt->templateID == templateId)) {
			if ((bt->expires) && (bt->expires < now)) {
				destroyBufferedTemplate(templateBuffer, sourceId, templateId);
				return 0;
			}
			return bt;
		}
		bt = (ExpressBufferedTemplate*)bt->next;
	}
	return 0;
}

/**
 * Saves a TemplateInfo, OptionsTemplateInfo, DataTemplateInfo overwriting existing Templates
 */
void bufferTemplate(TemplateBuffer* templateBuffer, ExpressBufferedTemplate* bt) 
{
	destroyBufferedTemplate(templateBuffer, &bt->sourceID, bt->templateID);
	bt->next = templateBuffer->head;
	bt->expires = 0;
	templateBuffer->head = bt;
}

/**
 * Frees memory, marks Template unused.
 */
void destroyBufferedTemplate(TemplateBuffer* templateBuffer, Exp_SourceID* sourceId, TemplateID templateId) 
{
	ExpressBufferedTemplate* predecessor = 0;
	ExpressBufferedTemplate* bt = templateBuffer->head;
	while (bt != 0) {
		if ((bt->sourceID.observationDomainId == sourceId->observationDomainId) && (bt->templateID == templateId)) {
			break;
		}
		predecessor = bt;
		bt = (ExpressBufferedTemplate*)bt->next;
	}
	if (bt == 0) return;
	if (predecessor != 0) {
		predecessor->next = bt->next;
	} else {
		templateBuffer->head = (ExpressBufferedTemplate*)bt->next;
	}
	if (bt->setID == IPFIX_SetId_Template) {
		free(bt->templateInfo->fieldInfo);

		/* Invoke all registered callback functions */
		int n;
		for (n = 0; n < templateBuffer->ipfixParser->callbackCount; n++) {
			ExpressCallbackInfo* ci = &templateBuffer->ipfixParser->callbackInfo[n];
			if (ci->templateDestructionCallbackFunction) {
				ci->templateDestructionCallbackFunction(ci->handle, sourceId, bt->templateInfo);
			}
		}

		free(bt->templateInfo);
	} else
#ifdef SUPPORT_NETFLOWV9
		if (bt->setID == NetflowV9_SetId_Template) {
			free(bt->templateInfo->fieldInfo);

			/* Invoke all registered callback functions */
			int n;
			for (n = 0; n < templateBuffer->ipfixParser->callbackCount; n++) {
				ExpressCallbackInfo* ci = &templateBuffer->ipfixParser->callbackInfo[n];
				if (ci->templateDestructionCallbackFunction) {
					ci->templateDestructionCallbackFunction(ci->handle, sourceId, bt->templateInfo);
				}
			}

			free(bt->templateInfo);
		} else
#endif
		if (bt->setID == IPFIX_SetId_OptionsTemplate) {
			free(bt->optionsTemplateInfo->scopeInfo);
			free(bt->optionsTemplateInfo->fieldInfo);

			/* Invoke all registered callback functions */
			int n;
			for (n = 0; n < templateBuffer->ipfixParser->callbackCount; n++) {
				ExpressCallbackInfo* ci = &templateBuffer->ipfixParser->callbackInfo[n];
				if (ci->optionsTemplateDestructionCallbackFunction) {
					ci->optionsTemplateDestructionCallbackFunction(ci->handle, sourceId, bt->optionsTemplateInfo);
				}
			}

			free(bt->optionsTemplateInfo);
		} else {
			if (bt->setID == IPFIX_SetId_DataTemplate) {
				free(bt->dataTemplateInfo->fieldInfo);
				free(bt->dataTemplateInfo->dataInfo);
				free(bt->dataTemplateInfo->data);

				/* Invoke all registered callback functions */
				int n;
				for (n = 0; n < templateBuffer->ipfixParser->callbackCount; n++) {
			        	ExpressCallbackInfo* ci = &templateBuffer->ipfixParser->callbackInfo[n];
					if (ci->dataTemplateDestructionCallbackFunction) {
						ci->dataTemplateDestructionCallbackFunction(ci->handle, sourceId, bt->dataTemplateInfo);
					}
				}

				free(bt->dataTemplateInfo);
			} else {
				msg(MSG_FATAL, "Unknown template type requested to be freed: %d", bt->setID);
			}
                }
        free(bt);
}

/**
 * initializes the buffer
 */
TemplateBuffer* createTemplateBuffer(ExpressIpfixParser* parentIpfixParser) {
	TemplateBuffer* templateBuffer = (TemplateBuffer*)malloc(sizeof(TemplateBuffer));

	templateBuffer->head = 0;
	templateBuffer->ipfixParser = parentIpfixParser;

	return templateBuffer;
}

/**
 * Destroys all buffered templates
 */
void destroyTemplateBuffer(TemplateBuffer* templateBuffer) {
	while (templateBuffer->head != 0) {
		ExpressBufferedTemplate* bt = templateBuffer->head;
		ExpressBufferedTemplate* bt2 = (ExpressBufferedTemplate*)bt->next;
		destroyBufferedTemplate(templateBuffer, &bt->sourceID, bt->templateID);
		templateBuffer->head = bt2;
	}
	free(templateBuffer);
}

