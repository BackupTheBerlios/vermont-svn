#ifndef _SW_STORE_H_
#define _SW_STORE_H_

#include "datastore.h"
#include "stuff.h"
#include <concentrator/ipfix.h>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>


// ==================== STORAGE CLASS SWStore ====================

class SWStore : public DataStore {

 private:

  uint64_t packet_nb;               // data coming
  uint64_t byte_nb;                 // from the
  IpAddress SourceIP, DestIP;       // current record
	uint16_t SourcePort, DestPort;
	byte protocol;

  std::map<EndPoint,Info> Data;    	// data collected from all records received
                                   	// since last call to Stat::test()

 public:

  SWStore();
  ~SWStore();

  bool recordStart(SourceID sourceId);
  void recordEnd();
  void addFieldData(int id, byte * fieldData, int fieldDataLength,
		    EnterpriseNo eid = 0);

  std::map<EndPoint,Info> getData() const {return Data;}

 private:

  //static std::vector<EndPoint> MonitoredEndPoints;

  static bool BeginMonitoring;
  // Set to "true" by Stat::init() when all the static information about IP,
  // protocols and ports is ready, else set to "false" by the Stat constructor.
  // It prevents recordStart to give the go-ahead when requested by
  // DetectionBase if something is not ready (cf. Stat::Stat(), where
  // DetectionBase<StatStore> begins to run while Stat::init() is not over).

  // All these are static because they are the same for every StatStore object.
  // As they will be set by a function, Stat::init(), that doesn't have any
  // StatStore object argument to help call these functions, we absolutely need
  // static and public "setters", wich are programmed hereafter.

 public:

  // Public "setters" we just spoke about:

  static bool & setBeginMonitoring () {
    return BeginMonitoring;
  }

};

#endif
