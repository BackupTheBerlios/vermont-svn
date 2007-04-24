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
  : e_source(IpAddress(0,0,0,0),0,0), e_dest(IpAddress(0,0,0,0),0,0) {

  skip_both = skip_source = skip_dest = false;
  packet_nb = byte_nb = 0;

}

StatStore::~StatStore() {

  PreviousData = Data;

}

bool StatStore::recordStart(SourceID sourceId) {

  if (BeginMonitoring != true)
    return false;

  skip_both = skip_source = skip_dest = false;
  packet_nb = byte_nb = 0;
  e_source = e_dest = EndPoint(IpAddress(0,0,0,0),0,0);

  return true;

}

void StatStore::addFieldData(int id, byte * fieldData, int fieldDataLength, EnterpriseNo eid) {
  // we subscribed to: see Stat::init_*()-functions
  // addFieldData will be executed until there are no more fieldData
  // in the current IPFIX record; so we are sure to get everything
  // we subscribed to (so don't get worried because of
  // the "breaks" in the "switch" loop hereafter)

  // if one of the filters was matched, we dont want that endpoint
  // and so dont need to further addFieldData to it
  // But maybe one of the involved endpoints, either e_source or e_dest
  // is wanted, so we skip only, if really both arent wanted, i. e.
  // skip_both --> protocol isnt wanted
  // skip_source/dest --> source/dest ip or source/dest port isnt wanted
  if (skip_both == true || (skip_source == true && skip_dest == true))
    return;

  IpAddress SourceIP = IpAddress(0,0,0,0);
  IpAddress DestIP = IpAddress(0,0,0,0);

  switch (id) {

    case IPFIX_TYPEID_protocolIdentifier:

      if (fieldDataLength != IPFIX_LENGTH_protocolIdentifier) {
        std::cerr << "Error! Got invalid IPFIX field data (protocol)! "
      << "Skipping record.\n";
        return;
      }

      if ( find(MonitoredProtocols.begin(), MonitoredProtocols.end(), *fieldData)
            != MonitoredProtocols.end() ) {
        // *fielData is a protocol number, so is 1 byte (= uint8_t) long
        // remember to cast it into an uint16_t (= unsigned int)
        // if you want to print it!
        e_source.setProtocolID(*fieldData);
        e_dest.setProtocolID(*fieldData);
      }
      else
        skip_both = true;

      break;


    case IPFIX_TYPEID_sourceIPv4Address:

      if (fieldDataLength != IPFIX_LENGTH_sourceIPv4Address) {
        std::cerr << "Error! Got invalid IPFIX field data (source IP)! "
      << "Skipping record.\n";
        return;
      }

      SourceIP.setAddress(fieldData[0],fieldData[1],fieldData[2],fieldData[3]);
      SourceIP.remanent_mask(subnetMask);

      // filtering ...
      if ( MonitorEveryIp == true
        || find(MonitoredIpAddresses.begin(),MonitoredIpAddresses.end(),SourceIP)
            != MonitoredIpAddresses.end())
        e_source.setIpAddress(SourceIP);
      else
        // we dont consider all ip addresses and the (masked) SourceIP
        // is not one of the IPs in the given IpList
        // so we arent interested in that endpoint
        skip_source = true;

      break;


    case IPFIX_TYPEID_destinationIPv4Address:

      if (fieldDataLength != IPFIX_LENGTH_destinationIPv4Address) {
        std::cerr << "Error! Got invalid IPFIX field data (destination IP)! "
      << "Skipping record.\n";
        return;
      }

      DestIP.setAddress(fieldData[0],fieldData[1],fieldData[2],fieldData[3]);
      DestIP.remanent_mask(subnetMask);

      // filtering ...
      if ( MonitorEveryIp == true
       || find(MonitoredIpAddresses.begin(),MonitoredIpAddresses.end(),DestIP)
           != MonitoredIpAddresses.end()  ) {
        e_dest.setIpAddress(DestIP);
      }
      else
        skip_dest = true;

      break;

    // Ports do only matter, if
    // endpoint_key contains "port"
    // AND
    // endpoint_key contains "protocol" AND TCP and/or UDP are selected
    // OR
    // protocols dont matter
    case IPFIX_TYPEID_sourceTransportPort:

      if (fieldDataLength != IPFIX_LENGTH_sourceTransportPort
      && fieldDataLength != IPFIX_LENGTH_sourceTransportPort-1) {
        std::cerr << "Error! Got invalid IPFIX field data (source port)! "
                  << "Skipping record.\n";
        return;
      }

      if (fieldDataLength == IPFIX_LENGTH_sourceTransportPort) {
        if ( MonitorAllPorts == true || MonitoredPorts.end() !=
            find (MonitoredPorts.begin(), MonitoredPorts.end(),
            ntohs(*(uint16_t*)fieldData)) ) {
          // fieldData must be casted into an uint16_t (= unsigned int)
          // as it is a port number
          // (and, also, converted from network order to host order)
          e_source.setPortNr(ntohs(*(uint16_t*)fieldData));
          return;
        }
        else
          skip_source = true;
      }

      if (fieldDataLength == IPFIX_LENGTH_sourceTransportPort-1) {
        if ( MonitorAllPorts == true || MonitoredPorts.end() !=
            find (MonitoredPorts.begin(), MonitoredPorts.end(),
            (uint16_t)*fieldData) ) {
          // fieldData must be casted into an uint16_t (= unsigned int)
          // as it is a port number
          e_source.setPortNr((uint16_t)*fieldData);
          return;
        }
        else
          skip_source = true;
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
        if ( MonitorAllPorts == true || MonitoredPorts.end() !=
            find (MonitoredPorts.begin(), MonitoredPorts.end(),
            ntohs(*(uint16_t*)fieldData)) ) {
          e_dest.setPortNr(ntohs(*(uint16_t*)fieldData));
          return;
        }
        else
          skip_dest = true;
      }

      if (fieldDataLength == IPFIX_LENGTH_destinationTransportPort-1) {
        if ( MonitorAllPorts == true || MonitoredPorts.end() !=
            find (MonitoredPorts.begin(), MonitoredPorts.end(),
            (uint16_t)*fieldData) ) {
          e_dest.setPortNr((uint16_t)*fieldData);
          return;
        }
        else
          skip_dest = true;
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


void StatStore::recordEnd() {

  // If one of the filters was matched, we dont want that endpoint
  // and so dont need to do further checks and updates ...
  // But maybe one of the involved endpoints, either e_source or e_dest
  // is wanted, so we skip only, if really both arent wanted, i. e.
  // skip_both --> protocol isnt wanted (matches for both endpoints)
  // skip_source/dest --> source/dest ip or source/dest port isnt wanted
  if (skip_both == true || (skip_source == true && skip_dest == true))
    return;

  std::stringstream Warning;
  Warning
  << "WARNING: New EndPoint observed but EndPointListMaxSize reached!\n"
  << "Couldn't monitor new EndPoint: ";

  if (skip_source == false) {
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
//         Data[e_source].packets_out = packet_nb;
//         Data[e_source].bytes_out = byte_nb;
//         Data[e_source].records_out = 1;
        EndPointList.push_back(e_source);
      }
      else
        std::cerr << Warning.str() << e_source << std::endl;
    }
  }

  if (skip_dest == false) {
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
//         Data[e_dest].packets_in = packet_nb;
//         Data[e_dest].bytes_in = byte_nb;
//         Data[e_dest].records_in = 1;
        EndPointList.push_back(e_dest);
      }
      else
        std::cerr << Warning.str() << e_dest << std::endl;
    }
  }

  return;
}


void StatStore::writeToFile() {

  std::ofstream file("data.txt", std::ios_base::app);
  if (file.is_open() == true) {
    file << Data << "---" << "\n";
    file.close();
    return;
  }
  else {
    std::cerr << "ERROR: Couldnt open file @ StatStore::writeToFile()!\nExiting.\n";
    exit(0);
  }
}


// ========== INITIALISATIONS OF STATIC MEMBERS OF CLASS StatStore ===========

std::map<EndPoint,Info> StatStore::PreviousData;

// even if the following members will be given their actual values
// by the Stat::init() function, we have to provide some initial values
// in the implementation file of the related class;

std::vector<IpAddress> StatStore::MonitoredIpAddresses;
byte StatStore::subnetMask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
bool StatStore::MonitorEveryIp = false;

std::vector<EndPoint> StatStore::EndPointList;
int StatStore::EndPointListMaxSize = 0;

std::vector<byte> StatStore::MonitoredProtocols;
bool StatStore::MonitorAllProtocols = false;

std::vector<uint16_t> StatStore::MonitoredPorts;
bool StatStore::MonitorAllPorts = false;

bool StatStore::BeginMonitoring = false;
