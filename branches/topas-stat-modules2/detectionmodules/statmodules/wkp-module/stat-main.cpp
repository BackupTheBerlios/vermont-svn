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

/*
TODO(1)
XMLConfObj-Exceptions
------------------------------------
Bisher:
Wenn in der Konfigurationsdatei ein Node, der initialisiert werden soll, nicht angegeben wird, so wird in der XMLConfObj::getValue-Funktion eine Exception geworfen und das Modul beendet, ergo: Man muss alles angeben, wenn auch leer (z. B. <ports></ports>)
Besser:
Exception ignorieren/unterbinden und das Problem den Modulen überlassen?
------------------------------------

TODO(2)
Container für Metriken
------------------------------------
Bisher:
struct Values enthält alle möglichen Metriken
Besser:
nur die tatsächlich gewollten Metriken speichern

TODO(3)
Port- oder IP-Monitoring
------------------------------------
Bisher:
Es werden sowohl IP-Adresse als auch Port (und Protokoll) in einem EndPoint gespeichert, was reines Port-Monitoring verkompliziert
Besser:
Man soll wählen können, ob man an Ports oder IP-Adressen interessiert (oder an beidem) ist und das jeweils andere dann auf 0 belassen (um die Vergleichsfunktionen nicht zu beinflussen, also z. B. wenn man in Ports interessiert ist, sollen alle EndPoints mit dem gleichen Port identisch sein, ungeachtet der IP-Adresse). Spielt das Protokoll hierbei eine Rolle?
*/


#include<signal.h>

#include "stat-main.h"
#include "wmw-test.h"
#include "ks-test.h"
#include "pcs-test.h"

// ==================== CONSTRUCTOR FOR CLASS Stat ====================


Stat::Stat(const std::string & configfile)
  : DetectionBase<StatStore>(configfile) {

  // signal handlers
  if (signal(SIGTERM, sigTerm) == SIG_ERR) {
    msg(MSG_ERROR, "wkp-module: Couldn't install signal handler for SIGTERM.\n ");
  }
  if (signal(SIGINT, sigInt) == SIG_ERR) {
    msg(MSG_ERROR, "wkp-module: Couldn't install signal handler for SIGINT.\n ");
  }

  // lock, will be unlocked at the end of init() (cf. StatStore class header):
  StatStore::setBeginMonitoring () = false;

  test_counter = 0;
  init(configfile);
}


// =========================== init FUNCTION ==========================


// init()'s job is to extract user's preferences
// and test parameters from the XML config file
//
void Stat::init(const std::string & configfile) {

  XMLConfObj * config;
  config = new XMLConfObj(configfile, XMLConfObj::XML_FILE);

#ifdef IDMEF_SUPPORT_ENABLED
	/* register module */
	registerModule("wkp-module");
#endif

  // the order of the following operations is important,
  // as some of these functions use Stat members initialized
  // by the preceding functions; so beware if you change the order

  config->enterNode("preferences");

  // extracting output file's name
  init_output_file(config);

  // extracting source id's to accept
  // init_accept_source_ids(config);

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
  init_alarm_time(config);

  // extracting warning verbosity
  init_warning_verbosity(config);

  // extracting output verbosity
  init_output_verbosity(config);

  // extracting monitored values
  init_monitored_values(config);

  // extracting noise reduction preferences
  init_noise_thresholds(config);

  // extracting monitored protocols
  init_protocols(config);

  // extracting netmask to apply to all IPs
  init_netmask(config);

  // extracting monitored port numbers
  init_ports(config);

  // extracts the IP addresses to monitor or the maximal number of IPs
  // to monitor (in case the user doesn't give IP addresses), and
  // initialises some static members of the StatStore class
  init_ip_addresses(config);

  // now everything is ready to begin monitoring:
  StatStore::setBeginMonitoring() = true;

  // extracting statistical test frequency preference
  init_stat_test_freq(config);

  // extracting report_only_first_attack parameter
  init_report_only_first_attack(config);

  // extracting pause_update_when_attack parameter
  init_pause_update_when_attack(config);

  config->leaveNode();

  config->enterNode("test-params");

  // extracting type of test
  // (Wilcoxon and/or Kolmogorov and/or Pearson chi-square)
  init_which_test(config);

  // extracting sample sizes
  init_sample_sizes(config);

  // extracting one/two-sided parameter for the test
  init_two_sided(config);

  // extracting significance level parameter
  init_significance_level(config);

  config->leaveNode();

  /* one should not forget to free "config" after use */
  delete config;

}


#ifdef IDMEF_SUPPORT_ENABLED
void Stat::update(XMLConfObj* xmlObj)
{
	std::cout << "Update received!" << std::endl;
	if (xmlObj->nodeExists("stop")) {
		std::cout << "-> stopping module..." << std::endl;
		stop();
	} else if (xmlObj->nodeExists("restart")) {
		std::cout << "-> restarting module..." << std::endl;
		restart();
	} else if (xmlObj->nodeExists("config")) {
		std::cout << "-> updating module configuration..." << std::endl;
	} else { // add your commands here
		std::cout << "-> unknown operation" << std::endl;
	}
}
#endif

// ================== FUNCTIONS USED BY init FUNCTION =================


void Stat::init_output_file(XMLConfObj * config) {

  // extracting output file's name
  if (!(config->getValue("output_file")).empty()) {
    outfile.open(config->getValue("output_file").c_str());
    if (!outfile) {
      std::cerr << "Error: could not open output file "
		<< config->getValue("output_file") << ". "
		<< "Check if you have enough rights to create or write to it. "
		<< "Exiting.\n";
      exit(0);
    }
  }
  else {
    std::cerr <<"Error! No output_file parameter defined in XML config file!\n"
	      <<"Please give one and restart. Exiting.\n";
    exit(0);
  }

  return;

}

void Stat::init_accept_source_ids(XMLConfObj * config) {
	if (!(config->getValue("accept_source_ids")).empty()) {
		std::string str = config->getValue("accept_source_ids");
		unsigned res, IDEnd = 0, last = 0;
		bool more = true;
		std::string temp;
		do {
			res = str.find(',', last);
			if (res == std::string::npos) {
				more = false;
				res = str.size();
			}
			if (IDEnd == 0) {
				IDEnd = res;
				if (IDEnd > 0) {
					temp = std::string(str.begin(), str.begin() + res);
					accept_source_ids.push_back(atoi(temp.c_str()));
				}
			} else {
				temp = std::string(str.begin() + last, str.begin() + res);
				if (!temp.empty()) {accept_source_ids.push_back(atoi(temp.c_str())); }
			}
			last = res + 1; // one past last space
		} while (more);
		StatStore::accept_source_ids = &accept_source_ids;
	}
	if (accept_source_ids.size() == 0) {
		std::stringstream Error;
		Error << "Error! No accept_source_ids parameter defined in XML config file!\n"
		      << "Please give one and restart. Exiting.\n";
		std::cerr << Error.str();
		outfile << Error.str() << std::flush;
		exit(0);
	}
}

void Stat::init_alarm_time(XMLConfObj * config) {

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
  if (!(config->getValue("alarm_time")).empty())
    setAlarmTime( atoi(config->getValue("alarm_time").c_str()) );

  else {
    std::stringstream Error;
    Error << "Error! No alarm_time parameter defined in XML config file!\n"
	  << "Please give one and restart. Exiting.\n";
    std::cerr << Error.str();
    outfile << Error.str() << std::flush;
    exit(0);
  }

  return;

}

void Stat::init_warning_verbosity(XMLConfObj * config) {

  // extracting warning verbosity
  std::stringstream Warning, Default, Usage;
  Warning << "Warning! Parameter warning_verbosity "
	  << "defined in XML config file should be 0 or 1.\n";
  Default << "Warning! No warning_verbosity parameter defined "
	  << "in XML config file! \""
	  << DEFAULT_warning_verbosity << "\" assumed.\n";
  Usage   << "O: warnings are sent to stderr\n"
	  << "1: warnings are sent to stderr and output file\n";

  if (!(config->getValue("warning_verbosity")).empty()) {
    if ( 0 == atoi( config->getValue("warning_verbosity").c_str() )
	 ||
	 1 == atoi( config->getValue("warning_verbosity").c_str() ) )
      warning_verbosity =
	atoi( config->getValue("warning_verbosity").c_str() );
    else {
      std::cerr << Warning.str() << Usage.str() << "Exiting.\n";
      outfile << Warning.str() << Usage.str() << "Exiting.\n" << std::flush;
      exit(0);
    }
  }

  else {
    std::cerr << Default.str() << Usage.str();
    warning_verbosity = DEFAULT_warning_verbosity;
  }

  return;

}

void Stat::init_output_verbosity(XMLConfObj * config) {

  // extracting output verbosity
  std::stringstream Warning, Default, Usage;
  Warning << "Warning! Parameter output_verbosity defined in XML config file "
	  << "should be between 0 and 5.\n";
  Default << "Warning! No output_verbosity parameter defined "
	  << "in XML config file! \""
	  << DEFAULT_output_verbosity << "\" assumed.\n";
  Usage   << "O: no output generated\n"
	  << "1: only p-values and attacks are recorded\n"
	  << "2: same as 1, plus some cosmetics\n"
	  << "3: same as 2, plus learning phases, updates and empty records events\n"
	  << "4: same as 3, plus sample printing\n"
	  << "5: same as 4, plus all details from statistical tests\n";

  if (!(config->getValue("output_verbosity")).empty()) {
    if ( 0 <= atoi( config->getValue("output_verbosity").c_str() )
	 &&
	 5 >= atoi( config->getValue("output_verbosity").c_str() ) )
      output_verbosity =
	atoi( config->getValue("output_verbosity").c_str() );
    else {
      std::cerr << Warning.str() << Usage.str() << "Exiting.\n";
      if (warning_verbosity==1)
	outfile << Warning.str() << Usage.str() << "Exiting." << std::endl;
      exit(0);
    }
  }

  else {
    std::cerr << Default.str() << Usage.str();
    if (warning_verbosity==1)
      outfile << Default.str() << Usage.str();
    output_verbosity = DEFAULT_output_verbosity;
  }

  return;

}

// extracting monitored values to vector<Metric>;
// the values (some of them splitted) are stored there as
// constants defined in enum Metric in stat_main.h
void Stat::init_monitored_values(XMLConfObj * config) {

  std::stringstream Warning1, Warning2, Warning3, Usage;
  Warning1
    << "Warning! No monitored_values parameter in XML config file!\n";
  Warning2
    << "Warning! No value parameter(s) defined for monitored_values in XML config file!\n";
  Warning3
    << "Warning! Unknown value parameter(s) defined for monitored_values in XML config file!\n";
  Usage
    << "Use packets, bytes, records, bytes/packet, packets_out-packets_in, "
    << "bytes_out-bytes_in,\n"
    << "packets(t)-packets(t-1) or bytes(t)-bytes(t-1).\n";

  if (!config->nodeExists("monitored_values")) {
    std::cerr << Warning1.str() << Usage.str() << "Exiting.\n";
    if (warning_verbosity==1)
      outfile << Warning1.str() << Usage.str() << "Exiting." << std::endl;
    exit(0);
  }

  config->enterNode("monitored_values");

  // more circumstantial than the above way ... but it works
  std::vector<std::string> tmp_monitored_data;

  /* get all monitored values */
  if (config->nodeExists("value")) {
    tmp_monitored_data.push_back(config->getValue("value"));
    while (config->nextNodeExists())
      tmp_monitored_data.push_back(config->getNextValue());
  }
  else {
    std::cerr << Warning2.str() << Usage.str() << "Exiting.\n";
    if (warning_verbosity==1)
      outfile << Warning2.str() << Usage.str() << "Exiting." << std::endl;
    exit(0);
  }

  config->leaveNode();

  // just in case the user provided multiple same values
  sort(tmp_monitored_data.begin(),tmp_monitored_data.end());
  std::vector<std::string>::iterator new_end = unique(tmp_monitored_data.begin(),tmp_monitored_data.end());
  std::vector<std::string> tmp (tmp_monitored_data.begin(), new_end);
  tmp_monitored_data.clear();
  tmp_monitored_data = tmp;

  std::stringstream monValues;

  // extract the values from tmp_monitored_data (string)
  // to monitored_values (vector<enum>)
  std::vector<std::string>::iterator it = tmp_monitored_data.begin();
  while ( it != tmp_monitored_data.end() ) {
    if ( 0 == strcasecmp("packets", (*it).c_str()) ) {
      monitored_values.push_back(PACKETS_IN);
      monitored_values.push_back(PACKETS_OUT);
      monValues << "packets, ";
    }
    else if ( 0 == strcasecmp("bytes", (*it).c_str()) ) {
      monitored_values.push_back(BYTES_IN);
      monitored_values.push_back(BYTES_OUT);
      monValues << "octets, ";
    }
    else if ( 0 == strcasecmp("octets",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_IN);
      monitored_values.push_back(BYTES_OUT);
      monValues << "octets, ";
    }
    else if ( 0 == strcasecmp("records",(*it).c_str()) ) {
      monitored_values.push_back(RECORDS_IN);
      monitored_values.push_back(RECORDS_OUT);
      monValues << "records, ";
    }
    else if ( 0 == strcasecmp("octets/packet",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_IN_PER_PACKET_IN);
      monitored_values.push_back(BYTES_OUT_PER_PACKET_OUT);
      monValues << "octets/packet, ";
    }
    else if ( 0 == strcasecmp("bytes/packet",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_IN_PER_PACKET_IN);
      monitored_values.push_back(BYTES_OUT_PER_PACKET_OUT);
      monValues << "bytes/packet, ";
    }
    else if ( 0 == strcasecmp("packets_out-packets_in",(*it).c_str()) ) {
      monitored_values.push_back(PACKETS_OUT_MINUS_PACKETS_IN);
      monValues << "packets_out-packets_in, ";
    }
    else if ( 0 == strcasecmp("octets_out-octets_in",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_OUT_MINUS_BYTES_IN);
      monValues << "octets_out-octets_in, ";
    }
    else if ( 0 == strcasecmp("bytes_out-bytes_in",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_OUT_MINUS_BYTES_IN);
      monValues << "octets_out-octets_in, ";
    }
    else if ( 0 == strcasecmp("packets(t)-packets(t-1)",(*it).c_str()) ) {
      monitored_values.push_back(PACKETS_T_IN_MINUS_PACKETS_T_1_IN);
      monitored_values.push_back(PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT);
      monValues << "packets(t)-packets(t-1), ";
    }
    else if ( 0 == strcasecmp("octets(t)-octets(t-1)",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_T_IN_MINUS_BYTES_T_1_IN);
      monitored_values.push_back(BYTES_T_OUT_MINUS_BYTES_T_1_OUT);
      monValues << "octets(t)-octets(t-1), ";
    }
    else if ( 0 == strcasecmp("bytes(t)-bytes(t-1)",(*it).c_str()) ) {
      monitored_values.push_back(BYTES_T_IN_MINUS_BYTES_T_1_IN);
      monitored_values.push_back(BYTES_T_OUT_MINUS_BYTES_T_1_OUT);
      monValues << "octets(t)-octets(t-1), ";
    }
    else {
        std::cerr << Warning3.str() << Usage.str() << "Exiting.\n";
        if (warning_verbosity==1)
          outfile << Warning3.str() << Usage.str() << "Exiting." << std::endl;
        exit(0);
      }
    it++;
  }

  if (output_verbosity >= 3)
      outfile << "Monitored values: " << monValues.str() << std::endl;

  // subscribing to all needed IPFIX_TYPEID-fields

  // to prevent multiple subscription
  bool packetsSubscribed = false;
  bool bytesSubscribed = false;

  for (int i = 0; i != monitored_values.size(); i++) {
    // we only need to check the IN-metrics, because if the
    // corresponding monitored value was chosen, both metrics
    // have to be in monitored_values
    if ( (monitored_values.at(i) == PACKETS_IN
      || monitored_values.at(i) == PACKETS_OUT_MINUS_PACKETS_IN
      || monitored_values.at(i) == PACKETS_T_IN_MINUS_PACKETS_T_1_IN)
      && packetsSubscribed == false) {
      subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
      packetsSubscribed = true;
    }

    if ( (monitored_values.at(i) == BYTES_IN
      || monitored_values.at(i) == BYTES_OUT_MINUS_BYTES_IN
      || monitored_values.at(i) == BYTES_T_IN_MINUS_BYTES_T_1_IN)
      && bytesSubscribed == false ) {
      subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
      bytesSubscribed = true;
    }

    if ( monitored_values.at(i) == BYTES_IN_PER_PACKET_IN
      && packetsSubscribed == false
      && bytesSubscribed == false) {
      subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
      subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
    }

  }

  // whatever the above monitored values are, we always monitor IP addresses
  // TODO(3): Maybe if we are only interested in Ports, we dont need IPs
  subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
  subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);

  return;
}

void Stat::init_noise_thresholds(XMLConfObj * config) {

  // extracting noise threshold for packets
  if (config->getValue("noise_threshold_packets").empty()) {
    std::stringstream Default1;
    Default1 << "Warning! No noise threshold for packets was provided!\n"
        << "There will be no noise reduction for packets.\n";
    std::cerr << Default1.str();

    if (warning_verbosity==1)
      outfile << Default1.str() << std::flush;

    noise_threshold_packets = 0;
  }

  else {
    noise_threshold_packets = atoi(config->getValue("noise_threshold_packets").c_str());
  }

  // extracting noise threshold for bytes
  if (config->getValue("noise_threshold_bytes").empty()) {
    std::stringstream Default2;
    Default2 << "Warning! No noise threshold for bytes was provided!\n"
        << "There will be no noise reduction for bytes.\n";
    std::cerr << Default2.str();
    if (warning_verbosity==1)
      outfile << Default2.str() << std::flush;

    noise_threshold_bytes = 0;
  }

  else {
    noise_threshold_bytes =	atoi(config->getValue("noise_threshold_bytes").c_str());
  }

  return;

}

void Stat::init_protocols(XMLConfObj * config) {

  port_monitoring = false;

  // extracting monitored protocols
  std::stringstream Default, Usage;
  Default << "Warning! No protocol(s) to monitor was (were) provided!\n"
	  << "All protocols will be monitored (ICMP, TCP, UDP, RAW).\n";
  Usage << "Please use ICMP, TCP, UDP or RAW (or "
	<< IPFIX_protocolIdentifier_ICMP << ", "
	<< IPFIX_protocolIdentifier_TCP  << ", "
	<< IPFIX_protocolIdentifier_UDP << " or "
	<< IPFIX_protocolIdentifier_RAW  << ").\n";

  if (!(config->getValue("protocols")).empty()) {

    std::string Proto = config->getValue("protocols");
    std::istringstream ProtoStream (Proto);

    std::string protocol;
    while (ProtoStream >> protocol) {

      if ( strcasecmp(protocol.c_str(),"ICMP") == 0
	    || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_ICMP )
	      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_ICMP);
      else if ( strcasecmp(protocol.c_str(),"TCP") == 0
		  || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_TCP ) {
	      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_TCP);
	      port_monitoring = true;
      }
      else if ( strcasecmp(protocol.c_str(),"UDP") == 0
      || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_UDP ) {
	      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_UDP);
	      port_monitoring = true;
      }
      else if ( strcasecmp(protocol.c_str(),"RAW") == 0
		  || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_RAW )
	      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_RAW);
      else {
	      std::cerr << "Warning! An unknown protocol (" << protocol
		    << ") was provided!\n"
		    << Usage.str() << "Exiting.\n";
	      if (warning_verbosity==1)
	        outfile << "Warning! An unknown protocol (" << protocol
		      << ") was provided!\n"
		      << Usage.str() << "Exiting." << std::endl;
	      exit(0);
      }

    }

  }

  else {
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_ICMP);
    StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_TCP);
    StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_UDP);
    StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_RAW);
    port_monitoring = true;
  }

  // in either way
  subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);

  return;

}

void Stat::init_netmask(XMLConfObj * config) {

  std::stringstream Default, Warning, Usage;
  Default << "Warning! No netmask parameter defined in XML config file! "
	  << "32 assumed.\n";
  Warning << "Warning! Netmask was provided in an unknown format!\n";
  Usage   << "Use xxx.yyy.zzz.ttt, hexadecimal or an int between 0 and 32.\n";

  const char * netmask;
  unsigned int mask[4];

  if (!(config->getValue("netmask")).empty()) {
    netmask = config->getValue("netmask").c_str();
  }

  else {
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str();
    StatStore::InitialiseSubnetMask (0xFF,0xFF,0xFF,0xFF);
    return;
  }

  // is netmask provided as xxx.yyy.zzz.ttt?
  if ( netmask[0]=='.' || netmask[1]=='.' ||
       netmask[2]=='.' || netmask[3]=='.' ) {
    std::istringstream maskstream (netmask);
    char dot;
    maskstream >> mask[0] >> dot >> mask[1] >> dot >> mask[2] >> dot >>mask[3];
  }

  // is netmask provided as 0xABCDEFGH?
  else if ( strncmp(netmask, "0x", 2)==0 ) {
    char mask1[3] = {netmask[2],netmask[3],'\0'};
    char mask2[3] = {netmask[4],netmask[5],'\0'};
    char mask3[3] = {netmask[6],netmask[7],'\0'};
    char mask4[3] = {netmask[8],netmask[9],'\0'};
    mask[0] = strtol (mask1, NULL, 16);
    mask[1] = strtol (mask2, NULL, 16);
    mask[2] = strtol (mask3, NULL, 16);
    mask[3] = strtol (mask4, NULL, 16);
  }

  // is netmask provided as 0,1,..,32?
  else if ( atoi(netmask) >= 0 && atoi(netmask) <= 32 ) {
    std::string Mask;
    switch( atoi(netmask) ) {
    case  0: Mask="0x00000000"; break;
    case  1: Mask="0x80000000"; break;
    case  2: Mask="0xC0000000"; break;
    case  3: Mask="0xE0000000"; break;
    case  4: Mask="0xF0000000"; break;
    case  5: Mask="0xF8000000"; break;
    case  6: Mask="0xFC000000"; break;
    case  7: Mask="0xFE000000"; break;
    case  8: Mask="0xFF000000"; break;
    case  9: Mask="0xFF800000"; break;
    case 10: Mask="0xFFC00000"; break;
    case 11: Mask="0xFFE00000"; break;
    case 12: Mask="0xFFF00000"; break;
    case 13: Mask="0xFFF80000"; break;
    case 14: Mask="0xFFFC0000"; break;
    case 15: Mask="0xFFFE0000"; break;
    case 16: Mask="0xFFFF0000"; break;
    case 17: Mask="0xFFFF8000"; break;
    case 18: Mask="0xFFFFC000"; break;
    case 19: Mask="0xFFFFE000"; break;
    case 20: Mask="0xFFFFF000"; break;
    case 21: Mask="0xFFFFF800"; break;
    case 22: Mask="0xFFFFFC00"; break;
    case 23: Mask="0xFFFFFE00"; break;
    case 24: Mask="0xFFFFFF00"; break;
    case 25: Mask="0xFFFFFF80"; break;
    case 26: Mask="0xFFFFFFC0"; break;
    case 27: Mask="0xFFFFFFE0"; break;
    case 28: Mask="0xFFFFFFF0"; break;
    case 29: Mask="0xFFFFFFF8"; break;
    case 30: Mask="0xFFFFFFFC"; break;
    case 31: Mask="0xFFFFFFFE"; break;
    case 32: Mask="0xFFFFFFFF"; break;
    }
    char mask1[3] = {Mask[2],Mask[3],'\0'};
    char mask2[3] = {Mask[4],Mask[5],'\0'};
    char mask3[3] = {Mask[6],Mask[7],'\0'};
    char mask4[3] = {Mask[8],Mask[9],'\0'};
    mask[0] = strtol (mask1, NULL, 16);
    mask[1] = strtol (mask2, NULL, 16);
    mask[2] = strtol (mask3, NULL, 16);
    mask[3] = strtol (mask4, NULL, 16);
  }

  // if not, then netmask is provided in an unknown format
  else {
    std::cerr << Warning.str() << Usage.str() << "Exiting.\n";
    if (warning_verbosity==1)
      outfile << Warning.str() << Usage.str() << "Exiting." << std::endl;
    exit(0);
  }

  // if everything is OK:
  StatStore::InitialiseSubnetMask (mask[0], mask[1], mask[2], mask[3]);
  return;

}

void Stat::init_ports(XMLConfObj * config) {

  std::stringstream Warning, Default;
  Warning << "Warning! No port number(s) to monitor was (were) provided!\n";
  Default << "Every port will be monitored.\n";

  // extracting port numbers to monitor
  if (!(config->getValue("ports")).empty()) {

    if (port_monitoring == false)
      return;
      // only ICMP and/or RAW are monitored: no need to subscribe to
      // IPFIX port information

    std::string Ports = config->getValue("ports");
    std::istringstream PortsStream (Ports);

    unsigned int port;
    while (PortsStream >> port)
      StatStore::AddPortToMonitoredPorts(port);

    StatStore::setMonitorAllPorts() = false;
  }
  else {

    if (port_monitoring == false)
      return;
      // only ICMP and/or RAW are monitored: no need to issue any
      // warning message

    std::cerr << Warning.str() << Default.str();
    if (warning_verbosity==1)
      outfile << Warning.str() << Default.str();

    StatStore::setMonitorAllPorts() = true;

  }

  // in either way
  //TODO(3) Maybe we arent interested in Ports, but only in IPs?
  subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
  subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);

  return;

}

void Stat::init_ip_addresses(XMLConfObj * config) {

  std::stringstream Warning, Default;
  Warning
    << "Warning! I got no file with IP addresses to monitor in  config file!\n"
    << "I suppose you want me to monitor everything; I'll try.\n";
  Default
    << "Warning! I got no iplist_maxsize preference in XML config file!\n"
    << "I'll use default value, " << DEFAULT_iplist_maxsize << ".\n";

  // the following section extracts either the IP addresses to monitor
  // or the maximal number of IPs to monitor in case the user doesn't
  // give IP addresses to monitor
  bool gotIpfile = false;

  if (!(config->getValue("ip_addresses_to_monitor")).empty()) {
    ipfile = config->getValue("ip_addresses_to_monitor");
    gotIpfile = true;
  }
  else {
    std::cerr << Warning.str();
    if (warning_verbosity==1)
      outfile << Warning.str() << std::flush;
    if (!(config->getValue("iplist_maxsize")).empty()) {
      iplist_maxsize = atoi( config->getValue("iplist_maxsize").c_str() );
    }
    else {
      std::cerr << Default.str();
      if (warning_verbosity==1)
	outfile << Default.str() << std::flush;
      iplist_maxsize = DEFAULT_iplist_maxsize;
    }
  }

  // and the following section uses the extracted ipfile or iplist_maxsize
  // information to initialise some static members of the StatStore class
  if (gotIpfile == true) {

    std::stringstream Error;
    Error << "Error! I could not open IP-file " << ipfile << "!\n"
    << "Please check that the file exists, "
    << "and that you have enough rights to read it.\n"
    << "Exiting.\n";

    std::ifstream ipstream(ipfile.c_str());
      // .c_str() because the constructor only accepts C-style strings

    if (!ipstream) {
      std::cerr << Error.str();
      if (warning_verbosity==1)
  outfile << Error.str() << std::flush;
      exit(0);
    }

    else {

      std::vector<EndPoint> IpVector;
      unsigned int ip[4];
      char dot;

      while (ipstream >> ip[0] >> dot >> ip[1] >> dot >> ip[2] >> dot>>ip[3]) {
  // save read IPs in a vector; mask them at the same time
  IpVector.push_back(EndPoint(IpAddress(ip[0],ip[1],ip[2],ip[3]).mask(StatStore::getSubnetMask()), 0, 0));
      }

      // just in case the user provided a file with multiples IPs,
      // or the mask function made multiples IPs to appear:
      sort(IpVector.begin(),IpVector.end());
      std::vector<EndPoint>::iterator new_end = unique(IpVector.begin(),IpVector.end());
      std::vector<EndPoint> IpVectorBis (IpVector.begin(), new_end);

      // finally, add these IPs to monitor to static member
      // StatStore::MonitoredIpAddresses
      StatStore::AddIpToMonitoredIp (IpVectorBis);

      // no need to monitor every IP:
      StatStore::setMonitorEveryIp() = false;
    }

  }
  else { // gotIpfile == false
    StatStore::setMonitorEveryIp() = true;
    StatStore::setIpListMaxSize() = iplist_maxsize;
  }

  return;

}

void Stat::init_stat_test_freq(XMLConfObj * config) {

  // extracting statistical test frequency
  if (!(config->getValue("stat_test_frequency")).empty()) {
    stat_test_frequency =
      atoi( config->getValue("stat_test_frequency").c_str() );
  }

  else {
    std::stringstream Default;
    Default << "Warning! No stat_test_frequency parameter "
	    << "defined in XML config file! \""
	    << DEFAULT_stat_test_frequency << "\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str();
    stat_test_frequency = DEFAULT_stat_test_frequency;
  }

  return;

}

void Stat::init_report_only_first_attack(XMLConfObj * config) {

  // extracting report_only_first_attack preference
  if (!(config->getValue("report_only_first_attack")).empty()) {
    report_only_first_attack =
      ( 0 == strcasecmp("true", config->getValue("report_only_first_attack").c_str()) ) ? true:false;
  }

  else {
    std::stringstream Default;
    Default << "Warning! No report_only_first_attack parameter "
	    << "defined in XML config file! \"true\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str();
    report_only_first_attack = true;
  }

  // whatever the choice of the user is, we set last_xyz_test_was_an_attack
  // flags = false as the first test isn't an attack and as these flags are
  // also needed by the update function
  last_wmw_test_was_an_attack = false;
  last_ks_test_was_an_attack  = false;
  last_pcs_test_was_an_attack = false;

  return;

}

void Stat::init_pause_update_when_attack(XMLConfObj * config) {

  // extracting pause_update_when_attack preference
  if (!(config->getValue("pause_update_when_attack")).empty()) {
    pause_update_when_attack =
      ( 0 == strcasecmp("true", config->getValue("pause_update_when_attack").c_str()) ) ? true:false;
  }

  else {
    std::stringstream Default;
    Default << "Warning! No pause_update_when_attack parameter "
	    << "defined in XML config file! \"true\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str();
    pause_update_when_attack = true;
  }

  return;

}

void Stat::init_which_test(XMLConfObj * config) {

  std::stringstream WMWdefault, KSdefault, PCSdefault;
  WMWdefault << "Warning! No wilcoxon_test parameter "
	     << "defined in XML config file! \"true\" assumed.\n";
  KSdefault  << "Warning! No kolmogorov_test parameter "
	     << "defined in XML config file! \"true\" assumed.\n";
  PCSdefault << "Warning! No pearson_chi-square_test parameter "
	     << "defined in XML config file! \"true\" assumed.\n";

  // extracting type of test
  // (Wilcoxon and/or Kolmogorov and/or Pearson chi-square)
  if (!(config->getValue("wilcoxon_test")).empty()) {
    enable_wmw_test = ( 0 == strcasecmp("true", config->getValue("wilcoxon_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << WMWdefault.str();
    if (warning_verbosity==1)
      outfile << WMWdefault.str() << std::flush;
    enable_wmw_test = true;
  }

  if (!(config->getValue("kolmogorov_test")).empty()) {
    enable_ks_test = ( 0 == strcasecmp("true", config->getValue("kolmogorov_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << KSdefault.str();
    if (warning_verbosity==1)
      outfile << KSdefault.str() << std::flush;
    enable_ks_test = true;
  }

  if (!(config->getValue("pearson_chi-square_test")).empty()) {
    enable_pcs_test = ( 0 == strcasecmp("true", config->getValue("pearson_chi-square_test").c_str()) ) ?true:false;
  }
  else {
    std::cerr << PCSdefault.str();
    if (warning_verbosity==1)
      outfile << PCSdefault.str() << std::flush;
    enable_pcs_test = true;
  }

  return;

}

void Stat::init_sample_sizes(XMLConfObj * config) {

  std::stringstream Default_old, Default_new;
  Default_old << "Warning! No sample_old_size defined in XML config file! "
	      << "Using default value, "
	      << DEFAULT_sample_old_size << ".\n";
  Default_new << "Warning! No sample_new_size defined in XML config file! "
	      << "Using default value, "
	      << DEFAULT_sample_new_size << ".\n";

  // extracting size of sample_old
  if (!(config->getValue("sample_old_size")).empty()) {
    sample_old_size =
      atoi( config->getValue("sample_old_size").c_str() );
  }
  else {
    std::cerr << Default_old.str();
    if (warning_verbosity==1)
      outfile << Default_old.str() << std::flush;
    sample_old_size = DEFAULT_sample_old_size;
  }

  // extracting size of sample_new
  if (!(config->getValue("sample_new_size")).empty()) {
    sample_new_size =
      atoi( config->getValue("sample_new_size").c_str() );
  }
  else {
    std::cerr << Default_new.str();
    if (warning_verbosity==1)
      outfile << Default_new.str() << std::flush;
    sample_new_size = DEFAULT_sample_new_size;
  }

  return;

}

void Stat::init_two_sided(XMLConfObj * config) {

  // extracting one/two-sided parameter for the test
  if (!(config->getValue("two_sided")).empty()) {
    two_sided = ( 0 == strcasecmp("true", config->getValue("two_sided").c_str()) ) ? true:false;
  }

  else if (enable_wmw_test == true) {
    // one/two-sided parameter is active only for Wilcoxon-Mann-Whitney
    // statistical test; Pearson chi-square test and Kolmogorov-Smirnov
    // test are one-sided only.
    std::stringstream Default;
    Default << "Warning! No two_sided parameter defined in XML config file! "
	    << "\"false\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str();
    two_sided = false;
  }

  return;

}

void Stat::init_significance_level(XMLConfObj * config) {

  // extracting significance level parameter
  if (!(config->getValue("significance_level")).empty()) {
    if ( 0 <= atof( config->getValue("significance_level").c_str() )
	 &&
	 1 >= atof( config->getValue("significance_level").c_str() ) )
      significance_level =
	atof( config->getValue("significance_level").c_str() );
    else {
      std::stringstream Warning("Warning! Parameter significance_level should be between 0 and 1! Exiting.\n");
      std::cerr << Warning.str();
      if (warning_verbosity==1)
	outfile << Warning.str() << std::flush;
      exit(0);
    }
  }

  else {
    significance_level = -1;
    // means no alarmist verbose effect ("Attack!!!", etc):
    // as 0 <= p-value <= 1, we will always have
    // p-value > "significance_level" = -1,
    // i.e. nothing will be interpreted as an attack
    // and no verbose effect will be outputed
  }

  return;

}


// ============================= TEST FUNCTION ===============================


void Stat::test(StatStore * store) {


#ifdef IDMEF_SUPPORT_ENABLED
  idmefMessage = getNewIdmefMessage("wkp-module", "statistical anomaly detection");
#endif

  outfile << "########## Stat::test(...)-call number: " << test_counter
	  << " ##########" << std::endl;

  // Getting whole Data from store
  std::map<EndPoint,Info> Data = store->getData();
  std::map<EndPoint,Info> PreviousData = store->getPreviousData();

  // Print some warning message if it appears that maximal size of IP list
  // was reached (in the StatStore object "store") and that a new IP
  // had to be rejected:
  if (store->IpListMaxSizeReachedAndNewIpWantedToEnterIt == 1) {
    std::cerr << "Could not monitor a new IP address, "
	      << "maximal size of sample container reached\n";
    if (output_verbosity >= 3) {
      outfile << "Could not monitor a new IP address, "
	      << "maximal size of sample container reached" << std::endl;
    }
  }

  // Dumping empty records:
  if (Data.empty()==true) {
    if (output_verbosity>=3 || warning_verbosity==1)
      outfile << "Got empty record; "
	      << "dumping it and waiting for another record" << std::endl;
    return;
  }

  // 1) LEARN/UPDATE PHASE

  // Parsing data to see whether the recorded EndPoints already exist
  // in our  "std::map<EndPoint, Samples> Records"  sample container.
  // If not, then we add it as a new pair <EndPoint, Samples>.
  // If yes, then we update the corresponding Samples using
  // Values extracted data.

  outfile << "#### LEARN/UPDATE PHASE" << std::endl;

  std::map<EndPoint,Info>::iterator Data_it = Data.begin();

  // Needed for extraction of packets(t)-packets(t-1) and bytes(t)-bytes(t-1)
  // Holds information about the Info used in the last call to test()
  Info prev;

  // for every EndPoint, extract the data
  while (Data_it != Data.end()) {

    prev = PreviousData[Data_it->first];
    // it doesn't matter much if Data_it->first is an EndPoint that exists
    // only in Data, but not in PreviousData, because
    // PreviousData[Data_it->first] will automaticaly be an Info structure
    // with all fields set to 0.

    std::map<EndPoint, Samples>::iterator Records_it =
      Records.find(Data_it->first);

    outfile << "[[ " << Data_it->first << " ]]" << std::endl;

    if (Records_it == Records.end()) {

      // We didn't find the recorded EndPoint Data_it->first
      // in our sample container "Records"; that means it's a new one,
      // so we just add it in "Records"; there will not be jeopardy
      // of memory exhaustion through endless growth of the "Records" map
      // as limits are implemented in the StatStore class

      Samples S;
      // extract_data has enum Metric as output
      (S.Old).push_back(extract_data(Data_it->second, prev));
      Records[Data_it->first] = S;
      if (output_verbosity >= 3) {
	      outfile << "New monitored EndPoint added" << std::endl;
	      if (output_verbosity >= 4) {
          outfile << "with first sample_old: " << S.Old.back() << std::endl;
	      }
      }

    }
    else {
      // We found the recorded EndPoint Data_it->first
      // in our sample container "Records"; so we update the samples
      // (Records_it->second).Old and (Records_it->second).New
      // thanks to the recorded new value in Data_it->second:
      update ( (Records_it->second).Old , (Records_it->second).New ,
	       extract_data(Data_it->second, prev) );

    }

    Data_it++;

  }

  // 1.5) MAP PRINTING (OPTIONAL, DEPENDS ON VERBOSITY SETTINGS)


  // Problem: (Records_it->second).Old und (Records_it->second).New
  // waren bisher int, jetzt sind es Structs Values.
  // Hier sollen aber nicht alle darin enthaltenen Metriken
  // ausgegeben werden, sondern nur die relevanten ...
  if (output_verbosity >= 4) {
    outfile << "#### STATE OF ALL MONITORED ENDPOINTS:" << std::endl;
    std::map<EndPoint, Samples>::iterator Records_it =
      Records.begin();
    while (Records_it != Records.end()) {
      outfile << "[[ " << Records_it->first << " ]]" << std::endl;
      outfile << "size of old: " << ((Records_it->second).Old).size();
      outfile << " ## size of new: " << ((Records_it->second).New).size() << std::endl;
      Records_it++;
    }
    outfile << std::flush;
  }

  // 2) STATISTICAL TEST
  // (OPTIONAL, DEPENDS ON HOW OFTEN THE USER WISHES TO DO IT)

  bool MakeStatTest;
  if (stat_test_frequency == 0)
    MakeStatTest = false;
  else
    MakeStatTest = (test_counter % stat_test_frequency == 0);

  // (i.e., if stat_test_frequency=X, then the test will be conducted when
  // test_counter is a multiple of X; if X=0, then the test will never
  // be conducted)

  if (MakeStatTest == true) {

    outfile << "#### STATISTICAL TESTS" << std::endl;
    // We begin testing as soon as possible, i.e. as soon as a sample
    // is big enough to test, i.e. when its learning phase is over.
    // The other samples in the "Records" map are let learning.

    std::map<EndPoint,Samples>::iterator Records_it = Records.begin();

    while (Records_it != Records.end()) {
      if ( ((Records_it->second).New).size() == sample_new_size ) {
        // i.e., learning phase over
        outfile << "\n[[ " << Records_it->first << " ]]\n";
        stat_test ( (Records_it->second).Old, (Records_it->second).New );
      }
      Records_it++;
    }

  }

  test_counter++;

  /* don't forget to free the store-object! */
  delete store;
  return;

}



// =================== FUNCTIONS USED BY THE TEST FUNCTION ====================


// ------ FUNCTIONS USED TO EXTRACT DATA FROM THE STORAGE CLASS StatStore -----

// extracts interesting data from StatStore according to monitored_values:
//

Values Stat::extract_data (const Info & info, const Info & prev) {

  Values result;
  init_values(result); // int64_t doesnt seem to be initialized to 0 by default
  bool gotValue = false; // false means: unknown type of value occurred

  std::vector<Metric>::iterator it = monitored_values.begin();

  // we only need to check the IN-metrics, because if the
  // corresponding monitored value was chosen, both metrics
  // have to be in monitored_values
  while (it != monitored_values.end() ) {

    switch ( *it ) {
      case PACKETS_IN:
        extract_data_packets (info, result);
        gotValue = true;
        break;
      case BYTES_IN:
        extract_data_octets (info, result);
        gotValue = true;
        break;
      case RECORDS_IN:
        extract_data_records (info, result);
        gotValue = true;
        break;
      case BYTES_IN_PER_PACKET_IN:
        extract_data_octets_per_packets (info, result);
        gotValue = true;
        break;
      case PACKETS_OUT_MINUS_PACKETS_IN:
        extract_data_packets_out_minus_packets_in (info, result);
        gotValue = true;
        break;
      case BYTES_OUT_MINUS_BYTES_IN:
        extract_data_octets_out_minus_octets_in (info, result);
        gotValue = true;
        break;
      case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
        extract_packets_t_minus_packets_t_1 (info, prev, result);
        gotValue = true;
        break;
      case BYTES_T_IN_MINUS_BYTES_T_1_IN:
        extract_octets_t_minus_octets_t_1 (info, prev, result);
        gotValue = true;
        break;
    }

    it++;
  }

  if (gotValue == false) {
    std::cerr << "Error! monitored_values seems to be empty "
        << "or it holds an unknown type which isnt supported yet."
        << "But this shouldnt happen as the init_monitored_values"
        << "-function handles that.\n";
    exit(0);
  }

  return result;

}

// functions called by the extract_data()-function:
//
void Stat::extract_data_packets (const Info & info, Values & result) {

  if (info.packets_out >= noise_threshold_packets)
    result.packets_out = info.packets_out;
  if (info.packets_in  >= noise_threshold_packets)
    result.packets_in = info.packets_in;

  return;
}

void Stat::extract_data_octets (const Info & info, Values & result) {

  if (info.bytes_out >= noise_threshold_bytes)
    result.bytes_out = info.bytes_out;
  if (info.bytes_in  >= noise_threshold_bytes)
    result.bytes_in  = info.bytes_in;

  return;
}

void Stat::extract_data_records (const Info & info, Values & result) {

  result.records_out = info.records_out;
  result.records_in  = info.records_in;

  return;
}

void Stat::extract_data_octets_per_packets (const Info & info, Values & result) {

  int64_t packets_in;
  int64_t packets_out;
  int64_t bytes_in;
  int64_t bytes_out;

  if (info.packets_out >= noise_threshold_packets ||
      info.bytes_out   >= noise_threshold_bytes ) {
    packets_out  = info.packets_out;
    bytes_out    = info.bytes_out;
  }
  if (info.packets_in >= noise_threshold_packets ||
      info.bytes_in   >= noise_threshold_bytes ) {
    packets_in  = info.packets_in;
    bytes_in    = info.bytes_in;
  }

  if (packets_out == 0)
    result.bytes_per_packet_out = 0;
  // i.e. bytes_per_packet_out[EndPoint] = 0
  else
    result.bytes_per_packet_out = (1000 * bytes_out) / packets_out;
  // i.e. bytes_per_packet_out[EndPoint] = (1000 * #bytes) / #packets
  // the multiplier 1000 enables us to increase precision and "simulate"
  // a float result, while keeping an integer result: thanks to this trick,
  // we do not have to write new versions of the tests to support floats
  if (packets_in == 0)
    result.bytes_per_packet_in = 0;
  else
    result.bytes_per_packet_in = (1000 * bytes_in) / packets_in;

  return;
}

void Stat::extract_data_packets_out_minus_packets_in (const Info & info, Values & result) {

  if (info.packets_out >= noise_threshold_packets ||
      info.packets_in  >= noise_threshold_packets )
    result.p_out_minus_p_in = info.packets_out - info.packets_in;

  return;
}

void Stat::extract_data_octets_out_minus_octets_in (const Info & info, Values & result) {

  if (info.bytes_out >= noise_threshold_bytes ||
      info.bytes_in  >= noise_threshold_bytes )
    result.b_out_minus_b_in = info.bytes_out - info.bytes_in;

  return;
}

void Stat::extract_packets_t_minus_packets_t_1 (const Info & info, const Info & prev, Values & result) {

  // prev holds the data for the same EndPoint as info
  // from the last call to test()
  // it is updated at the beginning of the while-loop in test()
  if (info.packets_out >= noise_threshold_packets ||
      prev.packets_out >= noise_threshold_packets)
    result.pt_minus_pt1_out = info.packets_out - prev.packets_out;
  if (info.packets_in  >= noise_threshold_packets ||
      prev.packets_in  >= noise_threshold_packets)
    result.pt_minus_pt1_in = info.packets_in  - prev.packets_in;

  return;
}

void Stat::extract_octets_t_minus_octets_t_1 (const Info & info, const Info & prev, Values & result) {

  // prev holds the data for the same EndPoint as info
  // from the last call to test()
  // it is updated at the beginning of the while-loop in test()
  if (info.bytes_out >= noise_threshold_bytes ||
      prev.bytes_out >= noise_threshold_bytes)
    result.bt_minus_bt1_out = info.bytes_out - prev.bytes_out;
  if (info.bytes_in >= noise_threshold_bytes ||
      prev.bytes_in >= noise_threshold_bytes)
    result.bt_minus_bt1_in = info.bytes_in - prev.bytes_in;

  return;
}



// -------------------------- LEARN/UPDATE FUNCTION ---------------------------

// learn/update function for samples (called everytime test() is called)
//
void Stat::update ( std::list<Values> & sample_old,
		    std::list<Values> & sample_new,
		    const Values & new_value ) {

  // Learning phase?
  if (sample_old.size() != sample_old_size) {

    sample_old.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << "Learning phase for sample_old..." << std::endl;
      // TODO(2)
      // sample_old und sample_new waren bisher list<int>
      // Jetzt sind es list<Values>. Hier sollen aber nicht alle
      // darin enthaltenen Metriken ausgegeben werden, sondern
      // nur die relevanten ...
      /*
      if (output_verbosity >= 4) {
        outfile << "  sample_old: " << sample_old << std::endl;
        outfile << "  sample_new: " << sample_new << std::endl;
      }
      */
    }

    return;
  }
  else if (sample_new.size() != sample_new_size) {

    sample_new.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << "Learning phase for sample_new..." << std::endl;
      // TODO(2)
      // sample_old und sample_new waren bisher list<int>
      // Jetzt sind es list<Values>. Hier solle naber nicht alle
      // darin enthaltenen Metriken ausgegeben werden, sondern
      // nur die relevanten ...
      /*
      if (output_verbosity >= 4) {
        outfile << "  sample_old: " << sample_old << std::endl;
        outfile << "  sample_new: " << sample_new << std::endl;
      }
      */
    }

    return;
  }

  // Learning phase over: update:
  if (pause_update_when_attack == false) {
    sample_old.pop_front();
    sample_old.push_back(sample_new.front());
    sample_new.pop_front();
    sample_new.push_back(new_value);
  }
  else if (last_wmw_test_was_an_attack == false ||
	   last_ks_test_was_an_attack  == false ||
	   last_pcs_test_was_an_attack == false ) {
    sample_old.pop_front();
    sample_old.push_back(sample_new.front());
    sample_new.pop_front();
    sample_new.push_back(new_value);
  }
  else {
    sample_new.pop_front();
    sample_new.push_back(new_value);
  }

  if (output_verbosity >= 3) {
    outfile << "Update done" << std::endl;
    // TODO(2)
    // sample_old und sample_new waren bisher list<int>
    // Jetzt sind es list<Values>. Hier solle naber nicht alle
    // darin enthaltenen Metriken ausgegeben werden, sondern
    // nur die relevanten ...
    /*
    if (output_verbosity >= 4) {
      outfile << "  sample_old: " << sample_old << std::endl;
      outfile << "  sample_new: " << sample_new << std::endl;
    }
    */
  }

  return;

}

// ------- FUNCTIONS USED TO CONDUCT STATISTICAL TESTS ON THE SAMPLES ---------

// statistical test function
// (optional, depending on how often the user wishes to do it)
//
void Stat::stat_test (std::list<Values> & sample_old,
		      std::list<Values> & sample_new) {

  // Containers for the values of single metrics
  std::list<int64_t> sample_old_single_metric;
  std::list<int64_t> sample_new_single_metric;

  std::vector<Metric>::iterator it = monitored_values.begin();

  // for every value (represented by *it) in monitored_values,
  // do the tests; and if a value was splitted into two,
  // do the tests for both of them
  while (it != monitored_values.end()) {

    if (output_verbosity >= 4)
      outfile << "### Performing Statistical Tests for metric " << getMetricName(*it) << ":\n";

    sample_old_single_metric = getSingleMetric(sample_old, *it);
    sample_new_single_metric = getSingleMetric(sample_new, *it);

    // Wilcoxon-Mann-Whitney test:
    if (enable_wmw_test == true)
      stat_test_wmw(sample_old_single_metric, sample_new_single_metric);

    // Kolmogorov-Smirnov test:
    if (enable_ks_test == true)
      stat_test_ks (sample_old_single_metric, sample_new_single_metric);

    // Pearson chi-square test:
    if (enable_pcs_test == true)
      stat_test_pcs(sample_old_single_metric, sample_new_single_metric);

    it++;
  }

  return;

}

// functions called by the stat_test()-function
std::list<int64_t> Stat::getSingleMetric(const std::list<Values> & l, const enum Metric & m) {

  std::list<int64_t> result;
  std::list<Values>::const_iterator it = l.begin();

  switch(m) {
    case PACKETS_IN:
      while ( it != l.end() ) {
        result.push_back(it->packets_in);
        it++;
      }
      break;
    case PACKETS_OUT:
      while ( it != l.end() ) {
        result.push_back(it->packets_out);
        it++;
      }
      break;
    case BYTES_IN:
      while ( it != l.end() ) {
        result.push_back(it->bytes_in);
        it++;
      }
      break;
    case BYTES_OUT:
      while ( it != l.end() ) {
        result.push_back(it->bytes_out);
        it++;
      }
      break;
    case RECORDS_IN:
      while ( it != l.end() ) {
        result.push_back(it->records_in);
        it++;
      }
      break;
    case RECORDS_OUT:
      while ( it != l.end() ) {
        result.push_back(it->records_out);
        it++;
      }
      break;
    case BYTES_IN_PER_PACKET_IN:
      while ( it != l.end() ) {
        result.push_back(it->bytes_per_packet_in);
        it++;
      }
      break;
    case BYTES_OUT_PER_PACKET_OUT:
      while ( it != l.end() ) {
        result.push_back(it->bytes_per_packet_out);
        it++;
      }
      break;
    case PACKETS_OUT_MINUS_PACKETS_IN:
      while ( it != l.end() ) {
        result.push_back(it->p_out_minus_p_in);
        it++;
      }
      break;
    case BYTES_OUT_MINUS_BYTES_IN:
      while ( it != l.end() ) {
        result.push_back(it->b_out_minus_b_in);
        it++;
      }
      break;
    case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
      while ( it != l.end() ) {
        result.push_back(it->pt_minus_pt1_in);
        it++;
      }
      break;
    case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
      while ( it != l.end() ) {
        result.push_back(it->pt_minus_pt1_out);
        it++;
      }
      break;
    case BYTES_T_IN_MINUS_BYTES_T_1_IN:
      while ( it != l.end() ) {
        result.push_back(it->bt_minus_bt1_in);
        it++;
      }
      break;
    case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
      while ( it != l.end() ) {
        result.push_back(it->bt_minus_bt1_out);
        it++;
      }
      break;
    default:
      std::cerr << "Error! Got unknown metric in Stat::getSingleMetric(...)!\n"
                << "init_monitored_values() should not let this Error happen!";
      exit(0);
  }

  return result;
}

std::string Stat::getMetricName(const enum Metric & m) {
  switch(m) {
    case PACKETS_IN:
      return std::string("packets_in");
    case PACKETS_OUT:
      return std::string("packets_out");
    case BYTES_IN:
      return std::string("octets_in");
    case BYTES_OUT:
      return std::string("octets_out");
    case RECORDS_IN:
      return std::string("records_in");
    case RECORDS_OUT:
      return std::string("records_out");
    case BYTES_IN_PER_PACKET_IN:
      return std::string("octets_in/packet_in");
    case BYTES_OUT_PER_PACKET_OUT:
      return std::string("octets_out/packet_out");
    case PACKETS_OUT_MINUS_PACKETS_IN:
      return std::string("packets_out-packets_in");
    case BYTES_OUT_MINUS_BYTES_IN:
      return std::string("octets_out-octets_in");
    case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
      return std::string("packets_in(t)-packets_in(t-1)");
    case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
      return std::string("packets_out(t)-packets_out(t-1)");
    case BYTES_T_IN_MINUS_BYTES_T_1_IN:
      return std::string("octets_in(t)-octets_in(t-1)");
    case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
      return std::string("octets_out(t)-octets_out(t-1)");
    default:
      std::cerr << "Error! Unknown type of Metric in getMetricName().\n"
                << "Exiting.";
      exit(0);
  }
}

void Stat::stat_test_wmw (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new) {

  double p;

  if (output_verbosity >= 5) {
    outfile << "Wilcoxon-Mann-Whitney test details:\n";
    p = wmw_test(sample_old, sample_new, two_sided, outfile);
  }
  else {
    std::ofstream dump("/dev/null");
    p = wmw_test(sample_old, sample_new, two_sided, dump);
  }

  if (output_verbosity >= 2) {
    outfile << "Wilcoxon-Mann-Whitney test returned:\n"
	    << "  p-value: " << p << std::endl;
    outfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	  || last_wmw_test_was_an_attack == false) {
	outfile << "    ATTACK! ATTACK! ATTACK!\n"
		<< "    Wilcoxon-Mann-Whitney test says we're under attack!\n"
		<< "    ALARM! ALARM! Women und children first!" << std::endl;
	std::cout << "  ATTACK! ATTACK! ATTACK!\n"
		  << "  Wilcoxon-Mann-Whitney test says we're under attack!\n"
		  << "  ALARM! ALARM! Women und children first!" << std::endl;
#ifdef IDMEF_SUPPORT_ENABLED
	idmefMessage.setAnalyzerAttr("", "", "wmw-test", "");
	sendIdmefMessage("DDoS", idmefMessage);
	idmefMessage = getNewIdmefMessage();
#endif
      }
      last_wmw_test_was_an_attack = true;
    }
    else {
      last_wmw_test_was_an_attack = false;
    }
  }

  if (output_verbosity == 1) {
    outfile << "wmw: " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	  || last_wmw_test_was_an_attack == false) {
	outfile << "attack to significance level "
		<< significance_level << "!" << std::endl;
	std::cout << "attack to significance level "
		  << significance_level << "!" << std::endl;
      }
      last_wmw_test_was_an_attack = true;
    }
    else {
      last_wmw_test_was_an_attack = false;
    }
  }

  return;
}


void Stat::stat_test_ks (std::list<int64_t> & sample_old,
			 std::list<int64_t> & sample_new) {

  double p;

  if (output_verbosity>=5) {
    outfile << "Kolmogorov-Smirnov test details:\n";
    p = ks_test(sample_old, sample_new, outfile);
  }
  else {
    std::ofstream dump ("/dev/null");
    p = ks_test(sample_old, sample_new, dump);
  }

  if (output_verbosity >= 2) {
    outfile << "Kolmogorov-Smirnov test returned:\n"
	    << "  p-value: " << p << std::endl;
    outfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	  || last_ks_test_was_an_attack == false) {
	outfile << "    ATTACK! ATTACK! ATTACK!\n"
		<< "    Kolmogorov-Smirnov test says we're under attack!\n"
		<< "    ALARM! ALARM! Women und children first!" << std::endl;
	std::cout << "  ATTACK! ATTACK! ATTACK!\n"
		  << "  Kolmogorov-Smirnov test says we're under attack!\n"
		  << "  ALARM! ALARM! Women und children first!" << std::endl;
#ifdef IDMEF_SUPPORT_ENABLED
	idmefMessage.setAnalyzerAttr("", "", "ks-test", "");
	sendIdmefMessage("DDoS", idmefMessage);
	idmefMessage = getNewIdmefMessage();
#endif
      }
      last_ks_test_was_an_attack = true;
    }
    else {
      last_ks_test_was_an_attack = false;
    }
  }

  if (output_verbosity == 1) {
    outfile << "ks : " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	  || last_ks_test_was_an_attack == false) {
	outfile << "attack to significance level "
		<< significance_level << "!" << std::endl;
	std::cout << "attack to significance level "
		  << significance_level << "!" << std::endl;
      }
      last_ks_test_was_an_attack = true;
    }
    else {
      last_ks_test_was_an_attack = false;
    }
  }

  return;
}


void Stat::stat_test_pcs (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new) {

  double p;

  if (output_verbosity>=5) {
    outfile << "Pearson chi-square test details:\n";
    p = pcs_test(sample_old, sample_new, outfile);
  }
  else {
    std::ofstream dump ("/dev/null");
    p = pcs_test(sample_old, sample_new, dump);
  }

  if (output_verbosity >= 2) {
    outfile << "Pearson chi-square test returned:\n"
	    << "  p-value: " << p << std::endl;
    outfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	  || last_pcs_test_was_an_attack == false) {
	outfile << "    ATTACK! ATTACK! ATTACK!\n"
		<< "    Pearson chi-square test says we're under attack!\n"
		<< "    ALARM! ALARM! Women und children first!" << std::endl;
	std::cout << "  ATTACK! ATTACK! ATTACK!\n"
		  << "  Pearson chi-square test says we're under attack!\n"
		  << "  ALARM! ALARM! Women und children first!" << std::endl;
#ifdef IDMEF_SUPPORT_ENABLED
	idmefMessage.setAnalyzerAttr("", "", "pcs-test", "");
	sendIdmefMessage("DDoS", idmefMessage);
	idmefMessage = getNewIdmefMessage();
#endif
      }
      last_pcs_test_was_an_attack = true;
    }
    else {
      last_pcs_test_was_an_attack = false;
    }
  }

  if (output_verbosity == 1) {
    outfile << "pcs: " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	  || last_pcs_test_was_an_attack == false) {
	outfile << "attack to significance level "
		<< significance_level << "!" << std::endl;
	std::cout << "attack to significance level "
		  << significance_level << "!" << std::endl;
      }
      last_pcs_test_was_an_attack = true;
    }
    else {
      last_pcs_test_was_an_attack = false;
    }
  }

  return;
}

void Stat::init_values(Values & m) {

  m.packets_in = 0;
  m.packets_out = 0;
  m.bytes_in = 0;
  m.bytes_out = 0;
  m.records_in = 0;
  m.records_out = 0;
  m.bytes_per_packet_in = 0;
  m.bytes_per_packet_out = 0;
  m.p_out_minus_p_in = 0;
  m.b_out_minus_b_in = 0;
  m.pt_minus_pt1_in = 0;
  m.pt_minus_pt1_out = 0;
  m.bt_minus_bt1_in = 0;
  m.bt_minus_bt1_out = 0;

  return;
}


void Stat::sigTerm(int signum)
{
	stop();
}

void Stat::sigInt(int signum)
{
	stop();
}
