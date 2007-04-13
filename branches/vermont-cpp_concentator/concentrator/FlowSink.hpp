/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef INCLUDE_FlowSink_hpp
#define INCLUDE_FlowSink_hpp

#include <stdint.h>

#define MAX_ADDRESS_LEN 16

struct ExporterAddress {
	char ip[MAX_ADDRESS_LEN];
	uint8_t len;
};

struct SourceID {
	uint32_t observationDomainId;
	ExporterAddress exporterAddress;
};

typedef uint16_t TemplateID;
typedef uint16_t TypeId;
typedef uint16_t FieldLength;
typedef uint32_t EnterpriseNo;
typedef uint8_t FieldData;

/**
 * IPFIX field type and length.
 * if "id" is < 0x8000, i.e. no user-defined type, "eid" is 0
 */ 
struct FieldType {
	TypeId id; /**< type tag of this field, according to [INFO] */
	FieldLength length; /**< length in bytes of this field */
	int isVariableLength; /**< true if this field's length might change from record to record, false otherwise */
	EnterpriseNo eid; /**< enterpriseNo for user-defined data types (i.e. type >= 0x8000) */	
};

/**
 * Information describing a single field in the fields passed via various callback functions.
 */
struct FieldInfo {
	FieldType type;
	uint16_t offset; /**< offset in bytes from a data start pointer. For internal purposes 65535 is defined as yet unknown */
};

/**
 * Template description passed to the callback function when a new Template arrives.
 */
struct TemplateInfo {
	uint16_t templateId; /**< the template id assigned to this template or 0 if we don't know or don't care */
	uint16_t fieldCount; /**< number of regular fields */
	FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
	void* userData; /**< pointer to a field that can be used by higher-level modules */
};

/**
 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
 */
struct OptionsTemplateInfo {
	uint16_t templateId; /**< the template id assigned to this template or 0 if we don't know or don't care */
	uint16_t scopeCount; /**< number of scope fields */
	FieldInfo* scopeInfo; /**< array of FieldInfos describing each of these fields */
	uint16_t fieldCount; /**< number of regular fields. This is NOT the number of all fields */
	FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
	void* userData; /**< pointer to a field that can be used by higher-level modules */
};

/**
 * DataTemplate description passed to the callback function when a new DataTemplate arrives.
 */
struct DataTemplateInfo {
	uint16_t id; /**< the template id assigned to this template or 0 if we don't know or don't care */
	uint16_t preceding; /**< the preceding rule field as defined in the draft */
	uint16_t fieldCount; /**< number of regular fields */
	FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
	uint16_t dataCount; /**< number of fixed-value fields */
	FieldInfo* dataInfo; /**< array of FieldInfos describing each of these fields */
	FieldData* data; /**< data start pointer for fixed-value fields */
	void* userData; /**< pointer to a field that can be used by higher-level modules */
};

/*
 * IPFIX Flow Sink class
 *
 * The IPFIX Flow Sink class servers as a base class for all modules 
 * which can receive and act upon IPFIX flows.
 */
class FlowSink {

	public:
		virtual ~FlowSink() {}

		/**
		 * Callback function invoked when a new Template arrives.
		 * @param sourceID SourceID of the exporter that sent this Template
		 * @param templateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onTemplate(SourceID* sourceID, TemplateInfo* templateInfo) { return -1; };

		/**
		 * Callback function invoked when a new DataTemplate arrives.
		 * @param sourceID SourceID of the exporter that sent this DataTemplate
		 * @param optionsTemplateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onOptionsTemplate(SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo) { return -1; };

		/**
		 * Callback function invoked when a new DataTemplate arrives.
		 * @param sourceID SourceID of the exporter that sent this DataTemplate
		 * @param dataTemplateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataTemplate(SourceID* sourceID, DataTemplateInfo* dataTemplateInfo) { return -1; };

		/**
		 * Callback function invoked when a new Data Record arrives.
		 * @param sourceID SourceID of the exporter that sent this Record
		 * @param templateInfo Pointer to a structure defining the Template used
		 * @param length Length of the data block supplied
		 * @param data Pointer to a data block containing all fields
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataRecord(SourceID* sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data) { return -1; };

		/**
		 * Callback function invoked when a new Options Record arrives.
		 * @param sourceID SourceID of the exporter that sent this Record
		 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
		 * @param length Length of the data block supplied
		 * @param data Pointer to a data block containing all fields
		 * @return 0 if packet handled successfully
		 */
		virtual int onOptionsRecord(SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data) { return -1; };

		/**
		 * Callback function invoked when a new Data Record with associated Fixed Values arrives.
		 * @param sourceID SourceID of the exporter that sent this Record
		 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
		 * @param length Length of the data block supplied
		 * @param data Pointer to a data block containing all variable fields
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataDataRecord(SourceID* sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) { return -1; };

		/**
		 * Callback function invoked when a Template is being destroyed.
		 * Particularly useful for cleaning up userData associated with this Template
		 * @param sourceID SourceID of the exporter that sent this Template
		 * @param templateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onTemplateDestruction(SourceID* sourceID, TemplateInfo* templateInfo) { return -1; };

		/**
		 * Callback function invoked when a OptionsTemplate is being destroyed.
		 * Particularly useful for cleaning up userData associated with this Template
		 * @param sourceID SourceID of the exporter that sent this OptionsTemplate
		 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
		 * @return 0 if packet handled successfully
		 */
		virtual int onOptionsTemplateDestruction(SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo) { return -1; };

		/**
		 * Callback function invoked when a DataTemplate is being destroyed.
		 * Particularly useful for cleaning up userData associated with this Template
		 * @param sourceID SourceID of the exporter that sent this DataTemplate
		 * @param dataTemplateInfo Pointer to a structure defining this DataTemplate
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataTemplateDestruction(SourceID* sourceID, DataTemplateInfo* dataTemplateInfo) { return -1; };
};

#endif

