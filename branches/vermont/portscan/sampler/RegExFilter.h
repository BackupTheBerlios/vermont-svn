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

#ifndef REGEXFILTER_H
#define REGEXFILTER_H

#include "common/msg.h"
#include "PacketProcessor.h"
#include "idmef/IDMEFExporter.h"

#include <list>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <boost/regex.hpp>



class RegExFilter : public PacketProcessor
{
	public:
		int filtertype;
		std::string match;
		boost::regex rexp;

		RegExFilter(IDMEFExporter* idmefexp, string filterid);
		virtual ~RegExFilter();

		void regcre();
		virtual bool processPacket (const Packet * p);


	private:
		IDMEFExporter* idmefExporter;
		string filterId;

		bool compare (char *data);
};

#endif
