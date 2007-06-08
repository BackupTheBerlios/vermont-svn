/**************************************************************************/
/*    Copyright (C) 2006 Romain Michalec                                  */
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
/*    License along with this library; if not, write to the Free Software   */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,          */
/*    MA  02110-1301, USA                                                 */
/*                                                                        */
/**************************************************************************/

#include "stat-store.h"
#include <fstream>


// ==================== STORAGE CLASS StatStore ====================


StatStore::StatStore()
  : e_source(IpAddress(0,0,0,0),32,0,0), e_dest(IpAddress(0,0,0,0),32,0,0) {

  packet_nb = byte_nb = 0;

}

StatStore::~StatStore() {

  PreviousData = Data;

}

bool StatStore::recordStart(SourceID sourceId) {

  if (BeginMonitoring != true)
    return false;

  packet_nb = byte_nb = 0;
  e_source = e_dest = EndPoint(IpAddress(0,0,0,0),32,0,0);

  return true;

}

void StatStore::addFieldData(int id, byte * fieldData, int fieldDataLength, EnterpriseNo eid) {
  // we subscribed to: see Stat::init_*()-functions
  // addFieldData will be executed until there are no more fieldData
  // in the current IPFIX record; so we are sure to get everything
  // we subscribed to (so don't get worried because of
  // the "breaks" in the "switch" loop hereafter)

  IpAddress SourceIP = IpAddress(0,0,0,0);
  IpAddress DestIP = IpAddress(0,0,0,0);

  switch (id) {

    case IPFIX_TYPEID_protocolIdentifier:

      if (fieldDataLength != IPFIX_LENGTH_protocolIdentifier) {
        std::cerr << "Error! Got invalid IPFIX field data (protocol)! "
      << "Skipping record.\n";
        return;
      }

      e_source.setProtocolID(*fieldData);
      e_dest.setProtocolID(*fieldData);

      break;


    case IPFIX_TYPEID_sourceIPv4Address:

      if (fieldDataLength != IPFIX_LENGTH_sourceIPv4Address) {
        std::cerr << "Error! Got invalid IPFIX field data (source IP)! "
      << "Skipping record.\n";
        return;
      }

      SourceIP.setAddress(fieldData[0],fieldData[1],fieldData[2],fieldData[3]);
      e_source.setIpAddress(SourceIP);

      break;


    case IPFIX_TYPEID_destinationIPv4Address:

      if (fieldDataLength != IPFIX_LENGTH_destinationIPv4Address) {
        std::cerr << "Error! Got invalid IPFIX field data (destination IP)! "
      << "Skipping record.\n";
        return;
      }

      DestIP.setAddress(fieldData[0],fieldData[1],fieldData[2],fieldData[3]);
      e_dest.setIpAddress(DestIP);

      break;

    // Ports do only matter, if endpoint_key contains "port"
    // AND (endpoint_key contains "protocol" AND TCP and/or UDP are selected
    // OR protocols dont matter)
    case IPFIX_TYPEID_sourceTransportPort:

      if (fieldDataLength != IPFIX_LENGTH_sourceTransportPort
      && fieldDataLength != IPFIX_LENGTH_sourceTransportPort-1) {
        std::cerr << "Error! Got invalid IPFIX field data (source port)! "
                  << "Skipping record.\n";
        return;
      }

      if (fieldDataLength == IPFIX_LENGTH_sourceTransportPort) {
        // fieldData must be casted into an int
        // (and, also, converted from network order to host order)
        e_source.setPortNr(ntohs(*(int*)fieldData));
        return;
      }

      if (fieldDataLength == IPFIX_LENGTH_sourceTransportPort-1) {
        // fieldData must be casted into an int
        // as it is a port number
        e_source.setPortNr((int)*fieldData);
        return;
      }

      break;


    case IPFIX_TYPEID_destinationTransportPort:

      if (fieldDataLength != IPFIX_LENGTH_destinationTransportPort
      && fieldDataLength != IPFIX_LENGTH_destinationTransportPort-1) {
        std::cerr << "Error! Got invalid IPFIX field data (destination port)! "
                  << "Skipping record.\n";
        return;
      }

      if (fieldDataLength == IPFIX_LENGTH_destinationTransportPort) {
        e_dest.setPortNr(ntohs(*(int*)fieldData));
        return;
      }

      if (fieldDataLength == IPFIX_LENGTH_destinationTransportPort-1) {
        e_dest.setPortNr((int)*fieldData);
        return;
      }

      break;


    case IPFIX_TYPEID_packetDeltaCount:

      if (fieldDataLength != IPFIX_LENGTH_packetDeltaCount) {
        std::cerr << "Error! Got invalid IPFIX field data (#packets)! "
                  << "Skipping record.\n";
        return;
      }
      packet_nb = ntohll(*(uint64_t*)fieldData);

      break;


    case IPFIX_TYPEID_octetDeltaCount:

      if (fieldDataLength != IPFIX_LENGTH_octetDeltaCount) {
        std::cerr << "Error! Got invalid IPFIX field data (#octets)! "
                  << "Skipping record.\n";
        return;
      }
      byte_nb = ntohll(*(uint64_t*)fieldData);

      break;


    default:

      std::cerr
      <<"Warning! Got unknown record in StatStore::addFieldData(...)!\n"
      <<"A programmer has probably added some record type in Stat::init()\n"
      <<"but has forgotten to ensure its support in StatStore::addFieldData().\n"
      <<"I'll try to keep working, but I can't tell for sure I won't crash.\n";
  }

  return;
}


// For every call to recordEnd, two endpoints will be considered:
// One consisting of SourceIP, SourcePort and Protocol (e_source)
// And one consisting of DestIP, DestPort and Protocol (e_dest)
void StatStore::recordEnd() {

  std::stringstream Warning;
  Warning
  << "WARNING: New EndPoint observed but EndPointListMaxSize reached!\n"
  << "Couldn't monitor new EndPoint: ";

// Handle EndPoint e_source (with SourceIP and SourcePort)

  // Consider only EndPoints we are interested in
  if (monitorEndPoint(e_source) == true || MonitorEveryEndPoint == true) {
    // EndPoint already known and thus in our List?
    if ( find(EndPointList.begin(), EndPointList.end(), e_source) != EndPointList.end() ) {
      // Since Data is destroyed after every test()-run,
      // we need to check, if the endpoint was already seen in the
      // current run
      std::map<EndPoint,Info>::iterator it = Data.find(e_source);
      if( it != Data.end() ) {
        it->second.packets_out += packet_nb;
        it->second.bytes_out += byte_nb;
        it->second.records_out++;
      }
      else {
        Data[e_source].packets_out += packet_nb;
        it = Data.find(e_source);
        it->second.bytes_out += byte_nb;
        it->second.records_out++;
      }
    }
    // EndPoint not known, still place to add it?
    else {
      if (EndPointList.size() < EndPointListMaxSize) {
        Info newinfo;
        newinfo.bytes_in = newinfo.packets_in = newinfo.records_in = 0;
        newinfo.packets_out = packet_nb;
        newinfo.bytes_out = byte_nb;
        newinfo.records_out = 1;
        Data.insert(std::make_pair<EndPoint,Info>(e_source, newinfo));
        EndPointList.push_back(e_source);
      }
      else
        std::cerr << Warning.str() << e_source << std::endl;
    }
  }


// Handle EndPoint e_dest (with DestIP and DestPort)

  // Consider only EndPoints we are interested in
  if (monitorEndPoint(e_dest) == true || MonitorEveryEndPoint == true) {
    // EndPoint already known and thus in our List?
    if ( find(EndPointList.begin(), EndPointList.end(), e_dest) != EndPointList.end() ) {
      // Since Data is destroyed after every test()-run,
      // we need to check, if the endpoint was already seen in the
      // current run
      std::map<EndPoint,Info>::iterator it = Data.find(e_dest);
      if ( it != Data.end() ) {
        it->second.packets_in += packet_nb;
        it->second.bytes_in += byte_nb;
        it->second.records_in++;
      }
      else {
        Data[e_dest].packets_in += packet_nb;
        it = Data.find(e_dest);
        it->second.bytes_in += byte_nb;
        it->second.records_in++;
      }
    }
    // EndPoint not known, still place to add it?
    else {
      if (EndPointList.size() < EndPointListMaxSize) {
        Info newinfo;
        newinfo.bytes_out = newinfo.packets_out = newinfo.records_out = 0;
        newinfo.packets_in = packet_nb;
        newinfo.bytes_in = byte_nb;
        newinfo.records_in = 1;
        Data.insert(std::make_pair<EndPoint,Info>(e_dest, newinfo));
        EndPointList.push_back(e_dest);
      }
      else
        std::cerr << Warning.str() << e_dest << std::endl;
    }
  }

  return;
}

// input from file (for offline usage)
std::ifstream& operator>>(std::ifstream& is, StatStore* store)
{

  if ( is.eof() ) {
    std::cerr << "INFORMATION: All Data read from file.\n";
    is.close();
    return is;
  }

  std::string tmp;
  store->Data.clear();
  while ( getline(is, tmp) ) {
    if (0 == strcasecmp(tmp.c_str(), "---") )
      break;
    else if ( is.eof() ) {
      std::cerr << "INFORMATION: All Data read from file.\n";
      is.close();
      return is;
    }

    // extract endpoint-data
    std::string::size_type i = tmp.find(':', 0);
    std::string ipstr(tmp, 0, i);
    std::string::size_type j = tmp.find('|', i);
    std::string portstr(tmp, i+1, j-i-1);
    std::string::size_type k = tmp.find('_', j);
    std::string protostr(tmp, j+1, k-j-1);

    IpAddress ip = IpAddress(0,0,0,0);
    ip.fromString(ipstr);
    EndPoint e = EndPoint(ip, 32, atoi(portstr.c_str()), atoi(protostr.c_str()));
    //std::cout << "e: " << e << std::endl;

    // extract metric-data
    std::stringstream tmp1(tmp.substr(k+1));
    Info info;
    tmp1 >> info.packets_in >> info.packets_out >> info.bytes_in >> info.bytes_out >> info.records_in >> info.records_out;

    // put it into is ...
    store->Data[e] = info;

    tmp.clear();
    tmp1.clear();
  }

  return is;
}

// returns true, if we are interested in EndPoint e
// (false otherwise)
bool StatStore::monitorEndPoint (EndPoint & e) {
  
  std::vector<EndPoint>::iterator it = EndPointFilter.begin();

  while ( it != EndPointFilter.end() ) {
    // apply netmask of *it (the current wanted endpoint)
    // to e (the tested endpoint)
    // (the netmask will be applied automatically by the call to setNetMask())
    // but only, if netmask is > 0 (Wildcard!) and < 32 (useless)
    if (it->getNetMask() > 0 && it->getNetMask() < 32)
      e.setNetMask(it->getNetMask());
    // compare ip addresses (netmask already applied); wildcard for ip address is netmask = 0
    // compare port number; wildcard for port is -1
    // compare protocol id; wildcard for protocol is -1
    if ( (e.getIpAddress() == it->getIpAddress() || it->getNetMask() == 0)
      && (e.getPortNr() == it->getPortNr() || it->getPortNr() == -1)
      && (e.getProtocolID() == it->getProtocolID() || it->getProtocolID() != -1) )
      return true;
    it++;
  }

  return false;
}

// ========== INITIALISATIONS OF STATIC MEMBERS OF CLASS StatStore ===========

std::map<EndPoint,Info> StatStore::PreviousData;

// even if the following members will be given their actual values
// by the Stat::init() function, we have to provide some initial values
// in the implementation file of the related class;

std::vector<EndPoint> StatStore::EndPointFilter;
bool StatStore::MonitorEveryEndPoint = false;

std::vector<EndPoint> StatStore::EndPointList;
int StatStore::EndPointListMaxSize = 0;

bool StatStore::BeginMonitoring = false;
