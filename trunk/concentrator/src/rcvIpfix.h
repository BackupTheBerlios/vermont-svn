#ifndef RCVIPFIX_H
#define RCVIPFIX_H

#include <pthread.h>
#include <stdint.h>

/***** Constants ************************************************************/


/***** Data Types ***********************************************************/

typedef uint16_t SourceID;
typedef uint16_t TemplateID;
typedef uint16_t TypeId;
typedef uint16_t FieldLength;
typedef uint32_t EnterpriseNo;
typedef uint8_t FieldData;

/**
 * IPFIX field type and length.
 * if "id" is < 0x8000, i.e. no user-defined type, "eid" is 0
 */ 
typedef struct {
	TypeId id;          /**< type tag of this field, according to [INFO] */
	FieldLength length; /**< length in bytes of this field */	
	EnterpriseNo eid;   /**< enterpriseNo for user-defined data types (i.e. type >= 0x8000) */	
	} FieldType;

/**
 * Information describing a single field in the fields passed via various callback functions.
 */
typedef struct {
	FieldType type;
	uint16_t offset;          /**< offset in bytes from a data start pointer. For internal purposes 65535 is defined as yet unknown */
	} FieldInfo;

/**
 * Template description passed to the callback function when a new Template arrives.
 */
typedef struct {
	uint16_t   fieldCount;    /**< number of regular fields */
	FieldInfo* fieldInfo;     /**< array of FieldInfos describing each of these fields */
	void*      userData;      /**< pointer to a field that can be used by higher-level modules */
	} TemplateInfo;

/**
 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
 */
typedef struct {
	uint16_t   scopeCount;  /**< number of scope fields */
	FieldInfo* scopeInfo;   /**< array of FieldInfos describing each of these fields */
	uint16_t   fieldCount;  /**< number of regular fields. This is NOT the number of all fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	void*      userData;    /**< pointer to a field that can be used by higher-level modules */
	} OptionsTemplateInfo;

/**
 * DataTemplate description passed to the callback function when a new DataTemplate arrives.
 */
typedef struct {
	uint16_t   fieldCount;  /**< number of regular fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	uint16_t   dataCount;   /**< number of fixed-value fields */
	FieldInfo* dataInfo;    /**< array of FieldInfos describing each of these fields */
	FieldData* data;        /**< data start pointer for fixed-value fields */
	void*      userData;    /**< pointer to a field that can be used by higher-level modules */
	} DataTemplateInfo;

/*** Template Callbacks ***/

/**
 * Callback function invoked when a new Template arrives.
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 * @return 0 if packet handled successfully
 */
typedef int(TemplateCallbackFunction)(void* ipfixAggregator, SourceID sourceID, TemplateInfo* templateInfo);

/**
 * Callback function invoked when a new OptionsTemplate arrives.
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 * @return 0 if packet handled successfully
 */
typedef int(OptionsTemplateCallbackFunction)(void* ipfixAggregator, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo);

/**
 * Callback function invoked when a new DataTemplate arrives.
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this DataTemplate
 * @return 0 if packet handled successfully
 */
typedef int(DataTemplateCallbackFunction)(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);

/*** Template Destruction Callbacks ***/
  	 
/**
 * Callback function invoked when a Template is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 * @return 0 if packet handled successfully
 */
typedef int(TemplateDestructionCallbackFunction)(void* ipfixAggregator, SourceID sourceID, TemplateInfo* templateInfo);

/**
 * Callback function invoked when a OptionsTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 * @return 0 if packet handled successfully
 */
typedef int(OptionsTemplateDestructionCallbackFunction)(void* ipfixAggregator, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo);

/**
 * Callback function invoked when a DataTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this DataTemplate
 * @return 0 if packet handled successfully
 */
typedef int(DataTemplateDestructionCallbackFunction)(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);

/*** Data Callbacks ***/

/**
 * Callback function invoked when a new Data Record arrives.
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Record
 * @param templateInfo Pointer to a structure defining the Template used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return 0 if packet handled successfully
 */
typedef int(DataRecordCallbackFunction)(void* ipfixAggregator, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data);

/**
 * Callback function invoked when a new Options Record arrives.
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Record
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return 0 if packet handled successfully
 */
typedef int(OptionsRecordCallbackFunction)(void* ipfixAggregator, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data);

/**
 * Callback function invoked when a new Data Record with associated Fixed Values arrives.
 * @param ipfixAggregator handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Record
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 * @return 0 if packet handled successfully
 */
typedef int(DataDataRecordCallbackFunction)(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

/**
 * Represents a Collector.
 * Create with @c rcvIpfixUdpIpv4()
 */
typedef struct {
	int socket;
	pthread_mutex_t mutex;  /**< Mutex to pause receiving thread */
	pthread_t thread;	/**< Thread ID for this particular instance, to sync against etc */
	
	void* ipfixAggregator; /**< handle passed to the callback functions */

	TemplateCallbackFunction* templateCallbackFunction;
	DataTemplateCallbackFunction* dataTemplateCallbackFunction;
	OptionsTemplateCallbackFunction* optionsTemplateCallbackFunction;

	TemplateDestructionCallbackFunction* templateDestructionCallbackFunction;
	DataTemplateDestructionCallbackFunction* dataTemplateDestructionCallbackFunction;
	OptionsTemplateDestructionCallbackFunction* optionsTemplateDestructionCallbackFunction;
	
	DataRecordCallbackFunction* dataRecordCallbackFunction;
	OptionsRecordCallbackFunction* optionsRecordCallbackFunction;
	DataDataRecordCallbackFunction* dataDataRecordCallbackFunction;
	
	void* templateBuffer;  /**< TemplateBuffer* structure */
	} IpfixReceiver;

/***** Prototypes ***********************************************************/

int initializeRcvIpfix();
int deinitializeRcvIpfix();

void printFieldData(FieldType type, FieldData* pattern);

FieldInfo* getTemplateFieldInfo(TemplateInfo* ti, FieldType* type);
FieldInfo* getDataTemplateFieldInfo(DataTemplateInfo* ti, FieldType* type);
FieldInfo* getDataTemplateDataInfo(DataTemplateInfo* ti, FieldType* type);

IpfixReceiver* rcvIpfixUdpIpv4(uint16_t port);
void rcvIpfixClose(IpfixReceiver* ipfixReceiver);

void startRcvIpfix(IpfixReceiver* ipfixReceiver);
void stopRcvIpfix(IpfixReceiver* ipfixReceiver);

/*** Template Callbacks ***/
void setTemplateCallback(IpfixReceiver* ipfixReceiver, TemplateCallbackFunction* f, void* ipfixAggregator);
void setOptionsTemplateCallback(IpfixReceiver* ipfixReceiver, OptionsTemplateCallbackFunction* f, void* ipfixAggregator);
void setDataTemplateCallback(IpfixReceiver* ipfixReceiver, DataTemplateCallbackFunction* f, void* ipfixAggregator);

/*** Template Destruction Callbacks ***/
void setTemplateDestructionCallback(IpfixReceiver* ipfixReceiver, TemplateDestructionCallbackFunction* f, void* ipfixAggregator);
void setOptionsTemplateDestructionCallback(IpfixReceiver* ipfixReceiver, OptionsTemplateDestructionCallbackFunction* f, void* ipfixAggregator);
void setDataTemplateDestructionCallback(IpfixReceiver* ipfixReceiver, DataTemplateDestructionCallbackFunction* f, void* ipfixAggregator);

/*** Data Callbacks ***/
void setDataRecordCallback(IpfixReceiver* ipfixReceiver, DataRecordCallbackFunction* f, void* ipfixAggregator);
void setOptionsRecordCallback(IpfixReceiver* ipfixReceiver, OptionsRecordCallbackFunction* f, void* ipfixAggregator);
void setDataDataRecordCallback(IpfixReceiver* ipfixReceiver, DataDataRecordCallbackFunction* f, void* ipfixAggregator);

#endif
