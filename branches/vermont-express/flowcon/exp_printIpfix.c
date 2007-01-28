/** @file
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib"
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "exp_printIpfix.h"

/***** Global Variables ******************************************************/

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int ExpressinitializeIpfixPrinters() {
	return 0;
}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int ExpressdeinitializeIpfixPrinters() {
	return 0;
}

/**
 * Creates a new IpfixPrinter. Do not forget to call @c startIpfixPrinter() to begin printing
 * @return handle to use when calling @c destroyIpfixPrinter()
 */
ExpressIpfixPrinter* ExpresscreateIpfixPrinter() {
	ExpressIpfixPrinter* ipfixPrinter = (ExpressIpfixPrinter*)malloc(sizeof(ExpressIpfixPrinter));

	return ipfixPrinter;
}

/**
 * Frees memory used by an IpfixPrinter
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void ExpressdestroyIpfixPrinter(ExpressIpfixPrinter* ipfixPrinter) {
	free(ipfixPrinter);
}

/**
 * Starts or resumes printing messages
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void ExpressstartIpfixPrinter(ExpressIpfixPrinter* ipfixPrinter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses printing messages
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void ExpressstopIpfixPrinter(ExpressIpfixPrinter* ipfixPrinter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Prints a Template
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int ExpressprintTemplate(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressTemplateInfo* templateInfo) {
	printf("\n-+--- Template\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a Template that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int ExpressprintTemplateDestruction(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressTemplateInfo* templateInfo) {
	printf("Destroyed a Template\n");

	return 0;
}

/**
 * Prints a DataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int ExpressprintDataRecord(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressTemplateInfo* templateInfo, uint16_t length, FieldData* data) {
	int i;

	printf("\n-+--- DataRecord\n");
	printf(" `- variable data\n");
	for (i = 0; i < templateInfo->fieldCount; i++) {
		printf(" '   `- ");
		ExpressprintFieldData(templateInfo->fieldInfo[i].type, (data + templateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a OptionsTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int ExpressprintOptionsTemplate(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressOptionsTemplateInfo* optionsTemplateInfo) {

	printf("\n-+--- OptionsTemplate\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int ExpressprintOptionsTemplateDestruction(void* ipfixPrinter_, Exp_SourceID* sourceID, ExpressOptionsTemplateInfo* optionsTemplateInfo) {
	printf("Destroyed an OptionsTemplate\n");

	return 0;
}

/**
 * Prints an OptionsRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int ExpressprintOptionsRecord(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressOptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data) {

	printf("\n-+--- OptionsDataRecord\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int ExpressprintDataTemplate(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo) {
	int i;

	printf("\n-+--- DataTemplate\n");
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		ExpressprintFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int ExpressprintDataTemplateDestruction(void* ipfixPrinter_, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo) {
	printf("Destroyed a DataTemplate\n");

	return 0;
}

/**
 * Prints a DataDataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID Exp_SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int ExpressprintDataDataRecord(void* ipfixPrinter, Exp_SourceID* sourceID, ExpressDataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	int i;

	printf("\n-+--- DataDataRecord\n");
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		ExpressprintFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `- variable data\n");
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		printf(" '   `- ");
		ExpressprintFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

ExpressCallbackInfo ExpressgetIpfixPrinterCallbackInfo(ExpressIpfixPrinter* ipfixPrinter) {
	ExpressCallbackInfo ci;
	bzero(&ci, sizeof(ExpressCallbackInfo));
	ci.handle = ipfixPrinter;
	ci.templateCallbackFunction = ExpressprintTemplate;
	ci.dataRecordCallbackFunction = ExpressprintDataRecord;
	ci.templateDestructionCallbackFunction = ExpressprintTemplateDestruction;
	ci.optionsTemplateCallbackFunction = ExpressprintOptionsTemplate;
	ci.optionsRecordCallbackFunction = ExpressprintOptionsRecord;
	ci.optionsTemplateDestructionCallbackFunction = ExpressprintOptionsTemplateDestruction;
	ci.dataTemplateCallbackFunction = ExpressprintDataTemplate;
	ci.dataDataRecordCallbackFunction = ExpressprintDataDataRecord;
	ci.dataTemplateDestructionCallbackFunction = ExpressprintDataTemplateDestruction;
	return ci;
}
