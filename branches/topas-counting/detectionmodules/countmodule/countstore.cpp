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
bool CountStore::countPerSrcIp = false;
bool CountStore::countPerDstIp = false;
bool CountStore::countPerSrcPort = false;
bool CountStore::countPerDstPort = false;


CountStore::CountStore() : recordStarted(false)
{
    bfilter.clear();
}

CountStore::~CountStore() 
{
}

void CountStore::addFieldData(int id, byte* fieldData, int fieldDataLength, EnterpriseNo eid) 
{
    switch (id) {
	case IPFIX_TYPEID_sourceIPv4Address:
	    /* fieldDataLength == 5 means fieldData == xxx.yyy.zzz/aa (ip address and netmask)
	       fieldDataLength == 4 means fieldData == xxx.yyy.zzz/0  (ip address)
	       */
	    if(fieldDataLength>=4)
	    {
		srcIp.setAddress(fieldData);
		flowKey.append(fieldData, 4); // we ignore the netmask
	    }
	    else
		msg(MSG_ERROR, "CountStore: IP address field too short.\n");
	    std::cout << "Ip ";
	    break;
	case IPFIX_TYPEID_destinationIPv4Address:
	    /* fieldDataLength == 5 means fieldData == xxx.yyy.zzz/aa (ip address and netmask)
	       fieldDataLength == 4 means fieldData == xxx.yyy.zzz/0  (ip address)
	       */
	    if(fieldDataLength>=4)
	    {
		dstIp.setAddress(fieldData);
		flowKey.append(fieldData, 4); // we ignore the netmask
	    }
	    else
		msg(MSG_ERROR, "CountStore: IP address field too short.\n");
	    std::cout << "Ip ";
	    break;
	case IPFIX_TYPEID_sourceTransportPort:
	    if (fieldDataLength == 2)
	    {
		srcPort = (srcPort & 0xFFFF0000) + ntohs(*(uint16_t*)fieldData);
		flowKey.append(fieldData, 2);
	    }
	    else
		msg(MSG_ERROR, "CountStore: Invalide port field length.\n");
	    std::cout << "Port ";
	    break;
	case IPFIX_TYPEID_destinationTransportPort:
	    if (fieldDataLength == 2)
	    {
		dstPort = (dstPort & 0xFFFF0000) + ntohs(*(uint16_t*)fieldData);
		flowKey.append(fieldData, 2);
	    }
	    else
		msg(MSG_ERROR, "CountStore: Invalide port field length.\n");
	    std::cout << "Port ";
	    break;
	case IPFIX_TYPEID_protocolIdentifier:
	    if (fieldDataLength == 1)
	    {
		srcPort = (srcPort & 0x0000FFFF) + ((*fieldData)<<16);
		dstPort = (dstPort & 0x0000FFFF) + ((*fieldData)<<16);
		flowKey.append(fieldData, 1);
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
    recordStarted = true;

    srcIp.setAddress(0,0,0,0);
    dstIp.setAddress(0,0,0,0);
    srcPort = 0;
    dstPort = 0;

    return true;
}

void CountStore::recordEnd() 
{
    assert(recordStarted == true);

    bool newFlowKeyBf = (bfilter.testBeforeInsert(flowKey.data,flowKey.len) == false);
    bool newFlowKey = newFlowKeyBf;

    IpCountMap::iterator srcIpIter, dstIpIter; 
    PortCountMap::iterator srcPortIter, dstPortIter; 
    
    if(countPerSrcIp)
    {	
	srcIpIter = srcIpCounts.find(srcIp);
	if(srcIpIter == srcIpCounts.end())
	    newFlowKey = true;
    }
    if(countPerDstIp)
    {	
	dstIpIter = dstIpCounts.find(dstIp);
	if(dstIpIter == dstIpCounts.end())
	    newFlowKey = true;
    }
    if(countPerSrcPort)
    {	
	srcPortIter = srcPortCounts.find(srcPort);
	if(srcPortIter == srcPortCounts.end())
	    newFlowKey = true;
    }
    if(countPerDstPort)
    {	
	dstPortIter = dstPortCounts.find(dstPort);
	if(dstPortIter == dstPortCounts.end())
	    newFlowKey = true;
    }

    if(newFlowKey != newFlowKeyBf)
	std::cout << "This was a bloomfilter collision!" << std::endl;

    std::cout << std::endl;

    // SrcIp
    if(countPerSrcIp)
    {
	std::cout << "SrcIp: This IP-5-tuple ";
	if(srcIpIter != srcIpCounts.end()) 
	{
	    if(newFlowKey) 
	    {
		srcIpIter->second.flows +=1;
		std::cout << "is new, update." << std::endl;
	    }
	    else
		std::cout << "is already known." << std::endl;
	}
	else if(srcIpCounts.size() < (srcIpCounts.max_size()-1))
	{
	    std::cout << "is new, new entry." << std::endl;
	    srcIpCounts[srcIp].flows = 1;
	}
	else
	{
	    std::cout << "is new, but I'm out of memory. Update default." << std::endl;
	    srcIp.setAddress(0,0,0,0);
	    if((srcIpIter = srcIpCounts.find(srcIp)) != srcIpCounts.end())
		srcIpIter->second.flows += 1;
	    else
		srcIpCounts[srcIp].flows = 1;
	}
    }

    // DstIp
    if(countPerDstIp)
    {
	std::cout << "DstIp: This IP-5-tuple ";
	if(dstIpIter != dstIpCounts.end()) 
	{
	    if(newFlowKey) 
	    {
		dstIpIter->second.flows +=1;
		std::cout << "is new, update." << std::endl;
	    }
	    else
		std::cout << "is already known." << std::endl;
	}
	else if(dstIpCounts.size() < (dstIpCounts.max_size()-1))
	{
	    std::cout << "is new, new entry." << std::endl;
	    dstIpCounts[dstIp].flows = 1;
	}
	else
	{
	    std::cout << "is new, but I'm out of memory. Update default." << std::endl;
	    dstIp.setAddress(0,0,0,0);
	    if((dstIpIter = dstIpCounts.find(dstIp)) != dstIpCounts.end())
		dstIpIter->second.flows += 1;
	    else
		dstIpCounts[dstIp].flows = 1;
	}
    }

    // SrcPort
    if(countPerSrcPort)
    {
	std::cout << "SrcPort: This IP-5-tuple ";
	if(srcPortIter != srcPortCounts.end()) 
	{
	    if(newFlowKey) 
	    {
		srcPortIter->second.flows +=1;
		std::cout << "is new, update." << std::endl;
	    }
	    else
		std::cout << "is already known." << std::endl;
	}
	else if(srcPortCounts.size() < (srcPortCounts.max_size()-1))
	{
	    std::cout << "is new, new entry." << std::endl;
	    srcPortCounts[srcPort].flows = 1;
	}
	else
	{
	    std::cout << "is new, but I'm out of memory. Update default." << std::endl;
	    srcPort = 0;
	    if((srcPortIter = srcPortCounts.find(srcPort)) != srcPortCounts.end())
		srcPortIter->second.flows += 1;
	    else
		srcPortCounts[srcPort].flows = 1;
	}
    }

    // DstPort
    if(countPerDstPort)
    {
	std::cout << "DstPort: This IP-5-tuple ";
	if(dstPortIter != dstPortCounts.end()) 
	{
	    if(newFlowKey) 
	    {
		dstPortIter->second.flows +=1;
		std::cout << "is new, update." << std::endl;
	    }
	    else
		std::cout << "is already known." << std::endl;
	}
	else if(dstPortCounts.size() < (dstPortCounts.max_size()-1))
	{
	    std::cout << "is new, new entry." << std::endl;
	    dstPortCounts[dstPort].flows = 1;
	}
	else
	{
	    std::cout << "is new, but I'm out of memory. Update default." << std::endl;
	    dstPort = 0;
	    if((dstPortIter = dstPortCounts.find(dstPort)) != dstPortCounts.end())
		dstPortIter->second.flows += 1;
	    else
		dstPortCounts[dstPort].flows = 1;
	}
    }

    recordStarted = false;
}
