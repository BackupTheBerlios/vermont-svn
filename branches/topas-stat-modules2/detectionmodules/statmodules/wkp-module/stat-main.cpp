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

// for pca (eigenvectors etc.)
#include <gsl/gsl_eigen.h>
// for pca (matrix multiplication)
#include <gsl/gsl_blas.h>

// for directory functions (mkdir, chdir ...)
#ifdef __unix__
   #include <sys/types.h>
   #include <sys/stat.h>
#elif __WIN32__ || _MS_DOS_
    #include <dir.h>
#endif

#include<signal.h>

#include "stat-main.h"
#include "wmw-test.h"
#include "ks-test.h"
#include "pcs-test.h"
#include "cusum-test.h"

// ==================== CONSTRUCTOR FOR CLASS Stat ====================


Stat::Stat(const std::string & configfile)
#ifdef OFFLINE_ENABLED
      : DetectionBase<StatStore, OfflineInputPolicy<StatStore> >(configfile)
#else
      : DetectionBase<StatStore>(configfile)
#endif
{

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

  // parameter initialization
  init(configfile);

#ifdef OFFLINE_ENABLED
  /* open file with offline data */
  if(!OfflineInputPolicy<StatStore>::openOfflineFile(offlineFile.c_str())) {
      std::cerr << "ERROR: Could not open offline data file!\n Exiting." << std::endl;
      stop();
  }
#else
  if (0 != strcasecmp(offlineFile.c_str(), "none"))
    /* open file to store data for offline use */
    storefile.open(offlineFile.c_str());
#endif

}


Stat::~Stat() {

  logfile.close();
  if(storefile.is_open())
      storefile.close();

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
    stop();
  }

  if (!config->nodeExists("cusum-test-params")) {
    std::cerr
      << "ERROR: No cusum-test-params node defined in XML config file!\n"
      << "  Define one, fill it with some parameters and restart.\n"
      << "  Exiting.\n";
    delete config;
    stop();
  }

  if (!config->nodeExists("wkp-test-params")) {
    std::cerr
      << "ERROR: No wkp-test-params node defined in XML config file!\n"
      << "  Define one, fill it with some parameters and restart.\n"
      << "  Exiting.\n";
    delete config;
    stop();
  }

  // ATTENTION:
  // the order of the following operations is important,
  // as some of these functions use Stat members initialized
  // by the preceding functions; so beware if you change the order

  config->enterNode("preferences");

  // extracting logfile's name and output verbosity
  init_logfile(config);

#ifndef OFFLINE_ENABLED
  // extracting source id's to accept
  init_accepted_source_ids(config);
#endif

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
#ifdef OFFLINE_ENABLED
  setAlarmTime(0);
#else
  init_alarm_time(config);
#endif

  // extracting warning verbosity
  init_warning_verbosity(config);

  // extract filename for the file where
  // all the data will be written into
  init_offline_file(config);

#ifndef OFFLINE_ENABLED
  // extracting the key of the endpoints
  // and thus determining, which IPFIX_TYPEIDs we need to subscribe to
  // NOTE: only needed for ONLINE MODE
  init_endpoint_key(config);

  // extracting the netmask, which will be applied
  // to the ip of each endpoint; for aggregating
  init_netmask(config);
#endif

  // initialize pca parameters
  init_pca(config);

  // extracting metrics
  init_metrics(config);

  // extract directory name for
  // putput files of metrics and test-params
  init_output_dir(config);

  // extracting noise reduction preferences
  init_noise_thresholds(config);

  // extract the maximum size of the endpoint list
  // i. e. how many endpoints can be monitored
  init_endpointlist_maxsize(config);

  // extracting endpoints to monitor
  init_endpoints_to_monitor(config);

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
      logfile << Error.str() << std::flush;
    delete config;
    stop();
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


void Stat::init_logfile(XMLConfObj * config) {

  // extracting output file's name
  if(!config->nodeExists("logfile")) {
    std::cerr
      << "WARNING: No logfile parameter defined in XML config file!\n"
      << "  Default outputfile used (wkp_log.txt).\n";
    logfile.open("wkp_log.txt");
  }
  else if (!(config->getValue("logfile")).empty())
    logfile.open(config->getValue("logfile").c_str());
  else {
    std::cerr
      << "WARNING: No value for logfile parameter defined in XML config file!\n"
      << "  Default logfile used (wkp_log.txt).\n";
    logfile.open("wkp_log.txt");
  }

  if (!logfile) {
      std::cerr << "ERROR: could not open wkp-module's logfile!\n"
        << "  Check if you have enough rights to create or write to it.\n"
        << "  Exiting.\n";
      stop();
  }

  return;
}

#ifndef OFFLINE_ENABLED

void Stat::init_accepted_source_ids(XMLConfObj * config) {

  if (!config->nodeExists("accepted_source_ids")) {
    std::stringstream Warning;
    Warning
      << "WARNING: No accepted_source_ids parameter defined in XML config file!\n"
      << "  All source ids will be accepted.\n";
    std::cerr << Warning.str();
    logfile << Warning.str() << std::flush;
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
    logfile << Warning.str() << std::flush;
  }

  return;
}

#endif


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
    logfile << Warning.str() << std::flush;
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
    logfile << Warning.str() << std::flush;
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
      logfile << Error.str() << Usage.str() << "  Exiting.\n" << std::flush;
      stop();
    }
  }
  else {
    std::cerr << Default.str() << Usage.str();
    warning_verbosity = DEFAULT_warning_verbosity;
  }

  return;
}

void Stat::init_logfile_output_verbosity(XMLConfObj * config) {

  std::stringstream Error, Warning, Default, Usage;
  Error
    << "ERROR: logfile_output_verbosity parameter defined in XML config file "
	  << "should be between 0 and 5.\n"
    << "  Please define it that way and restart.\n";
  Warning
    << "WARNING: No logfile_output_verbosity parameter defined "
    << "in XML config file!\n"
    << "  \"" << DEFAULT_logfile_output_verbosity << "\" assumed.\n";
  Default
    << "WARNING: No value for logfile_output_verbosity parameter defined "
	  << "in XML config file!\n"
	  << "  \"" << DEFAULT_logfile_output_verbosity << "\" assumed.\n";
  Usage
    << "  O: no output generated\n"
	  << "  1: only p-values and attacks are recorded\n"
	  << "  2: same as 1, plus some cosmetics\n"
	  << "  3: same as 2, plus learning phases, updates and empty records events\n"
	  << "  4: same as 3, plus sample printing\n"
	  << "  5: same as 4, plus all details from statistical tests\n";

  // extracting output verbosity
  if(!config->nodeExists("logfile_output_verbosity")) {
    std::cerr << Warning.str() << Usage.str();
    if (warning_verbosity==1)
      logfile << Warning.str() << Usage.str() << std::flush;
    logfile_output_verbosity = DEFAULT_logfile_output_verbosity;
  }
  else if (!(config->getValue("logfile_output_verbosity")).empty()) {
    if ( 0 <= atoi( config->getValue("logfile_output_verbosity").c_str() )
      && 5 >= atoi( config->getValue("logfile_output_verbosity").c_str() ) )
      logfile_output_verbosity = atoi( config->getValue("logfile_output_verbosity").c_str() );
    else {
      std::cerr << Error.str() << Usage.str() << "  Exiting.\n";
      if (warning_verbosity==1)
        logfile << Error.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
      stop();
    }
  }
  else {
    std::cerr << Default.str() << Usage.str();
    if (warning_verbosity==1)
      logfile << Default.str() << Usage.str() << std::flush;
    logfile_output_verbosity = DEFAULT_logfile_output_verbosity;
  }

  return;
}

void Stat::init_offline_file(XMLConfObj * config) {
  std::stringstream Warning, Default, Error1, Error2;
  Warning
    << "WARNING: No offline_file parameter in XML config file!\n"
    << "  The file will be named \"data.txt\".\n";
  Default
    << "WARNING: No value defined for offline_file parameter in XML config file!\n"
    << "  The file will be named \"data.txt\".\n";
  Error1
    << "ERROR: No offline_file parameter in XML config file!\n"
    << "  Please specify one and restart.\n  Exiting.\n";
  Error2
    << "ERROR: No value for offline_file parameter in XML config file!\n"
    << "  Please specify one and restart.\n  Exiting.\n";

  if (!config->nodeExists("offline_file")) {
#ifdef OFFLINE_ENABLED
    std::cerr << Error1.str();
    if (warning_verbosity==1)
      logfile << Error1.str() << std::flush;
    stop();
#else
    std::cerr << Warning.str();
    if (warning_verbosity==1)
      logfile << Warning.str() << std::flush;
    offlineFile = "data.txt";
#endif
  }
  else if ((config->getValue("offline_file")).empty()) {
#ifdef OFFLINE_ENABLED
    std::cerr << Error2.str();
    if (warning_verbosity==1)
      logfile << Error2.str() << std::flush;
    stop();
#else
    std::cerr << Default.str();
    if (warning_verbosity==1)
      logfile << Default.str() << std::flush;
    offlineFile = "data.txt";
#endif
  }
  else if (0 == strcasecmp(config->getValue("offline_file").c_str(),"none")) {
    offlineFile = "none";
  }
  else {
    offlineFile = config->getValue("offline_file");
  }

  return;
}

#ifndef OFFLINE_ENABLED

void Stat::init_endpoint_key(XMLConfObj * config) {

  std::stringstream Warning, Error, Default;
  Warning
    << "WARNING: No endpoint_key parameter in XML config file!\n"
    << "  endpoint_key will be \"none\", i. e. that all traffic will be aggregated to endpoint 0.0.0.0:0|0";
  Error
    << "ERROR: Unknown value defined for endpoint_key parameter in XML config file!\n"
    << "  Value should be either port, ip, protocol OR any combination of them (seperated via spaces) OR \"all\" OR \"none\"!\n";
  Default
    << "WARNING: No value for endpoint_key parameter defined in XML config file!\n"
    << "  endpoint_key will be \"none\", i. e. that all traffic will be aggregated to endpoint 0.0.0.0:0|0";

  // extracting key of endpoints to monitor
  if (!config->nodeExists("endpoint_key")) {
    std::cerr << Warning.str() << "\n";
    if (warning_verbosity==1)
      logfile << Warning.str() << std::endl << std::flush;
  }
  else if (!(config->getValue("endpoint_key")).empty()) {

    std::string Keys = config->getValue("endpoint_key");

    if ( 0 == strcasecmp(Keys.c_str(), "all") ) {
      subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
      subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
      subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
      subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
      subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
      return;
    }

    // all traffic will be aggregated to endpoint 0.0.0.0:0|0
    if ( 0 == strcasecmp(Keys.c_str(), "none") )
      return;

    std::istringstream KeyStream (Keys);
    std::string key;

    while (KeyStream >> key) {
      if ( 0 == strcasecmp(key.c_str(), "ip") ) {
        subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
        subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
      }
      else if ( 0 == strcasecmp(key.c_str(), "port") ) {
        subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
        subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
      }
      else if ( 0 == strcasecmp(key.c_str(), "protocol") ) {
        subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
      }
      else {
        std::cerr << Error.str() << "  Exiting.\n";
        if (warning_verbosity==1)
          logfile << Error.str() << "  Exiting." << std::endl << std::flush;
        stop();
      }
    }
  }
  // all traffic will be aggregated to endpoint 0.0.0.0:0|0
  else {
    std::cerr << Default.str() << "\n";
    if (warning_verbosity==1)
      logfile << Default.str() << std::endl << std::flush;
  }

  return;
}

void Stat::init_netmask(XMLConfObj * config) {

  std::stringstream Default, Warning, Error;
  Default
    << "WARNING: No netmask parameter defined in XML config file!\n"
    << "  \"32\" assumed.\n";
  Warning
    << "WARNING: No value for netmask parameter defined in XML config file!\n"
    << "  \"32\" assumed.\n";
  Error
    << "ERROR: Unknown value for netmask parameter in XML config file!\n"
    << "  Please choose a value between 0 and 32 and restart.\n"
    << "  Exiting.\n";

  if (!config->nodeExists("netmask")) {
    std::cerr << Default.str();
    if (warning_verbosity == 1)
      logfile << Default.str();
    StatStore::setNetmask() = 32;
  }
  else if ((config->getValue("netmask")).empty()) {
    std::cerr << Warning.str();
    if (warning_verbosity == 1)
      logfile << Warning.str();
    StatStore::setNetmask() = 32;
  }

  if (atoi(config->getValue("netmask").c_str()) >= 0 && atoi(config->getValue("netmask").c_str()) <= 32)
    StatStore::setNetmask() = atoi(config->getValue("netmask").c_str());
  else {
    std::cerr << Error.str();
    if (warning_verbosity == 1)
      logfile << Error.str();
    stop();
  }

  return;
}

#endif

// initialize pca
void Stat::init_pca(XMLConfObj * config) {

  if (!config->nodeExists("use_pca")) {
    use_pca = false;
    return;
  }
  else if ((config->getValue("use_pca").empty())) {
    use_pca = false;
    return;
  }


  std::stringstream Default2, Default3;
  Default2
    << "WARNING: No pca_learning_phase parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_pca << "\" assumed.\n";
  Default3
    << "WARNING: No value for pca_learning_phase parameter defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_pca << "\" assumed.\n";


  use_pca = ( 0 == strcasecmp("true", config->getValue("use_pca").c_str()) ) ? true:false;

  // initialize the other parameters only if pca is used
  if (use_pca == true) {

    // learning_phase
    if (!config->nodeExists("pca_learning_phase")) {
      std::cerr << Default2.str();
      if (warning_verbosity==1)
        logfile << Default2.str() << std::flush;
      learning_phase_for_pca = DEFAULT_learning_phase_for_pca;
    }
    else if ( !(config->getValue("pca_learning_phase").empty()) ) {
      learning_phase_for_pca = atoi(config->getValue("pca_learning_phase").c_str());
    }
    else {
      std::cerr << Default3.str();
      if (warning_verbosity==1)
        logfile << Default3.str() << std::flush;
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
    << "  bytes_in/packet_in, bytes_out/packet_out, packets_in/record_in, "
    << "  packets_out/record_out, bytes_in/record_in, bytes_out/record_out, "
    << "  packets_out-packets_in, bytes_out-bytes_in, packets_in(t)-packets_in(t-1), "
    << "  packets_out(t)-packets_out(t-1), bytes_in(t)-bytes_in(t-1) or "
    << "  bytes_out(t)-bytes_out(t-1).\n";

  if (!config->nodeExists("metrics")) {
    std::cerr << Error1.str() << Usage.str() << "  Exiting.\n";
    if (warning_verbosity==1)
      logfile << Error1.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
    stop();
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
      logfile << Error2.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
    stop();
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
    else if ( 0 == strcasecmp("packets_in/record_in",(*it).c_str()) )
      metrics.push_back(PACKETS_IN_PER_RECORD_IN);
    else if ( 0 == strcasecmp("packets_out/record_out",(*it).c_str()) )
      metrics.push_back(PACKETS_OUT_PER_RECORD_OUT);
    else if ( 0 == strcasecmp("octets_in/record_in",(*it).c_str())
           || 0 == strcasecmp("bytes_in/record_in",(*it).c_str()) )
      metrics.push_back(BYTES_IN_PER_RECORD_IN);
    else if ( 0 == strcasecmp("octets_out/record_out",(*it).c_str())
           || 0 == strcasecmp("bytes_out/record_out",(*it).c_str()) )
      metrics.push_back(BYTES_OUT_PER_RECORD_OUT);
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
          logfile << Error3.str() << Usage.str() << "  Exiting." << std::endl << std::flush;
        stop();
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
  if (use_pca == false)
    Information
    << "INFORMATION: Tests will be performed directly on the metrics.\n"
    << "Values in the lists sample_old and sample_new will be "
    << "stored in the following order:\n";
  else
    Information
    << "INFORMATION: Tests will be performed on the principal components of the following metrics:\n";
  std::vector<Metric>::iterator val = metrics.begin();
  Information << "( ";
  while (val != metrics.end() ) {
    Information << getMetricName(*val) << " ";
    val++;
  }
  Information << ")\n";
  std::cerr << Information.str();
    if (warning_verbosity==1)
      logfile << Information.str() << std::flush;

#ifndef OFFLINE_ENABLED
  bool packetsSubscribed = false;
  bool bytesSubscribed = false;
  // subscribing to the needed IPFIX_TYPEID-fields
  for (int i = 0; i != metrics.size(); i++) {
    if ( (metrics.at(i) == PACKETS_IN
      || metrics.at(i) == PACKETS_OUT
      || metrics.at(i) == PACKETS_OUT_MINUS_PACKETS_IN
      || metrics.at(i) == PACKETS_T_IN_MINUS_PACKETS_T_1_IN
      || metrics.at(i) == PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT
      || metrics.at(i) == PACKETS_IN_PER_RECORD_IN
      || metrics.at(i) == PACKETS_OUT_PER_RECORD_OUT)
      && packetsSubscribed == false) {
      subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);
      packetsSubscribed = true;
    }

    if ( (metrics.at(i) == BYTES_IN
      || metrics.at(i) == BYTES_OUT
      || metrics.at(i) == BYTES_OUT_MINUS_BYTES_IN
      || metrics.at(i) == BYTES_T_IN_MINUS_BYTES_T_1_IN
      || metrics.at(i) == BYTES_T_OUT_MINUS_BYTES_T_1_OUT
      || metrics.at(i) == BYTES_IN_PER_RECORD_IN
      || metrics.at(i) == BYTES_OUT_PER_RECORD_OUT)
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
#endif

  return;
}

void Stat::init_output_dir(XMLConfObj * config) {

  createFiles = false;

  if (!config->nodeExists("output_dir"))
    return;
  else if((config->getValue("output_dir")).empty())
    return;

  output_dir = config->getValue("output_dir");

  if (mkdir(output_dir.c_str(), 0777) == -1) {
    std::cerr << "WARNING: Directory \"" << output_dir << "\" couldn't be created. It either already "
      << "exists or you dont't have enough rights.\nNo output files will be generated!\n";
    return;
  }

  createFiles = true;
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
      logfile << Default1.str() << std::flush;
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
      logfile << Default1.str() << std::flush;
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
      logfile << Default2.str() << std::flush;
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
      logfile << Default2.str() << std::flush;
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
      logfile << Warning.str() << std::flush;
    endpointlist_maxsize = DEFAULT_endpointlist_maxsize;
  }
  else if (!(config->getValue("endpointlist_maxsize")).empty())
    endpointlist_maxsize = atoi( config->getValue("endpointlist_maxsize").c_str() );
  else {
    std::cerr << Warning1.str();
    if (warning_verbosity==1)
      logfile << Warning1.str() << std::flush;
    endpointlist_maxsize = DEFAULT_endpointlist_maxsize;
  }

  StatStore::setEndPointListMaxSize() = endpointlist_maxsize;

  return;
}

void Stat::init_endpoints_to_monitor(XMLConfObj * config) {

// OFFLINE MODE
#ifdef OFFLINE_ENABLED
// if parameter <x_frequently_endpoints> was provided,
// read data from file and find the most X frequently appearing
// EndPoints to create EndPointFilter
// otherwise use the endpoints_to_filter parameter
  if (config->nodeExists("x_frequently_endpoints")) {

    std::stringstream Warning;
      Warning << "WARNING: No value for x_frequently_endpoints parameter defined in XML "
      << "config file!\n"
      << "  \"" << DEFAULT_x_frequent_endpoints << "\" assumed.\n";

    if ((config->getValue("x_frequently_endpoints")).empty()) {
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        logfile << Warning.str() << std::flush;
      x_frequently_endpoints = DEFAULT_x_frequent_endpoints;
    }
    else
      x_frequently_endpoints = atoi((config->getValue("x_frequently_endpoints")).c_str());

    std::ifstream dataFile;
    dataFile.open(offlineFile.c_str());
    if (!dataFile) {
      std::stringstream Error;
      Error << "ERROR: Could't open offline file.\n  Exiting.\n";
      std::cerr << Error.str();
      if (warning_verbosity==1)
        logfile << Error.str() << std::flush;
      stop();
    }

    while (true) {
      std::vector<FilterEndPoint> tmpData;
      std::string tmp;

      if ( dataFile.eof() ) {
        std::cerr << "INFORMATION: All Data read from file.\n";
        dataFile.close();
        break;
      }

      while ( getline(dataFile, tmp) ) {

        if (0 == strcasecmp("---",tmp.c_str()) )
          break;

        // extract endpoint-data
        FilterEndPoint fep;
        fep.fromString(tmp, false);
        tmpData.push_back(fep);

        tmp.clear();
      }

      // count number of appearings of each EndPoint
      std::vector<FilterEndPoint>::iterator tmpData_it = tmpData.begin();
      while (tmpData_it != tmpData.end()) {
        endPointCount[*tmpData_it]++;
        tmpData_it++;
      }
    }

    // if all data was read
    // search the X most frequently appeared endpoints
    std::map<FilterEndPoint,int> mostFrequentEndPoints;
    for (int j = 0; j < x_frequently_endpoints; j++) {
      std::pair<FilterEndPoint,int> tmpmax;
      tmpmax.first = (endPointCount.begin())->first;
      tmpmax.second = (endPointCount.begin())->second;
      for (std::map<FilterEndPoint,int>::iterator i = endPointCount.begin(); i != endPointCount.end(); i++) {
        if (tmpmax.second < i->second) {
          tmpmax.first = i->first;
          tmpmax.second = i->second;
        }
      }
      mostFrequentEndPoints.insert(tmpmax);
      endPointCount.erase(tmpmax.first);
    }

    // push these endpoints in the endPointFilter
    std::map<FilterEndPoint,int>::iterator iter;
    for (iter = mostFrequentEndPoints.begin(); iter != mostFrequentEndPoints.end(); iter++)
      StatStore::AddEndPointToFilter(iter->first);

    return;
  }
#endif

// ONLINE MODE + OFFLINE MODE (if no <x_frequently_endpoints> parameter is provided)
// create EndPointFilter from a file specified in XML config file
// if no such file is specified --> MonitorEveryEndPoint = true
// The endpoints in the file are in the format
// ip1.ip2.ip3.ip4/netmask:port|protocol, e. g. 192.13.17.1/24:80|6
  std::stringstream Default, Warning;
    Default
      << "WARNING: No endpoints_to_monitor parameter defined in XML config file!\n"
      << "  All endpoints will be monitored.\n";
    Warning
      << "WARNING: No value for endpoints_to_monitor parameter defined in XML "
      << "config file!\n"
      << "  All endpoints will be monitored.\n";

  if (!config->nodeExists("endpoints_to_monitor")) {
    std::cerr << Default.str();
    if (warning_verbosity==1)
      logfile << Default.str() << std::flush;
    StatStore::setMonitorEveryEndPoint() = true;
    return;
  }
  else if ((config->getValue("endpoints_to_monitor")).empty()) {
    std::cerr << Warning.str();
    if (warning_verbosity==1)
      logfile << Warning.str() << std::flush;
    StatStore::setMonitorEveryEndPoint() = true;
    return;
  }
  else if (0 == strcasecmp("all", config->getValue("endpoints_to_monitor").c_str())) {
    StatStore::setMonitorEveryEndPoint() = true;
    return;
  }

  std::ifstream f(config->getValue("endpoints_to_monitor").c_str());
  if (f.is_open() == false) {
    std::stringstream Error;
    Error << "ERROR: The endpoints_to_monitor file ("
      << config->getValue("endpoints_to_monitor")
      << ") couldnt be opened!\n  Please define the correct filter file "
      << "or set this parameter to \"all\" to consider every EndPoint.\n  Exiting.\n";
    std::cerr << Error.str();
    if (warning_verbosity==1)
      logfile << Error.str() << std::flush;
    stop();
  }
  std::string tmp;
  while ( getline(f, tmp) ) {
    FilterEndPoint fep;
    fep.fromString(tmp, true); // with netmask applied
    StatStore::AddEndPointToFilter(fep);
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
      logfile << Default.str() << std::flush;
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
      logfile << Default.str() << std::flush;
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
      logfile << Default.str() << std::flush;
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
      logfile << Default.str() << std::flush;
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
      logfile << Default.str() << std::flush;
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
          logfile << Error.str() << std::flush;
        stop();
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
      logfile << Default.str() << std::flush;
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
      logfile << Warning.str() << std::flush;
    enable_cusum_test = true;
  }
  else if (!(config->getValue("cusum_test")).empty()) {
    enable_cusum_test = ( 0 == strcasecmp("true", config->getValue("cusum_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << Warning1.str();
    if (warning_verbosity==1)
      logfile << Warning1.str() << std::flush;
    enable_cusum_test = true;
  }

  // the other parameters need only to be initialized,
  // if the cusum-test is enabled
  if (enable_cusum_test == true) {
    // amplitude_percentage
    if (!config->nodeExists("amplitude_percentage")) {
      std::cerr << Warning2.str();
      if (warning_verbosity==1)
        logfile << Warning2.str() << std::flush;
      amplitude_percentage = DEFAULT_amplitude_percentage;
    }
    else if (!(config->getValue("amplitude_percentage")).empty())
      amplitude_percentage = atof(config->getValue("amplitude_percentage").c_str());
    else {
      std::cerr << Warning3.str();
      if (warning_verbosity==1)
        logfile << Warning3.str() << std::flush;
      amplitude_percentage = DEFAULT_amplitude_percentage;
    }

    // learning_phase_for_alpha
    if (!config->nodeExists("learning_phase_for_alpha")) {
      std::cerr << Warning4.str();
      if (warning_verbosity==1)
        logfile << Warning4.str() << std::flush;
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }
    else if (!(config->getValue("learning_phase_for_alpha")).empty()) {
      if ( atoi(config->getValue("learning_phase_for_alpha").c_str()) > 0)
        learning_phase_for_alpha = atoi(config->getValue("learning_phase_for_alpha").c_str());
      else {
        std::cerr << Error1.str();
        if (warning_verbosity==1)
          logfile << Error1.str() << std::flush;
        stop();
      }
    }
    else {
      std::cerr << Warning5.str();
      if (warning_verbosity==1)
        logfile << Warning5.str() << std::flush;
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }

    // smoothing constant for updating alpha per EWMA
    if (!config->nodeExists("smoothing_constant")) {
      std::cerr << Warning6.str();
      if (warning_verbosity==1)
        logfile << Warning6.str() << std::flush;
      smoothing_constant = DEFAULT_smoothing_constant;
    }
    else if (!(config->getValue("smoothing_constant")).empty()) {
      if ( atof(config->getValue("smoothing_constant").c_str()) > 0.0)
        smoothing_constant = atof(config->getValue("smoothing_constant").c_str());
      else {
        std::cerr << Error2.str();
        if (warning_verbosity==1)
          logfile << Error2.str() << std::flush;
        stop();
      }
    }
    else {
      std::cerr << Warning7.str();
      if (warning_verbosity==1)
        logfile << Warning7.str() << std::flush;
      smoothing_constant = DEFAULT_smoothing_constant;
    }

    // repetition factor
    if (!config->nodeExists("repetition_factor")) {
      std::cerr << Warning8.str();
      if (warning_verbosity==1)
        logfile << Warning8.str() << std::flush;
      repetition_factor = DEFAULT_repetition_factor;
    }
    else if (!(config->getValue("repetition_factor")).empty()) {
      if ( atoi(config->getValue("repetition_factor").c_str()) > 0)
        repetition_factor = atoi(config->getValue("repetition_factor").c_str());
      else {
        std::cerr << Error3.str();
        if (warning_verbosity==1)
          logfile << Error3.str() << std::flush;
        stop();
      }
    }
    else {
      std::cerr << Warning9.str();
      if (warning_verbosity==1)
        logfile << Warning9.str() << std::flush;
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
      logfile << WMWdefault.str() << std::flush;
    enable_wmw_test = true;
  }
  else if (!(config->getValue("wilcoxon_test")).empty()) {
    enable_wmw_test = ( 0 == strcasecmp("true", config->getValue("wilcoxon_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << WMWdefault1.str();
    if (warning_verbosity==1)
      logfile << WMWdefault1.str() << std::flush;
    enable_wmw_test = true;
  }

  if (!config->nodeExists("kolmogorov_test")) {
    std::cerr << KSdefault.str();
    if (warning_verbosity==1)
      logfile << KSdefault.str() << std::flush;
    enable_ks_test = true;
  }
  else if (!(config->getValue("kolmogorov_test")).empty()) {
    enable_ks_test = ( 0 == strcasecmp("true", config->getValue("kolmogorov_test").c_str()) ) ? true:false;
  }
  else {
    std::cerr << KSdefault1.str();
    if (warning_verbosity==1)
      logfile << KSdefault1.str() << std::flush;
    enable_ks_test = true;
  }

  if (!config->nodeExists("pearson_chi-square_test")) {
    std::cerr << PCSdefault.str();
    if (warning_verbosity==1)
      logfile << PCSdefault.str() << std::flush;
    enable_pcs_test = true;
  }
  else if (!(config->getValue("pearson_chi-square_test")).empty()) {
    enable_pcs_test = ( 0 == strcasecmp("true", config->getValue("pearson_chi-square_test").c_str()) ) ?true:false;
  }
  else {
    std::cerr << PCSdefault1.str();
    if (warning_verbosity==1)
      logfile << PCSdefault1.str() << std::flush;
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
      logfile << Default_old.str() << std::flush;
    sample_old_size = DEFAULT_sample_old_size;
  }
  else if (!(config->getValue("sample_old_size")).empty()) {
    sample_old_size =
      atoi( config->getValue("sample_old_size").c_str() );
  }
  else {
    std::cerr << Default1_old.str();
    if (warning_verbosity==1)
      logfile << Default1_old.str() << std::flush;
    sample_old_size = DEFAULT_sample_old_size;
  }

  // extracting size of sample_new
  if (!config->nodeExists("sample_new_size")) {
    std::cerr << Default_new.str();
    if (warning_verbosity==1)
      logfile << Default_new.str() << std::flush;
    sample_new_size = DEFAULT_sample_new_size;
  }
  else if (!(config->getValue("sample_new_size")).empty()) {
    sample_new_size =
      atoi( config->getValue("sample_new_size").c_str() );
  }
  else {
    std::cerr << Default1_new.str();
    if (warning_verbosity==1)
      logfile << Default1_new.str() << std::flush;
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
      logfile << Default.str() << std::flush;
    two_sided = false;
  }
  else if ( enable_wmw_test == true && config->getValue("two_sided").empty() ) {
    std::stringstream Default;
    Default
      << "WARNING: No value for two_sided parameter defined in XML config file!\n"
      << "  \"false\" assumed.\n";
    std::cerr << Default.str();
    if (warning_verbosity==1)
      logfile << Default.str() << std::flush;
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
          logfile << Error.str() << std::flush;
        stop();
      }
    }
    else {
      std::stringstream Warning;
      Warning
        << "WARNING: No value for significance_level parameter defined in XML config file!\n"
        << "  Nothing will be interpreted as an attack.\n";
      std::cerr << Warning.str();
      if (warning_verbosity==1)
        logfile << Warning.str() << std::flush;
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
        logfile << Warning.str() << std::flush;
    significance_level = -1;
  }

  return;
}


// ============================= TEST FUNCTION ===============================
void Stat::test(StatStore * store) {

#ifdef IDMEF_SUPPORT_ENABLED
  idmefMessage = getNewIdmefMessage("wkp-module", "statistical anomaly detection");
#endif

  std::map<EndPoint,Info> Data = store->getData();

  // Dumping empty records:
  if (Data.empty()==true) {
    if (logfile_output_verbosity>=3 || warning_verbosity==1)
      logfile << std::endl << "INFORMATION: Got empty record; "
        << "dumping it and waiting for another record" << std::endl << std::flush;
    return;
  }

#ifndef OFFLINE_ENABLED
  if (storefile.is_open() == true)
    // store data storage for offline use
    storefile << Data;
#endif

  if (logfile_output_verbosity > 0) {
    logfile << std::endl
      << "####################################################" << std::endl
      << "########## Stat::test(...)-call number: " << test_counter
      << " ##########" << std::endl
      << "####################################################" << std::endl;
  }

  std::map<EndPoint,Info>::iterator Data_it = Data.begin();

  std::map<EndPoint,Info> PreviousData = store->getPreviousData();
  //std::map<EndPoint,Info> PreviousData = store->getPreviousDataFromFile();
    // Needed for extraction of packets(t)-packets(t-1) and bytes(t)-bytes(t-1)
    // Holds information about the Info used in the last call to test()
  Info prev;

  // 1) LEARN/UPDATE PHASE
  // Parsing data to see whether the recorded EndPoints already exist
  // in our  "std::map<EndPoint, WkpParams> WkpData" respective
  // "std::map<EndPoint, CusumParams> CusumData" container.
  // If not, then we add it as a new pair <EndPoint, *>.
  // If yes, then we update the corresponding entry using
  // std::vector<int64_t> extracted data.
  if (logfile_output_verbosity > 0)
    logfile << "#### LEARN/UPDATE PHASE" << std::endl;

  // for every EndPoint, extract the data
  while (Data_it != Data.end()) {
    if (logfile_output_verbosity >= 3)
      logfile << "[[ " << Data_it->first << " ]]" << std::endl;

    prev = PreviousData[Data_it->first];
    // it doesn't matter much if Data_it->first is an EndPoint that exists
    // only in Data, but not in PreviousData, because
    // PreviousData[Data_it->first] will automaticaly be an Info structure
    // with all fields set to 0.

    std::vector<int64_t> metric_data;
    std::vector<int64_t> pca_metric_data;

    // Do stuff for wkp-tests (if at least one of them is enabled)
    if (enable_wkp_test == true) {

      std::map<EndPoint, WkpParams>::iterator WkpData_it =
        WkpData.find(Data_it->first);

      if (WkpData_it == WkpData.end()) {

        // We didn't find the recorded EndPoint Data_it->first
        // in our sample container "WkpData"; that means it's a new one,
        // so we just add it in "WkpData"; there will not be jeopardy
        // of memory exhaustion through endless growth of the "WkpData" map
        // as limits are implemented in the StatStore class (EndPointListMaxSize)

        WkpParams S;
        S.correspondingEndPoint = (Data_it->first).toString();
        for (int i=0; i < metrics.size(); i++) {
          (S.wmw_alarms).push_back(0);
          (S.ks_alarms).push_back(0);
          (S.pcs_alarms).push_back(0);
        }

        if (use_pca == true) {
          // initialize pca stuff
          S.init(metrics.size());
          pca_metric_data = extract_pca_data(S, Data_it->second, prev);
        }
        else {
          metric_data = extract_data(Data_it->second, prev);
          (S.Old).push_back(metric_data);
        }

        WkpData[Data_it->first] = S;

        if (logfile_output_verbosity >= 3) {
          logfile << " (WKP): New monitored EndPoint added" << std::endl;
          if (logfile_output_verbosity >= 4 && use_pca == false) {
            logfile << "   with first element of sample_old: " << S.Old.back() << std::endl;
          }
        }

      }
      else {
        // We found the recorded EndPoint Data_it->first
        // in our sample container "WkpData"; so we update the samples
        // (WkpData_it->second).Old and (WkpData_it->second).New
        // thanks to the recorded new value in Data_it->second:
        if (use_pca == true) {
          pca_metric_data = extract_pca_data(WkpData_it->second, Data_it->second, prev);
          wkp_update ( WkpData_it->second, pca_metric_data );
        }
        else {
          metric_data = extract_data(Data_it->second, prev);
          wkp_update ( WkpData_it->second, metric_data );
        }
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
          (C.last_cusum_test_was_attack).push_back(false);
          (C.X_curr).push_back(0);
          (C.X_last).push_back(0);
          (C.cusum_alarms).push_back(0);
        }

        if (use_pca == true) {
          // initialize pca stuff
          C.init(metrics.size());
          C.correspondingEndPoint = (Data_it->first).toString();
          pca_metric_data = extract_pca_data(C, Data_it->second, prev);
        }
        else { // no learning phase for pca ...
          metric_data = extract_data(Data_it->second, prev);
          // add first value of each metric to the sum, which is needed
          // only to calculate the initial alpha after learning_phase_for_alpha
          // is over
          for (int i = 0; i != metric_data.size(); i++)
            C.sum.at(i) += metric_data.at(i);
          C.learning_phase_nr_for_alpha = 1;
          C.correspondingEndPoint = (Data_it->first).toString();
        }

        CusumData[Data_it->first] = C;
        if (logfile_output_verbosity >= 3)
          logfile << " (CUSUM): New monitored EndPoint added" << std::endl;
      }
      else {
        // We found the recorded EndPoint Data_it->first
        // in our cusum container "CusumData"; so we update alpha
        // thanks to the recorded new values in Data_it->second:
        if (use_pca == true) {
          pca_metric_data = extract_pca_data(CusumData_it->second, Data_it->second, prev);
          cusum_update ( CusumData_it->second, pca_metric_data );
        }
        else {
          metric_data = extract_data(Data_it->second, prev);
          cusum_update ( CusumData_it->second, metric_data );
        }
      }
    }

    // Create metric files, if wished so
    // (this can be done here, because metrics are the same for both tests)
    if (createFiles == true) {

      std::string fname;

      if (use_pca == true)
        fname = "pca_metrics_" + (Data_it->first).toString() + ".txt";
      else
        fname = "metrics_" + (Data_it->first).toString() + ".txt";

      chdir(output_dir.c_str());

      std::ofstream file(fname.c_str(),std::ios_base::app);

      // are we at the beginning of the file?
      // if yes, write the metric names to the file ...
      long pos;
      pos = file.tellp();

      if (use_pca == true) {
        if (pos == 0) {
          for (int i = 0; i != pca_metric_data.size(); i++)
            file << "pca_comp_" << i << "\t";
          file << "Test-Run" << "\n";
        }
        for (int i = 0; i != pca_metric_data.size(); i++)
          file << pca_metric_data.at(i) << "\t";
        file << test_counter << "\n";
      }
      else {
        if (pos == 0) {
          for (int i = 0; i != metric_data.size(); i++)
            file << getMetricName(metrics.at(i)) << "\t";
          file << "Test-Run" << "\n";
        }
        for (int i = 0; i != metric_data.size(); i++)
          file << metric_data.at(i) << "\t";
        file << test_counter << "\n";
      }

      file.close();

      chdir("..");

    }

    Data_it++;
  }

  // 1.5) MAP PRINTING (OPTIONAL, DEPENDS ON VERBOSITY SETTINGS)

  // how many endpoints do we already monitor?
  int ep_nr;
  if (enable_wkp_test == true)
    ep_nr = WkpData.size();
  else if (enable_cusum_test == true)
    ep_nr = CusumData.size();

  if (logfile_output_verbosity >= 4) {
    logfile << std::endl << "#### STATE OF ALL MONITORED ENDPOINTS (" << ep_nr << "):" << std::endl;

    if (enable_wkp_test == true) {
      logfile << "### WKP OVERVIEW" << std::endl;
      std::map<EndPoint,WkpParams>::iterator WkpData_it =
        WkpData.begin();
      while (WkpData_it != WkpData.end()) {
        logfile
          << "[[ " << WkpData_it->first << " ]]\n"
          << "  sample_old (" << (WkpData_it->second).Old.size()  << ") : "
          << (WkpData_it->second).Old << "\n"
          << "  sample_new (" << (WkpData_it->second).New.size() << ") : "
          << (WkpData_it->second).New << "\n";
        WkpData_it++;
      }
    }
    if (enable_cusum_test == true) {
      logfile << "### CUSUM OVERVIEW" << std::endl;
      std::map<EndPoint,CusumParams>::iterator CusumData_it =
        CusumData.begin();
      while (CusumData_it != CusumData.end()) {
        logfile
          << "[[ " << CusumData_it->first << " ]]\n"
          << "  alpha: " << (CusumData_it->second).alpha << "\n"
          << "  g: " << (CusumData_it->second).g << "\n";
        CusumData_it++;
      }
    }
    logfile << std::flush;
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
    // for WKP: as soon as a sample is big enough to test, i.e. when its
    // learning phase is over.
    // for CUSUM: as soon as we have enough values for calculating the initial
    // values of alpha
    // The other endpoints in the "WkpData" and "CusumData"
    // maps are let learning.

    if (enable_wkp_test == true) {
      std::map<EndPoint,WkpParams>::iterator WkpData_it = WkpData.begin();
      while (WkpData_it != WkpData.end()) {
        if ( ((WkpData_it->second).New).size() == sample_new_size ) {
          // i.e., learning phase over
          if (logfile_output_verbosity > 0)
            logfile << "\n#### WKP TESTS for EndPoint [[ " << WkpData_it->first << " ]]\n";
          wkp_test ( WkpData_it->second );
        }
        WkpData_it++;
      }
    }

    if (enable_cusum_test == true) {
      std::map<EndPoint,CusumParams>::iterator CusumData_it = CusumData.begin();
      while (CusumData_it != CusumData.end()) {
        if ( (CusumData_it->second).ready_to_test == true ) {
          // i.e. learning phase for alpha is over and it has an initial value
          if (logfile_output_verbosity > 0)
            logfile << "\n#### CUSUM TESTS for EndPoint [[ " << CusumData_it->first << " ]]\n";
          cusum_test ( CusumData_it->second );
        }
        CusumData_it++;
      }
    }
  }

  test_counter++;
  // don't forget to free the store-object!
  delete store;
  logfile << std::flush;
  return;

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
        else
          result.push_back(0);
        break;

      case PACKETS_OUT:
        if (info.packets_out >= noise_threshold_packets)
          result.push_back(info.packets_out);
        else
          result.push_back(0);
        break;

      case BYTES_IN:
        if (info.bytes_in >= noise_threshold_bytes)
          result.push_back(info.bytes_in);
        else
          result.push_back(0);
        break;

      case BYTES_OUT:
        if (info.bytes_out >= noise_threshold_bytes)
          result.push_back(info.bytes_out);
        else
          result.push_back(0);
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
        else
          result.push_back(0);
        break;

      case BYTES_OUT_PER_PACKET_OUT:
        if (info.packets_out >= noise_threshold_packets ||
            info.bytes_out   >= noise_threshold_bytes ) {
          if (info.packets_out == 0)
            result.push_back(0);
          else
            result.push_back((1000 * info.bytes_out) / info.packets_out);
        }
        else
          result.push_back(0);
        break;

      case PACKETS_IN_PER_RECORD_IN:
        if ( info.packets_in >= noise_threshold_packets) {
          if (info.packets_in == 0)
            result.push_back(0);
          else
            result.push_back((info.records_in * 1000) / info.packets_in );
        }
        else
          result.push_back(0);
        break;

      case PACKETS_OUT_PER_RECORD_OUT:
        if ( info.packets_out >= noise_threshold_packets) {
          if (info.packets_out == 0)
            result.push_back(0);
          else
            result.push_back((info.records_out * 1000) / info.packets_out);
        }
        else
          result.push_back(0);
        break;

      case BYTES_IN_PER_RECORD_IN:
        if ( info.bytes_in >= noise_threshold_bytes) {
          if (info.bytes_in == 0)
            result.push_back(0);
          else
            result.push_back((info.records_in * 1000) / info.bytes_in);
        }
        else
          result.push_back(0);
        break;

      case BYTES_OUT_PER_RECORD_OUT:
        if ( info.bytes_out >= noise_threshold_bytes) {
          if (info.bytes_out == 0)
            result.push_back(0);
          else
            result.push_back((info.records_out * 1000) / info.bytes_out);
        }
        else
          result.push_back(0);
        break;

      case PACKETS_OUT_MINUS_PACKETS_IN:
        if (info.packets_out >= noise_threshold_packets
         || info.packets_in  >= noise_threshold_packets )
          result.push_back(info.packets_out - info.packets_in);
        else
          result.push_back(0);
        break;

      case BYTES_OUT_MINUS_BYTES_IN:
        if (info.bytes_out >= noise_threshold_bytes
         || info.bytes_in  >= noise_threshold_bytes )
          result.push_back(info.bytes_out - info.bytes_in);
        else
          result.push_back(0);
        break;

      case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
        if (info.packets_in  >= noise_threshold_packets
         || prev.packets_in  >= noise_threshold_packets)
          // prev holds the data for the same EndPoint as info
          // from the last call to test()
          // it is updated at the beginning of the while-loop in test()
          result.push_back(info.packets_in - prev.packets_in);
        else
          result.push_back(0);
        break;

      case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
        if (info.packets_out >= noise_threshold_packets
         || prev.packets_out >= noise_threshold_packets)
          result.push_back(info.packets_out - prev.packets_out);
        else
          result.push_back(0);
        break;

      case BYTES_T_IN_MINUS_BYTES_T_1_IN:
        if (info.bytes_in >= noise_threshold_bytes
         || prev.bytes_in >= noise_threshold_bytes)
          result.push_back(info.bytes_in - prev.bytes_in);
        else
          result.push_back(0);
        break;

      case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
        if (info.bytes_out >= noise_threshold_bytes
         || prev.bytes_out >= noise_threshold_bytes)
          result.push_back(info.bytes_out - prev.bytes_out);
        else
          result.push_back(0);
        break;

      default:
        std::cerr << "ERROR: metrics seems to be empty "
        << "or it holds an unknown type which isnt supported yet."
        << "But this shouldnt happen as the init_metrics"
        << "-function handles that.\nExiting.\n";
        stop();
    }

    it++;
  }

  return result;
}

// needed for pca to extract the new values (for learning and testing)
std::vector<int64_t> Stat::extract_pca_data (Params & P, const Info & info, const Info & prev) {

  std::vector<int64_t> result;

  // learning phase
  if (P.pca_ready == false) {
    if (P.learning_phase_nr_for_pca < learning_phase_for_pca) {
      // update sumsOfMetrics and sumsOfProducts
      std::vector<int64_t> v = extract_data(info, prev);
      for (int i = 0; i < metrics.size(); i++) {
        P.sumsOfMetrics.at(i) += v.at(i);
        for (int j = 0; j < metrics.size(); j++) {
          // sumsOfProducts is a matrix which holds all the sums
          // of the product of each two metrics;
          // elements beneath the main diagonal (j < i) are irrelevant
          // because of commutativity of multiplication
          // i. e. metric1*metric2 = metric2*metric1
          if (j >= i)
            P.sumsOfProducts.at(i).at(j) += v.at(i)*v.at(j);
        }
      }

      P.learning_phase_nr_for_pca++;
      return result; // empty
    }
    // end of learning phase
    else if (P.learning_phase_nr_for_pca == learning_phase_for_pca) {

      // calculate covariance matrix
      for (int i = 0; i < metrics.size(); i++) {
        for (int j = 0; j < metrics.size(); j++)
          gsl_matrix_set(P.cov,i,j,covariance(P.sumsOfProducts.at(i).at(j),P.sumsOfMetrics.at(i),P.sumsOfMetrics.at(j)));
      }

      // calculate eigenvectors and -values
      gsl_vector *eval = gsl_vector_alloc (metrics.size());
      // some workspace needed for computation
      gsl_eigen_symmv_workspace * w = gsl_eigen_symmv_alloc (metrics.size());
      // computation of eigenvectors (evec) and -values (eval) from
      // covariance matrix (cov)
      gsl_eigen_symmv (P.cov, eval, P.evec, w);
      gsl_eigen_symmv_free (w);
      // sort the eigenvectors by their corresponding eigenvalue
      gsl_eigen_symmv_sort (eval, P.evec, GSL_EIGEN_SORT_VAL_DESC);
      gsl_vector_free (eval);

      // now, we have our components stored in each column of
      // evec, first column = most important, last column = least important

      // From now on, matrix evec can be used to transform the new arriving data

      P.pca_ready = true; // so this code will never be visited again

      if (logfile_output_verbosity >= 3)
        logfile << "(CUSUM): PCA learning phase is over! PCA is now ready!" << std::endl;

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
    gsl_matrix_set(new_metric_data,0,i,v.at(i));

  // matrix multiplication to get the transformed data
  // transformed_data = data * evec
  gsl_matrix *transformed_metric_data = gsl_matrix_calloc (1, metrics.size());
  gsl_blas_dgemm (CblasNoTrans, CblasNoTrans,
                       1.0, new_metric_data, P.evec,
                       0.0, transformed_metric_data);

  // transform the matrix with the transformed data back into a vector
  for (int i = 0; i < metrics.size(); i++)
    result.push_back((int64_t) gsl_matrix_get(transformed_metric_data,0,i));

  gsl_matrix_free(new_metric_data);
  gsl_matrix_free(transformed_metric_data);

  return result; // filled with new transformed values
}


// -------------------------- LEARN/UPDATE FUNCTION ---------------------------

// learn/update function for samples (called everytime test() is called)
//
void Stat::wkp_update ( WkpParams & S, const std::vector<int64_t> & new_value ) {

  // Updates for pca metrics have to wait for the pca learning phase
  // needed to calculate the eigenvectors
  // NOTE: The overall learning phase will thus sum up to
  // learning_phase_pca + leraning_phase_samples
  if (use_pca == true && S.pca_ready == false) {
    if (logfile_output_verbosity >= 3)
      logfile << "(WKP): learning phase for PCA ..." << std::endl;
    return;
  }

  // this case happens exactly one time: when PCA is ready for the first time
  if (new_value.empty() == true)
    return;

  // Learning phase?
  if (S.Old.size() != sample_old_size) {

    S.Old.push_back(new_value);

    if (logfile_output_verbosity >= 3) {
      logfile << " (WKP): Learning phase for sample_old ..." << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << S.Old << std::endl;
        logfile << "   sample_new: " << S.New << std::endl;
      }
    }

    return;
  }
  else if (S.New.size() != sample_new_size) {

    S.New.push_back(new_value);

    if (logfile_output_verbosity >= 3) {
      logfile << " (WKP): Learning phase for sample_new..." << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << S.Old << std::endl;
        logfile << "   sample_new: " << S.New << std::endl;
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

    if (logfile_output_verbosity >= 3) {
      logfile << " (WKP): Update done (for new sample only)" << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << S.Old << std::endl;
        logfile << "   sample_new: " << S.New << std::endl;
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

    if (logfile_output_verbosity >= 3) {
      logfile << " (WKP): Update done (for both samples)" << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << S.Old << std::endl;
        logfile << "   sample_new: " << S.New << std::endl;
      }
    }
  }

  logfile << std::flush;
  return;
}


// and the update funciotn for the cusum-test
void Stat::cusum_update ( CusumParams & C, const std::vector<int64_t> & new_value ) {

  // Updates for pca metrics have to wait for the pca learning phase
  // needed to calculate the eigenvectors
  // NOTE: The overall learning phase will thus sum up to
  // learning_phase_pca + leraning_phase_alpha
  if (use_pca == true && C.pca_ready == false) {
    if (logfile_output_verbosity >= 3)
      logfile << "(CUSUM): learning phase for PCA ..." << std::endl;
    return;
  }

  // this case happens exactly one time: when PCA is ready for the first time
  if (new_value.empty() == true)
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

      if (logfile_output_verbosity >= 3)
        logfile
          << " (CUSUM): Learning phase for alpha ...\n";

      C.learning_phase_nr_for_alpha++;

      return;
    }

    // Enough values? Calculate initial alpha per simple average
    // and set ready_to_test-flag to true (so we never visit this
    // code here again for the current endpoint)
    if (C.learning_phase_nr_for_alpha == learning_phase_for_alpha) {

      if (logfile_output_verbosity >= 3)
        logfile << " (CUSUM): Learning phase for alpha is over\n"
                << "   Calculated initial alphas: ( ";

      for (int i = 0; i != C.alpha.size(); i++) {
        // alpha = sum(values) / #(values)
        // Note: learning_phase_for_alpha is never 0, because
        // this is handled in init_cusum_test()
        C.alpha.at(i) = C.sum.at(i) / learning_phase_for_alpha;
        C.X_curr.at(i) = new_value.at(i);
        if (logfile_output_verbosity >= 3)
          logfile << C.alpha.at(i) << " ";
      }
      if (logfile_output_verbosity >= 3)
          logfile << ")\n";

      C.ready_to_test = true;
      return;
    }
  }

  // pausing update for alpha (depending on pause_update parameter)
  bool at_least_one_test_was_attack = false;
  for (int i = 0; i < C.last_cusum_test_was_attack.size(); i++)
    if (C.last_cusum_test_was_attack.at(i) == true)
      at_least_one_test_was_attack = true;

  bool all_tests_were_attacks = true;
  for (int i = 0; i < C.last_cusum_test_was_attack.size(); i++)
    if (C.last_cusum_test_was_attack.at(i) == false)
      all_tests_were_attacks = false;

  // pause, if at least one metric yielded an alarm
  if ( pause_update_when_attack == 1
    && at_least_one_test_was_attack == true) {
    if (logfile_output_verbosity >= 3)
      logfile << " (CUSUM): Pausing update for alpha (at least one test was attack)\n";
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
    if (logfile_output_verbosity >= 3)
      logfile << " (CUSUM): Pausing update for alpha (all tests were attacks)\n";
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
    if (logfile_output_verbosity >= 3)
      logfile << " (CUSUM): Pausing update for alpha (for those metrics which were attacks)\n";
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

  if (logfile_output_verbosity >= 3)
    logfile << " (CUSUM): Update done for alpha: " << C.alpha << "\n";

  logfile << std::flush;
  return;
}

// ------- FUNCTIONS USED TO CONDUCT TESTS ON THE SAMPLES ---------

// statistical test function for wkp-tests
// (optional, depending on how often the user wishes to do it)
void Stat::wkp_test (WkpParams & S) {

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

    if (logfile_output_verbosity >= 4) {
      if (use_pca == false)
        logfile << "### Performing WKP-Tests for metric " << getMetricName(*it) << ":\n";
      else {
        std::stringstream tmp;
        tmp << index;
        logfile << "### Performing WKP-Tests for pca component " << tmp.str() << ":\n";
      }
    }

    sample_old_single_metric = getSingleMetric(S.Old, index);
    sample_new_single_metric = getSingleMetric(S.New, index);

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

    // generate output files, if wished
    if (createFiles == true) {

      std::string filename;
      if (use_pca == false)
        filename = "wkpparams_" + S.correspondingEndPoint + "_" + getMetricName(*it) + ".txt";
      else {
        std::stringstream tmp;
        tmp << index;
        filename = "wkpparams_" + S.correspondingEndPoint  + "_pca_component_" + tmp.str() + ".txt";
      }

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

      chdir(output_dir.c_str());

      std::ofstream file(filename.c_str(), std::ios_base::app);

      // are we at the beginning of the file?
      // if yes, write the param names to the file ...
      long pos;
      pos = file.tellp();

      if (pos == 0) {
        file << "Value" << "\t" << "p (wmw)" << "\t" << "alarms (wmw)" << "\t"
        << "p (ks)" << "\t" << "alarms (ks)" << "\t" << "p (pcs)" << "\t"
        << "alarms (pcs)" << "\t" << "Test-Run\n";
      }

      // metric p-value(wmw) #alarms(wmw) p-value(ks) #alarms(ks)
      // p-value(pcs) #alarms(pcs) counter
      file << sample_new_single_metric.back() << "\t" << str_p_wmw << "\t"
          << (S.wmw_alarms).at(index) << "\t" << str_p_ks << "\t"
          << (S.ks_alarms).at(index) << "\t" << str_p_pcs << "\t"
          << (S.pcs_alarms).at(index) << "\t" << test_counter << "\n";
      file.close();
      chdir("..");
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

  logfile << std::flush;
  return;

}

// statistical test function / cusum-test
// (optional, depending on how often the user wishes to do it)
void Stat::cusum_test(CusumParams & C) {

  // we have to store, for which metrics an attack was detected and set
  // the corresponding last_cusum_test_was_attack-flags to true/false
  std::vector<bool> was_attack;
  for (int i = 0; i < metrics.size(); i++)
    was_attack.push_back(false);
  // index, as we cant use the iterator for dereferencing the elements of
  // CusumParams
  int i = 0;
  // current adapted threshold
  double N = 0.0;
  // beta, needed to make (Xn - beta) slightly negative in the mean
  double beta = 0.0;

  for (std::vector<Metric>::iterator it = metrics.begin(); it != metrics.end(); it++) {

    if (logfile_output_verbosity >= 4) {
      if (use_pca == false)
        logfile << "### Performing CUSUM-Test for metric " << getMetricName(*it) << ":\n";
      else {
        std::stringstream tmp;
        tmp << i;
        logfile << "### Performing CUSUM-Test for pca component " << tmp.str() << ":\n";
      }
    }

    // Calculate N and beta
    N = repetition_factor * (amplitude_percentage * C.alpha.at(i) / 2.0);
    beta = C.alpha.at(i) + (amplitude_percentage * C.alpha.at(i) / 2.0);

    if (logfile_output_verbosity >= 4) {
      logfile << " Cusum test returned:\n"
              << "  Threshold: " << N << std::endl;
      logfile << "  reject H0 (no attack) if current value of statistic g > "
              << N << std::endl;
    }

    // TODO
    // "attack still in progress"-message?

    // perform the test and if g > N raise an alarm
    if ( cusum(C.X_curr.at(i), beta, C.g.at(i)) > N ) {

      if (report_only_first_attack == false
        || C.last_cusum_test_was_attack.at(i) == false) {
        if (logfile_output_verbosity >= 2) {
          logfile
            << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
            << "    " << C.correspondingEndPoint << " for metric " << getMetricName(*it) << "\n"
            << "    Cusum test says we're under attack (g = " << C.g.at(i) << ")!\n"
            << "    ALARM! ALARM! Women und children first!" << std::endl;
          std::cout
            << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
            << "  " << C.correspondingEndPoint << " for metric " << getMetricName(*it) << "\n"
            << "  Cusum test says we're under attack!\n"
            << "  ALARM! ALARM! Women und children first!" << std::endl;
        }
        #ifdef IDMEF_SUPPORT_ENABLED
          idmefMessage.setAnalyzerAttr("", "", "cusum-test", "");
          sendIdmefMessage("DDoS", idmefMessage);
          idmefMessage = getNewIdmefMessage();
        #endif
      }

      (C.cusum_alarms).at(i)++;
      was_attack.at(i) = true;

    }

    if (createFiles == true) {

      std::string filename;
      if (use_pca == false)
        filename = "cusumparams_" + C.correspondingEndPoint  + "_" + getMetricName(*it) + ".txt";
      else {
        std::stringstream tmp;
        tmp << i;
        filename = "cusumparams_" + C.correspondingEndPoint  + "_pca_component_" + tmp.str() + ".txt";
      }

      chdir(output_dir.c_str());

      std::ofstream file(filename.c_str(), std::ios_base::app);

      // are we at the beginning of the file?
      // if yes, write the param names to the file ...
      long pos;
      pos = file.tellp();

      if (pos == 0) {
        file << "Value" << "\t" << "g" << "\t" << "N" << "\t"
        << "alpha" << "\t" << "beta" << "\t" << "alarms"
        << "\t" << "Test-Run\n";
      }

      // X  g N alpha beta #alarms counter
      file << (int) C.X_curr.at(i) << "\t" << (int) C.g.at(i)
          << "\t" << (int) N << "\t" << (int) C.alpha.at(i) << "\t"  << (int) beta
          << "\t" << (C.cusum_alarms).at(i) << "\t" << test_counter << "\n";
      file.close();
      chdir("..");
    }

    i++;
  }

  for (int i = 0; i != was_attack.size(); i++) {
    if (was_attack.at(i) == true)
      C.last_cusum_test_was_attack.at(i) = true;
    else
      C.last_cusum_test_was_attack.at(i) = false;
  }

  logfile << std::flush;
  return;
}

// functions called by the wkp_test()-function
std::list<int64_t> Stat::getSingleMetric(const std::list<std::vector<int64_t> > & l, const short & i) {
  std::list<int64_t> result;
  std::list<std::vector<int64_t> >::const_iterator it = l.begin();
  while ( it != l.end() ) {
    result.push_back(it->at(i));
    it++;
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
    case PACKETS_IN_PER_RECORD_IN:
      return std::string("packets_in_per_record_in");
    case PACKETS_OUT_PER_RECORD_OUT:
      return std::string("packets_out_per_record_out");
    case BYTES_IN_PER_RECORD_IN:
      return std::string("bytes_in_per_record_in");
    case BYTES_OUT_PER_RECORD_OUT:
      return std::string("bytes_out_per_record_out");
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
      stop();
  }
}

double Stat::stat_test_wmw (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new, bool & last_wmw_test_was_attack) {

  double p;

  if (logfile_output_verbosity >= 5) {
    logfile << " Wilcoxon-Mann-Whitney test details:\n";
    p = wmw_test(sample_old, sample_new, two_sided, logfile);
  }
  else {
    std::ofstream dump("/dev/null");
    p = wmw_test(sample_old, sample_new, two_sided, dump);
  }

  if (logfile_output_verbosity >= 2) {
    logfile << " Wilcoxon-Mann-Whitney test returned:\n"
	    << "  p-value: " << p << std::endl;
    logfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_wmw_test_was_attack == false) {
       logfile
          << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
		      << "    Wilcoxon-Mann-Whitney test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout
          << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
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

  if (logfile_output_verbosity == 1) {
    logfile << "wmw: " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
      || last_wmw_test_was_attack == false) {
	      logfile << "attack to significance level "
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

  if (logfile_output_verbosity>=5) {
    logfile << " Kolmogorov-Smirnov test details:\n";
    p = ks_test(sample_old, sample_new, logfile);
  }
  else {
    std::ofstream dump ("/dev/null");
    p = ks_test(sample_old, sample_new, dump);
  }

  if (logfile_output_verbosity >= 2) {
    logfile << " Kolmogorov-Smirnov test returned:\n"
	    << "  p-value: " << p << std::endl;
    logfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_ks_test_was_attack == false) {
	      logfile << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
		      << "    Kolmogorov-Smirnov test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
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

  if (logfile_output_verbosity == 1) {
    logfile << "ks : " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_ks_test_was_attack == false) {
        logfile << "attack to significance level "
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

  if (logfile_output_verbosity>=5) {
    logfile << " Pearson chi-square test details:\n";
    p = pcs_test(sample_old, sample_new, logfile);
  }
  else {
    std::ofstream dump ("/dev/null");
    p = pcs_test(sample_old, sample_new, dump);
  }

  if (logfile_output_verbosity >= 2) {
    logfile << " Pearson chi-square test returned:\n"
	    << "  p-value: " << p << std::endl;
    logfile << "  reject H0 (no attack) to any significance level alpha > "
	    << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_pcs_test_was_attack == false) {
	      logfile << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
		      << "    Pearson chi-square test says we're under attack!\n"
		      << "    ALARM! ALARM! Women und children first!" << std::endl;
	      std::cout << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
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

  if (logfile_output_verbosity == 1) {
    logfile << "pcs: " << p << std::endl;
    if (significance_level > p) {
      if (report_only_first_attack == false
      || last_pcs_test_was_attack == false) {
	      logfile << "attack to significance level "
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
