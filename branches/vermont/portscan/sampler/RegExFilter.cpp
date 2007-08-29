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

#include "RegExFilter.h"

#include "StringFilter.h"
#include "common/Misc.h"


RegExFilter::RegExFilter(IDMEFExporter* idmefexp, string filterid)
	: idmefExporter(idmefexp), filterId(filterid)
{
	idmefExporter->setVariable(StringFilter::PAR_FILTER_ID, filterId);
	idmefExporter->setVariable(StringFilter::PAR_FILTER_TYPE, "Regexfilter");
}

RegExFilter::~RegExFilter()
{
}

void RegExFilter::regcre()
{
	rexp.assign(match);

}

bool RegExFilter::compare(char *pdata)
{

	if (boost::regex_search(pdata, rexp)) {
		return true;
	} 

	return false;
}

bool RegExFilter::processPacket(const Packet* p)
{
	const unsigned char* pdata;
	unsigned int plength;
	unsigned int payloadOffset;
	bool result;

	payloadOffset = p->payloadOffset;
	if (payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if (pdata == NULL) return false;

	result = compare((char*)pdata);

	if (result) {
		idmefExporter->setVariable(IDMEFExporter::PAR_SOURCE_ADDRESS, IPToString(*reinterpret_cast<uint32_t*>(p->netHeader+12)));
		idmefExporter->setVariable(IDMEFExporter::PAR_TARGET_ADDRESS, IPToString(*reinterpret_cast<uint32_t*>(p->netHeader+16)));
		idmefExporter->exportMessage();
	}

	return result;
}
