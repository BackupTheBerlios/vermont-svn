/**************************************************************************/
/*    Copyright (C) 2007 Gerhard Muenz                                    */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

#include "countstore.h"

#include <cassert>


BloomFilter CountStore::bfilter;
int CountStore::countKeyId = 0;


CountStore::CountStore() 
{
    recordStarted = false;
    bfilter.clear();
}

CountStore::~CountStore() 
{
}

void CountStore::addFieldData(int id, byte* fieldData, int fieldDataLength, EnterpriseNo eid) 
{
    switch (id) {
	case IPFIX_TYPEID_sourceIPv4Address:
	case IPFIX_TYPEID_destinationIPv4Address:
	    /* fieldDataLength == 5 means fieldData == xxx.yyy.zzz/aa (ip address and netmask)
	       fieldDataLength == 4 means fieldData == xxx.yyy.zzz/0  (ip address)
	       */
	    if(fieldDataLength>=4)
	    {
		flowKey.append(fieldData, 4); // we ignore the netmask
		if(countKeyId == id)
		    countKey.set(fieldData, 4);
	    }
	    else
		msg(MSG_ERROR, "CountStore: IP address field too short.\n");
	    std::cout << "Ip ";
	    break;
	case IPFIX_TYPEID_sourceTransportPort:
	case IPFIX_TYPEID_destinationTransportPort:
	    if (fieldDataLength == 2)
	    {
		flowKey.append(fieldData, 2);
		if(countKeyId == id)
		    countKey.set(fieldData, 2);
	    }
	    else
		msg(MSG_ERROR, "CountStore: Invalide port field length.\n");
	    std::cout << "Port ";
	    break;
	case IPFIX_TYPEID_protocolIdentifier:
	    if (fieldDataLength == 1)
	    {
		flowKey.append(fieldData, 1);
		if(countKeyId == id)
		    countKey.set(fieldData, 1);
	    }
	    else
		msg(MSG_ERROR, "CountStore: Invalide protocol field length.\n");
	    std::cout << "Protocol ";
	    break;
	default:
	    break;
    }
}


bool CountStore::recordStart(SourceID id) 
{
    assert(recordStarted == false);
    flowKey.reset();
    countKey.reset();
    recordStarted = true;
    return true;
}

void CountStore::recordEnd() 
{
    assert(recordStarted == true);

    bool newFlowKey = (bfilter.testBeforeInsert(flowKey.data,flowKey.len) == false);

    CountMap::iterator iter = data.find(countKey);
    std::cout << "This IP-5-tuple ";
    if(iter != data.end()) 
    {
       if(newFlowKey) 
       {
	   iter->second +=1;
	   std::cout << "is new, update." << std::endl;
       }
       else
	   std::cout << "is already known." << std::endl;
    }
    else if(data.size() < (data.max_size()-1))
    {
	std::cout << "is new, new entry." << std::endl;
	data[countKey] = 1;
	if(!newFlowKey)
	    std::cout << "This was a bloomfilter colision!" << std::endl;
    }
    else
    {
	std::cout << "is new, but I'm out of memory. Update default." << std::endl;
	countKey.reset();
	if((iter = data.find(countKey)) != data.end())
	    iter->second += 1;
	else
	    data[countKey] = 1;
    }

    recordStarted = false;
}
