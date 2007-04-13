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

#ifndef PRINTIPFIX_H
#define PRINTIPFIX_H

#include "IpfixParser.hpp"

/**
 * IPFIX Printer module.
 *
 * Prints received flows to stdout 
 */
class IpfixPrinter : public FlowSink {
	public:
		IpfixPrinter();
		~IpfixPrinter();

		void start();
		void stop();

		int onDataTemplate(SourceID* sourceID, DataTemplateInfo* dataTemplateInfo);
		int onDataDataRecord(SourceID* sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);
		int onDataTemplateDestruction(SourceID* sourceID, DataTemplateInfo* dataTemplateInfo);

		int onOptionsTemplate(SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo);
		int onOptionsRecord(SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data);
		int onOptionsTemplateDestruction(SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo);

		int onTemplate(SourceID* sourceID, TemplateInfo* templateInfo);
		int onDataRecord(SourceID* sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data);
		int onTemplateDestruction(SourceID* sourceID, TemplateInfo* templateInfo);

	protected:
		void* lastTemplate;
};

#endif
