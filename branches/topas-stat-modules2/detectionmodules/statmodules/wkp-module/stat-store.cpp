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


// ==================== STORAGE CLASS StatStore ====================


StatStore::StatStore()
  : e_source(IpAddress(0,0,0,0),0,0), e_dest(IpAddress(0,0,0,0),0,0) {

  skip_both = skip_source = skip_dest = false;

  packet_nb = byte_nb = 0;

  IpListMaxSizeReachedAndNewIpWantedToEnterIt = 0;

}

StatStore::~StatStore() {

  PreviousData = Data;

}

bool StatStore::recordStart(SourceID sourceId) {

  if (BeginMonitoring != true)
    return false;

  // IDMEF
  /*if (find(accept_source_ids->begin(),accept_source_ids->end(),(int)sourceId)==accept_source_ids->end()){
    return false;
  }*/

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
  // is wanted, so we skip only, if really both arent wanted
  // skip_both --> protocol isnt wanted
  // skip_source/dest --> source/dest ip or source/dest port isnt wanted

  if (skip_both == true || (skip_source == true && skip_dest == true)) {
    std::cout << "### skipping ... ###" << std::endl;
    return;
  }

  IpAddress SourceIP = IpAddress(0,0,0,0);
  IpAddress DestIP = IpAddress(0,0,0,0);

  switch (id) {

    case IPFIX_TYPEID_protocolIdentifier:
      std::cout << "### Protocol ###" << std::endl;
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
      std::cout << "### sourceIP ###" << std::endl;
      if (fieldDataLength != IPFIX_LENGTH_sourceIPv4Address) {
        std::cerr << "Error! Got invalid IPFIX field data (source IP)! "
      << "Skipping record.\n";
        return;
      }

      SourceIP.setAddress(fieldData[0],fieldData[1],fieldData[2],fieldData[3]);
      SourceIP.remanent_mask(subnetMask);
      e_source.setIpAddress(SourceIP);

      if (MonitorEveryIp == true) {
        if(find(MonitoredIpAddresses.begin(),MonitoredIpAddresses.end(),e_source)
            != MonitoredIpAddresses.end())
          // i.e. we already saw this IP
          return;
        else if (MonitoredIpAddresses.size() < IpListMaxSize)
          // i.e. we never saw this IP,
          // that's a new one, so we add it to our IP-list,
          // provided there is still place
          MonitoredIpAddresses.push_back(e_source);
        else
          IpListMaxSizeReachedAndNewIpWantedToEnterIt = 1;
          // there isn't still place,
          // so we just set the "max-size" flag to 1
          // (ToDo: replace this flag with an exception)
      }
      else {
        if(find(MonitoredIpAddresses.begin(),MonitoredIpAddresses.end(),e_source)
            != MonitoredIpAddresses.end())
            // i.e. (masked) SourceIP is one of the IPs in the given IpList
          return;
        else
          // i. e. that we arent interested in that endpoint
          skip_source = true;
      }

      break;


    case IPFIX_TYPEID_destinationIPv4Address:
      std::cout << "### destIP ###" << std::endl;
      if (fieldDataLength != IPFIX_LENGTH_destinationIPv4Address) {
        std::cerr << "Error! Got invalid IPFIX field data (destination IP)! "
      << "Skipping record.\n";
        return;
      }

      DestIP.setAddress(fieldData[0], fieldData[1], fieldData[2], fieldData[3]);
      DestIP.remanent_mask(subnetMask);
      e_dest.setIpAddress(DestIP);

      if (MonitorEveryIp == true) {
        if ( find(MonitoredIpAddresses.begin(),MonitoredIpAddresses.end(),e_dest)
              != MonitoredIpAddresses.end() )
          return;
        else if ( MonitoredIpAddresses.size() < IpListMaxSize )
          MonitoredIpAddresses.push_back(e_dest);
        else
          IpListMaxSizeReachedAndNewIpWantedToEnterIt = 1;
      }
      else {
        if ( find(MonitoredIpAddresses.begin(),MonitoredIpAddresses.end(),e_dest)
            != MonitoredIpAddresses.end() )
          return;
        else
          skip_dest = true;
      }

      break;


    // The following two cases may happen ONLY if:
    // - the endpoint_key contains "port" and so
    //   we subscribed to IPFIX_TYPEID_*TransportPort,
    //   i.e. TCP and/or UDP protocols are monitored
    // - AND a TCP or UDP packet was received from the collector
    //
    // This case CANNOT happen when:
    // - the endpoint_key doesnt contain "port"
    // - only ICMP and/or RAW are monitored (as, in this case,
    //   we do not subscribe to IPFIX_TYPEID_sourceTransportPort);
    //   EVEN IF a TCP or UDP packet was received from the collector
    case IPFIX_TYPEID_sourceTransportPort:
      std::cout << "### sourcePort ###" << std::endl;
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
      std::cout << "### destPort ###" << std::endl;
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
      std::cout << "### pdelta ###" << std::endl;
      if (fieldDataLength != IPFIX_LENGTH_packetDeltaCount) {
        std::cerr << "Error! Got invalid IPFIX field data (#packets)! "
                  << "Skipping record.\n";
        return;
      }
      packet_nb = ntohll(*(uint64_t*)fieldData);
      break;


    case IPFIX_TYPEID_octetDeltaCount:
      std::cout << "### bdelta ###" << std::endl;
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

  if (skip_both == true || (skip_source == true && skip_dest == true))
    return;

  if (skip_source == false) {
    Data[e_source].packets_out += packet_nb;
    Data[e_source].bytes_out += byte_nb;
    Data[e_source].records_out++;
  }
  if (skip_dest == false) {
    Data[e_dest].packets_in += packet_nb;
    Data[e_dest].bytes_in += byte_nb;
    Data[e_dest].records_in++;
  }
/*
  if (skip_source == false) {
    std::map<EndPoint,Info>::iterator it1 = find(Data.begin(), Data.end(), e_source);
    if ( it1 != Data.end() ) {
      it1->second.packets_out += packet_nb;
      it1->second.bytes_out   += byte_nb;
      it1->second.records_out++;
    }
    else {
      Data[e_source].packets_out += packet_nb;
      it1 = find(Data.begin(),Data.end(),e_source);
      it1->second.bytes_out += byte_nb;
      it1->second.records_out++;
    }
  }

  if (skip_dest == false) {
    std::map<EndPoint,Info>::iterator it2 = find(Data.begin(),Data.end(),e_dest);
    if ( it2 != Data.end() ) {
      it2->second.packets_in += packet_nb;
      it2->second.bytes_in   += byte_nb;
      it2->second.records_in++;
    }
    else {
      Data[e_dest].packets_in += packet_nb;
      it2 = find(Data.begin(),Data.end(),e_dest);
      it2->second.bytes_in += byte_nb;
      it2->second.records_in++;
    }
  }
*/
  return;
}


// ========== INITIALISATIONS OF STATIC MEMBERS OF CLASS StatStore ===========

std::map<EndPoint,Info> StatStore::PreviousData;

// even if the following members will be given their actual values
// by the Stat::init() function, we have to provide some initial values
// in the implementation file of the related class;

std::vector<EndPoint> StatStore::MonitoredIpAddresses;

byte StatStore::subnetMask[4] = {0xFF, 0xFF, 0xFF, 0xFF};

bool StatStore::MonitorEveryIp = false;
int StatStore::IpListMaxSize = 0;

std::vector<byte> StatStore::MonitoredProtocols;
bool StatStore::MonitorAllProtocols = false;

std::vector<uint16_t> StatStore::MonitoredPorts;
bool StatStore::MonitorAllPorts = false;

bool StatStore::BeginMonitoring = false;

std::vector<int>* StatStore::accept_source_ids = NULL;
