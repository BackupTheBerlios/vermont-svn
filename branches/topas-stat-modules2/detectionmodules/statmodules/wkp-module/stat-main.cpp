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
Mehr Protokolle
--------------------------------------
Bisher: Nur TCP, UDP, ICMP und RAW
Besser: Alle möglichen
Stand:

TODO(2)
Port-Unterscheidung (Source/Dest)
---------------------------------
Bisher: Ports werden aggregiert, d. h. Informationen darüber, ob der Port ein Quell- oder Zielport war, gehen verloren.
Besser: Diese Informationen erhalten?
Stand:

TODO(4)
Initialisierung sparen, falls bestimmte Tests inaktiv
------------------------------------------------------
- Wenn kein WKP-Test aktiviert wird, weitere Initialisierung ersparen
- Wenn Cusum-Test nicht aktiviert ist, weitere Initialisierung sparen
- Wenn generell kein Test aktiviert ist, meckern!

TODO(5)
CUSUM
-----------------------------------------------------
Stand: Initialisierung überlegen (welche Parameter?)
*/


#include<signal.h>

#include "stat-main.h"
#include "wmw-test.h"
#include "ks-test.h"
#include "pcs-test.h"
#include "cusum-test.h"

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

  ip_monitoring = false;
  port_monitoring = false; ports_relevant = false;
  protocol_monitoring = false;

  test_counter = 0;
  init(configfile);
}


Stat::~Stat() {

  outfile.close();

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

  if (!config->nodeExists("preferences")) {
    std::cerr
      << "ERROR: No preferences node defined in XML config file!\n"
      << "Define one, fill it with some parameters and restart.\nExiting.\n";
    exit(0);
  }

  if (!config->nodeExists("cusum-test-params")) {
    std::cerr
      << "ERROR: No cusum-test-params node defined in XML config file!\n"
      << "Define one, fill it with some parameters and restart.\nExiting.\n";
    exit(0);
  }

  if (!config->nodeExists("wkp-test-params")) {
    std::cerr
      << "ERROR: No wkp-test-params node defined in XML config file!\n"
      << "Define one, fill it with some parameters and restart.\nExiting.\n";
    exit(0);
  }

  // ATTENTION:
  // the order of the following operations is important,
  // as some of these functions use Stat members initialized
  // by the preceding functions; so beware if you change the order

  config->enterNode("preferences");

  // extracting output file's name
  init_output_file(config);

  // extracting source id's to accept
  init_accepted_source_ids(config);

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
  init_alarm_time(config);

  // extracting warning verbosity
  init_warning_verbosity(config);

  // extracting output verbosity
  init_output_verbosity(config);

  // extracting the key of the endpoints
  init_endpoint_key(config);

  // extracting monitored values
  init_monitored_values(config);

  // extracting noise reduction preferences
  init_noise_thresholds(config);

  // extract the maximum size of the endpoint list
  // i. e. how many endpoints can be monitored
  init_endpointlist_maxsize(config);

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

  //TODO(5)
  config->enterNode("cusum-test-params");

  // extracting cusum parameters
  init_cusum_test(config);

  config->leaveNode();

  config->enterNode("wkp-test-params");

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
  if(!config->nodeExists("output_file")) {
    std::cerr
      << "WARNING: No output_file parameter defined in XML config file!\n"
      << "Default outputfile used (wkp_output.txt).\n";
    outfile.open("wkp_output.txt");
  }
  else if (!(config->getValue("output_file")).empty())
    outfile.open(config->getValue("output_file").c_str());
  else {
    std::cerr
      << "WARNING: No value for output_file parameter defined in XML config file!\n"
      << "Default output_file used (wkp_output.txt).\n";
    outfile.open("wkp_output.txt");
  }

  if (!outfile) {
      std::cerr << "ERROR: could not open output file!\n"
        << "Check if you have enough rights to create or write to it.\n"
        << "Exiting.\n";
      exit(0);
  }

  return;
}

void Stat::init_accepted_source_ids(XMLConfObj * config) {

  if (!config->nodeExists("accepted_source_ids")) {
    std::stringstream Warning;
    Warning
      << "WARNING: No accepted_source_ids parameter defined in XML config file!\n"
      << "All source ids will be accepted.\n";
    std::cerr << Warning.str();
    outfile << Warning.str() << std::flush;
  }
  else if (!(config->getValue("accepted_source_ids")).empty()) {

    std::string str = config->getValue("accepted_source_ids");

    if ( 0 == strcasecmp(str.c_str(), "all") )
      return;
    else {
      unsigned startpos = 0, endpos = 0;
      do {
          endpos = str.find(',', endpos);
          if (endpos == std::string::npos) {
              subscribeSourceId(atoi((str.substr(startpos)).c_str()));
              break;
          }
          subscribeSourceId(atoi((str.substr(startpos, endpos-startpos)).c_str()));
          endpos++;
      }
      while(true);
    }
  }
  else {
    std::stringstream Warning;
    Warning
      << "WARNING: No value for accepted_source_ids parameter defined in XML config file!\n"
      << "All source ids will be accepted.\n";
    std::cerr << Warning.str();
    outfile << Warning.str() << std::flush;
  }

  return;
}

void Stat::init_alarm_time(XMLConfObj * config) {

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
  if(!config->nodeExists("alarm_time")) {
    std::stringstream Warning;
    Warning
      << "WARNING: No alarm_time parameter defined in XML config file!\n"
      << DEFAULT_alarm_time << " assumed.\n";
    std::cerr << Warning.str();
    outfile << Warning.str() << std::flush;
    setAlarmTime(DEFAULT_alarm_time);
  }
  else if (!(config->getValue("alarm_time")).empty())
    setAlarmTime( atoi(config->getValue("alarm_time").c_str()) );
  else {
    std::stringstream Warning;
    Warning
      << "Warning: No value for alarm_time parameter defined in XML config file!\n"
      << DEFAULT_alarm_time << " assumed.\n";
    std::cerr << Warning.str();
    outfile << Warning.str() << std::flush;
    setAlarmTime(DEFAULT_alarm_time);
  }

  return;
}

void Stat::init_warning_verbosity(XMLConfObj * config) {

  std::stringstream Error, Warning, Default, Usage;
  Error
    << "ERROR: warning_verbosity parameter "
	  << "defined in XML config file should be 0 or 1.\n"
    << "Please define it that way and restart.\n";
  Warning
    << "WARNING: No warning_verbosity parameter defined in XML config file!\n"
    << DEFAULT_warning_verbosity << "\" assumed.\n";
  Default
    << "WARNING: No value for warning_verbosity parameter defined "
	  << "in XML config file! \""
	  << DEFAULT_warning_verbosity << "\" assumed.\n";
  Usage
    << "O: warnings are sent to stderr\n"
	  << "1: warnings are sent to stderr and output file\n";

  // extracting warning verbosity
  if(!config->nodeExists("warning_verbosity")) {
    std::cerr << Warning.str() << Usage.str();
    warning_verbosity = DEFAULT_warning_verbosity;
  }
  else if (!(config->getValue("warning_verbosity")).empty()) {
    if ( 0 == atoi( config->getValue("warning_verbosity").c_str() )
    ||   1 == atoi( config->getValue("warning_verbosity").c_str() ) )
      warning_verbosity = atoi( config->getValue("warning_verbosity").c_str() );
    else {
      std::cerr << Error.str() << Usage.str() << "Exiting.\n";
      outfile << Error.str() << Usage.str() << "Exiting.\n" << std::flush;
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

  std::stringstream Error, Warning, Default, Usage;
  Error
    << "ERROR: output_verbosity parameter defined in XML config file "
	  << "should be between 0 and 5.\n"
    << "Please define it that way and restart.\n";
  Warning
    << "WARNING: No output_verbosity parameter defined "
    << "in XML config file! \""
    << DEFAULT_output_verbosity << "\" assumed.\n";
  Default
    << "WARNING: No value for output_verbosity parameter defined "
	  << "in XML config file! \""
	  << DEFAULT_output_verbosity << "\" assumed.\n";
  Usage
    << "O: no output generated\n"
	  << "1: only p-values and attacks are recorded\n"
	  << "2: same as 1, plus some cosmetics\n"
	  << "3: same as 2, plus learning phases, updates and empty records events\n"
	  << "4: same as 3, plus sample printing\n"
	  << "5: same as 4, plus all details from statistical tests\n";

  // extracting output verbosity
  if(!config->nodeExists("output_verbosity")) {
    std::cerr << Warning.str() << Usage.str();
    if (warning_verbosity==1)
      outfile << Warning.str() << Usage.str() << std::flush;
    output_verbosity = DEFAULT_output_verbosity;
  }
  else if (!(config->getValue("output_verbosity")).empty()) {
    if ( 0 <= atoi( config->getValue("output_verbosity").c_str() )
      && 5 >= atoi( config->getValue("output_verbosity").c_str() ) )
      output_verbosity = atoi( config->getValue("output_verbosity").c_str() );
    else {
      std::cerr << Error.str() << Usage.str() << "Exiting.\n";
      if (warning_verbosity==1)
        outfile << Error.str() << Usage.str() << "Exiting." << std::endl << std::flush;
      exit(0);
    }
  }
  else {
    std::cerr << Default.str() << Usage.str();
    if (warning_verbosity==1)
      outfile << Default.str() << Usage.str() << std::flush;
    output_verbosity = DEFAULT_output_verbosity;
  }

  return;
}

void Stat::init_endpoint_key(XMLConfObj * config) {

  std::stringstream Warning, Error, Default;
  Warning
    << "WARNING: No endpoint_key parameter in XML config file!\n"
    << "endpoint_key will be ip + port + protocol!";
  Error
    << "ERROR: Unknown value defined for endpoint_key parameter in XML config file!\n"
    << "Value should be either port, ip, protocol or any combination of them (seperated via spaces)!\n";
  Default
    << "WARNING: No value for endpoint_key parameter defined in XML config file!\n"
    << "endpoint_key will be ip + port + protocol!";

  // extracting key of endpoints to monitor
  if (!config->nodeExists("endpoint_key")) {
    std::cerr << Warning.str() << "\n";
    if (warning_verbosity==1)
      outfile << Warning.str() << std::endl << std::flush;
    ip_monitoring = true;
    port_monitoring = true;
    protocol_monitoring = true;
  }
  else if (!(config->getValue("endpoint_key")).empty()) {

    std::string Keys = config->getValue("endpoint_key");

    if ( 0 == strcasecmp(Keys.c_str(), "all") ) {
      ip_monitoring = true;
      port_monitoring = true;
      protocol_monitoring = true;
      return;
    }

    std::istringstream KeyStream (Keys);
    std::string key;

    while (KeyStream >> key) {
      if ( 0 == strcasecmp(key.c_str(), "ip") )
        ip_monitoring = true;
      else if ( 0 == strcasecmp(key.c_str(), "port") )
        port_monitoring = true;
      else if ( 0 == strcasecmp(key.c_str(), "protocol") )
        protocol_monitoring = true;
      else {
        std::cerr << Error.str() << "Exiting.\n";
        if (warning_verbosity==1)
          outfile << Error.str() << "Exiting." << std::endl << std::flush;
        exit(0);
      }
    }
  }
  else {
    std::cerr << Default.str() << "\n";
    if (warning_verbosity==1)
      outfile << Default.str() << std::endl << std::flush;
    ip_monitoring = true;
    port_monitoring = true;
    protocol_monitoring = true;
  }

  return;
}

// extracting monitored values to vector<Metric>;
// the values are stored there as constants
// defined in enum Metric in stat_main.h
void Stat::init_monitored_values(XMLConfObj * config) {

  std::stringstream Error1, Error2, Error3, Usage;
  Error1
    << "ERROR: No monitored_values parameter in XML config file!\n"
    << "Please define one and restart.\n";
  Error2
    << "ERROR: No value parameter(s) defined for monitored_values in XML config file!\n"
    << "Please define at least one and restart.\n";
  Error3
    << "ERROR: Unknown value parameter(s) defined for monitored_values in XML config file!\n"
    << "Please provide only valid <value>-parameters.\n";
  Usage
    << "Use for each <value>-Tag one of the following metrics:\n"
    << "packets_in, packets_out, bytes_in, bytes_out, records_in, records_out, "
    << "bytes_in/packet_in, bytes_out/packet_out, packets_out-packets_in, "
    << "bytes_out-bytes_in, packets_in(t)-packets_in(t-1), "
    << "packets_out(t)-packets_out(t-1), bytes_in(t)-bytes_in(t-1) or "
    << "bytes_out(t)-bytes_out(t-1).\n";

  if (!config->nodeExists("monitored_values")) {
    std::cerr << Error1.str() << Usage.str() << "Exiting.\n";
    if (warning_verbosity==1)
      outfile << Error1.str() << Usage.str() << "Exiting." << std::endl << std::flush;
    exit(0);
  }

  config->enterNode("monitored_values");

  std::vector<std::string> tmp_monitored_data;

  /* get all monitored values */
  if (config->nodeExists("value")) {
    tmp_monitored_data.push_back(config->getValue("value"));
    while (config->nextNodeExists())
      tmp_monitored_data.push_back(config->getNextValue());
  }
  else {
    std::cerr << Error2.str() << Usage.str() << "Exiting.\n";
    if (warning_verbosity==1)
      outfile << Error2.str() << Usage.str() << "Exiting." << std::endl << std::flush;
    exit(0);
  }

  config->leaveNode();

  // extract the values from tmp_monitored_data (string)
  // to monitored_values (vector<enum>)
  std::vector<std::string>::iterator it = tmp_monitored_data.begin();
  while ( it != tmp_monitored_data.end() ) {
    if ( 0 == strcasecmp("packets_in", (*it).c_str()) )
      monitored_values.push_back(PACKETS_IN);
    else if ( 0 == strcasecmp("packets_out", (*it).c_str()) )
      monitored_values.push_back(PACKETS_OUT);
    else if ( 0 == strcasecmp("bytes_in", (*it).c_str())
           || 0 == strcasecmp("octets_in",(*it).c_str()) )
      monitored_values.push_back(BYTES_IN);
    else if ( 0 == strcasecmp("bytes_out", (*it).c_str())
           || 0 == strcasecmp("octets_out",(*it).c_str()) )
      monitored_values.push_back(BYTES_OUT);
    else if ( 0 == strcasecmp("records_in",(*it).c_str()) )
      monitored_values.push_back(RECORDS_IN);
    else if ( 0 == strcasecmp("records_out",(*it).c_str()) )
      monitored_values.push_back(RECORDS_OUT);
    else if ( 0 == strcasecmp("octets_in/packet_in",(*it).c_str())
           || 0 == strcasecmp("bytes_in/packet_in",(*it).c_str()) )
      monitored_values.push_back(BYTES_IN_PER_PACKET_IN);
    else if ( 0 == strcasecmp("octets_out/packet_out",(*it).c_str())
           || 0 == strcasecmp("bytes_out/packet_out",(*it).c_str()) )
      monitored_values.push_back(BYTES_OUT_PER_PACKET_OUT);
    else if ( 0 == strcasecmp("packets_out-packets_in",(*it).c_str()) )
      monitored_values.push_back(PACKETS_OUT_MINUS_PACKETS_IN);
    else if ( 0 == strcasecmp("octets_out-octets_in",(*it).c_str())
           || 0 == strcasecmp("bytes_out-bytes_in",(*it).c_str()) )
      monitored_values.push_back(BYTES_OUT_MINUS_BYTES_IN);
    else if ( 0 == strcasecmp("packets_in(t)-packets_in(t-1)",(*it).c_str()) )
      monitored_values.push_back(PACKETS_T_IN_MINUS_PACKETS_T_1_IN);
    else if ( 0 == strcasecmp("packets_out(t)-packets_out(t-1)",(*it).c_str()) )
      monitored_values.push_back(PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT);
    else if ( 0 == strcasecmp("octets_in(t)-octets_in(t-1)",(*it).c_str())
           || 0 == strcasecmp("bytes_in(t)-bytes_in(t-1)",(*it).c_str()) )
      monitored_values.push_back(BYTES_T_IN_MINUS_BYTES_T_1_IN);
    else if ( 0 == strcasecmp("octets_out(t)-octets_out(t-1)",(*it).c_str())
           || 0 == strcasecmp("bytes_out(t)-bytes_out(t-1)",(*it).c_str()) )
      monitored_values.push_back(BYTES_T_OUT_MINUS_BYTES_T_1_OUT);
    else {
        std::cerr << Error3.str() << Usage.str() << "Exiting.\n";
        if (warning_verbosity==1)
          outfile << Error3.str() << Usage.str() << "Exiting." << std::endl << std::flush;
        exit(0);
      }
    it++;
  }

  // just in case the user provided multiple same values
  // (after these lines, monitored_values contains the Metrics
  // in the correct order)
  sort(monitored_values.begin(),monitored_values.end());
  std::vector<Metric>::iterator new_end = unique(monitored_values.begin(),monitored_values.end());
  std::vector<Metric> tmp(monitored_values.begin(), new_end);
  monitored_values = tmp;

  // print out, in which order the values will be stored
  // in the Samples-Lists (for better understanding the output
  // of test() etc.
  std::stringstream Information;
  Information
    << "INFORMATION: Values in the lists sample_old and sample_new will be "
    << "stored in the following order:\n";
  std::vector<Metric>::iterator val = monitored_values.begin();
  Information << "(";
  // we got at least one monitored value, so we dont need to check
  // before dereferencing "val" the first time
  Information << getMetricName(*val); val++;
  while (val != monitored_values.end() ) {
    Information << ", " << getMetricName(*val);
    val++;
  }
  Information << ")\n";
  std::cerr << Information.str();
    if (warning_verbosity==1)
      outfile << Information.str() << std::flush;


  // subscribing to the needed IPFIX_TYPEID-fields

  // to prevent multiple subscription
  bool packetsSubscribed = false;
  bool bytesSubscribed = false;

  for (int i = 0; i != monitored_values.size(); i++) {
    if ( (monitored_values.at(i) == PACKETS_IN
      || monitored_values.at(i) == PACKETS_OUT
      || monitored_values.at(i) == PACKETS_OUT_MINUS_PACKETS_IN
      || monitored_values.at(i) == PACKETS_T_IN_MINUS_PACKETS_T_1_IN
      || monitored_values.at(i) == PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT)
      && packetsSubscribed == false) {
      subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
      packetsSubscribed = true;
    }

    if ( (monitored_values.at(i) == BYTES_IN
      || monitored_values.at(i) == BYTES_OUT
      || monitored_values.at(i) == BYTES_OUT_MINUS_BYTES_IN
      || monitored_values.at(i) == BYTES_T_IN_MINUS_BYTES_T_1_IN
      || monitored_values.at(i) == BYTES_T_OUT_MINUS_BYTES_T_1_OUT)
      && bytesSubscribed == false ) {
      subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
      bytesSubscribed = true;
    }

    if ( (monitored_values.at(i) == BYTES_IN_PER_PACKET_IN
       || monitored_values.at(i) == BYTES_OUT_PER_PACKET_OUT)
      && packetsSubscribed == false
      && bytesSubscribed == false) {
      subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
      subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
    }
  }

  return;
}

void Stat::init_noise_thresholds(XMLConfObj * config) {

  // extracting noise threshold for packets
  if(!config->nodeExists("noise_threshold_packets")) {
    std::stringstream Default1;
    Default1
      << "WARNING: No noise_threshold_packets parameter defined in XML config file!\n"
      << "\"" << DEFAULT_noise_threshold_packets << "\" assumed.\n";
    std::cerr << Default1.str();
    if (warning_verbosity==1)
      outfile << Default1.str() << std::flush;
    noise_threshold_packets = DEFAULT_noise_threshold_packets;
  }
  else if ( !(config->getValue("noise_threshold_packets").empty()) )
    noise_threshold_packets = atoi(config->getValue("noise_threshold_packets").c_str());
  else {
    std::stringstream Default1;
    Default1
      << "WARNING: No value for noise_threshold_packets parameter defined in XML config file!\n"
      << "\"" << DEFAULT_noise_threshold_packets << "\" assumed.\n";
    std::cerr << Default1.str();
    if (warning_verbosity==1)
      outfile << Default1.str() << std::flush;
    noise_threshold_packets = DEFAULT_noise_threshold_packets;
  }

    // extracting noise threshold for bytes
  if(!config->nodeExists("noise_threshold_bytes")) {
    std::stringstream Default2;
    Default2
      << "WARNING: No noise_threshold_bytes parameter defined in XML config file!\n"
      << "\"" << DEFAULT_noise_threshold_bytes << "\" assumed.\n";
    std::cerr << Default2.str();
    if (warning_verbosity==1)
      outfile << Default2.str() << std::flush;
    noise_threshold_bytes = DEFAULT_noise_threshold_bytes;
  }
  else if ( !(config->getValue("noise_threshold_bytes").empty()) )
    noise_threshold_bytes = atoi(config->getValue("noise_threshold_bytes").c_str());
  else {
    std::stringstream Default2;
    Default2
      << "WARNING: No value for noise_threshold_bytes parameter defined in XML config file!\n"
      << "\"" << DEFAULT_noise_threshold_bytes << "\" assumed.\n";
    std::cerr << Default2.str();
    if (warning_verbosity==1)
      outfile << Default2.str() << std::flush;
    noise_threshold_bytes = DEFAULT_noise_threshold_bytes;
  }

  return;
}

void Stat::init_endpointlist_maxsize(XMLConfObj * config) {

  std::stringstream Warning, Warning1;
  Warning
    << "WARNING: No endpointlist_maxsize parameter defined in XML config file!\n"
    << DEFAULT_endpointlist_maxsize << " assumed.\n";
  Warning1
    << "WARNING: No value for endpointlist_maxsize parameter defined in XML config file!\n"
    << DEFAULT_endpointlist_maxsize << " assumed.\n";

  if (!config->nodeExists("endpointlist_maxsize")) {
    std::cerr << Warning.str();
    if (warning_verbosity==1)
      outfile << Warning.str() << std::flush;
    endpointlist_maxsize = DEFAULT_endpointlist_maxsize;
  }
  else if (!(config->getValue("endpointlist_maxsize")).empty())
    endpointlist_maxsize = atoi( config->getValue("endpointlist_maxsize").c_str() );
  else {
    std::cerr << Warning1.str();
    if (warning_verbosity==1)
      outfile << Warning1.str() << std::flush;
    endpointlist_maxsize = DEFAULT_endpointlist_maxsize;
  }

  StatStore::setEndPointListMaxSize() = endpointlist_maxsize;

  return;
}

//TODO(1)
// What about other protocols like sctp?
void Stat::init_protocols(XMLConfObj * config) {

  // shall protocols be monitored at all?
  // (that means, they were part of the endpoint_key-parameter)
  if (protocol_monitoring == true) {

    std::stringstream Default, Warning, Usage;
    Default
      << "WARNING: No protocols parameter defined in XML config file!\n"
      << "All protocols will be monitored (ICMP, TCP, UDP, RAW).\n";
    Warning
      << "WARNING: No value for protocols parameter defined in XML config file!\n"
      << "All protocols will be monitored (ICMP, TCP, UDP, RAW).\n";
    Usage
      << "Please use ICMP, TCP, UDP or RAW (or "
      << IPFIX_protocolIdentifier_ICMP << ", "
      << IPFIX_protocolIdentifier_TCP  << ", "
      << IPFIX_protocolIdentifier_UDP << " or "
      << IPFIX_protocolIdentifier_RAW  << ").\n";

    // extracting monitored protocols
    if (!config->nodeExists("protocols")) {
      std::cerr << Default.str();
      if (warning_verbosity==1)
        outfile << Default.str() << std::flush;
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_ICMP);
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_TCP);
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_UDP);
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_RAW);
      ports_relevant = true;
    }
    else if (!(config->getValue("protocols")).empty()) {

      std::string Proto = config->getValue("protocols");

      if ( 0 == strcasecmp(Proto.c_str(), "all") ) {
        StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_ICMP);
        StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_TCP);
        StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_UDP);
        StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_RAW);
        ports_relevant = true;
      }
      else {
        std::istringstream ProtoStream (Proto);

        std::string protocol;
        while (ProtoStream >> protocol) {

          if ( strcasecmp(protocol.c_str(),"ICMP") == 0
          || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_ICMP )
            StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_ICMP);
          else if ( strcasecmp(protocol.c_str(),"TCP") == 0
          || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_TCP ) {
            StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_TCP);
            ports_relevant = true;
          }
          else if ( strcasecmp(protocol.c_str(),"UDP") == 0
          || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_UDP ) {
            StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_UDP);
            ports_relevant = true;
          }
          else if ( strcasecmp(protocol.c_str(),"RAW") == 0
          || atoi(protocol.c_str()) == IPFIX_protocolIdentifier_RAW )
            StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_RAW);
          else {
            std::cerr << "ERROR: An unknown value (" << protocol
            << ") for the protocol parameter was defined in XML config file!\n"
            << Usage.str() << "Exiting.\n";
            if (warning_verbosity==1)
              outfile << "ERROR: An unknown value (" << protocol
                << ") for the protocol parameter was defined in XML config file!\n"
                << Usage.str() << "Exiting." << std::endl << std::flush;
            exit(0);
          }
        }
      }
    }
    else {
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        outfile << Warning.str() << std::flush;
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_ICMP);
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_TCP);
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_UDP);
      StatStore::AddProtocolToMonitoredProtocols(IPFIX_protocolIdentifier_RAW);
      ports_relevant = true;
    }

    subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
  }
  // dont consider protocols;
  // but we may be interested in ports, no matter which protocol is used,
  // so we set the flag to true (so we dont bother port_monitoring)
  else
    ports_relevant = true;

  return;
}

void Stat::init_netmask(XMLConfObj * config) {

  // shall ip-addresses be monitored at all?
  // (that means, they were part of the endpoint_key-parameter)
  // if not -> no need for netmask
  if (ip_monitoring == true) {

    std::stringstream Default, Warning, Error, Usage;
    Default
      << "WARNING: No value for netmask parameter defined in XML config file!\n"
      << "\"32\" assumed.\n";
    Warning
      << "WARNING: No netmask parameter defined in XML config file!\n"
      << "\"32\" assumed.\n";
    Error
      << "ERROR: Netmask parameter was provided in an unknown format in XML config file!\n";
    Usage
      << "Use xxx.yyy.zzz.ttt, hexadecimal or an int between 0 and 32.\n";

    if (config->nodeExists("netmask")) {

      const char * netmask;
      unsigned int mask[4];

      if (!(config->getValue("netmask")).empty()) {
        netmask = config->getValue("netmask").c_str();
      }
      else {
        std::cerr << Default.str();
        if (warning_verbosity==1)
          outfile << Default.str() << std::flush;
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
        std::cerr << Error.str() << Usage.str() << "Exiting.\n";
        if (warning_verbosity==1)
          outfile << Error.str() << Usage.str() << "Exiting." << std::endl << std::flush;
        exit(0);
      }
      // if everything is OK:
      StatStore::InitialiseSubnetMask (mask[0], mask[1], mask[2], mask[3]);
    }
    else {
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        outfile << Warning.str() << std::flush;
      StatStore::InitialiseSubnetMask (0xFF,0xFF,0xFF,0xFF);
    }
  }

  return;
}

void Stat::init_ports(XMLConfObj * config) {

  // shall ports be monitored at all?
  // (that means, they are part of the endpoint_key-parameter);
  // if TCP or UDP are monitored or we arent interested
  // in protocols: subscribe to IPFIX port information (ports relevant)
  if (port_monitoring == true && ports_relevant == true) {

    std::stringstream Warning1, Warning2, Default;
    Warning1
      << "WARNING: No ports parameter was provided in XML config file!\n";
    Warning2
      << "WARNING: No value for ports parameter was provided in XML config file!\n";
    Default
      << "Every port will be monitored.\n";

    // extracting port numbers to monitor
    if (config->nodeExists("ports")) {

      if (!(config->getValue("ports")).empty()) {

        std::string Ports = config->getValue("ports");

        if ( 0 == strcasecmp(Ports.c_str(), "all") )
          StatStore::setMonitorAllPorts() = true;
        else {
          std::istringstream PortsStream (Ports);
          unsigned int port;
          while (PortsStream >> port)
            StatStore::AddPortToMonitoredPorts(port);
          StatStore::setMonitorAllPorts() = false;
        }
      }
      else {
        std::cerr << Warning2.str() << Default.str();
        if (warning_verbosity==1)
          outfile << Warning2.str() << Default.str() << std::flush;
        StatStore::setMonitorAllPorts() = true;
      }
    }
    else {
      std::cerr << Warning1.str() << Default.str();
      if (warning_verbosity==1)
        outfile << Warning1.str() << Default.str() << std::flush;
      StatStore::setMonitorAllPorts() = true;
    }
    subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
    subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
  }

  return;
}

void Stat::init_ip_addresses(XMLConfObj * config) {

  // shall ip-addresses be monitored at all?
  // (that means, they were part of the endpoint_key-parameter)
  if (ip_monitoring == true) {

    std::stringstream Warning, Warning1;
    Warning
      << "WARNING: No ip_addresses_to_monitor parameter defined in XML config file!\n"
      << "I suppose you want me to monitor everything; I'll try.\n";
    Warning1
      << "WARNING: No value for ip_addresses_to_monitor parameter defined in XML config file!\n"
      << "I suppose you want me to monitor everything; I'll try.\n";

    // the following section extracts either the IP addresses to monitor
    // or the maximal number of IPs to monitor in case the user doesn't
    // give IP addresses to monitor
    bool gotIpfile = false;

    if (!config->nodeExists("ip_addresses_to_monitor")) {
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        outfile << Warning.str() << std::flush;
    }
    else if (!(config->getValue("ip_addresses_to_monitor")).empty()) {
      ipfile = config->getValue("ip_addresses_to_monitor");
      if ( 0 != strcasecmp(ipfile.c_str(), "all") )
        gotIpfile = true;
    }
    else {
      std::cerr << Warning1.str();
      if (warning_verbosity==1)
        outfile << Warning1.str() << std::flush;
    }

    // and the following section uses the extracted ipfile
    // information to initialise some static members of the StatStore class
    if (gotIpfile == true) {

      std::stringstream Error;
      Error
        << "ERROR: I could not open IP-file " << ipfile << "!\n"
        << "Please check that the file exists, "
        << "and that you have enough rights to read it.\n"
        << "Exiting.\n";

      std::ifstream ipstream(ipfile.c_str());

      if (!ipstream) {
        std::cerr << Error.str();
        if (warning_verbosity==1)
          outfile << Error.str() << std::flush;
        exit(0);
      }
      else {
        std::vector<IpAddress> IpVector;
        unsigned int ip[4];
        char dot;

        // save read IPs in a vector; mask them at the same time
        while (ipstream >> ip[0] >> dot >> ip[1] >> dot >> ip[2] >> dot >> ip[3])
          IpVector.push_back(IpAddress(ip[0],ip[1],ip[2],ip[3]).mask(StatStore::getSubnetMask()) );

        // just in case the user provided a file with multiples IPs,
        // or the mask function made multiple IPs to appear:
        sort(IpVector.begin(),IpVector.end());
        std::vector<IpAddress>::iterator new_end = unique(IpVector.begin(),IpVector.end());
        std::vector<IpAddress> IpVectorBis (IpVector.begin(), new_end);

        // finally, add these IPs to monitor to static member
        // StatStore::MonitoredIpAddresses
        StatStore::AddIpToMonitoredIp (IpVectorBis);
        // no need to monitor every IP:
        StatStore::setMonitorEveryIp() = false;
      }

    }
    else
      StatStore::setMonitorEveryIp() = true;

    subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
    subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
  }

  return;
}

void Stat::init_stat_test_freq(XMLConfObj * config) {

  // extracting statistical test frequency
  if (!config->nodeExists("stat_test_frequency")) {
    std::stringstream Default;
    Default
      << "WARNING: No stat_test_frequency parameter "
      << "defined in XML config file!\n\""
      << DEFAULT_stat_test_frequency << "\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    stat_test_frequency = DEFAULT_stat_test_frequency;
  }
  else if (!(config->getValue("stat_test_frequency")).empty()) {
    stat_test_frequency =
      atoi( config->getValue("stat_test_frequency").c_str() );
  }
  else {
    std::stringstream Default;
    Default
      << "WARNING: No value for stat_test_frequency parameter "
	    << "defined in XML config file!\n\""
	    << DEFAULT_stat_test_frequency << "\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    stat_test_frequency = DEFAULT_stat_test_frequency;
  }

  return;
}

void Stat::init_report_only_first_attack(XMLConfObj * config) {

  // extracting report_only_first_attack preference
  if (!config->nodeExists("report_only_first_attack")) {
    std::stringstream Default;
    Default
      << "WARNING: No report_only_first_attack parameter "
      << "defined in XML config file!\n"
      << "\"true\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    report_only_first_attack = true;
  }
  else if (!(config->getValue("report_only_first_attack")).empty()) {
    report_only_first_attack =
      ( 0 == strcasecmp("true", config->getValue("report_only_first_attack").c_str()) ) ? true:false;
  }
  else {
    std::stringstream Default;
    Default
      << "WARNING: No value for report_only_first_attack parameter "
	    << "defined in XML config file!\n"
      << "\"true\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    report_only_first_attack = true;
  }

  return;
}

void Stat::init_pause_update_when_attack(XMLConfObj * config) {

  // extracting pause_update_when_attack preference
  if (!config->nodeExists("pause_update_when_attack")) {
    std::stringstream Default;
    Default
      << "WARNING: No pause_update_when_attack parameter "
      << "defined in XML config file!\n"
      << "No pausing assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    pause_update_when_attack = 0;
  }
  else if (!(config->getValue("pause_update_when_attack")).empty()) {
    switch (atoi(config->getValue("pause_update_when_attack").c_str())) {
      case 0:
        pause_update_when_attack = 0;
        break;
      case 1:
        pause_update_when_attack = 1;
        break;
      case 2:
        pause_update_when_attack = 2;
        break;
      default:
        std::stringstream Error;
        Error
          << "ERROR: Unknown value ("
          << config->getValue("pause_update_when_attack")
          << ") for pause_update_when_attack parameter!\n"
          << "Please chose one of the following and restart:\n"
          << "0: no pausing\n"
          << "1: pause, if one test detected an attack\n"
          << "2: pause, if all tests detected an attack\n";
        std::cerr << Error.str();
        if (warning_verbosity==1)
          outfile << Error.str() << std::flush;
        exit(0);
        break;
    }
  }
  else {
    std::stringstream Default;
    Default
      << "WARNING: No value for pause_update_when_attack parameter "
	    << "defined in XML config file!\n"
      << "No pausing assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    pause_update_when_attack = 0;
  }

  return;
}

//TODO(5)
void Stat::init_cusum_test(XMLConfObj * config) {
  std::stringstream Warning, Warning1, Warning2, Warning3, Warning4, Warning5, Error;
  Warning
    << "WARNING: No cusum_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  Warning1
    << "WARNING: No value for cusum_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  Warning2
    << "WARNING: No amplitude_percentage parameter "
    << "defined in XML config file!\n"
    << "\"" << DEFAULT_amplitude_percentage << "\" assumed.\n";
  Warning3
    << "WARNING: No value for amplitude_percentage parameter "
    << "defined in XML config file!\n"
    << "\"" << DEFAULT_amplitude_percentage << "\" assumed.\n";
  Warning4
    << "WARNING: No learning_phase_for_alpha parameter "
    << "defined in XML config file!\n"
    << "\"" << DEFAULT_learning_phase_for_alpha << "\" assumed.\n";
  Warning5
    << "WARNING: No value for learning_phase_for_alpha parameter "
    << "defined in XML config file!\n"
    << "\"" << DEFAULT_learning_phase_for_alpha << "\" assumed.\n";
  Error
    << "ERROR: Value for learning_phase_for_alpha parameter "
    << "is zero or negative.\nPlease define a positive value and restart!\n"
    << " Exiting.\n";

  // cusum_test
  if (!config->nodeExists("cusum_test")) {
    std::cerr << Warning.str();
    if (warning_verbosity==1)
      outfile << Warning.str() << std::flush;
    enable_cusum_test = true;
  }
  else if (!(config->getValue("cusum_test")).empty()) {
    enable_cusum_test = ( 0 == strcasecmp("true", config->getValue("cusum_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << Warning1.str();
    if (warning_verbosity==1)
      outfile << Warning1.str() << std::flush;
    enable_cusum_test = true;
  }

  // the other parameters need only to be initialized,
  // if the cusum-test is enabled
  if (enable_cusum_test == true) {
    // amplitude_percentage
    if (!config->nodeExists("amplitude_percentage")) {
      std::cerr << Warning2.str();
      if (warning_verbosity==1)
        outfile << Warning2.str() << std::flush;
      amplitude_percentage = DEFAULT_amplitude_percentage;
    }
    else if (!(config->getValue("amplitude_percentage")).empty())
      amplitude_percentage = atof(config->getValue("amplitude_percentage").c_str());
    else {
      std::cerr << Warning3.str();
      if (warning_verbosity==1)
        outfile << Warning3.str() << std::flush;
      amplitude_percentage = DEFAULT_amplitude_percentage;
    }

    // learning_phase_for_alpha
    if (!config->nodeExists("learning_phase_for_alpha")) {
      std::cerr << Warning4.str();
      if (warning_verbosity==1)
        outfile << Warning4.str() << std::flush;
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }
    else if (!(config->getValue("learning_phase_for_alpha")).empty()) {
      if ( atoi(config->getValue("learning_phase_for_alpha").c_str()) > 0)
        learning_phase_for_alpha = atoi(config->getValue("learning_phase_for_alpha").c_str());
      else {
        std::cerr << Error.str();
        if (warning_verbosity==1)
          outfile << Error.str() << std::flush;
        exit(0);
      }
    }
    else {
      std::cerr << Warning5.str();
      if (warning_verbosity==1)
        outfile << Warning5.str() << std::flush;
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }
  }

  return;
}

void Stat::init_which_test(XMLConfObj * config) {

  std::stringstream WMWdefault, WMWdefault1, KSdefault, KSdefault1, PCSdefault, PCSdefault1;
  WMWdefault
    << "WARNING: No wilcoxon_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  WMWdefault1
    << "WARNING: No value for wilcoxon_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  KSdefault
    << "WARNING: No kolmogorov_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  KSdefault1
    << "WARNING: No value for kolmogorov_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  PCSdefault
    << "WARNING: No pearson_chi-square_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";
  PCSdefault1
    << "WARNING: No value for pearson_chi-square_test parameter "
    << "defined in XML config file!\n\"true\" assumed.\n";

  // extracting type of test
  // (Wilcoxon and/or Kolmogorov and/or Pearson chi-square)
  if (!config->nodeExists("wilcoxon_test")) {
    std::cerr << WMWdefault.str();
    if (warning_verbosity==1)
      outfile << WMWdefault.str() << std::flush;
    enable_wmw_test = true;
  }
  else if (!(config->getValue("wilcoxon_test")).empty()) {
    enable_wmw_test = ( 0 == strcasecmp("true", config->getValue("wilcoxon_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << WMWdefault1.str();
    if (warning_verbosity==1)
      outfile << WMWdefault1.str() << std::flush;
    enable_wmw_test = true;
  }

  if (!config->nodeExists("kolmogorov_test")) {
    std::cerr << KSdefault.str();
    if (warning_verbosity==1)
      outfile << KSdefault.str() << std::flush;
    enable_ks_test = true;
  }
  else if (!(config->getValue("kolmogorov_test")).empty()) {
    enable_ks_test = ( 0 == strcasecmp("true", config->getValue("kolmogorov_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << KSdefault1.str();
    if (warning_verbosity==1)
      outfile << KSdefault1.str() << std::flush;
    enable_ks_test = true;
  }

  if (!config->nodeExists("pearson_chi-square_test")) {
    std::cerr << PCSdefault.str();
    if (warning_verbosity==1)
      outfile << PCSdefault.str() << std::flush;
    enable_pcs_test = true;
  }
  else if (!(config->getValue("pearson_chi-square_test")).empty()) {
    enable_pcs_test = ( 0 == strcasecmp("true", config->getValue("pearson_chi-square_test").c_str()) ) ?true:false;
  }
  else {
    std::cerr << PCSdefault1.str();
    if (warning_verbosity==1)
      outfile << PCSdefault1.str() << std::flush;
    enable_pcs_test = true;
  }

  return;
}

void Stat::init_sample_sizes(XMLConfObj * config) {

  std::stringstream Default_old, Default1_old, Default_new, Default1_new;
  Default_old
    << "WARNING: No sample_old_size parameter defined in XML config file! "
	  << DEFAULT_sample_old_size << " assumed.\n";
  Default1_old
    << "WARNING: No value for sample_old_size parameter defined in XML config file! "
    << DEFAULT_sample_old_size << " assumed.\n";
  Default_new
    << "WARNING: No sample_new_size parameter defined in XML config file! "
    << DEFAULT_sample_new_size << " assumed.\n";
  Default1_new
    << "WARNING: No value for sample_new_size parameter defined in XML config file! "
    << DEFAULT_sample_new_size << " assumed.\n";

  // extracting size of sample_old
  if (!config->nodeExists("sample_old_size")) {
    std::cerr << Default_old.str();
    if (warning_verbosity==1)
      outfile << Default_old.str() << std::flush;
    sample_old_size = DEFAULT_sample_old_size;
  }
  else if (!(config->getValue("sample_old_size")).empty()) {
    sample_old_size =
      atoi( config->getValue("sample_old_size").c_str() );
  }
  else {
    std::cerr << Default1_old.str();
    if (warning_verbosity==1)
      outfile << Default1_old.str() << std::flush;
    sample_old_size = DEFAULT_sample_old_size;
  }

  // extracting size of sample_new
  if (!config->nodeExists("sample_new_size")) {
    std::cerr << Default_new.str();
    if (warning_verbosity==1)
      outfile << Default_new.str() << std::flush;
    sample_new_size = DEFAULT_sample_new_size;
  }
  else if (!(config->getValue("sample_new_size")).empty()) {
    sample_new_size =
      atoi( config->getValue("sample_new_size").c_str() );
  }
  else {
    std::cerr << Default1_new.str();
    if (warning_verbosity==1)
      outfile << Default1_new.str() << std::flush;
    sample_new_size = DEFAULT_sample_new_size;
  }

  return;
}

void Stat::init_two_sided(XMLConfObj * config) {

  // extracting one/two-sided parameter for the test
  if ( enable_wmw_test == true && !config->nodeExists("two_sided") ) {
    // one/two-sided parameter is active only for Wilcoxon-Mann-Whitney
    // statistical test; Pearson chi-square test and Kolmogorov-Smirnov
    // test are one-sided only.
    std::stringstream Default;
    Default
      << "WARNING: No two_sided parameter defined in XML config file!\n"
      << "\"false\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    two_sided = false;
  }
  else if ( enable_wmw_test == true && config->getValue("two_sided").empty() ) {
    std::stringstream Default;
    Default
      << "WARNING: No value for two_sided parameter defined in XML config file!\n"
      << "\"false\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    two_sided = false;
  }
  // Every value but "true" is assumed to be false!
  else if (config->nodeExists("two_sided") && !(config->getValue("two_sided")).empty() )
    two_sided = ( 0 == strcasecmp("true", config->getValue("two_sided").c_str()) ) ? true:false;

  return;
}

void Stat::init_significance_level(XMLConfObj * config) {

  // extracting significance level parameter
  if (config->nodeExists("significance_level")) {
    if (!(config->getValue("significance_level")).empty()) {
      if ( 0 <= atof( config->getValue("significance_level").c_str() )
        && 1 >= atof( config->getValue("significance_level").c_str() ) )
        significance_level = atof( config->getValue("significance_level").c_str() );
      else {
        std::stringstream Error("ERROR: significance_level parameter should be between 0 and 1!\nExiting.\n");
        std::cerr << Error.str();
        if (warning_verbosity==1)
          outfile << Error.str() << std::flush;
        exit(0);
      }
    }
    else {
      std::stringstream Warning;
      Warning
        << "WARNING: No value for significance_level parameter defined in XML config file!\n"
        << "-1 assumed (nothing will be interpreted as an attack).\n";
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        outfile << Warning.str() << std::flush;
      significance_level = -1;
      // means no alarmist verbose effect ("Attack!!!", etc):
      // as 0 <= p-value <= 1, we will always have
      // p-value > "significance_level" = -1,
      // i.e. nothing will be interpreted as an attack
      // and no verbose effect will be outputed
    }
  }
  else {
    std::stringstream Warning;
    Warning
      << "WARNING: No significance_level parameter defined in XML config file!\n"
      << "-1 assumed (nothing will be interpreted as an attack).\n";
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        outfile << Warning.str() << std::flush;
    significance_level = -1;
  }

  return;
}


// ============================= TEST FUNCTION ===============================
void Stat::test(StatStore * store) {

#ifdef IDMEF_SUPPORT_ENABLED
  idmefMessage = getNewIdmefMessage("wkp-module", "statistical anomaly detection");
#endif

  // Getting whole Data from store
  std::map<EndPoint,Info> Data = store->getData();

  // Dumping empty records:
  if (Data.empty()==true) {
    if (output_verbosity>=3 || warning_verbosity==1)
      outfile << "INFORMATION: Got empty record; "
	      << "dumping it and waiting for another record" << std::endl << std::flush;
    return;
  }

  outfile
    << "####################################################" << std::endl
    << "########## Stat::test(...)-call number: " << test_counter
    << " ##########" << std::endl
    << "####################################################" << std::endl;

  std::map<EndPoint,Info>::iterator Data_it = Data.begin();

  // is at least one of the wkp-tests activated?
  bool enable_wkp_test = (enable_wmw_test == true || enable_ks_test == true || enable_pcs_test == true)?true:false;

  std::map<EndPoint,Info> PreviousData = store->getPreviousData();
    // Needed for extraction of packets(t)-packets(t-1) and bytes(t)-bytes(t-1)
    // Holds information about the Info used in the last call to test()
  Info prev;

  // 1) LEARN/UPDATE PHASE
  // Parsing data to see whether the recorded EndPoints already exist
  // in our  "std::map<EndPoint, Samples> SampleData" respective
  // "std::map<EndPoint, CusumParams> CusumData" container.
  // If not, then we add it as a new pair <EndPoint, *>.
  // If yes, then we update the corresponding entry using
  // std::vector<int64_t> extracted data.
  outfile << "#### LEARN/UPDATE PHASE" << std::endl;

  // for every EndPoint, extract the data
  while (Data_it != Data.end()) {

    outfile << "[[ " << Data_it->first << " ]]" << std::endl;

    prev = PreviousData[Data_it->first];
    // it doesn't matter much if Data_it->first is an EndPoint that exists
    // only in Data, but not in PreviousData, because
    // PreviousData[Data_it->first] will automaticaly be an Info structure
    // with all fields set to 0.

    // Do stuff for wkp-tests
    if (enable_wkp_test == true) {

      std::map<EndPoint, Samples>::iterator SampleData_it =
        SampleData.find(Data_it->first);

      if (SampleData_it == SampleData.end()) {

        // We didn't find the recorded EndPoint Data_it->first
        // in our sample container "SampleData"; that means it's a new one,
        // so we just add it in "SampleData"; there will not be jeopardy
        // of memory exhaustion through endless growth of the "SampleData" map
        // as limits are implemented in the StatStore class (EndPointListMaxSize)

        Samples S;
        // extract_data has vector<int64_t> as output
        (S.Old).push_back(extract_data(Data_it->second, prev));
        SampleData[Data_it->first] = S;
        if (output_verbosity >= 3) {
          outfile << "(WKP): New monitored EndPoint added" << std::endl;
          if (output_verbosity >= 4) {
            outfile << "with first element of sample_old: " << S.Old.back() << std::endl;
          }
        }

      }
      else {
        // We found the recorded EndPoint Data_it->first
        // in our sample container "SampleData"; so we update the samples
        // (SampleData_it->second).Old and (SampleData_it->second).New
        // thanks to the recorded new value in Data_it->second:
        update ( SampleData_it->second, extract_data(Data_it->second, prev) );
      }
    }

    // Do stuff for cusum-test
    if (enable_cusum_test == true) {

      std::map<EndPoint, CusumParams>::iterator CusumData_it = CusumData.find(Data_it->first);

      if (CusumData_it == CusumData.end()) {

        // We didn't find the recorded EndPoint Data_it->first
        // in our cusum container "CusumData"; that means it's a new one,
        // so we just add it in "CusumData"; there will not be jeopardy
        // of memory exhaustion through endless growth of the "CusumData" map
        // as limits are implemented in the StatStore class (EndPointListMaxSize)

        CusumParams C;
        // initialize the vectors of C
        // as several metrics could be monitored, we need vectors instead of
        // single values here
        for (int i = 0; i != monitored_values.size(); i++) {
          (C.sum).push_back(0);
          (C.alpha).push_back(0.0);
          (C.g).push_back(0.0);
          (C.X).push_back(0);
        }

        // extract_data
        std::vector<int64_t> v = extract_data(Data_it->second, prev);

        // add first value of each metric to the sum, which is needed
        // only to calculate the initial alpha after learning_phase_for_alpha
        // is over
        for (int i = 0; i != v.size(); i++)
          C.sum.at(i) += v.at(i);

        C.learning_phase_nr = 1;

        CusumData[Data_it->first] = C;
        if (output_verbosity >= 3)
          outfile << "(CUSUM): New monitored EndPoint added" << std::endl;
      }
      else {
        // We found the recorded EndPoint Data_it->first
        // in our cusum container "CusumData"; so we update alpha
        // thanks to the recorded new values in Data_it->second:
        update_c ( CusumData_it->second, extract_data(Data_it->second, prev) );
      }

    }

    Data_it++;
  }

  outfile << std::endl << std::flush;

  // 1.5) MAP PRINTING (OPTIONAL, DEPENDS ON VERBOSITY SETTINGS)
  if (output_verbosity >= 4) {
    outfile << "#### STATE OF ALL MONITORED ENDPOINTS (" << SampleData.size() << "):" << std::endl;

    if (enable_wkp_test == true) {
      outfile << "### WKP OVERVIEW" << std::endl;
      std::map<EndPoint,Samples>::iterator SampleData_it =
        SampleData.begin();
      while (SampleData_it != SampleData.end()) {
        outfile
          << "[[ " << SampleData_it->first << " ]]\n"
          << " sample_old (" << (SampleData_it->second).Old.size()  << ") : "
          << (SampleData_it->second).Old << "\n"
          << " sample_new (" << (SampleData_it->second).New.size() << ") : "
          << (SampleData_it->second).New << "\n";
        SampleData_it++;
      }
    }
    if (enable_cusum_test == true) {
      outfile << "### CUSUM OVERVIEW" << std::endl;
      std::map<EndPoint,CusumParams>::iterator CusumData_it =
        CusumData.begin();
      while (CusumData_it != CusumData.end()) {
        // TODO: Mehr Zeug ausgeben (alphas, gs usw.)
        outfile
          << "[[ " << CusumData_it->first << " ]]\n"
          << "   !!! Cusum-Parameter ausgeben !!!\n";
        CusumData_it++;
      }
    }
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

    // We begin testing as soon as possible, i.e.
    // for WKP:
      // as soon as a sample is big enough to test, i.e. when its
      // learning phase is over.
    // for CUSUM:
      // as soon as we have enough values for calculating the initial
      // values of alpha
    // The other endpoints in the "SampleData" and "CusumData"
    // maps are let learning.

    if (enable_wkp_test == true) {
      std::map<EndPoint,Samples>::iterator SampleData_it = SampleData.begin();
      while (SampleData_it != SampleData.end()) {
        if ( ((SampleData_it->second).New).size() == sample_new_size ) {
          // i.e., learning phase over
          outfile << "\n#### WKP TESTS for EndPoint [[ " << SampleData_it->first << " ]]\n";
          stat_test ( SampleData_it->second );
        }
        SampleData_it++;
      }
    }

    if (enable_cusum_test == true) {
      std::map<EndPoint,CusumParams>::iterator CusumData_it = CusumData.begin();
      while (CusumData_it != CusumData.end()) {
        if ( (CusumData_it->second).ready_to_test == true ) {
          // i.e. learning phase for alpha is over and it has an initial value
          outfile << "\n#### CUSUM TESTS for EndPoint [[ " << CusumData_it->first << " ]]\n";
          cusum_test ( CusumData_it->second );
        }
        CusumData_it++;
      }
    }

  }

  test_counter++;

  /* don't forget to free the store-object! */
  delete store;
  outfile << std::endl << std::flush;
  return;

}



// =================== FUNCTIONS USED BY THE TEST FUNCTION ====================

// extracts interesting data from StatStore according to monitored_values:
std::vector<int64_t>  Stat::extract_data (const Info & info, const Info & prev) {

  std::vector<int64_t>  result;

  std::vector<Metric>::iterator it = monitored_values.begin();

  while (it != monitored_values.end() ) {

    switch ( *it ) {

      case PACKETS_IN:
        if (info.packets_in >= noise_threshold_packets)
          result.push_back(info.packets_in);
        break;

      case PACKETS_OUT:
        if (info.packets_out >= noise_threshold_packets)
          result.push_back(info.packets_out);
        break;

      case BYTES_IN:
        if (info.bytes_in >= noise_threshold_bytes)
          result.push_back(info.bytes_in);
        break;

      case BYTES_OUT:
        if (info.bytes_out >= noise_threshold_bytes)
          result.push_back(info.bytes_out);
        break;

      case RECORDS_IN:
        result.push_back(info.records_in);
        break;

      case RECORDS_OUT:
        result.push_back(info.records_out);
        break;

      case BYTES_IN_PER_PACKET_IN:
        if ( info.packets_in >= noise_threshold_packets
          || info.bytes_in   >= noise_threshold_bytes ) {
          if (info.packets_in == 0)
            result.push_back(0);
          else
            result.push_back((1000 * info.bytes_in) / info.packets_in);
            // the multiplier 1000 enables us to increase precision and "simulate"
            // a float result, while keeping an integer result: thanks to this trick,
            // we do not have to write new versions of the tests to support floats
        }
        break;

      case BYTES_OUT_PER_PACKET_OUT:
        if (info.packets_out >= noise_threshold_packets ||
            info.bytes_out   >= noise_threshold_bytes ) {
          if (info.packets_out == 0)
            result.push_back(0);
          else
            result.push_back((1000 * info.bytes_out) / info.packets_out);
        }
        break;

      case PACKETS_OUT_MINUS_PACKETS_IN:
        if (info.packets_out >= noise_threshold_packets
         || info.packets_in  >= noise_threshold_packets )
          result.push_back(info.packets_out - info.packets_in);
        break;

      case BYTES_OUT_MINUS_BYTES_IN:
        if (info.bytes_out >= noise_threshold_bytes
         || info.bytes_in  >= noise_threshold_bytes )
          result.push_back(info.bytes_out - info.bytes_in);
        break;

      case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
        if (info.packets_in  >= noise_threshold_packets
         || prev.packets_in  >= noise_threshold_packets)
          // prev holds the data for the same EndPoint as info
          // from the last call to test()
          // it is updated at the beginning of the while-loop in test()
          result.push_back(info.packets_in - prev.packets_in);
        break;

      case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
        if (info.packets_out >= noise_threshold_packets
         || prev.packets_out >= noise_threshold_packets)
          result.push_back(info.packets_out - prev.packets_out);
        break;

      case BYTES_T_IN_MINUS_BYTES_T_1_IN:
        if (info.bytes_in >= noise_threshold_bytes
         || prev.bytes_in >= noise_threshold_bytes)
          result.push_back(info.bytes_in - prev.bytes_in);
        break;

      case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
        if (info.bytes_out >= noise_threshold_bytes
         || prev.bytes_out >= noise_threshold_bytes)
          result.push_back(info.bytes_out - prev.bytes_out);
        break;

      default:
        std::cerr << "ERROR: monitored_values seems to be empty "
        << "or it holds an unknown type which isnt supported yet."
        << "But this shouldnt happen as the init_monitored_values"
        << "-function handles that.\nExiting.\n";
        exit(0);
    }

    it++;
  }

  return result;
}


// -------------------------- LEARN/UPDATE FUNCTION ---------------------------

// learn/update function for samples (called everytime test() is called)
//
void Stat::update ( Samples & S, const std::vector<int64_t> & new_value ) {

  // Learning phase?
  if (S.Old.size() != sample_old_size) {

    S.Old.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << "(WKP): Learning phase for sample_old ..." << std::endl;
      if (output_verbosity >= 4) {
        outfile << "  sample_old: " << S.Old << std::endl;
        outfile << "  sample_new: " << S.New << std::endl;
      }
    }

    return;
  }
  else if (S.New.size() != sample_new_size) {

    S.New.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << "(WKP): Learning phase for sample_new..." << std::endl;
      if (output_verbosity >= 4) {
        outfile << "  sample_old: " << S.Old << std::endl;
        outfile << "  sample_new: " << S.New << std::endl;
      }
    }

    return;
  }

  // Learning phase over: update

  // pausing update for old sample,
  // if 1 of the last tests was an attack and parameter is 1
  // or all of the last tests were an attack and parameter is 2
  if ( (pause_update_when_attack == 1
        && ( S.last_wmw_test_was_attack == true
          || S.last_ks_test_was_attack  == true
          || S.last_pcs_test_was_attack == true ))
    || (pause_update_when_attack == 2
        && ( S.last_wmw_test_was_attack == true
          && S.last_ks_test_was_attack  == true
          && S.last_pcs_test_was_attack == true )) ) {

    S.New.pop_front();
    S.New.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << "(WKP): Update done (for new sample only)" << std::endl;
      if (output_verbosity >= 4) {
        outfile << "  sample_old: " << S.Old << std::endl;
        outfile << "  sample_new: " << S.New << std::endl;
      }
    }
  }
  // if parameter is 0 or there was no attack detected
  // update both samples
  else {
    S.Old.pop_front();
    S.Old.push_back(S.New.front());
    S.New.pop_front();
    S.New.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << "(WKP): Update done (for both samples)" << std::endl;
      if (output_verbosity >= 4) {
        outfile << "  sample_old: " << S.Old << std::endl;
        outfile << "  sample_new: " << S.New << std::endl;
      }
    }
  }

  outfile << std::flush;
  return;
}


// and the update funciotn for the cusum-test
void Stat::update_c ( CusumParams & C, const std::vector<int64_t> & new_value ) {

  // Learning phase for alpha?
  // that means, we dont have enough values to calculate alpha
  // until now (and so cannot perform the cusum test)
  if (C.ready_to_test == false) {
    if (C.learning_phase_nr < learning_phase_for_alpha) {

      // update the sum of the values of each metric
      // (needed to calculate the initial alpha)
      for (int i = 0; i != C.sum.size(); i++)
        C.sum.at(i) += new_value.at(i);

      if (output_verbosity >= 3)
        outfile
          << "(CUSUM): Learning phase for alpha ..." << std::endl;

      C.learning_phase_nr++;

      return;
    }

    // Enough values? Calculate initial alpha per simple average
    // and set ready_to_test-flag to true (so we never visit this
    // code here again for the current endpoint)
    if (C.learning_phase_nr == learning_phase_for_alpha) {

      if (output_verbosity >= 3)
        outfile << "(CUSUM): Learning phase is over --> Calculated initial alphas: (";
      // alpha = sum(values) / #(values)
      for (int i = 0; i != C.alpha.size(); i++) {
        C.alpha.at(i) = C.sum.at(i) / learning_phase_for_alpha;
        if (output_verbosity >= 3)
          outfile << C.alpha.at(i) << ", ";
      }
      if (output_verbosity >= 3)
          outfile << ")";

      C.ready_to_test = true;
      return;
    }
  }

  // pausing update for alpha, if last cusum-test
  // was an attack
  if ( pause_update_when_attack > 0
    && C.last_cusum_test_was_attack == true) {
    if (output_verbosity >= 3)
      outfile << "(CUSUM): Pausing update for alpha" << std::endl;
    return;
  }

  // Otherwise update all alphas per EWMA

  // smoothing constant
  float gamma = 0.15; // TODO: make it a parameter?

  for (int i = 0; i != C.alpha.size(); i++) {
    C.alpha.at(i) = C.alpha.at(i) * (1 - gamma) + new_value.at(i) * gamma;
    // TODO (folgende Zeile soll den für den cusum-test benötigten
    // aktuellen Wert setzen)
    C.X.at(i) = new_value.at(i);
  }

  if (output_verbosity >= 3) {
    outfile << "(CUSUM): Update done for alpha: (";
    for (int i = 0; i != C.alpha.size(); i++) {
        outfile << C.alpha.at(i) << ", ";
    outfile << ")";
  }

  outfile << std::flush;
  return;
}

// ------- FUNCTIONS USED TO CONDUCT TESTS ON THE SAMPLES ---------

// statistical test function / wkp-tests
// (optional, depending on how often the user wishes to do it)
void Stat::stat_test (Samples & S) {

  // Containers for the values of single metrics
  std::list<int64_t> sample_old_single_metric;
  std::list<int64_t> sample_new_single_metric;

  std::vector<Metric>::iterator it = monitored_values.begin();

  // for every value (represented by *it) in monitored_values,
  // do the tests
  short index = 0;
  // as the tests can be performed for several metrics, we have to
  // store, if at least one metric raised an alarm and if so, set
  // the last_test_was_attack-flag to true
  bool wmw_was_attack = false;
  bool ks_was_attack = false;
  bool pcs_was_attack = false;

  while (it != monitored_values.end()) {

    if (output_verbosity >= 4)
      outfile << "### Performing WKP-Tests for metric " << getMetricName(*it) << ":\n";

    sample_old_single_metric = getSingleMetric(S.Old, *it, index);
    sample_new_single_metric = getSingleMetric(S.New, *it, index);

    // Wilcoxon-Mann-Whitney test:
    if (enable_wmw_test == true) {
      stat_test_wmw(sample_old_single_metric, sample_new_single_metric, S.last_wmw_test_was_attack);
      if (S.last_wmw_test_was_attack == true)
        wmw_was_attack = true;
    }

    // Kolmogorov-Smirnov test:
    if (enable_ks_test == true) {
      stat_test_ks (sample_old_single_metric, sample_new_single_metric, S.last_ks_test_was_attack);
      if (S.last_ks_test_was_attack == true)
        ks_was_attack = true;
    }

    // Pearson chi-square test:
    if (enable_pcs_test == true) {
      stat_test_pcs(sample_old_single_metric, sample_new_single_metric, S.last_pcs_test_was_attack);
      if (S.last_pcs_test_was_attack == true)
        pcs_was_attack = true;
    }

    it++;
    index++;
  }

  // if there was at least one alarm, set the corresponding
  // flag to true
  if (wmw_was_attack == true)
    S.last_wmw_test_was_attack = true;
  if (ks_was_attack == true)
    S.last_ks_test_was_attack = true;
  if (pcs_was_attack == true)
    S.last_pcs_test_was_attack = true;

  outfile << std::flush;
  return;

}

// statistical test function / cusum-test
// (optional, depending on how often the user wishes to do it)
void Stat::cusum_test(CusumParams & C) {

  // as the tests can be performed for several metrics, we have to
  // store, if at least one metric raised an alarm and if so, set
  // the last_cusum_test_was_attack-flag to true
  bool was_attack = false;
  // index, as we cant use the iterator for dereferencing the elements of
  // CusumParams
  int i = 0;
  //adaptive threshold, calculated by amplitude_percentage * alpha / 2
  float N = 0.0;
  // beta = alpha + fi*alpha/2 with fi = amplitude_percentage
  float beta = 0.0;

  for (std::vector<Metric>::iterator it = monitored_values.begin(); it != monitored_values.end(); it++) {

    if (output_verbosity >= 4)
      outfile << "### Performing CUSUM-Test for metric " << getMetricName(*it) << ":\n";

    // calculate current threshold
    N = amplitude_percentage * C.alpha.at(i) / 2;
    // calculate current beta
    beta = C.alpha.at(i) + N;

    if (output_verbosity >= 4) {
      outfile << " Cusum test returned:\n"
              << "  Threshold: " << N << std::endl;
      outfile << "  reject H0 (no attack) if current value of statistic g > "
              << N << std::endl;
    }

    // perform the test and if g > N raise an alarm
    if ( cusum(C.X.at(i), beta, C.g.at(i)) > N ) {

      if (report_only_first_attack == false
        || C.last_cusum_test_was_attack == false) {
        outfile
          << "    ATTACK! ATTACK! ATTACK!\n"
          << "    Cusum test says we're under attack!\n"
          << "    ALARM! ALARM! Women und children first!" << std::endl;
        std::cout
          << "  ATTACK! ATTACK! ATTACK!\n"
          << "  Cusum test says we're under attack!\n"
          << "  ALARM! ALARM! Women und children first!" << std::endl;
        #ifdef IDMEF_SUPPORT_ENABLED
          idmefMessage.setAnalyzerAttr("", "", "cusum-test", "");
          sendIdmefMessage("DDoS", idmefMessage);
          idmefMessage = getNewIdmefMessage();
        #endif
      }

      was_attack = true;

    }

    i++;
  }

  if (was_attack == true)
    C.last_cusum_test_was_attack = true;

  outfile << std::flush;
  return;
}

// functions called by the stat_test()-function
std::list<int64_t> Stat::getSingleMetric(const std::list<std::vector<int64_t> > & l, const enum Metric & m, const short & i) {

  std::list<int64_t> result;
  std::list<std::vector<int64_t> >::const_iterator it = l.begin();

  switch(m) {
    case PACKETS_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case PACKETS_OUT:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_OUT:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case RECORDS_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case RECORDS_OUT:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_IN_PER_PACKET_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_OUT_PER_PACKET_OUT:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case PACKETS_OUT_MINUS_PACKETS_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_OUT_MINUS_BYTES_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_T_IN_MINUS_BYTES_T_1_IN:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
      while ( it != l.end() ) {
        result.push_back(it->at(i));
        it++;
      }
      break;
    default:
      std::cerr << "ERROR: Got unknown metric in Stat::getSingleMetric(...)!\n"
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
      return std::string("bytes_in");
    case BYTES_OUT:
      return std::string("bytes_out");
    case RECORDS_IN:
      return std::string("records_in");
    case RECORDS_OUT:
      return std::string("records_out");
    case BYTES_IN_PER_PACKET_IN:
      return std::string("bytes_in/packet_in");
    case BYTES_OUT_PER_PACKET_OUT:
      return std::string("bytes_out/packet_out");
    case PACKETS_OUT_MINUS_PACKETS_IN:
      return std::string("packets_out-packets_in");
    case BYTES_OUT_MINUS_BYTES_IN:
      return std::string("bytes_out-bytes_in");
    case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
      return std::string("packets_in(t)-packets_in(t-1)");
    case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
      return std::string("packets_out(t)-packets_out(t-1)");
    case BYTES_T_IN_MINUS_BYTES_T_1_IN:
      return std::string("bytes_in(t)-bytes_in(t-1)");
    case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
      return std::string("bytes_out(t)-bytes_out(t-1)");
    default:
      std::cerr << "ERROR: Unknown type of Metric in getMetricName().\n"
                << "Exiting.\n";
      exit(0);
  }
}

void Stat::stat_test_wmw (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new, bool & last_wmw_test_was_attack) {

  double p;

  if (output_verbosity >= 5) {
    outfile << " Wilcoxon-Mann-Whitney test details:\n";
    p = wmw_test(sample_old, sample_new, two_sided, outfile);
  }
  else {
    std::ofstream dump("/dev/null");
    p = wmw_test(sample_old, sample_new, two_sided, dump);
  }

  if (output_verbosity >= 2) {
    outfile << " Wilcoxon-Mann-Whitney test returned:\n"
	    << "  p-value: " << p << std::endl;
    outfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_wmw_test_was_attack == false) {
	      outfile
          << "    ATTACK! ATTACK! ATTACK!\n"
		      << "    Wilcoxon-Mann-Whitney test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout
          << "  ATTACK! ATTACK! ATTACK!\n"
          << "  Wilcoxon-Mann-Whitney test says we're under attack!\n"
		      << "  ALARM! ALARM! Women und children first!" << std::endl;
      #ifdef IDMEF_SUPPORT_ENABLED
        idmefMessage.setAnalyzerAttr("", "", "wmw-test", "");
        sendIdmefMessage("DDoS", idmefMessage);
        idmefMessage = getNewIdmefMessage();
      #endif
      }
      last_wmw_test_was_attack = true;
    }
    else
      last_wmw_test_was_attack = false;
  }

  if (output_verbosity == 1) {
    outfile << "wmw: " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
      || last_wmw_test_was_attack == false) {
	      outfile << "attack to significance level "
		      << significance_level << "!" << std::endl;
	      std::cout << "attack to significance level "
		      << significance_level << "!" << std::endl;
      }
      last_wmw_test_was_attack = true;
    }
    else
      last_wmw_test_was_attack = false;
  }

  return;
}


void Stat::stat_test_ks (std::list<int64_t> & sample_old,
			 std::list<int64_t> & sample_new, bool & last_ks_test_was_attack) {

  double p;

  if (output_verbosity>=5) {
    outfile << " Kolmogorov-Smirnov test details:\n";
    p = ks_test(sample_old, sample_new, outfile);
  }
  else {
    std::ofstream dump ("/dev/null");
    p = ks_test(sample_old, sample_new, dump);
  }

  if (output_verbosity >= 2) {
    outfile << " Kolmogorov-Smirnov test returned:\n"
	    << "  p-value: " << p << std::endl;
    outfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_ks_test_was_attack == false) {
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
      last_ks_test_was_attack = true;
    }
    else
      last_ks_test_was_attack = false;
  }

  if (output_verbosity == 1) {
    outfile << "ks : " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_ks_test_was_attack == false) {
        outfile << "attack to significance level "
          << significance_level << "!" << std::endl;
        std::cout << "attack to significance level "
          << significance_level << "!" << std::endl;
      }
      last_ks_test_was_attack = true;
    }
    else
      last_ks_test_was_attack = false;
  }

  return;
}


void Stat::stat_test_pcs (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new, bool & last_pcs_test_was_attack) {

  double p;

  if (output_verbosity>=5) {
    outfile << " Pearson chi-square test details:\n";
    p = pcs_test(sample_old, sample_new, outfile);
  }
  else {
    std::ofstream dump ("/dev/null");
    p = pcs_test(sample_old, sample_new, dump);
  }

  if (output_verbosity >= 2) {
    outfile << " Pearson chi-square test returned:\n"
	    << "  p-value: " << p << std::endl;
    outfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_pcs_test_was_attack == false) {
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
      last_pcs_test_was_attack = true;
    }
    else
      last_pcs_test_was_attack = false;
  }

  if (output_verbosity == 1) {
    outfile << "pcs: " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
      || last_pcs_test_was_attack == false) {
	      outfile << "attack to significance level "
		      << significance_level << "!" << std::endl;
	      std::cout << "attack to significance level "
		      << significance_level << "!" << std::endl;
      }
      last_pcs_test_was_attack = true;
    }
    else
      last_pcs_test_was_attack = false;
  }

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
