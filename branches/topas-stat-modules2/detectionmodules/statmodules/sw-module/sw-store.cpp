#include "sw-store.h"

// ==================== STORAGE CLASS SWStore ====================

SWStore::SWStore()
  : SourceIP (0,0,0,0), DestIP (0,0,0,0) {

  packet_nb = byte_nb = 0;
	SourcePort = DestPort = 0;
	protocol = 0;
}

SWStore::~SWStore() {}

bool SWStore::recordStart(SourceID sourceId) {

  if (BeginMonitoring != true)
    return false;

  packet_nb = byte_nb = 0;

  SourceIP = DestIP = IpAddress(0,0,0,0);
	SourcePort = DestPort = 0;
	protocol = 0;

  return true;

}

void SWStore::addFieldData(int id, byte * fieldData, int fieldDataLength, EnterpriseNo eid) {

  // addFieldData will be executed until there are no more fieldData
  // in the current IPFIX record; so we are sure to get everything
  // we subscribed to (so don't get worried because of
  // the "breaks" in the "switch" loop hereafter)

  switch (id) {

		case IPFIX_TYPEID_protocolIdentifier:

			if (fieldDataLength != IPFIX_LENGTH_protocolIdentifier) {
				std::cerr << "Error! Got invalid IPFIX field data (protocol)! "
				<< "Skipping record.\n";
				return;
			}

			protocol = *fieldData;

			break;


		case IPFIX_TYPEID_sourceIPv4Address:

			if (fieldDataLength != IPFIX_LENGTH_sourceIPv4Address) {
				std::cerr << "Error! Got invalid IPFIX field data (source IP)! "
				<< "Skipping record.\n";
				return;
			}

			SourceIP = IpAddress(fieldData[0],fieldData[1],fieldData[2],fieldData[3]);

			break;


		case IPFIX_TYPEID_destinationIPv4Address:

			if (fieldDataLength != IPFIX_LENGTH_destinationIPv4Address) {
				std::cerr << "Error! Got invalid IPFIX field data (destination IP)! "
				<< "Skipping record.\n";
				return;
			}

			DestIP = IpAddress(fieldData[0], fieldData[1], fieldData[2], fieldData[3]);

			break;


		case IPFIX_TYPEID_sourceTransportPort:

			if (fieldDataLength != IPFIX_LENGTH_sourceTransportPort
					&& fieldDataLength != IPFIX_LENGTH_sourceTransportPort-1) {
				std::cerr << "Error! Got invalid IPFIX field data (source port)! "
				<< "Skipping record.\n";
				return;
			}

			SourcePort = ntohs(*(uint16_t*)fieldData);

			break;


		case IPFIX_TYPEID_destinationTransportPort:

			if (fieldDataLength != IPFIX_LENGTH_destinationTransportPort
					&& fieldDataLength != IPFIX_LENGTH_destinationTransportPort-1) {
				std::cerr << "Error! Got invalid IPFIX field data (destination port)! "
				<< "Skipping record.\n";
				return;
			}

			DestPort = ntohs(*(uint16_t*)fieldData);

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
			<<"Warning! Got unknown record in SWStore::addFieldData(...)!\n"
			<<"A programmer has probably added some record type in SW::init()\n"
			<<"but has forgotten to ensure its support in SWStore::addFieldData().\n"
			<<"I'll try to keep working, but I can't tell for sure I won't crash.\n";
  }

  return;

}


void SWStore::recordEnd() {

	EndPoint e1, e2;

	e1 = EndPoint(SourceIP, SourcePort, protocol);
	e2 = EndPoint(DestIP, DestPort, protocol);

/*
  // Testing some stuff
  EndPoint test = EndPoint(e1);
  EndPoint test_c = EndPoint(test);
  test_c.setIpAddress(IpAddress(100, 100, 100, 100));
  IpAddress ip = test_c.getIpAddress();
  std::cout << "Test: " << test << std::endl << "Original: " << e1 << std::endl << "Test (changed): " << test_c << std::endl << "IP: " << ip << std::endl << "#############" << std::endl;
*/

/*
	Data[e1].packets_out += packet_nb;
	Data[e1].bytes_out   += byte_nb;
	Data[e1].records_out++;

	Data[e2].packets_in += packet_nb;
	Data[e2].bytes_in   += byte_nb;
	Data[e2].records_in++;
*/


	std::map<EndPoint,Info>::iterator it1 = Data.find(e1);
	std::map<EndPoint,Info>::iterator it2 = Data.find(e2);

	if (it1 != Data.end()){
		it1->second.packets_out 	+= packet_nb;
		it1->second.bytes_out			+= byte_nb;
		it1->second.records_out 	+= 1;
		std::cout << "EndPoint already known (updating ...): " << e1 << std::endl;
	}
	else {
		Data[e1].packets_out = packet_nb; // using Data[key] to create the entry,
		Data[e1].bytes_out   = byte_nb; // using iterators then for quicker access
		Data[e1].records_out = 1;
		std::cout << "EndPoint not known (initializing ...): " << e1 << std::endl;
	}

	if(it2 != Data.end()) {
		it2->second.packets_in += packet_nb;
		it2->second.bytes_in   += byte_nb;
		it2->second.records_in += 1;
		std::cout << "EndPoint already known (updating ...): " << e2 << std::endl;
	}
	else {
		Data[e2].packets_in = packet_nb;
		Data[e2].bytes_in   = byte_nb;
		Data[e2].records_in = 1;
		std::cout << "EndPoint not known (initializing ...): " << e2 << std::endl;
	}


  return;

}


// ========== INITIALISATIONS OF STATIC MEMBERS OF CLASS SWStore ===========

// even if the following members will be given their actual values
// by the Stat::init() function, we have to provide some initial values
// in the implementation file of the related class;

bool SWStore::BeginMonitoring = false;
