/** \file
 * Generic IPFIX Collector.
 * Uses "rcvMessage" to receive a raw message. Parses it into 
 * IPFIX Records. These Record are then passed on to higher levels
 */

#ifndef RCVIPFIX_H
#define RCVIPFIX_H

#include "tools.h"

/***** Constants ************************************************************/

#define TEMPLATE_EXPIRE_SECS  15

/***** Data Types ***********************************************************/

/**
 * Information describing a single field in the fields passed via various callback functions.
 */
typedef struct {
	uint16 type;           /**< type tag of this field, according to [INFO] */
	uint16 length;         /**< length in bytes of this field */
	uint32 enterpriseNo;   /**< enterpriseNo for user-defined data types (type >= 0x8000) */
	uint16 offset;         /**< offset in bytes from a data start pointer. 65535 means unknown */
	} FieldInfo;

/**
 * Template description passed to the callback function when a new Template arrives.
 */
typedef struct {
	uint16     fieldCount;  /**< number of regular fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	byte*      userData;    /**< pointer to a field that can be used by higher-level modules */
	} TemplateInfo;

/**
 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
 */
typedef struct {
	uint16     scopeCount;  /**< number of scope fields */
	FieldInfo* scopeInfo;   /**< array of FieldInfos describing each of these fields */
	uint16     fieldCount;  /**< number of regular fields. This is NOT the number of all fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	byte*      userData;    /**< pointer to a field that can be used by higher-level modules */
	} OptionsTemplateInfo;

/**
 * DataTemplate description passed to the callback function when a new DataTemplate arrives.
 */
typedef struct {
	uint16     fieldCount;  /**< number of regular fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	uint16     dataCount;   /**< number of fixed-value fields */
	FieldInfo* dataInfo;    /**< array of FieldInfos describing each of these fields */
	byte*      data;        /**< data start pointer for fixed-value fields */
	byte*      userData;    /**< pointer to a field that can be used by higher-level modules */
	} DataTemplateInfo;

/*** Template Callbacks ***/

/**
 * Callback function invoked when a new Template arrives.
 * @param sourceId SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(TemplateCallbackFunction)(SourceID sourceID, TemplateInfo* templateInfo);

/**
 * Callback function invoked when a new OptionsTemplate arrives.
 * @param sourceId SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(OptionsTemplateCallbackFunction)(SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo);

/**
 * Callback function invoked when a new DataTemplate arrives.
 * @param sourceId SourceID of the exporter that sent this DataTemplate
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(DataTemplateCallbackFunction)(SourceID sourceID, DataTemplateInfo* dataTemplateInfo);

/*** Template Destruction Callbacks ***/

/**
 * Callback function invoked when a Template is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param sourceId SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(TemplateDestructionCallbackFunction)(SourceID sourceID, TemplateInfo* templateInfo);

/**
 * Callback function invoked when a OptionsTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param sourceId SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(OptionsTemplateDestructionCallbackFunction)(SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo);

/**
 * Callback function invoked when a DataTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param sourceId SourceID of the exporter that sent this DataTemplate
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(DataTemplateDestructionCallbackFunction)(SourceID sourceID, DataTemplateInfo* dataTemplateInfo);

/*** Data Callbacks ***/

/**
 * Callback function invoked when a new Data Record arrives.
 * @param sourceId SourceID of the exporter that sent this Record
 * @param templateInfo Pointer to a structure defining the Template used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(DataRecordCallbackFunction)(SourceID sourceID, TemplateInfo* templateInfo, uint16 length, byte* data);

/**
 * Callback function invoked when a new Options Record arrives.
 * @param sourceId SourceID of the exporter that sent this Record
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(OptionsRecordCallbackFunction)(SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16 length, byte* data);

/**
 * Callback function invoked when a new Data Record with associated Fixed Values arrives.
 * @param sourceId SourceID of the exporter that sent this Record
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 * @return true if packet handled successfully, false otherwise
 */
typedef boolean(DataDataRecordCallbackFunction)(SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16 length, byte* data);

/***** Prototypes ***********************************************************/

/*** Initialization, Finalization ***/

/**
 * Initializes internal data.
 * Call once before using any function in this module
 * @return true if call succeeded, false otherwise
 */
boolean initializeRcvIpfix();

/**
 * Destroys internal data.
 * Call once to tidy up. Do not use any function in this module afterwards
 * @return true if call succeeded, false otherwise
 */
boolean deinitializeRcvIpfix();

/**
 * Prepares a UDP/IPv4 socket.
 * Call startRcvIpfix() to start processing messages.
 * @param port Port to listen on
 * @return handle for rcvIpfixClose()
 */
int rcvIpfixUdpIpv4(uint16 port);

/**
 * Closes a socket.
 * @param handle Handle returned by rcvIpfixUdpIpv4()
 */
void rcvIpfixClose(int handle);

/**
 * Starts processing messages.
 * All sockets prepared by calls to rcvIpfixUdpIpv4() will start
 * receiving messages until stopRcvIpfix() is called.
 */
void startRcvIpfix();

/**
 * Stops processing messages.
 * No more messages will be processed until the next startRcvIpfix() call.
 */
void stopRcvIpfix();

/*** Template Callbacks ***/

/**
 * Sets the callback function to invoke when a new Template arrives.
 * @param templateCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setTemplateCallback(TemplateCallbackFunction* templateCallbackFunction);

/**
 * Sets the callback function to invoke when a new OptionsTemplate arrives.
 * @param optionsTemplateCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setOptionsTemplateCallback(OptionsTemplateCallbackFunction* optionsTemplateCallbackFunction);

/**
 * Sets the callback function to invoke when a new DataTemplate arrives.
 * @param dataTemplateCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setDataTemplateCallback(DataTemplateCallbackFunction* dataTemplateCallbackFunction);

/*** Template Destruction Callbacks ***/

/**
 * Sets the callback function to invoke when a Template is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param templateDestructionCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setTemplateDestructionCallback(TemplateDestructionCallbackFunction* templateDestructionCallbackFunction);

/**
 * Sets the callback function to invoke when a OptionsTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param optionsTemplateDestructionCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setOptionsTemplateDestructionCallback(OptionsTemplateDestructionCallbackFunction* optionsTemplateDestructionCallbackFunction);

/**
 * Sets the callback function to invoke when a DataTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param dataTemplateDestructionCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setDataTemplateDestructionCallback(DataTemplateDestructionCallbackFunction* dataTemplateDestructionCallbackFunction);

/*** Data Callbacks ***/

/**
 * Sets the callback function to invoke when a new Data Record arrives.
 * @param dataCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setDataRecordCallback(DataRecordCallbackFunction* dataCallbackFunction);

/**
 * Sets the callback function to invoke when a new Options Record arrives.
 * @param optionsCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setOptionsRecordCallback(OptionsRecordCallbackFunction* optionsCallbackFunction);

/**
 * Sets the callback function to invoke when a new Data Record with fixed fields arrives.
 * @param dataDataCallbackFunction pointer to the callback function
 * @return true if call succeeded, false otherwise
 */
void setDataDataRecordCallback(DataDataRecordCallbackFunction* dataDataCallbackFunction);

#endif
