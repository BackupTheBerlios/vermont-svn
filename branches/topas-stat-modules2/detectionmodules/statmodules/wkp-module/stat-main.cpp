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
handle more protocols
--------------------------------------
now, only TCP, UDP, ICMP and RAW are inspected

TODO(2)
Tests
--------------------------------------
If report_only_first_attack = true:
We cant see from the output, whether an attack ceased. Maybe some kind of "attack still in progress"-message would be nice ...
*/

// for pca (eigenvectors etc.)
#include <gsl/gsl_eigen.h>
// for pca (matrix multiplication)
#include <gsl/gsl_blas.h>

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


  // BEGIN TESTING
  counter = 0;
  // Filtering: read most frequently X endpoints from file
  // and push them into filter vector
  // (Needed for test-scripts 3 and 4)
  // comment it, if not needed!

  std::ifstream f("darpa1_port_10_freq_eps.txt");
  std::string tmp;
  while ( getline(f, tmp) ) {
    EndPoint e = EndPoint(IpAddress(0,0,0,0),0,0);
    e.fromString(tmp);
    filter.push_back(e);
  }

  // END TESTING

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
      << "  Define one, fill it with some parameters and restart.\n"
      << "  Exiting.\n";
    delete config;
    exit(0);
  }

  if (!config->nodeExists("cusum-test-params")) {
    std::cerr
      << "ERROR: No cusum-test-params node defined in XML config file!\n"
      << "  Define one, fill it with some parameters and restart.\n"
      << "  Exiting.\n";
    delete config;
    exit(0);
  }

  if (!config->nodeExists("wkp-test-params")) {
    std::cerr
      << "ERROR: No wkp-test-params node defined in XML config file!\n"
      << "  Define one, fill it with some parameters and restart.\n"
      << "  Exiting.\n";
    delete config;
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

  // initialize pca parameters
  init_pca(config);

  // extracting metrics
  init_metrics(config);

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

  config->enterNode("cusum-test-params");

  // extracting cusum parameters
  init_cusum_test(config);

  config->leaveNode();

  config->enterNode("wkp-test-params");

  // extracting type of test
  // (Wilcoxon and/or Kolmogorov and/or Pearson chi-square)
  init_which_test(config);

  // no need for further initialization of wkp-params if
  // none of the three tests is enabled
  enable_wkp_test = (enable_wmw_test == true || enable_ks_test == true || enable_pcs_test == true)?true:false;

  if (enable_wkp_test == true) {

    // extracting sample sizes
    init_sample_sizes(config);

    // extracting one/two-sided parameter for the test
    init_two_sided(config);

    // extracting significance level parameter
    init_significance_level(config);

  }

  config->leaveNode();

  // if no test is enabled, it doesnt make sense to run the module
  if (enable_wkp_test == false && enable_cusum_test == false) {
    std::stringstream Error;
    Error
      << "ERROR: There is no test enabled!\n"
      << "  Please enable at least one test and restart!\n"
      << "  Exiting.\n";
    std::cerr << Error.str();
    if (warning_verbosity==1)
      outfile << Error.str() << std::flush;
    delete config;
    exit(0);
  }

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
      << "  Default outputfile used (wkp_output.txt).\n";
    outfile.open("wkp_output.txt");
  }
  else if (!(config->getValue("output_file")).empty())
    outfile.open(config->getValue("output_file").c_str());
  else {
    std::cerr
      << "WARNING: No value for output_file parameter defined in XML config file!\n"
      << "  Default output_file used (wkp_output.txt).\n";
    outfile.open("wkp_output.txt");
  }

  if (!outfile) {
      std::cerr << "ERROR: could not open output file!\n"
        << "  Check if you have enough rights to create or write to it.\n"
        << "  Exiting.\n";
      exit(0);
  }

  return;
}

void Stat::init_accepted_source_ids(XMLConfObj * config) {

  if (!config->nodeExists("accepted_source_ids")) {
    std::stringstream Warning;
    Warning
      << "WARNING: No accepted_source_ids parameter defined in XML config file!\n"
      << "  All source ids will be accepted.\n";
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
      << "  All source ids will be accepted.\n";
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
      << "  \"" << DEFAULT_alarm_time << "\" assumed.\n";
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
      << "  \"" << DEFAULT_alarm_time << "\" assumed.\n";
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
    << "  Please define it that way and restart.\n";
  Warning
    << "WARNING: No warning_verbosity parameter defined in XML config file!\n"
    << "  " << DEFAULT_warning_verbosity << "\" assumed.\n";
  Default
    << "WARNING: No value for warning_verbosity parameter defined "
	  << "in XML config file!\n"
	  << "  \"" << DEFAULT_warning_verbosity << "\" assumed.\n";
  Usage
    << "  O: warnings are sent to stderr\n"
	  << "  1: warnings are sent to stderr and output file\n";

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
      std::cerr << Error.str() << Usage.str() << "  Exiting.\n";
      outfile << Error.str() << Usage.str() << "  Exiting.\n" << std::flush;
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
    << "  Please define it that way and restart.\n";
  Warning
    << "WARNING: No output_verbosity parameter defined "
    << "in XML config file!\n"
    << "  \"" << DEFAULT_output_verbosity << "\" assumed.\n";
  Default
    << "WARNING: No value for output_verbosity parameter defined "
	  << "in XML config file!\n"
	  << "  \"" << DEFAULT_output_verbosity << "\" assumed.\n";
  Usage
    << "  O: no output generated\n"
	  << "  1: only p-values and attacks are recorded\n"
	  << "  2: same as 1, plus some cosmetics\n"
	  << "  3: same as 2, plus learning phases, updates and empty records events\n"
	  << "  4: same as 3, plus sample printing\n"
	  << "  5: same as 4, plus all details from statistical tests\n";

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
      std::cerr << Error.str() << Usage.str() << "  Exiting.\n";
      if (warning_verbosity==1)
        outfile << Error.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
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
    << "  endpoint_key will be ip + port + protocol!";
  Error
    << "ERROR: Unknown value defined for endpoint_key parameter in XML config file!\n"
    << "  Value should be either port, ip, protocol or any combination of them (seperated via spaces)!\n";
  Default
    << "WARNING: No value for endpoint_key parameter defined in XML config file!\n"
    << "  endpoint_key will be ip + port + protocol!";

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
        std::cerr << Error.str() << "  Exiting.\n";
        if (warning_verbosity==1)
          outfile << Error.str() << "  Exiting." << std::endl << std::flush;
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

// extract all needed parameters for the pca, if enabled
void Stat::init_pca(XMLConfObj * config) {

  if (!config->nodeExists("use_pca")) {
    use_pca = false;
    return;
  }

  std::stringstream Default1, Default2, Default3;
  Default1
    << "WARNING: No value for use_pca parameter defined in XML config file!\n"
    << "  \"true\" assumed.\n";
  Default2
    << "WARNING: No pca_learning_phase parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_pca << "\" assumed.\n";
  Default3
    << "WARNING: No value for pca_learning_phase parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_pca << "\" assumed.\n";


  if ( !(config->getValue("use_pca").empty()) ) {
    use_pca = ( 0 == strcasecmp("true", config->getValue("use_pca").c_str()) ) ? true:false;
  }
  else {
    std::cerr << Default1.str();
    if (warning_verbosity==1)
      outfile << Default1.str() << std::flush;
    use_pca = true;
  }

  // initialize the other parameters only if pca is used
  if (use_pca == true) {

    // learning_phase
    if (!config->nodeExists("pca_learning_phase")) {
      std::cerr << Default2.str();
      if (warning_verbosity==1)
        outfile << Default2.str() << std::flush;
      learning_phase_for_pca = DEFAULT_learning_phase_for_pca;
    }
    else if ( !(config->getValue("pca_learning_phase").empty()) ) {
      learning_phase_for_pca = atoi(config->getValue("pca_learning_phase").c_str());
    }
    else {
      std::cerr << Default3.str();
      if (warning_verbosity==1)
        outfile << Default3.str() << std::flush;
      learning_phase_for_pca = DEFAULT_learning_phase_for_pca;
    }
  }
  return;
}

// extracting metrics to vector<Metric>;
// the values are stored there as constants
// defined in enum Metric in stat_main.h
void Stat::init_metrics(XMLConfObj * config) {

  std::stringstream Error1, Error2, Error3, Usage;
  Error1
    << "ERROR: No metrics parameter in XML config file!\n"
    << "  Please define one and restart.\n";
  Error2
    << "ERROR: No value parameter(s) defined for metrics in XML config file!\n"
    << "  Please define at least one and restart.\n";
  Error3
    << "ERROR: Unknown value parameter(s) defined for metrics in XML config file!\n"
    << "  Please provide only valid <value>-parameters.\n";
  Usage
    << "  Use for each <value>-Tag one of the following metrics:\n"
    << "  packets_in, packets_out, bytes_in, bytes_out, records_in, records_out, "
    << "  bytes_in/packet_in, bytes_out/packet_out, packets_out-packets_in, "
    << "  bytes_out-bytes_in, packets_in(t)-packets_in(t-1), "
    << "  packets_out(t)-packets_out(t-1), bytes_in(t)-bytes_in(t-1) or "
    << "  bytes_out(t)-bytes_out(t-1).\n";

  if (!config->nodeExists("metrics")) {
    std::cerr << Error1.str() << Usage.str() << "  Exiting.\n";
    if (warning_verbosity==1)
      outfile << Error1.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
    exit(0);
  }

  config->enterNode("metrics");

  std::vector<std::string> tmp_monitored_data;

  /* get all monitored values */
  if (config->nodeExists("value")) {
    tmp_monitored_data.push_back(config->getValue("value"));
    while (config->nextNodeExists())
      tmp_monitored_data.push_back(config->getNextValue());
  }
  else {
    std::cerr << Error2.str() << Usage.str() << "  Exiting.\n";
    if (warning_verbosity==1)
      outfile << Error2.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
    exit(0);
  }

  config->leaveNode();

  // extract the values from tmp_monitored_data (string)
  // to metrics (vector<enum>)
  std::vector<std::string>::iterator it = tmp_monitored_data.begin();
  while ( it != tmp_monitored_data.end() ) {
    if ( 0 == strcasecmp("packets_in", (*it).c_str()) )
      metrics.push_back(PACKETS_IN);
    else if ( 0 == strcasecmp("packets_out", (*it).c_str()) )
      metrics.push_back(PACKETS_OUT);
    else if ( 0 == strcasecmp("bytes_in", (*it).c_str())
           || 0 == strcasecmp("octets_in",(*it).c_str()) )
      metrics.push_back(BYTES_IN);
    else if ( 0 == strcasecmp("bytes_out", (*it).c_str())
           || 0 == strcasecmp("octets_out",(*it).c_str()) )
      metrics.push_back(BYTES_OUT);
    else if ( 0 == strcasecmp("records_in",(*it).c_str()) )
      metrics.push_back(RECORDS_IN);
    else if ( 0 == strcasecmp("records_out",(*it).c_str()) )
      metrics.push_back(RECORDS_OUT);
    else if ( 0 == strcasecmp("octets_in/packet_in",(*it).c_str())
           || 0 == strcasecmp("bytes_in/packet_in",(*it).c_str()) )
      metrics.push_back(BYTES_IN_PER_PACKET_IN);
    else if ( 0 == strcasecmp("octets_out/packet_out",(*it).c_str())
           || 0 == strcasecmp("bytes_out/packet_out",(*it).c_str()) )
      metrics.push_back(BYTES_OUT_PER_PACKET_OUT);
    else if ( 0 == strcasecmp("packets_out-packets_in",(*it).c_str()) )
      metrics.push_back(PACKETS_OUT_MINUS_PACKETS_IN);
    else if ( 0 == strcasecmp("octets_out-octets_in",(*it).c_str())
           || 0 == strcasecmp("bytes_out-bytes_in",(*it).c_str()) )
      metrics.push_back(BYTES_OUT_MINUS_BYTES_IN);
    else if ( 0 == strcasecmp("packets_in(t)-packets_in(t-1)",(*it).c_str()) )
      metrics.push_back(PACKETS_T_IN_MINUS_PACKETS_T_1_IN);
    else if ( 0 == strcasecmp("packets_out(t)-packets_out(t-1)",(*it).c_str()) )
      metrics.push_back(PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT);
    else if ( 0 == strcasecmp("octets_in(t)-octets_in(t-1)",(*it).c_str())
           || 0 == strcasecmp("bytes_in(t)-bytes_in(t-1)",(*it).c_str()) )
      metrics.push_back(BYTES_T_IN_MINUS_BYTES_T_1_IN);
    else if ( 0 == strcasecmp("octets_out(t)-octets_out(t-1)",(*it).c_str())
           || 0 == strcasecmp("bytes_out(t)-bytes_out(t-1)",(*it).c_str()) )
      metrics.push_back(BYTES_T_OUT_MINUS_BYTES_T_1_OUT);
    else {
        std::cerr << Error3.str() << Usage.str() << "  Exiting.\n";
        if (warning_verbosity==1)
          outfile << Error3.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
        exit(0);
      }
    it++;
  }

  // just in case the user provided multiple same values
  // (after these lines, metrics contains the Metrics
  // in the correct order)
  sort(metrics.begin(),metrics.end());
  std::vector<Metric>::iterator new_end = unique(metrics.begin(),metrics.end());
  std::vector<Metric> tmp(metrics.begin(), new_end);
  metrics = tmp;

  // print out, in which order the values will be stored
  // in the Samples-Lists (for better understanding the output
  // of test() etc.
  std::stringstream Information;
  Information
    << "INFORMATION: Values in the lists sample_old and sample_new will be "
    << "stored in the following order:\n";
  std::vector<Metric>::iterator val = metrics.begin();
  Information << "( ";
  while (val != metrics.end() ) {
    Information << getMetricName(*val) << " ";
    val++;
  }
  Information << ")\n";
  std::cerr << Information.str();
    if (warning_verbosity==1)
      outfile << Information.str() << std::flush;

  bool packetsSubscribed = false;
  bool bytesSubscribed = false;
  // subscribing to the needed IPFIX_TYPEID-fields
  for (int i = 0; i != metrics.size(); i++) {
    if ( (metrics.at(i) == PACKETS_IN
      || metrics.at(i) == PACKETS_OUT
      || metrics.at(i) == PACKETS_OUT_MINUS_PACKETS_IN
      || metrics.at(i) == PACKETS_T_IN_MINUS_PACKETS_T_1_IN
      || metrics.at(i) == PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT)
      && packetsSubscribed == false) {
      subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
      packetsSubscribed = true;
    }

    if ( (metrics.at(i) == BYTES_IN
      || metrics.at(i) == BYTES_OUT
      || metrics.at(i) == BYTES_OUT_MINUS_BYTES_IN
      || metrics.at(i) == BYTES_T_IN_MINUS_BYTES_T_1_IN
      || metrics.at(i) == BYTES_T_OUT_MINUS_BYTES_T_1_OUT)
      && bytesSubscribed == false ) {
      subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
      bytesSubscribed = true;
    }

    if ( (metrics.at(i) == BYTES_IN_PER_PACKET_IN
       || metrics.at(i) == BYTES_OUT_PER_PACKET_OUT)
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
      << "  \"" << DEFAULT_noise_threshold_packets << "\" assumed.\n";
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
      << "  \"" << DEFAULT_noise_threshold_packets << "\" assumed.\n";
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
      << "  \"" << DEFAULT_noise_threshold_bytes << "\" assumed.\n";
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
      << "  \"" << DEFAULT_noise_threshold_bytes << "\" assumed.\n";
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
    << "  \"" << DEFAULT_endpointlist_maxsize << "\" assumed.\n";
  Warning1
    << "WARNING: No value for endpointlist_maxsize parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_endpointlist_maxsize << "\" assumed.\n";

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
      << "  All protocols will be monitored (ICMP, TCP, UDP, RAW).\n";
    Warning
      << "WARNING: No value for protocols parameter defined in XML config file!\n"
      << "  All protocols will be monitored (ICMP, TCP, UDP, RAW).\n";
    Usage
      << "  Please use ICMP, TCP, UDP or RAW (or "
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
            << Usage.str() << "  Exiting.\n";
            if (warning_verbosity==1)
              outfile << "ERROR: An unknown value (" << protocol
                << ") for the protocol parameter was defined in XML config file!\n"
                << Usage.str() << "  Exiting." << std::endl << std::flush;
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
      << "  \"32\" assumed.\n";
    Warning
      << "WARNING: No netmask parameter defined in XML config file!\n"
      << "  \"32\" assumed.\n";
    Error
      << "ERROR: Netmask parameter was provided in an unknown format in XML config file!\n";
    Usage
      << "  Use xxx.yyy.zzz.ttt, hexadecimal or an int between 0 and 32.\n";

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
      << "  Every port will be monitored.\n";

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
      << "  I suppose you want me to monitor everything; I'll try.\n";
    Warning1
      << "WARNING: No value for ip_addresses_to_monitor parameter defined in XML config file!\n"
      << "  I suppose you want me to monitor everything; I'll try.\n";

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
        << "  Please check that the file exists, "
        << "and that you have enough rights to read it.\n"
        << "  Exiting.\n";

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
      << "defined in XML config file!\n"
      << "  \"" << DEFAULT_stat_test_frequency << "\" assumed.\n";
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
	    << "defined in XML config file!\n"
	    << "  \"" << DEFAULT_stat_test_frequency << "\" assumed.\n";
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
      << "  \"true\" assumed.\n";
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
      << "  \"true\" assumed.\n";
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
      << "  No pausing assumed.\n";
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
      case 3:
        pause_update_when_attack = 3;
        break;
      default:
        std::stringstream Error;
        Error
          << "ERROR: Unknown value ("
          << config->getValue("pause_update_when_attack")
          << ") for pause_update_when_attack parameter!\n"
          << "  Please chose one of the following and restart:\n"
          << "  0: no pausing\n"
          << "  1: pause, if one test detected an attack\n"
          << "  2: pause, if all tests detected an attack\n"
          << "  3: pause for every metric individually (for cusum only)\n";
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
      << "  No pausing assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    pause_update_when_attack = 0;
  }

  return;
}

void Stat::init_cusum_test(XMLConfObj * config) {
  std::stringstream Warning, Warning1, Warning2, Warning3, Warning4, Warning5, Warning6, Warning7, Warning8, Warning9, Error1, Error2, Error3;
  Warning
    << "WARNING: No cusum_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  Warning1
    << "WARNING: No value for cusum_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  Warning2
    << "WARNING: No amplitude_percentage parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_amplitude_percentage << "\" assumed.\n";
  Warning3
    << "WARNING: No value for amplitude_percentage parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_amplitude_percentage << "\" assumed.\n";
  Warning4
    << "WARNING: No learning_phase_for_alpha parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_alpha << "\" assumed.\n";
  Warning5
    << "WARNING: No value for learning_phase_for_alpha parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_alpha << "\" assumed.\n";
  Error1
    << "ERROR: Value for learning_phase_for_alpha parameter "
    << "is zero or negative.\n  Please define a positive value and restart!\n"
    << "  Exiting.\n";
  Warning6
    << "WARNING: No smoothing_constant parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_smoothing_constant << "\" assumed.\n";
  Warning7
    << "WARNING: No value for smoothing_constant parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_smoothing_constant << "\" assumed.\n";
  Error2
    << "ERROR: Value for smoothing_constant parameter "
    << "is zero or negative.\n  Please define a positive value and restart!\n"
    << "  Exiting.\n";
  Warning8
    << "WARNING: No repetition_factor parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_repetition_factor << "\" assumed.\n";
  Warning9
    << "WARNING: No value for repetition_factor parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_repetition_factor << "\" assumed.\n";
  Error3
    << "ERROR: Value for repetition_factor parameter "
    << "is zero or negative.\n  Please define a positive integer value and restart!\n"
    << "  Exiting.\n";

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
        std::cerr << Error1.str();
        if (warning_verbosity==1)
          outfile << Error1.str() << std::flush;
        exit(0);
      }
    }
    else {
      std::cerr << Warning5.str();
      if (warning_verbosity==1)
        outfile << Warning5.str() << std::flush;
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }

    // smoothing constant for updating alpha per EWMA
    if (!config->nodeExists("smoothing_constant")) {
      std::cerr << Warning6.str();
      if (warning_verbosity==1)
        outfile << Warning6.str() << std::flush;
      smoothing_constant = DEFAULT_smoothing_constant;
    }
    else if (!(config->getValue("smoothing_constant")).empty()) {
      if ( atof(config->getValue("smoothing_constant").c_str()) > 0.0)
        smoothing_constant = atof(config->getValue("smoothing_constant").c_str());
      else {
        std::cerr << Error2.str();
        if (warning_verbosity==1)
          outfile << Error2.str() << std::flush;
        exit(0);
      }
    }
    else {
      std::cerr << Warning7.str();
      if (warning_verbosity==1)
        outfile << Warning7.str() << std::flush;
      smoothing_constant = DEFAULT_smoothing_constant;
    }

    // repetition factor
    if (!config->nodeExists("repetition_factor")) {
      std::cerr << Warning8.str();
      if (warning_verbosity==1)
        outfile << Warning8.str() << std::flush;
      repetition_factor = DEFAULT_repetition_factor;
    }
    else if (!(config->getValue("repetition_factor")).empty()) {
      if ( atoi(config->getValue("repetition_factor").c_str()) > 0)
        repetition_factor = atoi(config->getValue("repetition_factor").c_str());
      else {
        std::cerr << Error3.str();
        if (warning_verbosity==1)
          outfile << Error3.str() << std::flush;
        exit(0);
      }
    }
    else {
      std::cerr << Warning9.str();
      if (warning_verbosity==1)
        outfile << Warning9.str() << std::flush;
      repetition_factor = DEFAULT_repetition_factor;
    }

  }

  return;
}

void Stat::init_which_test(XMLConfObj * config) {

  std::stringstream WMWdefault, WMWdefault1, KSdefault, KSdefault1, PCSdefault, PCSdefault1;
  WMWdefault
    << "WARNING: No wilcoxon_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  WMWdefault1
    << "WARNING: No value for wilcoxon_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  KSdefault
    << "WARNING: No kolmogorov_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  KSdefault1
    << "WARNING: No value for kolmogorov_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  PCSdefault
    << "WARNING: No pearson_chi-square_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";
  PCSdefault1
    << "WARNING: No value for pearson_chi-square_test parameter "
    << "defined in XML config file!\n  \"true\" assumed.\n";

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
    << "WARNING: No sample_old_size parameter defined in XML config file!\n"
	  << "  \"" << DEFAULT_sample_old_size << "\" assumed.\n";
  Default1_old
    << "WARNING: No value for sample_old_size parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_sample_old_size << "\" assumed.\n";
  Default_new
    << "WARNING: No sample_new_size parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_sample_new_size << "\" assumed.\n";
  Default1_new
    << "WARNING: No value for sample_new_size parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_sample_new_size << "\" assumed.\n";

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
      << "  \"false\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      outfile << Default.str() << std::flush;
    two_sided = false;
  }
  else if ( enable_wmw_test == true && config->getValue("two_sided").empty() ) {
    std::stringstream Default;
    Default
      << "WARNING: No value for two_sided parameter defined in XML config file!\n"
      << "  \"false\" assumed.\n";
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
        std::stringstream Error("ERROR: significance_level parameter should be between 0 and 1!\n  Exiting.\n");
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
        << "  Nothing will be interpreted as an attack.\n";
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
      << "  Nothing will be interpreted as an attack.\n";
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

//
// EXPLANATION:
// ##############################################################
// 1 WRITE DATA TO FILE:
//      this code simply writes all the data collected by "store" to a file
// 2 READ DATA FROM FILE TO SEARCH ENDPOINTS
//      this file reads the data from the file created in 1 and finds the
//      most frequently appearing X endpoints and writes them to a file which
//      then can be used as a filter for the next step
// 3 READ DATA FROM FILE AND MAKE ENDPOINT METRIC FILES
//      only the most frequently appearing X endpoints from
//      the file created in 2 are considered (thanks to the filter vector, see
//      constructor) to calculate the metrics for them and write these metrics
//      to a file for every of these X endpoints.
// NOW, WE ARE ABLE TO MAKE SOME DIAGRAMS OF THE DATA AND BETTER UNDERSTAND
// WHAT'S GOING ON. THEN, CUSUM CAN BE APPLIED AND WE KNOW, WHERE IT WILL
// POSSIBLY DETECT ANOMALIES.
//
// 4 DO THE TESTS
//      reads data from file and executes the cusum-test on it. For every endpoint
//      and every metric, it creates a file where alle the cusum-params are stored
//      in. They can be visualized afterwards to be compared to the metric-diagrams
//      from 3 and so on.


/*
  // ++++++++++++++++++++++++++++++++++
  // BEGIN TESTING (1 WRITE DATA TO FILE)
  // ++++++++++++++++++++++++++++++++++
  // NOTE: adapt file name in StatStore::writeToFile() AND
  // comment filter in constructor AND
  // adapt config file (endpoint_key, alarm_time = 10 etc.)
  // AND start vermont ;)
  std::map<EndPoint,Info> Data = store->getData();

  // Dumping empty records:
  if (Data.empty()==true)
    return;

  // write data to file
  store->writeToFile();
  delete store;
  // ++++++++++++++++++++++++++++++++
  // END TESTING (1 WRITE DATA TO FILE)
  // ++++++++++++++++++++++++++++++++
*/



/*
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // BEGIN TESTING (2 READ DATA FROM FILE TO SEARCH ENDPOINTS)
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // read data from file and search the most frequently
  // appearing X endpoints
  // NOTE: dont forget to set alarm_time to 1 AND comment filter in constructor
  // AND adapt filenames (here and in StatStore)
  bool more = store->readFromFile();
  // still more data to read?
  if (more == true) {
    std::map<EndPoint,Info> Data = store->getDataFromFile();
    // Dumping empty records:
    if (Data.empty()==true)
      return;

    // count number of appearings of each endpoint
    // to let us filter the most often appearing ones
    std::map<EndPoint,Info>::iterator Data_it = Data.begin();
    while (Data_it != Data.end()) {
      endPointCount[Data_it->first]++;
      Data_it++;
    }
    std::cout << "Stand: " << counter << std::endl;
    counter++;
    delete store;
  }
  // if all data was read
  else {
    // search the X most frequently appeared endpoints
    int X = 10;
    std::map<EndPoint,int> mostFrequentEndPoints;
    for (int j = 0; j < X; j++) {
      std::pair<EndPoint,int> tmpmax;
      tmpmax.first = (endPointCount.begin())->first;
      tmpmax.second = (endPointCount.begin())->second;
      for (std::map<EndPoint,int>::iterator i = endPointCount.begin(); i != endPointCount.end(); i++) {
        if (tmpmax.second < i->second) {
          tmpmax.first = i->first;
          tmpmax.second = i->second;
        }
      }
      mostFrequentEndPoints.insert(tmpmax);
      endPointCount.erase(tmpmax.first);
    }
    // write the X most frequently endpoints to a file
    // Note: Now, we can use that file to determine the filters ...
    std::ofstream f("darpa1_port_10_freq_eps.txt", std::ios_base::app);
    std::map<EndPoint,int>::iterator iter;
    for (iter=mostFrequentEndPoints.begin(); iter != mostFrequentEndPoints.end(); iter++)
      f << iter->first << "\n";
    f.close();
    std::cout << "Done." << std::endl;
    delete store;
    exit(0);
  }
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++
  // END TESTING (2 READ DATA FROM FILE TO SEARCH ENDPOINTS)
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++
*/



/*
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // BEGIN TESTING (3 READ DATA FROM FILE AND MAKE ENDPOINT METRIC FILES)
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // write metrics from each endpoint to a file
  // so we can make tables of the data with external programs
  // NOTE: Dont forget to delete old metrics.txt-files AND adapt filename for the
  // uncommented filter in the constructor AND for StatStore::readFromFile AND
  // choose metrics to be saved in config file (alarm_time = 1)
  bool more = store->readFromFile();
  if (more == true) {
    std::map<EndPoint,Info> Data = store->getDataFromFile();
    // Dumping empty records:
    if (Data.empty()==true)
      return;

    std::map<EndPoint,Info>::iterator Data_it = Data.begin();
    std::map<EndPoint,Info> PreviousData = store->getPreviousDataFromFile();
    Info prev;

    // for every unfiltered EndPoint, extract the data to its file
    while (Data_it != Data.end()) {
      if ( find(filter.begin(), filter.end(), Data_it->first) != filter.end() ) {
        prev = PreviousData[Data_it->first];
        // extract metric data
        std::vector<int64_t> v = extract_data(Data_it->second, prev);
        // open endpoint's file
        std::string fname = "metrics_" + (Data_it->first).toString() + ".txt";
        std::ofstream file(fname.c_str(),std::ios_base::app);
        // write metric data to file
        for (int i = 0; i != v.size(); i++) {
          file << v.at(i) << " ";
        }
        file << counter << "\n";
        file.close();
      }
      Data_it++;
    }
    std::cout << "Stand: " << counter << std::endl;
    counter++;
    delete store;
  }
  else {
    std::cout << "Done." << std::endl;
    delete store;
    exit(0);
  }
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // END TESTING (3 READ DATA FROM FILE AND MAKE ENDPOINT METRIC FILES)
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/


/*
  // ++++++++++++++++++++++++++++
  // BEGIN TESTING (4 DO THE TESTS)
  // ++++++++++++++++++++++++++++
  // Do the Cusum-Test and print out the parameters to a file for
  // every of the X endpoints and their metrics (to compare it to our metric-diagrams)
  // NOTE: Dont forget to adapt the filename convention for the
  // uncommented filter in the constructor AND for StatStore::readFromFile
  // AND delete old files created by cusum_test() AND  set alarm_time = 1
  // AND chose some params
  bool more = store->readFromFile();
  if (more == true) {
    std::map<EndPoint,Info> Data = store->getDataFromFile();

    if (Data.empty()==true)
      return;

    std::map<EndPoint,Info>::iterator Data_it = Data.begin();

    std::map<EndPoint,Info> PreviousData = store->getPreviousDataFromFile();
    Info prev;

    while (Data_it != Data.end()) {
      // filter (do tests only for the X frequently appeared endpoints)
      if ( find(filter.begin(), filter.end(), Data_it->first) != filter.end() ) {
        outfile << "[[ " << Data_it->first << " ]]" << std::endl;

        prev = PreviousData[Data_it->first];

        if (enable_wkp_test == true) {
          std::map<EndPoint, Samples>::iterator SampleData_it =
            SampleData.find(Data_it->first);

          // initialize S
          if (SampleData_it == SampleData.end()) {
            Samples S;
            if (use_pca == true) {
              S.cov = gsl_matrix_calloc (metrics.size(), metrics.size());
              S.evec = gsl_matrix_calloc (metrics.size(), metrics.size());
              // initialize sumsOfMetrics and sumsOfProducts
              for (int i = 0; i < metrics.size(); i++) {
                S.sumsOfMetrics.push_back(0);
                std::vector<long long int> v;
                S.sumsOfProducts.push_back(v);
                for (int j = 0; j < metrics.size(); j++)
                  S.sumsOfProducts.at(i).push_back(0);
              }
              std::vector<int64_t> v = extract_pca_data(S, Data_it->second, prev);
            }
            else
              (S.Old).push_back(extract_data(Data_it->second, prev));

            for (int i = 0; i != metrics.size(); i++) {
              (S.wmw_alarms).push_back(0);
              (S.ks_alarms).push_back(0);
              (S.pcs_alarms).push_back(0);
            }

            SampleData[Data_it->first] = S;
            if (output_verbosity >= 3) {
              outfile << " (WKP): New monitored EndPoint added" << std::endl;
              if (output_verbosity >= 4) {
                outfile << "   with first element of sample_old: " << S.Old.back() << std::endl;
              }
            }

          }
          else {
            if (use_pca == true) {
              std::vector<int64_t> v = extract_pca_data(SampleData_it->second, Data_it->second, prev);
              update ( SampleData_it->second, v );
            }
            else
              update ( SampleData_it->second, extract_data(Data_it->second, prev) );
          }
        }

        if (enable_cusum_test == true) {
          std::map<EndPoint, CusumParams>::iterator CusumData_it = CusumData.find(Data_it->first);

          // initialize C
          if (CusumData_it == CusumData.end()) {
            CusumParams C;
            for (int i = 0; i != metrics.size(); i++) {
              (C.sum).push_back(0);
              (C.alpha).push_back(0.0);
              (C.g).push_back(0.0);
              (C.last_cusum_test_was_attack).push_back(false);
              (C.cusum_alarms).push_back(0);
              (C.X_curr).push_back(0);
              (C.X_last).push_back(0);
            }

            // extract_data
            std::vector<int64_t> v;
            if (use_pca == true) {
              C.cov = gsl_matrix_calloc (metrics.size(), metrics.size());
              C.evec = gsl_matrix_calloc (metrics.size(), metrics.size());
              // initialize sumsOfMetrics and sumsOfProducts
              for (int i = 0; i < metrics.size(); i++) {
                C.sumsOfMetrics.push_back(0);
                std::vector<long long int> v;
                C.sumsOfProducts.push_back(v);
                for (int j = 0; j < metrics.size(); j++)
                  C.sumsOfProducts.at(i).push_back(0);
              }
              v = extract_pca_data(C, Data_it->second, prev);
            }
            else {
              v = extract_data(Data_it->second, prev);
              for (int i = 0; i != v.size(); i++)
                C.sum.at(i) += v.at(i);
              C.learning_phase_nr_for_alpha = 1;
            }
            CusumData[Data_it->first] = C;
            if (output_verbosity >= 3)
              outfile << " (CUSUM): New monitored EndPoint added" << std::endl;
          }
          else {
            if (use_pca == true) {
              std::vector<int64_t> v = extract_pca_data(CusumData_it->second, Data_it->second, prev);
              update_c ( CusumData_it->second, v );
            }
            else
              update_c ( CusumData_it->second, extract_data(Data_it->second, prev) );
          }
        }
      }
      Data_it++;
    }

    int ep_nr;
    if (enable_wkp_test == true)
      ep_nr = SampleData.size();
    else if (enable_cusum_test == true)
      ep_nr = CusumData.size();

    if (output_verbosity >= 4) {
      outfile << "#### STATE OF ALL MONITORED ENDPOINTS (" << ep_nr << "):" << std::endl;
      if (enable_wkp_test == true) {
        outfile << "### WKP OVERVIEW" << std::endl;
        std::map<EndPoint,Samples>::iterator SampleData_it =
          SampleData.begin();
        while (SampleData_it != SampleData.end()) {
          outfile
            << "[[ " << SampleData_it->first << " ]]\n"
            << "  sample_old (" << (SampleData_it->second).Old.size()  << ") : "
            << (SampleData_it->second).Old << "\n"
            << "  sample_new (" << (SampleData_it->second).New.size() << ") : "
            << (SampleData_it->second).New << "\n";
          SampleData_it++;
        }
      }
      if (enable_cusum_test == true) {
        outfile << "### CUSUM OVERVIEW" << std::endl;
        std::map<EndPoint,CusumParams>::iterator CusumData_it =
          CusumData.begin();
        while (CusumData_it != CusumData.end()) {
          outfile
            << "[[ " << CusumData_it->first << " ]]\n"
            << "  alpha: " << (CusumData_it->second).alpha << "\n"
            << "  g: " << (CusumData_it->second).g << "\n";
          CusumData_it++;
        }
      }
    }

    bool MakeStatTest;
    if (stat_test_frequency == 0)
      MakeStatTest = false;
    else
      MakeStatTest = (test_counter % stat_test_frequency == 0);

    if (MakeStatTest == true) {
      if (enable_wkp_test == true) {
        std::map<EndPoint,Samples>::iterator SampleData_it = SampleData.begin();
        while (SampleData_it != SampleData.end()) {
          if ( ((SampleData_it->second).New).size() == sample_new_size ) {
            outfile << "\n#### WKP TESTS for EndPoint [[ " << SampleData_it->first << " ]]\n";
            T_stat_test ( SampleData_it->first, SampleData_it->second );
          }
          SampleData_it++;
        }
      }
      if (enable_cusum_test == true) {
        std::map<EndPoint,CusumParams>::iterator CusumData_it = CusumData.begin();
        while (CusumData_it != CusumData.end()) {
          if ( (CusumData_it->second).ready_to_test == true ) {
            outfile << "\n#### CUSUM TESTS for EndPoint [[ " << CusumData_it->first << " ]]\n";
            T_cusum_test (CusumData_it->first, CusumData_it->second);
          }
          CusumData_it++;
        }
      }
    }

    test_counter++;
    delete store;
    outfile << std::endl << std::flush;
    return;
  }
  else {
    delete store;
    std::cout << "Done." << std::endl;
    exit(0);
  }
  // ++++++++++++++++++++++++++
  // END TESTING (4 DO THE TESTS)
  // ++++++++++++++++++++++++++
*/




  // ++++++++++++++++++++++
  // BEGIN NORMAL BEHAVIOUR
  // ++++++++++++++++++++++

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

  std::map<EndPoint,Info> PreviousData = store->getPreviousData();
  //std::map<EndPoint,Info> PreviousData = store->getPreviousDataFromFile();
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

    // Do stuff for wkp-tests (if at least one of them is enabled)
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
        if (use_pca == true) {
          S.cov = gsl_matrix_calloc (metrics.size(), metrics.size());
          S.evec = gsl_matrix_calloc (metrics.size(), metrics.size());
          // initialize sumsOfMetrics and sumsOfProducts
          for (int i = 0; i < metrics.size(); i++) {
            S.sumsOfMetrics.push_back(0);
            std::vector<long long int> v;
            S.sumsOfProducts.push_back(v);
            for (int j = 0; j < metrics.size(); j++)
              S.sumsOfProducts.at(i).push_back(0);
          }
          // v is not used, but we need to call extract_pca_data anyhow
          std::vector<int64_t> v = extract_pca_data(S, Data_it->second, prev);
        }
        else
          (S.Old).push_back(extract_data(Data_it->second, prev));

        SampleData[Data_it->first] = S;
        if (output_verbosity >= 3) {
          outfile << " (WKP): New monitored EndPoint added" << std::endl;
          if (output_verbosity >= 4) {
            outfile << "   with first element of sample_old: " << S.Old.back() << std::endl;
          }
        }

      }
      else {
        // We found the recorded EndPoint Data_it->first
        // in our sample container "SampleData"; so we update the samples
        // (SampleData_it->second).Old and (SampleData_it->second).New
        // thanks to the recorded new value in Data_it->second:
        if (use_pca == true) {
          std::vector<int64_t> v = extract_pca_data(SampleData_it->second, Data_it->second, prev);
          update ( SampleData_it->second, v );
        }
        else
          update ( SampleData_it->second, extract_data(Data_it->second, prev) );
      }
    }

    // Do stuff for cusum-test (if enabled)
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
        for (int i = 0; i != metrics.size(); i++) {
          (C.sum).push_back(0);
          (C.alpha).push_back(0.0);
          (C.g).push_back(0.0);
          (C.X_curr).push_back(0);
          (C.X_last).push_back(0);
        }

        // extract_data
        std::vector<int64_t> v;
        if (use_pca == true) {
          C.cov = gsl_matrix_calloc (metrics.size(), metrics.size());
          C.evec = gsl_matrix_calloc (metrics.size(), metrics.size());
          // initialize sumsOfMetrics and sumsOfProducts
          for (int i = 0; i < metrics.size(); i++) {
            C.sumsOfMetrics.push_back(0);
            std::vector<long long int> v;
            C.sumsOfProducts.push_back(v);
            for (int j = 0; j < metrics.size(); j++)
              C.sumsOfProducts.at(i).push_back(0);
          }
          v = extract_pca_data(C, Data_it->second, prev);
        }
        else { // no learning phase for pca ...
          v = extract_data(Data_it->second, prev);
          // add first value of each metric to the sum, which is needed
          // only to calculate the initial alpha after learning_phase_for_alpha
          // is over
          for (int i = 0; i != v.size(); i++)
            C.sum.at(i) += v.at(i);

          C.learning_phase_nr_for_alpha = 1;
        }

        CusumData[Data_it->first] = C;
        if (output_verbosity >= 3)
          outfile << " (CUSUM): New monitored EndPoint added" << std::endl;
      }
      else {
        // We found the recorded EndPoint Data_it->first
        // in our cusum container "CusumData"; so we update alpha
        // thanks to the recorded new values in Data_it->second:
        if (use_pca == true) {
          std::vector<int64_t> v = extract_pca_data(CusumData_it->second, Data_it->second, prev);
          update_c ( CusumData_it->second, v );
        }
        else
          update_c ( CusumData_it->second, extract_data(Data_it->second, prev) );
      }
    }

    Data_it++;
  }

  outfile << std::endl << std::flush;

  // 1.5) MAP PRINTING (OPTIONAL, DEPENDS ON VERBOSITY SETTINGS)

  // how many endpoints do we already monitor?
  int ep_nr;
  if (enable_wkp_test == true)
    ep_nr = SampleData.size();
  else if (enable_cusum_test == true)
    ep_nr = CusumData.size();

  if (output_verbosity >= 4) {
    outfile << "#### STATE OF ALL MONITORED ENDPOINTS (" << ep_nr << "):" << std::endl;

    if (enable_wkp_test == true) {
      outfile << "### WKP OVERVIEW" << std::endl;
      std::map<EndPoint,Samples>::iterator SampleData_it =
        SampleData.begin();
      while (SampleData_it != SampleData.end()) {
        outfile
          << "[[ " << SampleData_it->first << " ]]\n"
          << "  sample_old (" << (SampleData_it->second).Old.size()  << ") : "
          << (SampleData_it->second).Old << "\n"
          << "  sample_new (" << (SampleData_it->second).New.size() << ") : "
          << (SampleData_it->second).New << "\n";
        SampleData_it++;
      }
    }
    if (enable_cusum_test == true) {
      outfile << "### CUSUM OVERVIEW" << std::endl;
      std::map<EndPoint,CusumParams>::iterator CusumData_it =
        CusumData.begin();
      while (CusumData_it != CusumData.end()) {
        outfile
          << "[[ " << CusumData_it->first << " ]]\n"
          << "  alpha: " << (CusumData_it->second).alpha << "\n"
          << "  g: " << (CusumData_it->second).g << "\n";
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
  // don't forget to free the store-object!
  delete store;
  outfile << std::endl << std::flush;
  return;
  // ++++++++++++++++++++
  // END NORMAL BEHAVIOUR
  // ++++++++++++++++++++

}



// =================== FUNCTIONS USED BY THE TEST FUNCTION ====================

// extracts interesting data from StatStore according to metrics:
std::vector<int64_t>  Stat::extract_data (const Info & info, const Info & prev) {

  std::vector<int64_t>  result;

  std::vector<Metric>::iterator it = metrics.begin();

  while (it != metrics.end() ) {

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
        std::cerr << "ERROR: metrics seems to be empty "
        << "or it holds an unknown type which isnt supported yet."
        << "But this shouldnt happen as the init_metrics"
        << "-function handles that.\nExiting.\n";
        exit(0);
    }

    it++;
  }

  return result;
}

// needed for pca to extract the new values (for learning and testing)
// cusum version
std::vector<int64_t> Stat::extract_pca_data (CusumParams & C, const Info & info, const Info & prev) {

  std::vector<int64_t> result;

  // learning phase
  if (C.pca_ready == false) {
    if (C.learning_phase_nr_for_pca < learning_phase_for_pca) {
      // update sumsOfMetrics and sumsOfProducts
      std::vector<int64_t> v = extract_data(info, prev);
      for (int i = 0; i < metrics.size(); i++) {
        C.sumsOfMetrics.at(i) += v.at(i);
        for (int j = 0; j < metrics.size(); j++) {
          // sumsOfProducts is a matrix which holds all the sums
          // of the product of each two metrics;
          // elements beneath the main diagonal (j < i) are irrelevant
          // because of commutativity of multiplication
          // i. e. metric1*metric2 = metric2*metric1
          if (j >= i)
            C.sumsOfProducts.at(i).at(j) += v.at(i)*v.at(j);
        }
      }

      C.learning_phase_nr_for_pca++;
      return result; // empty
    }
    // end of learning phase
    else if (C.learning_phase_nr_for_pca == learning_phase_for_pca) {

      // calculate covariance matrix
      for (int i = 0; i < metrics.size(); i++) {
        for (int j = 0; j < metrics.size(); j++)
          gsl_matrix_set(C.cov,i,j,covariance(C.sumsOfProducts.at(i).at(j),C.sumsOfMetrics.at(i),C.sumsOfMetrics.at(j)));
      }

      // calculate eigenvectors and -values
      gsl_vector *eval = gsl_vector_alloc (metrics.size());
      // some workspace needed for computation
      gsl_eigen_symmv_workspace * w = gsl_eigen_symmv_alloc (metrics.size());
      // computation of eigenvectors (evec) and -values (eval) from
      // covariance matrix (cov)
      gsl_eigen_symmv (C.cov, eval, C.evec, w);
      gsl_eigen_symmv_free (w);
      // sort the eigenvectors by their corresponding eigenvalue
      gsl_eigen_symmv_sort (eval, C.evec, GSL_EIGEN_SORT_VAL_DESC);

      // now, we have our components stored in each column of
      // evec, first column = most important, last column = least important

      // From now on, matrix evec can be used to transform the new arriving data
      C.pca_ready = true; // so this code will never be visited again
      return result; // empty
    }
  }

  // testing phase

  // fetch new metric data
  std::vector<int64_t> v = extract_data(info,prev);
  // transform it into a matrix (needed for multiplication)
  // 1*X matrix with X = #metrics,
  gsl_matrix *new_metric_data = gsl_matrix_calloc (1, metrics.size());
  for (int i = 0; i < metrics.size(); i++)
    gsl_matrix_set(new_metric_data,1,i,v.at(i));

  // matrix multiplication to get the transformed data
  gsl_matrix *transformed_metric_data = gsl_matrix_calloc (1, metrics.size());
  gsl_blas_dgemm (CblasNoTrans, CblasNoTrans,
                       1.0, new_metric_data, C.evec,
                       0.0, transformed_metric_data);

  // transform the matrix with the transformed data back into a vector
  for (int i = 0; i < metrics.size(); i++)
    result.push_back((int64_t) gsl_matrix_get(transformed_metric_data,1,i));

  gsl_matrix_free(new_metric_data);
  gsl_matrix_free(transformed_metric_data);

  return result; // filled with new values
}

// needed for pca to extract the new values (for learning and testing)
// cusum version
std::vector<int64_t> Stat::extract_pca_data (Samples & S, const Info & info, const Info & prev) {

  std::vector<int64_t> result;

  // learning phase
  if (S.pca_ready == false) {
    if (S.learning_phase_nr_for_pca < learning_phase_for_pca) {
      // update sumsOfMetrics and sumsOfProducts
      std::vector<int64_t> v = extract_data(info, prev);
      for (int i = 0; i < metrics.size(); i++) {
        S.sumsOfMetrics.at(i) += v.at(i);
        for (int j = 0; j < metrics.size(); j++) {
          // sumsOfProducts is a matrix which holds all the sums
          // of the product of each two metrics;
          // elements beneath the main diagonal (j < i) are irrelevant
          // because of commutativity of multiplication
          // i. e. metric1*metric2 = metric2*metric1
          if (j >= i)
            S.sumsOfProducts.at(i).at(j) += v.at(i)*v.at(j);
        }
      }

      S.learning_phase_nr_for_pca++;
      return result; // empty
    }
    // end of learning phase
    else if (S.learning_phase_nr_for_pca == learning_phase_for_pca) {

      // calculate covariance matrix
      for (int i = 0; i < metrics.size(); i++) {
        for (int j = 0; j < metrics.size(); j++)
          gsl_matrix_set(S.cov,i,j,covariance(S.sumsOfProducts.at(i).at(j),S.sumsOfMetrics.at(i),S.sumsOfMetrics.at(j)));
      }

      // calculate eigenvectors and -values
      gsl_vector *eval = gsl_vector_alloc (metrics.size());
      // some workspace needed for computation
      gsl_eigen_symmv_workspace * w = gsl_eigen_symmv_alloc (metrics.size());
      // computation of eigenvectors (evec) and -values (eval) from
      // covariance matrix (cov)
      gsl_eigen_symmv (S.cov, eval, S.evec, w);
      gsl_eigen_symmv_free (w);
      // sort the eigenvectors by their corresponding eigenvalue
      gsl_eigen_symmv_sort (eval, S.evec, GSL_EIGEN_SORT_VAL_DESC);

      // now, we have our components stored in each column of
      // evec, first column = most important, last column = least important

      // From now on, evec can be used to transform the new arriving data

      // gsl_matrix_transpose_memcpy (gsl_matrix * dest, const gsl_matrix * src)

      S.pca_ready = true; // so this code will never be visited again
      return result; // empty
    }
  }

  // testing phase

  // fetch new metric data
  std::vector<int64_t> v = extract_data(info,prev);
  // transform it into a matrix (needed for multiplication)
  // 1*X matrix with X = #metrics,
  gsl_matrix *new_metric_data = gsl_matrix_calloc (1, metrics.size());
  for (int i = 0; i < metrics.size(); i++)
    gsl_matrix_set(new_metric_data,1,i,v.at(i));

  // matrix multiplication to get the transformed data
  gsl_matrix *transformed_metric_data = gsl_matrix_calloc (1, metrics.size());
  gsl_blas_dgemm (CblasNoTrans, CblasNoTrans,
                       1.0, new_metric_data, S.evec,
                       0.0, transformed_metric_data);

  // transform the matrix with the transformed data back into a vector
  for (int i = 0; i < metrics.size(); i++)
    result.push_back((int64_t) gsl_matrix_get(transformed_metric_data,1,i));

  gsl_matrix_free(new_metric_data);
  gsl_matrix_free(transformed_metric_data);

  return result; // filled with new values
}


// -------------------------- LEARN/UPDATE FUNCTION ---------------------------

// learn/update function for samples (called everytime test() is called)
//
void Stat::update ( Samples & S, const std::vector<int64_t> & new_value ) {

  // Updates for pca metrics have to wait for the pca learning phase
  // needed to calculate the eigenvectors
  // NOTE: The overall learning phase will thus sum up to
  // learning_phase_pca + leraning_phase_samples
  if (use_pca == true && S.pca_ready == false)
    return;

  // Learning phase?
  if (S.Old.size() != sample_old_size) {

    S.Old.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << " (WKP): Learning phase for sample_old ..." << std::endl;
      if (output_verbosity >= 4) {
        outfile << "   sample_old: " << S.Old << std::endl;
        outfile << "   sample_new: " << S.New << std::endl;
      }
    }

    return;
  }
  else if (S.New.size() != sample_new_size) {

    S.New.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << " (WKP): Learning phase for sample_new..." << std::endl;
      if (output_verbosity >= 4) {
        outfile << "   sample_old: " << S.Old << std::endl;
        outfile << "   sample_new: " << S.New << std::endl;
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
      outfile << " (WKP): Update done (for new sample only)" << std::endl;
      if (output_verbosity >= 4) {
        outfile << "   sample_old: " << S.Old << std::endl;
        outfile << "   sample_new: " << S.New << std::endl;
      }
    }
  }
  // if parameter is 0 (or 3) or there was no attack detected
  // update both samples
  else {
    S.Old.pop_front();
    S.Old.push_back(S.New.front());
    S.New.pop_front();
    S.New.push_back(new_value);

    if (output_verbosity >= 3) {
      outfile << " (WKP): Update done (for both samples)" << std::endl;
      if (output_verbosity >= 4) {
        outfile << "   sample_old: " << S.Old << std::endl;
        outfile << "   sample_new: " << S.New << std::endl;
      }
    }
  }

  outfile << std::flush;
  return;
}


// and the update funciotn for the cusum-test
void Stat::update_c ( CusumParams & C, const std::vector<int64_t> & new_value ) {

  // Updates for pca metrics have to wait for the pca learning phase
  // needed to calculate the eigenvectors
  // NOTE: The overall learning phase will thus sum up to
  // learning_phase_pca + leraning_phase_alpha
  if (use_pca == true && C.pca_ready == false)
    return;

  // Learning phase for alpha?
  // that means, we dont have enough values to calculate alpha
  // until now (and so cannot perform the cusum test)
  if (C.ready_to_test == false) {
    if (C.learning_phase_nr_for_alpha < learning_phase_for_alpha) {

      // update the sum of the values of each metric
      // (needed to calculate the initial alpha)
      for (int i = 0; i != C.sum.size(); i++)
        C.sum.at(i) += new_value.at(i);

      if (output_verbosity >= 3)
        outfile
          << " (CUSUM): Learning phase for alpha ...\n";

      C.learning_phase_nr_for_alpha++;

      return;
    }

    // Enough values? Calculate initial alpha per simple average
    // and set ready_to_test-flag to true (so we never visit this
    // code here again for the current endpoint)
    if (C.learning_phase_nr_for_alpha == learning_phase_for_alpha) {

      if (output_verbosity >= 3)
        outfile << " (CUSUM): Learning phase is over\n"
                << "   Calculated initial alphas: ( ";

      for (int i = 0; i != C.alpha.size(); i++) {
        // alpha = sum(values) / #(values)
        // Note: learning_phase_for_alpha is never 0, because
        // this is handled in init_cusum_test()
        C.alpha.at(i) = C.sum.at(i) / learning_phase_for_alpha;
        C.X_curr.at(i) = new_value.at(i);
        if (output_verbosity >= 3)
          outfile << C.alpha.at(i) << " ";
      }
      if (output_verbosity >= 3)
          outfile << ")\n";

      C.ready_to_test = true;
      return;
    }
  }

  // pausing update for alpha (depending on pause_update parameter)
  bool at_least_one_test_was_attack = false;
  for (int i = 0; i != C.last_cusum_test_was_attack.size(); i++)
    if (C.last_cusum_test_was_attack.at(i) == true)
      at_least_one_test_was_attack = true;

  bool all_tests_were_attacks = true;
  for (int i = 0; i != C.last_cusum_test_was_attack.size(); i++)
    if (C.last_cusum_test_was_attack.at(i) == false)
      all_tests_were_attacks = false;

  // pause, if at least one metric yielded an alarm
  if ( pause_update_when_attack == 1
    && at_least_one_test_was_attack == true) {
    if (output_verbosity >= 3)
      outfile << " (CUSUM): Pausing update for alpha (at least one test was attack)\n";
    // update values for X
    for (int i = 0; i != C.X_curr.size(); i++) {
      C.X_last.at(i) = C.X_curr.at(i);
      C.X_curr.at(i) = new_value.at(i);
    }
    return;
  }
  // pause, if all metrics yielded an alarm
  else if ( pause_update_when_attack == 2
    && all_tests_were_attacks == true) {
    if (output_verbosity >= 3)
      outfile << " (CUSUM): Pausing update for alpha (all tests were attacks)\n";
    // update values for X
    for (int i = 0; i != C.X_curr.size(); i++) {
      C.X_last.at(i) = C.X_curr.at(i);
      C.X_curr.at(i) = new_value.at(i);
    }
    return;
  }
  // pause for those metrics, which yielded an alarm
  // and update only the others
  else if (pause_update_when_attack == 3
    && at_least_one_test_was_attack == true) {
    if (output_verbosity >= 3)
      outfile << " (CUSUM): Pausing update for alpha (for those metrics which were attacks)\n";
    for (int i = 0; i != C.alpha.size(); i++) {
      // update values for X
      C.X_last.at(i) = C.X_curr.at(i);
      C.X_curr.at(i) = new_value.at(i);
      if (C.last_cusum_test_was_attack.at(i) == false) {
        // update alpha
        C.alpha.at(i) = C.alpha.at(i) * (1 - smoothing_constant) + (double) C.X_last.at(i) * smoothing_constant;
      }
    }
    return;
  }


  // Otherwise update all alphas per EWMA
  for (int i = 0; i != C.alpha.size(); i++) {
    // update values for X
    C.X_last.at(i) = C.X_curr.at(i);
    C.X_curr.at(i) = new_value.at(i);
    // update alpha
    C.alpha.at(i) = C.alpha.at(i) * (1 - smoothing_constant) + (double) C.X_last.at(i) * smoothing_constant;
  }

  if (output_verbosity >= 3)
    outfile << " (CUSUM): Update done for alpha: " << C.alpha << "\n";

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

  std::vector<Metric>::iterator it = metrics.begin();

  // for every value (represented by *it) in metrics,
  // do the tests
  short index = 0;
  // as the tests can be performed for several metrics, we have to
  // store, if at least one metric raised an alarm and if so, set
  // the last_test_was_attack-flag to true
  bool wmw_was_attack = false;
  bool ks_was_attack = false;
  bool pcs_was_attack = false;

  while (it != metrics.end()) {

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

// same function as the above one, but for TESTING purposes!
void Stat::T_stat_test (const EndPoint & EP, Samples & S) {

  // Containers for the values of single metrics
  std::list<int64_t> sample_old_single_metric;
  std::list<int64_t> sample_new_single_metric;

  std::vector<Metric>::iterator it = metrics.begin();

  // for every value (represented by *it) in metrics,
  // do the tests
  short index = 0;
  // as the tests can be performed for several metrics, we have to
  // store, if at least one metric raised an alarm and if so, set
  // the last_test_was_attack-flag to true
  bool wmw_was_attack = false;
  bool ks_was_attack = false;
  bool pcs_was_attack = false;

  while (it != metrics.end()) {

    if (output_verbosity >= 4)
      outfile << "### Performing WKP-Tests for metric " << getMetricName(*it) << ":\n";

    sample_old_single_metric = getSingleMetric(S.Old, *it, index);
    sample_new_single_metric = getSingleMetric(S.New, *it, index);

    double p_wmw, p_ks, p_pcs;

    // Wilcoxon-Mann-Whitney test:
    if (enable_wmw_test == true) {
      p_wmw = stat_test_wmw(sample_old_single_metric, sample_new_single_metric, S.last_wmw_test_was_attack);
      if (significance_level > p_wmw)
        (S.wmw_alarms).at(index)++;
      if (S.last_wmw_test_was_attack == true)
        wmw_was_attack = true;
    }

    // Kolmogorov-Smirnov test:
    if (enable_ks_test == true) {
      p_ks = stat_test_ks (sample_old_single_metric, sample_new_single_metric, S.last_ks_test_was_attack);
      if (significance_level > p_ks)
        (S.ks_alarms).at(index)++;
      if (S.last_ks_test_was_attack == true)
        ks_was_attack = true;
    }

    // Pearson chi-square test:
    if (enable_pcs_test == true) {
      p_pcs = stat_test_pcs(sample_old_single_metric, sample_new_single_metric, S.last_pcs_test_was_attack);
      if (significance_level > p_pcs)
        (S.pcs_alarms).at(index)++;
      if (S.last_pcs_test_was_attack == true)
        pcs_was_attack = true;
    }

    std::string filename = "wkpparams_" + EP.toString() + "_" + getMetricName(*it) + ".txt";

    // replace the decimal point by a comma
    // (open office cant handle points in decimal numbers ;) )
    std::stringstream tmp1;
    tmp1 << p_wmw;
    std::string str_p_wmw = tmp1.str();
    std::string::size_type i = str_p_wmw.find('.',0);
    if (i != std::string::npos)
      str_p_wmw.replace(i, 1, 1, ',');

    std::stringstream tmp2;
    tmp2 << p_ks;
    std::string str_p_ks = tmp2.str();
    i = str_p_ks.find('.',0);
    if (i != std::string::npos)
      str_p_ks.replace(i, 1, 1, ',');

    std::stringstream tmp3;
    tmp3 << p_pcs;
    std::string str_p_pcs = tmp3.str();
    i = str_p_pcs.find('.',0);
    if (i != std::string::npos)
      str_p_pcs.replace(i, 1, 1, ',');

    std::ofstream file(filename.c_str(), std::ios_base::app);
    // metric p-value(wmw) #alarms(wmw) p-value(ks) #alarms(ks)
    // p-value(pcs) #alarms(pcs) counter
    file << sample_new_single_metric.back() << " " << str_p_wmw << " "
         << (S.wmw_alarms).at(index) << " " << str_p_ks << " "
         << (S.ks_alarms).at(index) << " " << str_p_pcs << " "
         << (S.pcs_alarms).at(index) << " " << test_counter << "\n";
    file.close();

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

  // we have to store, for which metrics an attack was detected and set
  // the corresponding last_cusum_test_was_attack-flags to true/false
  std::vector<bool> was_attack;
  for (int i = 0; i!= C.last_cusum_test_was_attack.size(); i++)
    was_attack.push_back(false);
  // index, as we cant use the iterator for dereferencing the elements of
  // CusumParams
  int i = 0;
  // current adapted threshold
  double N = 0.0;
  // beta, needed to make (Xn - beta) slightly negative in the mean
  double beta = 0.0;

  for (std::vector<Metric>::iterator it = metrics.begin(); it != metrics.end(); it++) {

    if (output_verbosity >= 4)
      outfile << "### Performing CUSUM-Test for metric " << getMetricName(*it) << ":\n";

    // Calculate N and beta
    N = repetition_factor * (amplitude_percentage * C.alpha.at(i) / 2.0);
    beta = C.alpha.at(i) + (amplitude_percentage * C.alpha.at(i) / 2.0);

    if (output_verbosity >= 4) {
      outfile << " Cusum test returned:\n"
              << "  Threshold: " << N << std::endl;
      outfile << "  reject H0 (no attack) if current value of statistic g > "
              << N << std::endl;
    }

    // TODO(2)
    // "attack still in progress"-message?

    // perform the test and if g > N raise an alarm
    if ( cusum(C.X_curr.at(i), beta, C.g.at(i)) > N ) {

      if (report_only_first_attack == false
        || C.last_cusum_test_was_attack.at(i) == false) {
        outfile
          << "    ATTACK! ATTACK! ATTACK!\n"
          << "    Cusum test says we're under attack (g = " << C.g.at(i) << ")!\n"
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

      was_attack.at(i) = true;

    }

    i++;
  }

  for (int i = 0; i != was_attack.size(); i++) {
    if (was_attack.at(i) == true)
      C.last_cusum_test_was_attack.at(i) = true;
    else
      C.last_cusum_test_was_attack.at(i) = false;
  }

  outfile << std::flush;
  return;
}

// same function as the above one, but for TESTING purposes!
void Stat::T_cusum_test(const EndPoint & EP, CusumParams & C) {

  // we have to store, for which metrics an attack was detected and set
  // the corresponding last_cusum_test_was_attack-flags to true/false
  std::vector<bool> was_attack;
  for (int i = 0; i!= C.last_cusum_test_was_attack.size(); i++)
    was_attack.push_back(false);
  // index, as we cant use the iterator for dereferencing the elements of
  // CusumParams
  int i = 0;
  // current adapted threshold
  double N = 0.0;
  // beta, needed to make (Xn - beta) slightly negative in the mean
  double beta = 0.0;

  for (std::vector<Metric>::iterator it = metrics.begin(); it != metrics.end(); it++) {

    if (output_verbosity >= 4)
      outfile << "### Performing CUSUM-Test for metric " << getMetricName(*it) << ":\n";

    // Calculate N and beta
    N = repetition_factor * (amplitude_percentage * C.alpha.at(i) / 2.0);
    beta = C.alpha.at(i) + (amplitude_percentage * C.alpha.at(i) / 2.0);

    if (output_verbosity >= 4) {
      outfile << " Cusum test returned:\n"
              << "  Threshold: " << N << std::endl;
      outfile << "  reject H0 (no attack) if current value of statistic g > "
              << N << std::endl;
    }

    // TODO(2)
    // "attack still in progress"-message?

    // perform the test and if g > N raise an alarm
    if ( cusum(C.X_curr.at(i), beta, C.g.at(i)) > N ) {

      if (report_only_first_attack == false
        || C.last_cusum_test_was_attack.at(i) == false) {
        outfile
          << "    ATTACK! ATTACK! ATTACK! (" << test_counter << ")\n"
          << "    " << EP.toString() << " for metric " << getMetricName(*it) << "\n"
          << "    Cusum test says we're under attack (g = " << C.g.at(i) << ")!\n"
          << "    ALARM! ALARM! Women und children first!" << std::endl;
        std::cout
          << "  ATTACK! ATTACK! ATTACK! (" << test_counter << ")\n"
          << "  " << EP.toString() << " for metric " << getMetricName(*it) << "\n"
          << "  Cusum test says we're under attack!\n"
          << "  ALARM! ALARM! Women und children first!" << std::endl;
        #ifdef IDMEF_SUPPORT_ENABLED
          idmefMessage.setAnalyzerAttr("", "", "cusum-test", "");
          sendIdmefMessage("DDoS", idmefMessage);
          idmefMessage = getNewIdmefMessage();
        #endif
      }

      (C.cusum_alarms).at(i)++;
      was_attack.at(i) = true;

    }

    // BEGIN TESTING
    std::string filename = "cusumparams_" + EP.toString() + "_" + getMetricName(*it) + ".txt";
    std::ofstream file(filename.c_str(), std::ios_base::app);
    // X  g N alpha beta #alarms counter
    file << (int) C.X_curr.at(i) << " " << (int) C.g.at(i)
         << " " << (int) N << " " << (int) C.alpha.at(i) << " "  << (int) beta
         << " " << (C.cusum_alarms).at(i) << " " << test_counter << "\n";
    file.close();
    // END TESTING

    i++;
  }

  for (int i = 0; i != was_attack.size(); i++) {
    if (was_attack.at(i) == true)
      C.last_cusum_test_was_attack.at(i) = true;
    else
      C.last_cusum_test_was_attack.at(i) = false;
  }

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
                << "init_metrics() should not let this Error happen!";
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
      return std::string("bytes_in_per_packet_in");
    case BYTES_OUT_PER_PACKET_OUT:
      return std::string("bytes_out_per_packet_out");
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

double Stat::stat_test_wmw (std::list<int64_t> & sample_old,
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
          << "    ATTACK! ATTACK! ATTACK!(" << test_counter << ")\n"
		      << "    Wilcoxon-Mann-Whitney test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout
          << "  ATTACK! ATTACK! ATTACK!(" << test_counter << ")\n"
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

  return p;
}

double Stat::stat_test_ks (std::list<int64_t> & sample_old,
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
	      outfile << "    ATTACK! ATTACK! ATTACK!(" << test_counter << ")\n"
		      << "    Kolmogorov-Smirnov test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout << "  ATTACK! ATTACK! ATTACK!(" << test_counter << ")\n"
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

  return p;
}


double Stat::stat_test_pcs (std::list<int64_t> & sample_old,
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
	      outfile << "    ATTACK! ATTACK! ATTACK!(" << test_counter << ")\n"
		      << "    Pearson chi-square test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout << "  ATTACK! ATTACK! ATTACK!(" << test_counter << ")\n"
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

  return p;
}

double Stat::covariance (const long long int & sumProduct, const int & sumX, const int & sumY) {
  // calculate the covariance
  // KOV = 1/N-1 * sum(x1 - n1)(x2 - n2)
  // = 1/N-1 * sum(x1x2 - x1n2 - x2n1 + n1n2)
  // = 1/N-1 * (sum(x1x2) - n2*sum(x1) - n1*sum(x2) + N*n1n2)
  // with n1 = 1/N * sum(x1) and n2 = 1/N * sum(x2)
  // KOV = 1/N-1 * ( sum(x1x2) - 1/N * (sum(x1)sum(x2)) )
  return (sumProduct - (sumX*sumY) / learning_phase_for_pca) / (learning_phase_for_pca - 1);
}

void Stat::sigTerm(int signum)
{
	stop();
}

void Stat::sigInt(int signum)
{
	stop();
}
