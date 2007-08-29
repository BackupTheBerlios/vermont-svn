/*
 * VERMONT 
 * Copyright (C) 2007 The Vermont development team (http://vermont.berlios.de)
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
 */

#ifndef StringFilter_H
#define StringFilter_H

#include <vector>
#include <string>
#include "common/msg.h"
#include "PacketProcessor.h"
#include "idmef/IDMEFExporter.h"



class StringFilter : public PacketProcessor
{

	public:
		static const char* PAR_FILTER_TYPE; // = "FILTER_TYPE";
		static const char* PAR_FILTER_ID; // = "FILTER_ID";

		std::vector<std::string> andFilters;
		std::vector<std::string> notFilters;
		IDMEFExporter* idmefExporter;

		StringFilter(IDMEFExporter* idmefexp, string filterid);
		virtual ~StringFilter();

		void addandFilter(std::string string);
		void addnotFilter(std::string string);
		std::string hexparser(const std::string input);
		virtual bool processPacket (const Packet * p);

	protected:
		string filterId;

		bool compare (unsigned char *data, std::string toMatch, unsigned int plength);
};

#endif
