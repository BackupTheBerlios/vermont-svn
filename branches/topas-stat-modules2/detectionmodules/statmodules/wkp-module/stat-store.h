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

#ifndef _STAT_STORE_H_
#define _STAT_STORE_H_

#include "shared.h"
#include <datastore.h>
#include <concentrator/ipfix.h>
#include <map>
#include <vector>
#include <algorithm> // <vector> does not have a member function find(...)
#include <iostream>
#include <sstream>


// ==================== STORAGE CLASS StatStore ====================

class StatStore : public DataStore {

 private:

  uint64_t packet_nb;               // data coming
  uint64_t byte_nb;                 // from the
                                    // current record
  EndPoint e_source;
  EndPoint e_dest;

  bool skip_both, skip_source, skip_dest; // flags for skipping a record if
                                          // some filters did match

  std::map<EndPoint,Info> Data;     // data collected from all records received
                                    // since last call to Stat::test()

  // TESTING
  std::map<EndPoint,Info> dataFromFile;
  static std::map<EndPoint,Info> previousDataFromFile;

  static std::map<EndPoint,Info> PreviousData;
   // data collected from all records received before last call to Stat::test()
   // but not before the call before last call to Stat::test()...
   // that means, PreviousData is short-term memory: it's Data as it was before
   // last call to Stat::test() (and it's static so that it survives the
   // StatStore object destruction that goes with a call to Stat::test())

 public:

  StatStore();
  ~StatStore();
  // it is ~StatStore which updates PreviousData (PreviousData = Data;)

  bool recordStart(SourceID sourceId);
  void recordEnd();
  void addFieldData(int id, byte * fieldData, int fieldDataLength,
		    EnterpriseNo eid = 0);

  std::map<EndPoint,Info> getData() const {return Data;}
  std::map<EndPoint,Info> getPreviousData() const {return PreviousData;}


  // TESTING
  std::map<EndPoint,Info> getDataFromFile() const {return dataFromFile;}
  std::map<EndPoint,Info> getPreviousDataFromFile() const {return previousDataFromFile;}
  // writes the Data in a file
  void writeToFile();
  // reads data from a file
  bool readFromFile();


 private:

  static std::vector<IpAddress> MonitoredIpAddresses;
  // IP addresses to monitor. They are provided in a file by the user;
  // this file is read by Stat::init(), which then initializes
  // MonitoredIpAddresses

  static byte subnetMask[4];
  // The file contains hosts and/or networks addresses, but they will all
  // be masked by the same subnet mask, provided by the user in the
  // XML configuration file and initialized by Stat::init()

  static bool MonitorEveryIp;
  // In case no file is given, then Stat::init() initializes MonitorEveryIp
  // to true. Any IP found in a record will be added to the
  // MonitoredIpAddresses vector until a maximal size (to prevent
  // memory exhaustion) is reached.

  static std::vector<EndPoint> EndPointList;
  // Currently monitored EndPoints. Every Endpoint we are interested in
  // is added to this List until EndPointListMaxSize is reached.

  static int EndPointListMaxSize;
  // This maximal size is defined by the user and set by Stat::init().
  // If forgotten by the user, default max size is provided.

  static std::vector<byte> MonitoredProtocols;
  // Protocols to monitor. Provided by the user in the XML config file
  // and initialised by Stat::init(). A protocol identifier (1, 6, 17...)
  // is 1 byte (= uint8_t) long, hence the vector<byte>

  static bool MonitorAllProtocols;
  // If no protocols to monitor are provided, Stat::init() will set MonitorAllProtocols
  // to "true" ("false" otherwise)

  static std::vector<uint16_t> MonitoredPorts;
  // Ports to monitor. Provided by the user in the XML config file
  // and initialised by Stat::init(). A port number is 2 bytes long,
  // hence the vector<uint16_t>.

  static bool MonitorAllPorts;
  // If no ports to monitor are provided, Stat::init() will set MonitorAllPorts
  // to "true" ("false" otherwise)

  static bool BeginMonitoring;
  // Set to "true" by Stat::init() when all the static information about IP,
  // protocols and ports is ready, else set to "false" by the Stat constructor.
  // It prevents recordStart to give the go-ahead when requested by
  // DetectionBase if something is not ready (cf. Stat::Stat(), where
  // DetectionBase<StatStore> begins to run while Stat::init() is not over).

  // TESTING
  static std::ifstream dataFile;
  // ifstream to read from the file with all the data in it
  // static as we need to know which data was already read and
  // thus keep the file-handle and its position alive


  // All these are static because they are the same for every StatStore object.
  // As they will be set by a function, Stat::init(), that doesn't have any
  // StatStore object argument to help call these functions, we absolutely need
  // static and public "setters", wich are programmed hereafter.


 public:

  // Public "setters" we just spoke about:

  static void AddIpToMonitoredIp (IpAddress IP) {
    MonitoredIpAddresses.push_back(IP);
  }

  static void AddIpToMonitoredIp (const std::vector<IpAddress> & IPvector) {
    MonitoredIpAddresses.insert(MonitoredIpAddresses.end(),
				IPvector.begin(), IPvector.end());
  }

  static void InitialiseSubnetMask (byte tab[4]) {
    subnetMask[0] = tab[0]; subnetMask[1] = tab[1];
    subnetMask[2] = tab[2]; subnetMask[3] = tab[3];
  }

  static void InitialiseSubnetMask (byte a, byte b, byte c, byte d) {
    subnetMask[0] = a; subnetMask[1] = b;
    subnetMask[2] = c; subnetMask[3] = d;
  }

  static bool & setMonitorEveryIp () {
    return MonitorEveryIp;
  }

  static int & setEndPointListMaxSize () {
    return EndPointListMaxSize;
  }

  static void AddProtocolToMonitoredProtocols (byte Protocol_id) {
    MonitoredProtocols.push_back(Protocol_id);
  }

  static void AddPortToMonitoredPorts (uint16_t port) {
    MonitoredPorts.push_back(port);
  }

  static bool & setMonitorAllPorts () {
    return MonitorAllPorts;
  }

  static bool & setMonitorAllProtocols () {
    return MonitorAllProtocols;
  }

  static bool & setBeginMonitoring () {
    return BeginMonitoring;
  }

  // and this is a "getter"; it's static too so that it can be used by
  // Stat::init() to mask some addresses
  // ToDo: find something else; it's really an ugly trick
  static byte * getSubnetMask () {
    return subnetMask;
  }

};

#endif
