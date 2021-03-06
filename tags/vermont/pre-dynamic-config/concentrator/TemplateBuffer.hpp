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

#ifndef TEMPLATEBUFFER_H
#define TEMPLATEBUFFER_H

#include "IpfixParser.hpp"
#include <time.h>
#include <boost/smart_ptr.hpp>

#define DEFAULT_TEMPLATE_EXPIRE_SECS  70

/**
 * Represents a Template Buffer
 */
class TemplateBuffer {
	public:

		struct BufferedTemplate;

		/**
		 * Represents a single Buffered Template
		 */
		struct BufferedTemplate {
			boost::shared_ptr<IpfixRecord::SourceID>	sourceID; /**< source identifier of exporter that sent this template */
			TemplateID	templateID; /**< template# this template defines */
			uint16_t	recordLength; /**< length of one Data Record that will be transferred in Data Sets. Variable-length carry -1 */
			TemplateID	setID; /**< should be 2,3,4 and determines the type of pointer used in the unions */
			time_t		expires; /**< Timestamp when this Template will expire or 0 if it will never expire */
			boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;
			boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;
			boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;
			TemplateBuffer::BufferedTemplate*	next; /**< Pointer to next buffered Template */
		};

		TemplateBuffer(IpfixParser* parentIpfixParser);
		~TemplateBuffer();

		TemplateBuffer::BufferedTemplate* getBufferedTemplate(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateID templateId);
		void destroyBufferedTemplate(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateID id);
		void bufferTemplate(TemplateBuffer::BufferedTemplate* bt);

	protected:
		TemplateBuffer::BufferedTemplate* head; /**< Start of BufferedTemplate chain */
		IpfixParser* ipfixParser; /**< Pointer to the ipfixParser which instantiated this TemplateBuffer */
};

#endif
