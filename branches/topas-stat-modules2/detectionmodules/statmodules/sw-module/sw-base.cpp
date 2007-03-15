#include<signal.h>

#include "sw-base.h"
#include "stuff.h"

// ==================== CONSTRUCTOR FOR CLASS SWBase ====================


SWBase::SWBase(const std::string & configfile)
  : DetectionBase<SWStore>(configfile) {

  // signal handlers
  if (signal(SIGTERM, sigTerm) == SIG_ERR) {
    msg(MSG_ERROR, "sw-module: Couldn't install signal handler for SIGTERM.\n ");
  }
  if (signal(SIGINT, sigInt) == SIG_ERR) {
    msg(MSG_ERROR, "sw-module: Couldn't install signal handler for SIGINT.\n ");
  }

  init(configfile);
}



// =========================== init FUNCTIONS ==========================

void SWBase::init(const std::string & configfile) {

	ConfObj * config;
  config = new ConfObj(configfile);

	init_alarm_time(config);

	delete config;

	// subscribe to the prefered IPFIX_TYPEIDs
	subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
	subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
	subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
  subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
	subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
	subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
	subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);

	// now everything is ready to begin monitoring:
  SWStore::setBeginMonitoring() = true;

}

void SWBase::init_alarm_time(ConfObj * config) {

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
  if (NULL != config->getValue("preferences", "alarm_time"))
    setAlarmTime( atoi(config->getValue("preferences", "alarm_time")) );

  else {
    std::stringstream Error;
    Error << "Error! No alarm_time parameter defined in XML config file!\n"
	  << "Please give one and restart. Exiting.\n";
    std::cerr << Error.str();
    exit(0);
  }

  return;

}

// ============================= TEST FUNCTION ===============================

void SWBase::test(SWStore * store) {

	std::cout << "############################################" << std::endl;
	std::cout << "############# begin test()-Run #############" << std::endl;
	std::cout << "############################################" << std::endl;

// Extracting data from store:
  std::map<EndPoint,int64_t> Data = extract_data (store);

	std::cout << "############################################" << std::endl;
	std::cout << "############# end test()-Run ###############" << std::endl;
	std::cout << "############################################" << std::endl;

  // Dumping empty records:
  //if (Data.empty()==true)
  //	std::cerr << "Got empty record; " << "dumping it and waiting for another record\n";

	//
	// TODO: Test something
	//

  delete store;
  return;
}


// ------ FUNCTIONS USED TO EXTRACT DATA FROM THE STORAGE CLASS SWStore -----

std::map<EndPoint,int64_t> SWBase::extract_data (SWStore * store) {

  std::map<EndPoint,int64_t> result;

	// TODO: Extract something
	std::map<EndPoint,Info> Data = store->getData();
	std::map<EndPoint,Info>::iterator it = Data.begin();

	while (it != Data.end()){
		std::cout << it->first << " --- packets_in: " << it->second.packets_in << ", packets_out: " << it->second.packets_out << ", bytes_in: " << it->second.bytes_in << ", bytes_out: " << it->second.bytes_out << ", Records: " << it->second.records << std::endl;
		it++;
	}

  return result;

}

void SWBase::sigTerm(int signum)
{
	stop();
}

void SWBase::sigInt(int signum)
{
	stop();
}
