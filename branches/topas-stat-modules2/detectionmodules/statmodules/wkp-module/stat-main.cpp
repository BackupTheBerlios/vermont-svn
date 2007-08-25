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
#include<math.h>

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
    msg(MSG_ERROR, "wkp-module: Couldn't install signal handler for SIGTERM. Exiting. ");
  }
  if (signal(SIGINT, sigInt) == SIG_ERR) {
    msg(MSG_ERROR, "wkp-module: Couldn't install signal handler for SIGINT. Exiting. ");
  }

  // lock, will be unlocked at the end of init() (cf. StatStore class header):
  StatStore::setBeginMonitoring () = false;

  test_counter = 0;

  // parameter initialization
  init(configfile);

#ifdef OFFLINE_ENABLED
  /* open file with offline data */
  if(!OfflineInputPolicy<StatStore>::openOfflineFile(offlineFile.c_str())) {
      msgStr.print(MsgStream::FATAL, "Could not open offline data file!", logfile, true);
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
    msgStr.print(MsgStream::FATAL, "No preferences node defined in XML config file! Define one, fill it with some parameters and restart.");
    delete config;
    stop();
  }

  if (!config->nodeExists("cusum-test-params")) {
    msgStr.print(MsgStream::FATAL, "No cusum-test-params node defined in XML config file! Define one, fill it with some parameters and restart.");
    delete config;
    stop();
  }

  if (!config->nodeExists("wkp-test-params")) {
    msgStr.print(MsgStream::FATAL, "No wkp-test-params node defined in XML config file! Define one, fill it with some parameters and restart.");
    delete config;
    stop();
  }

  // ATTENTION:
  // the order of the following operations is important,
  // as some of these functions use Stat members initialized
  // by the preceding functions; so beware if you change the order

  config->enterNode("preferences");

  // extracting warning verbosity
  init_warning_verbosity(config);

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

  // extract filename for the file where
  // all the data will be written into
  init_offline_file(config);

  // extracting the key of the endpoints
  // and thus determining,
  // ONLINE MODE: which IPFIX_TYPEIDs we need to subscribe to
  // OFFLINE MODE: which endpoint members we need to set to 0
  init_endpoint_key(config);

  // extracting the netmask, which will be applied
  // to the ip of each endpoint; for aggregating
  // in ON- and OFFLINE MODE
  // NOTE: In OFFLINE MODE, only used, if endpoint_key contains "ip" or "all"
  init_netmask(config);

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
    init_wmw_two_sided(config);

    // extracting significance level parameter
    init_significance_level(config);

  }

  config->leaveNode();

  // if no test is enabled, it doesnt make sense to run the module
  if (enable_wkp_test == false && enable_cusum_test == false) {
    msgStr.print(MsgStream::INFO, "There is no test enabled!", logfile, true);
  }

  // create file which contains all relevant config params
  if (createFiles == true){
    chdir(output_dir.c_str());
    std::ofstream file("CONFIG");
    if (use_pca == true) {
      file << "PCA\n"
           << "learning_phase_for_pca = " << learning_phase_for_pca << "\n"
           << "use_correlation_matrix = " << ((use_correlation_matrix == true)?"true":"false") << "\n";
    }
    else
      file << "NORMAL\n";
    file << "Metrics:\n";
    for(int i = 0; i < metrics.size(); i++)
      file << " - " << getMetricName(metrics.at(i)) << "\n";
    file << "pause_update_when_attack = " << pause_update_when_attack << "\n"
         << "report_only_first_attack = " << report_only_first_attack << "\n"
         << "stat_test_frequency = " << stat_test_frequency << "\n";
    config->enterNode("preferences");
    file << "endpoint_key = " << config->getValue("endpoint_key") << "\n";
    config->leaveNode();
    file << "netmask = " << StatStore::getNetmask() << "\n"
         << "noise_thresholds: packets = " << noise_threshold_packets
         << "; bytes = " << noise_threshold_bytes << "\n\n";

    if (enable_cusum_test == true) {
      file << "CUSUM-PARAMS:\n";
      file << "amplitude_percentage = " << amplitude_percentage << "\n"
           << "repetition_factor = " << repetition_factor << "\n"
           << "learning_phase_for_alpha = " << learning_phase_for_alpha << "\n"
           << "smoothing_constant = " << smoothing_constant << "\n\n";
    }
    if (enable_wkp_test == true) {
      file << "WKP-PARAMS:\n";
      file << "sample_old_size = " << sample_old_size << "\n"
           << "sample_new_size = " << sample_new_size << "\n"
           << "wmw_two_sided = " << wmw_two_sided << "\n"
           << "significance_level = " << significance_level << "\n";
    }

    file.close();

    chdir("..");
  }

  /* one should not forget to free "config" after use */
  delete config;
  msgStr.print(MsgStream::INFO, "Initialization complete.", logfile, true);
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

void Stat::init_warning_verbosity(XMLConfObj * config) {

  std::stringstream Warning1, Warning2, Usage;

  Warning1
    << "No warning_verbosity parameter defined in XML config file!"
    << " \"" << DEFAULT_warning_verbosity << "\" assumed.\n";
  Warning2
    << "No value for warning_verbosity parameter defined in XML config file!"
    << " \"" << DEFAULT_warning_verbosity << "\" assumed.\n";
  Usage
    << "  0: no module messages\n"
    << "  1: fatal errors only\n"
    << "  2: same as 1 + normal errors\n"
    << "  3: same as 2 + warnings\n"
    << "  4: same as 3 + infos\n"
    << "  5: same as 4 + debug infos";

  short warning_verbosity;

  // extracting warning verbosity
  if(!config->nodeExists("warning_verbosity")) {
    msgStr.print(MsgStream::WARN, Warning1.str() + Usage.str());
    warning_verbosity = DEFAULT_warning_verbosity;
  }
  else if (!(config->getValue("warning_verbosity")).empty()) {
    if ( atoi(config->getValue("warning_verbosity").c_str()) >= 0
        && atoi(config->getValue("warning_verbosity").c_str()) <= 5 )
      warning_verbosity = atoi(config->getValue("warning_verbosity").c_str());
    else {
      msgStr.print(MsgStream::ERROR, "Value of warning_verbosity parameter defined in XML config file should be between 0 and 5. Please define it that way and restart.\n" + Usage.str() + "\nExiting.");
      stop();
    }
  }
  else {
    msgStr.print(MsgStream::WARN, Warning2.str() + Usage.str());
    warning_verbosity = DEFAULT_warning_verbosity;
  }

  switch (warning_verbosity) {
    case 0:
      msgStr.setLevel(MsgStream::NONE);
      break;
    case 1:
      msgStr.setLevel(MsgStream::FATAL);
      break;
    case 2:
      msgStr.setLevel(MsgStream::ERROR);
      break;
    case 3:
      msgStr.setLevel(MsgStream::WARN);
      break;
    case 4:
      msgStr.setLevel(MsgStream::INFO);
      break;
    case 5:
      msgStr.setLevel(MsgStream::DEBUG);
      break;
  }

  msgStr.setName("wkp-/cusum-module");

  return;
}


void Stat::init_logfile(XMLConfObj * config) {

  // extracting output file's name
  if(!config->nodeExists("logfile")) {
    msgStr.print(MsgStream::WARN, "No logfile parameter defined in XML config file! Default outputfile used (wkp_log.txt).");
    logfile.open("wkp_log.txt");
  }
  else if (!(config->getValue("logfile")).empty())
    logfile.open(config->getValue("logfile").c_str());
  else {
    msgStr.print(MsgStream::WARN, "No value for logfile parameter defined in XML config file! Default logfile used (wkp_log.txt).");
    logfile.open("wkp_log.txt");
  }

  if (!logfile) {
      msgStr.print(MsgStream::FATAL, "Could not open wkp-module's logfile! Check if you have enough rights to create or write to it.");
      stop();
  }

  std::stringstream Usage;
  Usage
    << "  0: no output generated\n"
    << "  1: only p-values and attacks are recorded\n"
    << "  2: same as 1, plus some cosmetics\n"
    << "  3: same as 2, plus learning phases, updates and empty records events\n"
    << "  4: same as 3, plus sample printing\n"
    << "  5: same as 4, plus all details from statistical tests\n";

  // extracting logfile output verbosity
  if(!config->nodeExists("logfile_output_verbosity")) {
    std::stringstream Warning;
    Warning << "No logfile_output_verbosity parameter defined in XML config file! \"" << DEFAULT_logfile_output_verbosity << "\" assumed.\n";
    msgStr.print(MsgStream::WARN, Warning.str() + Usage.str(), logfile, true);
    logfile_output_verbosity = DEFAULT_logfile_output_verbosity;
  }
  else if (!(config->getValue("logfile_output_verbosity")).empty()) {
    if ( 0 <= atoi( config->getValue("logfile_output_verbosity").c_str() )
      && 5 >= atoi( config->getValue("logfile_output_verbosity").c_str() ) )
      logfile_output_verbosity = atoi( config->getValue("logfile_output_verbosity").c_str() );
    else {
      msgStr.print(MsgStream::ERROR, "logfile_output_verbosity parameter defined in XML config file should be between 0 and 5. Please define it that way and restart.\n" + Usage.str() + "Exiting." , logfile, true);
      stop();
    }
  }
  else {
    std::stringstream Warning;
    Warning << "No value for logfile_output_verbosity parameter defined in XML config file! \"" << DEFAULT_logfile_output_verbosity << "\" assumed.\n";
    msgStr.print(MsgStream::WARN, Warning.str() + Usage.str(), logfile, true);
    logfile_output_verbosity = DEFAULT_logfile_output_verbosity;
  }

  return;
}

#ifndef OFFLINE_ENABLED
void Stat::init_accepted_source_ids(XMLConfObj * config) {

  if (!config->nodeExists("accepted_source_ids")) {
    msgStr.print(MsgStream::WARN, "No accepted_source_ids parameter defined in XML config file! All source ids will be accepted.", logfile, true);
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
    msgStr.print(MsgStream::WARN, "No value for accepted_source_ids parameter defined in XML config file! All source ids will be accepted.", logfile, true);
  }

  return;
}
#endif

#ifndef OFFLINE_ENABLED
void Stat::init_alarm_time(XMLConfObj * config) {

  // extracting alarm_time
  // (that means that the test() methode will be called
  // atoi(alarm_time) seconds after the last test()-run ended)
  if(!config->nodeExists("alarm_time")) {
    std::stringstream Warning;
    Warning
      << "No alarm_time parameter defined in XML config file!\n"
      << " \"" << DEFAULT_alarm_time << "\" assumed.";
    msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
    setAlarmTime(DEFAULT_alarm_time);
  }
  else if (!(config->getValue("alarm_time")).empty())
    setAlarmTime( atoi(config->getValue("alarm_time").c_str()) );
  else {
    std::stringstream Warning;
    Warning
      << "No value for alarm_time parameter defined in XML config file!"
      << " \"" << DEFAULT_alarm_time << "\" assumed.";
    msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
    setAlarmTime(DEFAULT_alarm_time);
  }

  return;
}
#endif


void Stat::init_offline_file(XMLConfObj * config) {

  if (!config->nodeExists("offline_file")) {
#ifdef OFFLINE_ENABLED
    msgStr.print(MsgStream::FATAL, "No offline_file parameter in XML config file! Please specify one and restart.", logfile, true);
    stop();
#else
    msgStr.print(MsgStream::WARN, "No offline_file parameter in XML config file! The file will be named \"data.txt\"." , logfile, true);
    offlineFile = "data.txt";
#endif
  }
  else if ((config->getValue("offline_file")).empty()) {
#ifdef OFFLINE_ENABLED
    msgStr.print(MsgStream::FATAL, "No value for offline_file parameter in XML config file! Please specify one and restart.", logfile, true);
    stop();
#else
    msgStr.print(MsgStream::WARN, "No value defined for offline_file parameter in XML config file! The file will be named \"data.txt\".", logfile, true);
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


void Stat::init_endpoint_key(XMLConfObj * config) {

  // extracting key of endpoints to monitor
  if (!config->nodeExists("endpoint_key")) {
    msgStr.print(MsgStream::WARN, "No endpoint_key parameter in XML config file! endpoint_key will be \"none\", i. e. that all traffic will be aggregated to endpoint 0.0.0.0:0|0", logfile, true);
  }
  else if (!(config->getValue("endpoint_key")).empty()) {

    std::string Keys = config->getValue("endpoint_key");

    if ( 0 == strcasecmp(Keys.c_str(), "all") ) {
    #ifdef OFFLINE_ENABLED
      StatStore::setIpMonitoring() = true;
      StatStore::setPortMonitoring() = true;
      StatStore::setProtocolMonitoring() = true;
    #else
      subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
      subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
      subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
      subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
      subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
    #endif
      return;
    }

    // all traffic will be aggregated to endpoint 0.0.0.0:0|0
    if ( 0 == strcasecmp(Keys.c_str(), "none") )
      return;

    std::istringstream KeyStream (Keys);
    std::string key;

    while (KeyStream >> key) {
      if ( 0 == strcasecmp(key.c_str(), "ip") ) {
      #ifdef OFFLINE_ENABLED
        StatStore::setIpMonitoring() = true;
      #else
        subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
        subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
      #endif
      }
      else if ( 0 == strcasecmp(key.c_str(), "port") ) {
      #ifdef OFFLINE_ENABLED
        StatStore::setPortMonitoring() = true;
      #else
        subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
        subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
      #endif
      }
      else if ( 0 == strcasecmp(key.c_str(), "protocol") ) {
      #ifdef OFFLINE_ENABLED
        StatStore::setProtocolMonitoring() = true;
      #else
        subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
      #endif
      }
      else {
        msgStr.print(MsgStream::ERROR, "Unknown value defined for endpoint_key parameter in XML config file! Value should be either port, ip, protocol OR any combination of them (seperated via spaces) OR \"all\" OR \"none\". Exiting.", logfile, true);
        stop();
      }
    }
  }
  // all traffic will be aggregated to endpoint 0.0.0.0:0|0
  else {
    msgStr.print(MsgStream::WARN, "No value for endpoint_key parameter defined in XML config file! endpoint_key will be \"none\", i. e. that all traffic will be aggregated to endpoint 0.0.0.0:0|0", logfile, true);
  }

  return;
}


void Stat::init_netmask(XMLConfObj * config) {

  if (!config->nodeExists("netmask")) {
    msgStr.print(MsgStream::WARN, "No netmask parameter defined in XML config file! \"32\" assumed.", logfile, true);
    StatStore::setNetmask() = 32;
  }
  else if ((config->getValue("netmask")).empty()) {
    msgStr.print(MsgStream::WARN, "No value for netmask parameter defined in XML config file! \"32\" assumed.", logfile, true);
    StatStore::setNetmask() = 32;
  }

  if (atoi(config->getValue("netmask").c_str()) >= 0 && atoi(config->getValue("netmask").c_str()) <= 32)
    StatStore::setNetmask() = atoi(config->getValue("netmask").c_str());
  else {
    msgStr.print(MsgStream::ERROR, "Unknown value for netmask parameter in XML config file! Please choose a value between 0 and 32 and restart. Exiting.", logfile, true);
    stop();
  }

  return;
}


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

  use_pca = ( 0 == strcasecmp("true", config->getValue("use_pca").c_str()) ) ? true:false;

  // initialize the other parameters only if pca is used
  if (use_pca == true) {

    // learning_phase
    if (!config->nodeExists("pca_learning_phase")) {
      std::stringstream Warning;
      Warning << "No pca_learning_phase parameter defined in XML config file! \"" << DEFAULT_learning_phase_for_pca << "\" assumed.";
      msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
      learning_phase_for_pca = DEFAULT_learning_phase_for_pca;
    }
    else if ( !(config->getValue("pca_learning_phase").empty()) ) {
      learning_phase_for_pca = atoi(config->getValue("pca_learning_phase").c_str());
    }
    else {
      std::stringstream Warning;
      Warning << "No value for pca_learning_phase parameter defined in XML config file! \"" << DEFAULT_learning_phase_for_pca << "\" assumed.";
      msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
      learning_phase_for_pca = DEFAULT_learning_phase_for_pca;
    }

    // type of matrix (covariance or correlation)
    if (!config->nodeExists("use_correlation_matrix")) {
      std::stringstream Warning;
      Warning << "No use_correlation_matrix parameter defined in XML config file!\n\"false\" assumed, i. e. covariance matrix will be used.";
      msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
      use_correlation_matrix = false;
    }
    else if ( !(config->getValue("use_correlation_matrix").empty()) ) {
      use_correlation_matrix = ( 0 == strcasecmp("true", config->getValue("use_correlation_matrix").c_str()) ) ? true:false;
    }
    else {
      std::stringstream Warning;
      Warning << "No value for use_correlation_matrix parameter defined in XML config file!\n\"false\" assumed, i. e. covariance matrix will be used.";
      msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
      use_correlation_matrix = false;
    }
  }
  return;
}

// extracting metrics to vector<Metric>;
// the values are stored there as constants
// defined in enum Metric in stat_main.h
void Stat::init_metrics(XMLConfObj * config) {

  std::stringstream Usage;
  Usage
    << "  Use for each <value>-Tag one of the following metrics:\n"
    << "  packets_in, packets_out, bytes_in, bytes_out, records_in, records_out, "
    << "  bytes_in/packet_in, bytes_out/packet_out, packets_in/record_in, "
    << "  packets_out/record_out, bytes_in/record_in, bytes_out/record_out, "
    << "  packets_out-packets_in, bytes_out-bytes_in, records_out-records_in, packets_in(t)-packets_in(t-1), "
    << "  packets_out(t)-packets_out(t-1), bytes_in(t)-bytes_in(t-1), bytes_out(t)-bytes_out(t-1),"
    << "  records_in(t)-records_in(t-1) or records_out(t)-records_out(t-1).\n";

  if (!config->nodeExists("metrics")) {
    msgStr.print(MsgStream::ERROR, "No metrics parameter in XML config file! Please define one and restart.\n" + Usage.str() + "Exiting.", logfile, true);
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
    msgStr.print(MsgStream::ERROR, "No value parameter(s) defined for metrics in XML config file! Please define at least one and restart.\n" + Usage.str() + "Exiting.", logfile, true);
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
    else if( 0 == strcasecmp("records_out-records_in",(*it).c_str()) )
      metrics.push_back(RECORDS_OUT_MINUS_RECORDS_IN);
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
    else if ( 0 == strcasecmp("records_in(t)-records_in(t-1)",(*it).c_str()) )
      metrics.push_back(RECORDS_T_IN_MINUS_RECORDS_T_1_IN);
    else if ( 0 == strcasecmp("records_out(t)-records_out(t-1)",(*it).c_str()) )
      metrics.push_back(RECORDS_T_OUT_MINUS_RECORDS_T_1_OUT);
    else {
        msgStr.print(MsgStream::ERROR, "Unknown value parameter(s) defined for metrics in XML config file! Please provide only valid <value>-parameters.\n" + Usage.str() + "Exiting.", logfile, true);
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
  if (use_pca == false) {
    Information
    << "Tests will be performed directly on the metrics. Values in the lists sample_old and sample_new will be "
    << "stored in the following order:\n";
  }
  else {
    Information
    << "Tests will be performed on the principal components of the following metrics:\n";
  }

  std::vector<Metric>::iterator val = metrics.begin();
  Information << "( ";
  while (val != metrics.end() ) {
    Information << getMetricName(*val) << " ";
    val++;
  }
  Information << ")";
  msgStr.print(MsgStream::INFO, Information.str(), logfile, true);

  if (use_pca == true) {
    std::stringstream Information1;
    Information1 << "Eigenvectors of the " << ((use_correlation_matrix == true)?"correlation matrix":"covariance matrix") << " will be used to transform the data.";
    msgStr.print(MsgStream::INFO, Information1.str(), logfile, true);
  }

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
    msgStr.print(MsgStream::WARN, "Directory \"" + output_dir + "\" couldn't be created. It either already exists or you dont't have enough rights. No output files will be generated.", logfile, true);
    return;
  }

  createFiles = true;
  return;
}

void Stat::init_noise_thresholds(XMLConfObj * config) {

  // extracting noise threshold for packets
  if(!config->nodeExists("noise_threshold_packets")) {
    std::stringstream Default;
    Default
      << "No noise_threshold_packets parameter defined in XML config file!"
      << " \"" << DEFAULT_noise_threshold_packets << "\" assumed.";
    msgStr.print(MsgStream::WARN, Default.str(), logfile, true);
    noise_threshold_packets = DEFAULT_noise_threshold_packets;
  }
  else if ( !(config->getValue("noise_threshold_packets").empty()) )
    noise_threshold_packets = atoi(config->getValue("noise_threshold_packets").c_str());
  else {
    std::stringstream Default;
    Default
      << "No value for noise_threshold_packets parameter defined in XML config file!"
      << " \"" << DEFAULT_noise_threshold_packets << "\" assumed.";
    msgStr.print(MsgStream::WARN, Default.str(), logfile, true);
    noise_threshold_packets = DEFAULT_noise_threshold_packets;
  }

    // extracting noise threshold for bytes
  if(!config->nodeExists("noise_threshold_bytes")) {
    std::stringstream Default;
    Default
      << "No noise_threshold_bytes parameter defined in XML config file!"
      << " \"" << DEFAULT_noise_threshold_bytes << "\" assumed.";
    msgStr.print(MsgStream::WARN, Default.str(), logfile, true);
    noise_threshold_bytes = DEFAULT_noise_threshold_bytes;
  }
  else if ( !(config->getValue("noise_threshold_bytes").empty()) )
    noise_threshold_bytes = atoi(config->getValue("noise_threshold_bytes").c_str());
  else {
    std::stringstream Default;
    Default
      << "No value for noise_threshold_bytes parameter defined in XML config file!"
      << " \"" << DEFAULT_noise_threshold_bytes << "\" assumed.";
    msgStr.print(MsgStream::WARN, Default.str(), logfile, true);
    noise_threshold_bytes = DEFAULT_noise_threshold_bytes;
  }

  return;
}

void Stat::init_endpointlist_maxsize(XMLConfObj * config) {

  std::stringstream Warning, Warning1;
  Warning
    << "No endpointlist_maxsize parameter defined in XML config file!"
    << " \"" << DEFAULT_endpointlist_maxsize << "\" assumed.";
  Warning1
    << "No value for endpointlist_maxsize parameter defined in XML config file!"
    << " \"" << DEFAULT_endpointlist_maxsize << "\" assumed.";

  if (!config->nodeExists("endpointlist_maxsize")) {
    msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
    endpointlist_maxsize = DEFAULT_endpointlist_maxsize;
  }
  else if (!(config->getValue("endpointlist_maxsize")).empty())
    endpointlist_maxsize = atoi( config->getValue("endpointlist_maxsize").c_str() );
  else {
    msgStr.print(MsgStream::WARN, Warning1.str(), logfile, true);
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
      Warning << "No value for x_frequently_endpoints parameter defined in XML "
      << "config file! \"" << DEFAULT_x_frequent_endpoints << "\" assumed.";

    if ((config->getValue("x_frequently_endpoints")).empty()) {
      msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
      x_frequently_endpoints = DEFAULT_x_frequent_endpoints;
    }
    else
      x_frequently_endpoints = atoi((config->getValue("x_frequently_endpoints")).c_str());

    std::ifstream dataFile;
    dataFile.open(offlineFile.c_str());
    if (!dataFile) {
      msgStr.print(MsgStream::FATAL, "Could't open offline file \"" + offlineFile + "\"!", logfile, true);
      stop();
    }

    while (true) {
      std::vector<FilterEndPoint> tmpData;
      std::string tmp;

      if ( dataFile.eof() ) {
        dataFile.close();
        break;
      }

      while ( getline(dataFile, tmp) ) {

        if (0 == strncmp("---",tmp.c_str(),3) )
          break;

        // extract endpoint-data
        FilterEndPoint fep;
        fep.fromString(tmp, false);

        // AGGREGATION (specified by endpoint_key and netmask)
        // as our data will be aggregated, the x_frequently_endpoints
        // need to be aggregated, too
        if (StatStore::getIpMonitoring() == false)
          fep.setIpAddress(IpAddress(0,0,0,0));
        else
          // apply the global aggregation netmask to fep's ip address
          fep.applyNetmask(StatStore::getNetmask());
        if (StatStore::getPortMonitoring() == false)
          fep.setPortNr(0);
        if (StatStore::getProtocolMonitoring() == false)
          fep.setProtocolID(0);

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

    // but first check, if there are so many at all
    // and if there aren't, use only as many as exist!
    if (endPointCount.size() < x_frequently_endpoints) {
      uint16_t x_tmp = x_frequently_endpoints;
      x_frequently_endpoints = endPointCount.size();
      std::stringstream Warning;
      Warning << "There are less than " << x_tmp << " (= x_frequently_endpoints) EndPoints to be monitored, thus only those " << endPointCount.size()
        << " will be monitored.";
      msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
    }

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
    std::stringstream Information;
    Information << "The " << x_frequently_endpoints << " most frequently endpoints of the data are: ";
    std::map<FilterEndPoint,int>::iterator iter;
    for (iter = mostFrequentEndPoints.begin(); iter != mostFrequentEndPoints.end(); iter++) {
      StatStore::AddEndPointToFilter(iter->first);
      Information << iter->first << "  ";
    }
    msgStr.print(MsgStream::INFO, Information.str(), logfile, true);

    return;
  }
#endif

// ONLINE MODE + OFFLINE MODE (if no <x_frequently_endpoints> parameter is provided)
// create EndPointFilter from a file specified in XML config file
// if no such file is specified --> MonitorEveryEndPoint = true
// The endpoints in the file are in the format
// ip1.ip2.ip3.ip4/netmask:port|protocol, e. g. 192.13.17.1/24:80|6
  if (!config->nodeExists("endpoints_to_monitor")) {
    msgStr.print(MsgStream::WARN, "No endpoints_to_monitor parameter defined in XML config file! All endpoints will be monitored.", logfile, true);
    StatStore::setMonitorEveryEndPoint() = true;
    return;
  }
  else if ((config->getValue("endpoints_to_monitor")).empty()) {
    msgStr.print(MsgStream::WARN, "No value for endpoints_to_monitor parameter defined in XML config file! All endpoints will be monitored.", logfile, true);
    StatStore::setMonitorEveryEndPoint() = true;
    return;
  }
  else if (0 == strcasecmp("all", config->getValue("endpoints_to_monitor").c_str())) {
    StatStore::setMonitorEveryEndPoint() = true;
    return;
  }

  std::ifstream f(config->getValue("endpoints_to_monitor").c_str());
  if (f.is_open() == false) {
    msgStr.print(MsgStream::ERROR, "The endpoints_to_monitor file (" + config->getValue("endpoints_to_monitor") + ") couldnt be opened! Please define the correct filter file or set this parameter to \"all\" to consider every EndPoint. Exiting.", logfile, true);
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
    std::stringstream Warning;
    Warning
      << "No stat_test_frequency parameter defined in XML config file!"
      << " \"" << DEFAULT_stat_test_frequency << "\" assumed.";
    msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
    stat_test_frequency = DEFAULT_stat_test_frequency;
  }
  else if (!(config->getValue("stat_test_frequency")).empty()) {
    stat_test_frequency =
      atoi( config->getValue("stat_test_frequency").c_str() );
  }
  else {
    std::stringstream Warning;
    Warning
      << "No value for stat_test_frequency parameter defined in XML config file!"
	    << " \"" << DEFAULT_stat_test_frequency << "\" assumed.";
    msgStr.print(MsgStream::WARN, Warning.str(), logfile, true);
    stat_test_frequency = DEFAULT_stat_test_frequency;
  }

  return;
}

void Stat::init_report_only_first_attack(XMLConfObj * config) {

  // extracting report_only_first_attack preference
  if (!config->nodeExists("report_only_first_attack")) {
    msgStr.print(MsgStream::WARN, "No report_only_first_attack parameter defined in XML config file! \"true\" assumed.", logfile, true);
    report_only_first_attack = true;
  }
  else if (!(config->getValue("report_only_first_attack")).empty()) {
    report_only_first_attack =
      ( 0 == strcasecmp("true", config->getValue("report_only_first_attack").c_str()) ) ? true:false;
  }
  else {
    msgStr.print(MsgStream::WARN, "No value for report_only_first_attack parameter defined in XML config file! \"true\" assumed.", logfile, true);
    report_only_first_attack = true;
  }

  return;
}

void Stat::init_pause_update_when_attack(XMLConfObj * config) {

  // extracting pause_update_when_attack preference
  if (!config->nodeExists("pause_update_when_attack")) {
    msgStr.print(MsgStream::WARN, "No pause_update_when_attack parameter defined in XML config file! No pausing assumed.", logfile, true);
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
          << "  3: pause for every metric individually (for cusum only)\nExiting.";
        msgStr.print(MsgStream::ERROR, Error.str(), logfile, true);
        stop();
        break;
    }
  }
  else {
    msgStr.print(MsgStream::WARN, "No value for pause_update_when_attack parameter defined in XML config file! No pausing assumed.", logfile ,true);
    pause_update_when_attack = 0;
  }

  return;
}

void Stat::init_cusum_test(XMLConfObj * config) {
  std::stringstream Warning1, Warning2, Warning3, Warning4, Warning5, Warning6, Warning7, Warning8;

  Warning1
    << "WARNING: No amplitude_percentage parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_amplitude_percentage << "\" assumed.\n";
  Warning2
    << "WARNING: No value for amplitude_percentage parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_amplitude_percentage << "\" assumed.\n";
  Warning3
    << "WARNING: No learning_phase_for_alpha parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_alpha << "\" assumed.\n";
  Warning4
    << "WARNING: No value for learning_phase_for_alpha parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_learning_phase_for_alpha << "\" assumed.\n";
  Warning5
    << "WARNING: No smoothing_constant parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_smoothing_constant << "\" assumed.\n";
  Warning6
    << "WARNING: No value for smoothing_constant parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_smoothing_constant << "\" assumed.\n";
  Warning7
    << "WARNING: No repetition_factor parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_repetition_factor << "\" assumed.\n";
  Warning8
    << "WARNING: No value for repetition_factor parameter "
    << "defined in XML config file!\n"
    << "  \"" << DEFAULT_repetition_factor << "\" assumed.\n";

  // cusum_test
  if (!config->nodeExists("cusum_test")) {
    msgStr.print(MsgStream::WARN, "No cusum_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_cusum_test = true;
  }
  else if (!(config->getValue("cusum_test")).empty()) {
    enable_cusum_test = ( 0 == strcasecmp("true", config->getValue("cusum_test").c_str()) ) ? true:false;
  }
  else {
    msgStr.print(MsgStream::WARN, "No value for cusum_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_cusum_test = true;
  }

  // the other parameters need only to be initialized,
  // if the cusum-test is enabled
  if (enable_cusum_test == true) {
    // amplitude_percentage
    if (!config->nodeExists("amplitude_percentage")) {
      msgStr.print(MsgStream::WARN, Warning1.str(), logfile, true);
      amplitude_percentage = DEFAULT_amplitude_percentage;
    }
    else if (!(config->getValue("amplitude_percentage")).empty())
      amplitude_percentage = atof(config->getValue("amplitude_percentage").c_str());
    else {
      msgStr.print(MsgStream::WARN, Warning2.str(), logfile, true);
      amplitude_percentage = DEFAULT_amplitude_percentage;
    }

    // learning_phase_for_alpha
    if (!config->nodeExists("learning_phase_for_alpha")) {
      msgStr.print(MsgStream::WARN, Warning3.str(), logfile, true);
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }
    else if (!(config->getValue("learning_phase_for_alpha")).empty()) {
      if ( atoi(config->getValue("learning_phase_for_alpha").c_str()) > 0)
        learning_phase_for_alpha = atoi(config->getValue("learning_phase_for_alpha").c_str());
      else {
        msgStr.print(MsgStream::ERROR, "Value for learning_phase_for_alpha parameter is zero or negative. Please define a positive value and restart. Exiting.", logfile, true);
        stop();
      }
    }
    else {
      msgStr.print(MsgStream::WARN, Warning4.str(), logfile, true);
      learning_phase_for_alpha = DEFAULT_learning_phase_for_alpha;
    }

    // smoothing constant for updating alpha per EWMA
    if (!config->nodeExists("smoothing_constant")) {
      msgStr.print(MsgStream::WARN, Warning5.str(), logfile, true);
      smoothing_constant = DEFAULT_smoothing_constant;
    }
    else if (!(config->getValue("smoothing_constant")).empty()) {
      if ( atof(config->getValue("smoothing_constant").c_str()) > 0.0)
        smoothing_constant = atof(config->getValue("smoothing_constant").c_str());
      else {
        msgStr.print(MsgStream::ERROR, "Value for smoothing_constant parameter is zero or negative. Please define a positive value and restart. Exiting.", logfile, true);
        stop();
      }
    }
    else {
      msgStr.print(MsgStream::WARN, Warning6.str(), logfile, true);
      smoothing_constant = DEFAULT_smoothing_constant;
    }

    // repetition factor
    if (!config->nodeExists("repetition_factor")) {
      msgStr.print(MsgStream::WARN, Warning7.str(), logfile ,true);
      repetition_factor = DEFAULT_repetition_factor;
    }
    else if (!(config->getValue("repetition_factor")).empty()) {
      if ( atoi(config->getValue("repetition_factor").c_str()) > 0)
        repetition_factor = atoi(config->getValue("repetition_factor").c_str());
      else {
        msgStr.print(MsgStream::ERROR, "Value for repetition_factor parameter is zero or negative. Please define a positive integer value and restart. Exiting.", logfile, true);
        stop();
      }
    }
    else {
      msgStr.print(MsgStream::WARN, Warning8.str(), logfile, true);
      repetition_factor = DEFAULT_repetition_factor;
    }

  }

  return;
}

void Stat::init_which_test(XMLConfObj * config) {
  // extracting type of test
  // (Wilcoxon and/or Kolmogorov and/or Pearson chi-square)
  if (!config->nodeExists("wilcoxon_test")) {
    msgStr.print(MsgStream::WARN, "No wilcoxon_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_wmw_test = true;
  }
  else if (!(config->getValue("wilcoxon_test")).empty()) {
    enable_wmw_test = ( 0 == strcasecmp("true", config->getValue("wilcoxon_test").c_str()) ) ? true:false;
  }
  else {
    msgStr.print(MsgStream::WARN, "No value for wilcoxon_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_wmw_test = true;
  }

  if (!config->nodeExists("kolmogorov_test")) {
    msgStr.print(MsgStream::WARN, "No kolmogorov_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_ks_test = true;
  }
  else if (!(config->getValue("kolmogorov_test")).empty()) {
    enable_ks_test = ( 0 == strcasecmp("true", config->getValue("kolmogorov_test").c_str()) ) ? true:false;
  }
  else {
    msgStr.print(MsgStream::WARN, "No value for kolmogorov_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_ks_test = true;
  }

  if (!config->nodeExists("pearson_chi-square_test")) {
    msgStr.print(MsgStream::WARN, "No pearson_chi-square_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_pcs_test = true;
  }
  else if (!(config->getValue("pearson_chi-square_test")).empty()) {
    enable_pcs_test = ( 0 == strcasecmp("true", config->getValue("pearson_chi-square_test").c_str()) ) ?true:false;
  }
  else {
    msgStr.print(MsgStream::WARN, "No value for pearson_chi-square_test parameter defined in XML config file! \"true\" assumed.", logfile, true);
    enable_pcs_test = true;
  }

  return;
}

void Stat::init_sample_sizes(XMLConfObj * config) {

  std::stringstream Warning_old, Warning1_old, Warning_new, Warning1_new;
  Warning_old
    << "No sample_old_size parameter defined in XML config file!"
	  << " \"" << DEFAULT_sample_old_size << "\" assumed.";
  Warning1_old
    << "No value for sample_old_size parameter defined in XML config file!"
    << " \"" << DEFAULT_sample_old_size << "\" assumed.";
  Warning_new
    << "No sample_new_size parameter defined in XML config file!"
    << " \"" << DEFAULT_sample_new_size << "\" assumed.";
  Warning1_new
    << "No value for sample_new_size parameter defined in XML config file!"
    << " \"" << DEFAULT_sample_new_size << "\" assumed.";

  // extracting size of sample_old
  if (!config->nodeExists("sample_old_size")) {
    msgStr.print(MsgStream::WARN, Warning_old.str(), logfile, true);
    sample_old_size = DEFAULT_sample_old_size;
  }
  else if (!(config->getValue("sample_old_size")).empty()) {
    sample_old_size =
      atoi( config->getValue("sample_old_size").c_str() );
  }
  else {
    msgStr.print(MsgStream::WARN, Warning1_old.str(), logfile, true);
    sample_old_size = DEFAULT_sample_old_size;
  }

  // extracting size of sample_new
  if (!config->nodeExists("sample_new_size")) {
    msgStr.print(MsgStream::WARN, Warning_new.str(), logfile, true);
    sample_new_size = DEFAULT_sample_new_size;
  }
  else if (!(config->getValue("sample_new_size")).empty()) {
    sample_new_size =
      atoi( config->getValue("sample_new_size").c_str() );
  }
  else {
    msgStr.print(MsgStream::WARN, Warning1_new.str(), logfile, true);
    sample_new_size = DEFAULT_sample_new_size;
  }

  return;
}

void Stat::init_wmw_two_sided(XMLConfObj * config) {

  // extracting one/two-sided parameter for the test
  if ( enable_wmw_test == true && !config->nodeExists("wmw_two_sided") ) {
    // one/two-sided parameter is active only for Wilcoxon-Mann-Whitney
    // statistical test; Pearson chi-square test and Kolmogorov-Smirnov
    // test are one-sided only.
    msgStr.print(MsgStream::WARN, "No wmw_two_sided parameter defined in XML config file! \"false\" assumed.", logfile, true);
    wmw_two_sided = false;
  }
  else if ( enable_wmw_test == true && config->getValue("wmw_two_sided").empty() ) {
    msgStr.print(MsgStream::WARN, "No value for wmw_two_sided parameter defined in XML config file! \"false\" assumed.", logfile, true);
    wmw_two_sided = false;
  }
  // Every value but "true" is assumed to be false!
  else if (config->nodeExists("wmw_two_sided") && !(config->getValue("wmw_two_sided")).empty() )
    wmw_two_sided = ( 0 == strcasecmp("true", config->getValue("wmw_two_sided").c_str()) ) ? true:false;

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
        msgStr.print(MsgStream::ERROR, "significance_level parameter should be between 0 and 1. Exiting.", logfile, true);
        stop();
      }
    }
    else {
      msgStr.print(MsgStream::WARN, "No value for significance_level parameter defined in XML config file! Nothing will be interpreted as an attack.", logfile, true);
      significance_level = -1;
      // means no alarmist verbose effect ("Attack!!!", etc):
      // as 0 <= p-value <= 1, we will always have
      // p-value > "significance_level" = -1,
      // i.e. nothing will be interpreted as an attack
      // and no verbose effect will be outputed
    }
  }
  else {
    msgStr.print(MsgStream::WARN, "No significance_level parameter defined in XML config file! Nothing will be interpreted as an attack.", logfile, true);
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

#ifndef OFFLINE_ENABLED
  if (storefile.is_open() == true)
    // store data storage for offline use
    storefile << Data << "--- " << test_counter << std::endl << std::flush;
#endif

  // Dumping empty records:
  if (Data.empty()==true) {
    if (logfile_output_verbosity>=3) {
      logfile << std::endl << "INFORMATION: Got empty record; "
        << "dumping it and waiting for another record" << std::endl << std::flush;
    }
    test_counter++;
    return;
  }

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
  // in our  "std::map<EndPoint, Params> EndpointParams" container.
  // If not, then we add it as a new pair <EndPoint, Params>.
  // If yes, then we update the corresponding entry using
  // std::vector<int64_t> extracted data.
  if (logfile_output_verbosity >= 3)
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

    std::map<EndPoint, Params>::iterator EndpointParams_it =
      EndpointParams.find(Data_it->first);

    if (EndpointParams_it == EndpointParams.end()) {
      // We didn't find the recorded EndPoint Data_it->first
      // in our container "EndpointParams"; that means it's a new one,
      // so we just add it in "EndpointParams"; there will not be jeopardy
      // of memory exhaustion through endless growth of the "EndpointParams" map
      // as limits are implemented in the StatStore class (EndPointListMaxSize)

      // Initialization
      Params P;
      P.correspondingEndPoint = (Data_it->first).toString();
      if (enable_wkp_test == true) {
        for (int i=0; i < metrics.size(); i++) {
          (P.last_wmw_test_was_attack).push_back(false);
          (P.last_ks_test_was_attack).push_back(false);
          (P.last_pcs_test_was_attack).push_back(false);
          (P.wmw_alarms).push_back(0);
          (P.ks_alarms).push_back(0);
          (P.pcs_alarms).push_back(0);
        }
      }
      if (enable_cusum_test == true) {
        for (int i = 0; i != metrics.size(); i++) {
          (P.sum).push_back(0);
          (P.alpha).push_back(0.0);
          (P.g).push_back(0.0);
          (P.last_cusum_test_was_attack).push_back(false);
          (P.X_curr).push_back(0);
          (P.X_last).push_back(0);
          (P.cusum_alarms).push_back(0);
        }
      }

      if (use_pca == true) {
        // initialize pca stuff
        P.init(metrics.size());
        pca_metric_data = extract_pca_data(P, Data_it->second, prev);
      }
      else {
        metric_data = extract_data(Data_it->second, prev);
        if (enable_wkp_test == true) {
          (P.Old).push_back(metric_data);
        }
        if (enable_cusum_test == true) {
          // add first value of each metric to the sum, which is needed
          // only to calculate the initial alpha after learning_phase_for_alpha
          // is over
          for (int i = 0; i != metric_data.size(); i++)
            P.sum.at(i) += metric_data.at(i);
          P.learning_phase_nr_for_alpha = 1;
        }
      }

      EndpointParams[Data_it->first] = P;

      if (logfile_output_verbosity >= 3) {
        logfile << "Added as new monitored endpoint" << std::endl;
        if (enable_wkp_test == true && logfile_output_verbosity >= 4 && use_pca == false)
          logfile << "  (WKP): with first element of sample_old: " << P.Old.back() << std::endl;
        if (enable_cusum_test == true && logfile_output_verbosity >= 3)
          logfile << "  (CUSUM): with initial values for sums of metrics for calculating alpha." << std::endl;
      }
    }
    else {
      // We found the recorded EndPoint Data_it->first
      // in our container "EndpointParams"; so we update the data
      if (use_pca == true) {
        pca_metric_data = extract_pca_data(EndpointParams_it->second, Data_it->second, prev);
        // Updates for pca metrics have to wait for the pca learning phase
        // needed to calculate the eigenvectors
        // NOTE: The overall learning phases for the tests will thus sum up to
        // WKP: learning_phase_pca + learning_phase_samples
        // Cusum: learning_phase_pca + learning_phase_for_alpha
        if ((EndpointParams_it->second).pca_ready == false) {
          if (logfile_output_verbosity >= 3)
            logfile << "Learning phase for PCA ..." << std::endl;
        }
        // this case happens exactly one time: when PCA is ready for the first time
        else if ((EndpointParams_it->second).pca_ready == true && pca_metric_data.empty() == true) {
          if (logfile_output_verbosity >= 3)
            logfile << "PCA learning phase is over! PCA is now ready!\n";
          if (logfile_output_verbosity == 5) {
            logfile << "Calculated " << ((use_correlation_matrix == true)?"correlation matrix:\n":"covariance matrix:\n");
            for (int i = 0; i < metrics.size(); i++) {
              for (int j = 0; j < metrics.size(); j++)
                logfile << gsl_matrix_get((EndpointParams_it->second).cov,i,j) << "\t";
              logfile << "\n";
            }
          }
        }
        else {
          if (enable_wkp_test == true)
            wkp_update ( EndpointParams_it->second, pca_metric_data );
          if (enable_cusum_test == true)
            cusum_update ( EndpointParams_it->second, pca_metric_data );
        }
      }
      else {
        metric_data = extract_data(Data_it->second, prev);
        if (enable_wkp_test == true)
          wkp_update ( EndpointParams_it->second, metric_data );
        if (enable_cusum_test == true)
          cusum_update ( EndpointParams_it->second, metric_data );
      }
    }

    // Create metric files, if wished so
    // (this can be done here, because metrics are the same for both tests)
    // in case of pca: don't create file until learning phase is over
    if ((createFiles == true) && ((use_pca == false) || (pca_metric_data.size() > 0))) {

      std::string fname;

      fname = (Data_it->first).toString() + "_metrics.txt";

      chdir(output_dir.c_str());

      std::ofstream file(fname.c_str(),std::ios_base::app);

      // are we at the beginning of the file?
      // if yes, write the metric names to the file ...
      long pos;
      pos = file.tellp();

      if (use_pca == true) {
        if (pos == 0) {
          file << "# ";
          for (int i = 0; i < pca_metric_data.size(); i++)
            file << "pca_comp_" << i << "\t";
          file << "Test-Run" << "\n";
        }
        for (int i = 0; i < pca_metric_data.size(); i++)
          file << pca_metric_data.at(i) << "\t";
        file << test_counter << "\n";
      }
      else {
        if (pos == 0) {
          file << "# ";
          for (int i = 0; i < metric_data.size(); i++)
            file << getMetricName(metrics.at(i)) << "\t";
          file << "Test-Run" << "\n";
        }
        for (int i = 0; i < metric_data.size(); i++)
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
  ep_nr = EndpointParams.size();

  if (logfile_output_verbosity >= 4) {
    logfile << std::endl << "#### STATE OF ALL MONITORED ENDPOINTS (" << ep_nr << "):" << std::endl;

    std::map<EndPoint,Params>::iterator EndpointParams_it = EndpointParams.begin();
      while (EndpointParams_it != EndpointParams.end()) {
        logfile << "[[ " << EndpointParams_it->first << " ]]\n";
        if (enable_wkp_test == true) {
          logfile << " (WKP):\n"
                  << "   sample_old (" << (EndpointParams_it->second).Old.size()  << ") : "
                  << (EndpointParams_it->second).Old << "\n"
                  << "   sample_new (" << (EndpointParams_it->second).New.size() << ") : "
                  << (EndpointParams_it->second).New << "\n";
        }
        if (enable_cusum_test == true) {
          logfile << " (CUSUM):\n"
                  << "   alpha: " << (EndpointParams_it->second).alpha << "\n"
                  << "   g: " << (EndpointParams_it->second).g << "\n";
        }
        EndpointParams_it++;
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
    // The other endpoints in the EndpointParams map are let learning.

    std::map<EndPoint,Params>::iterator EndpointParams_it = EndpointParams.begin();
    while (EndpointParams_it != EndpointParams.end()) {
      if ( enable_wkp_test == true && ((EndpointParams_it->second).New).size() == sample_new_size ) {
        // i.e. learning phase over
        if (logfile_output_verbosity > 0)
          logfile << "\n#### WKP TESTS for EndPoint [[ " << EndpointParams_it->first << " ]]\n";
        wkp_test ( EndpointParams_it->second );
      }
      if ( enable_cusum_test == true && (EndpointParams_it->second).ready_to_test == true ) {
        // i.e. learning phase for alpha is over and it has an initial value
        if (logfile_output_verbosity > 0)
          logfile << "\n#### CUSUM TESTS for EndPoint [[ " << EndpointParams_it->first << " ]]\n";
        cusum_test ( EndpointParams_it->second );
      }
      EndpointParams_it++;
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
            result.push_back(info.bytes_in / info.packets_in);
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
            result.push_back(info.bytes_out / info.packets_out);
        }
        else
          result.push_back(0);
        break;

      case PACKETS_IN_PER_RECORD_IN:
        if ( info.packets_in >= noise_threshold_packets) {
          if (info.records_in == 0)
            result.push_back(0);
          else
            result.push_back(info.packets_in / info.records_in );
        }
        else
          result.push_back(0);
        break;

      case PACKETS_OUT_PER_RECORD_OUT:
        if ( info.packets_out >= noise_threshold_packets) {
          if (info.records_out == 0)
            result.push_back(0);
          else
            result.push_back(info.packets_out / info.records_out);
        }
        else
          result.push_back(0);
        break;

      case BYTES_IN_PER_RECORD_IN:
        if ( info.bytes_in >= noise_threshold_bytes) {
          if (info.records_in == 0)
            result.push_back(0);
          else
            result.push_back(info.bytes_in / info.records_in);
        }
        else
          result.push_back(0);
        break;

      case BYTES_OUT_PER_RECORD_OUT:
        if ( info.bytes_out >= noise_threshold_bytes) {
          if (info.records_out == 0)
            result.push_back(0);
          else
            result.push_back(info.bytes_out / info.records_out);
        }
        else
          result.push_back(0);
        break;

      // NOTE: for the following 8 metrics, we use the absolute value to get
      // better results with our tests (negative values cause bias)

      case PACKETS_OUT_MINUS_PACKETS_IN:
        if (info.packets_out >= noise_threshold_packets
         || info.packets_in  >= noise_threshold_packets )
          result.push_back(abs(info.packets_out - info.packets_in));
        else
          result.push_back(0);
        break;

      case BYTES_OUT_MINUS_BYTES_IN:
        if (info.bytes_out >= noise_threshold_bytes
         || info.bytes_in  >= noise_threshold_bytes )
          result.push_back(abs(info.bytes_out - info.bytes_in));
        else
          result.push_back(0);
        break;

      case RECORDS_OUT_MINUS_RECORDS_IN:
        result.push_back(abs(info.records_out - info.records_in));
        break;

      case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
        if (info.packets_in  >= noise_threshold_packets
         || prev.packets_in  >= noise_threshold_packets)
          // prev holds the data for the same EndPoint as info
          // from the last call to test()
          // it is updated at the beginning of the while-loop in test()
          result.push_back(abs(info.packets_in - prev.packets_in));
        else
          result.push_back(0);
        break;

      case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
        if (info.packets_out >= noise_threshold_packets
         || prev.packets_out >= noise_threshold_packets)
          result.push_back(abs(info.packets_out - prev.packets_out));
        else
          result.push_back(0);
        break;

      case BYTES_T_IN_MINUS_BYTES_T_1_IN:
        if (info.bytes_in >= noise_threshold_bytes
         || prev.bytes_in >= noise_threshold_bytes)
          result.push_back(abs(info.bytes_in - prev.bytes_in));
        else
          result.push_back(0);
        break;

      case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
        if (info.bytes_out >= noise_threshold_bytes
         || prev.bytes_out >= noise_threshold_bytes)
          result.push_back(abs(info.bytes_out - prev.bytes_out));
        else
          result.push_back(0);
        break;

      case RECORDS_T_IN_MINUS_RECORDS_T_1_IN:
        result.push_back(abs(info.records_in - prev.records_in));
        break;

      case RECORDS_T_OUT_MINUS_RECORDS_T_1_OUT:
        result.push_back(abs(info.records_out - prev.records_out));
        break;

      default:
        msgStr.print(MsgStream::ERROR, "metrics seems to be empty or it holds an unknown type which isnt supported yet. But this shouldnt happen as the init_metrics function handles that. Please ckeck your configuration file. Exiting.", logfile, true);
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
        for (int j = 0; j <= i; j++) {
          // sumsOfProducts is a matrix which holds all the sums
          // of the product of each two metrics
          P.sumsOfProducts.at(i).at(j) += v.at(i)*v.at(j);
          P.sumsOfProducts.at(j).at(i) = P.sumsOfProducts.at(i).at(j);
        }
      }

      P.learning_phase_nr_for_pca++;
      return result; // empty
    }
    // end of learning phase
    else if (P.learning_phase_nr_for_pca == learning_phase_for_pca) {

      // calculate covariance matrix
      for (int i = 0; i < metrics.size(); i++) {
        for (int j = 0; j < metrics.size(); j++) {
            gsl_matrix_set(P.cov,i,j,covariance(P.sumsOfProducts.at(i).at(j),P.sumsOfMetrics.at(i),P.sumsOfMetrics.at(j)));
        }
      }

      // if wanted, calculate correlation matrix based on the covariance matrix
      if (use_correlation_matrix == true) {
        for (int i = 0; i < metrics.size(); i++) {
          // calculate standard deviation of metric i
          double sd_i;
          sd_i = standard_deviation(P.sumsOfProducts.at(i).at(i),P.sumsOfMetrics.at(i));
          for (int j = 0; j < metrics.size(); j++) {
            // calculate standard deviation of metric j
            double sd_j;
            if (i != j) {
              sd_j = standard_deviation(P.sumsOfProducts.at(j).at(j),P.sumsOfMetrics.at(j));
              gsl_matrix_set(P.cov,i,j,gsl_matrix_get(P.cov,i,j)/(sd_i*sd_j));
              // store the standard deviations (we need them later to normalize new incoming data)
              P.stddevs.at(i) = sd_i;
              P.stddevs.at(j) = sd_j;
            }
            // elements on the main diagonal are equal to 1
            else
              gsl_matrix_set(P.cov,i,j,1.0);
          }
        }
      }

//       for (int i = 0; i < metrics.size(); i++) {
//         std::cout << "stddev(" << i << ") : " << P.stddevs.at(i) << std::endl;
//       }
//       std::cout << std::endl;


      // calculate eigenvectors and -values
      gsl_vector *eval = gsl_vector_alloc (metrics.size());
      // some workspace needed for computation
      gsl_eigen_symmv_workspace * w = gsl_eigen_symmv_alloc (metrics.size());
      // computation of eigenvectors (evec) and -values (eval) from
      // covariance or correlation matrix (cov)
      gsl_eigen_symmv (P.cov, eval, P.evec, w);
      gsl_eigen_symmv_free (w);
      // sort the eigenvectors by their corresponding eigenvalue
      gsl_eigen_symmv_sort (eval, P.evec, GSL_EIGEN_SORT_VAL_DESC);
      gsl_vector_free (eval);

      // now, we have our components stored in each column of
      // evec, first column = most important, last column = least important

      // From now on, matrix evec can be used to transform the new arriving data

      P.pca_ready = true; // so this code will never be visited again

      return result; // empty
    }
  }

  // testing phase

  // fetch new metric data
  std::vector<int64_t> v = extract_data(info,prev);
  // transform it into a matrix (needed for multiplication)
  // X*1 matrix with X = #metrics,
  gsl_matrix *new_metric_data = gsl_matrix_calloc (metrics.size(),1);
  if (use_correlation_matrix == true) {
    // Normalization: Divide the metric values by the stddevs, if correlation matrix is used
    for (int i = 0; i < metrics.size(); i++)
      gsl_matrix_set(new_metric_data,i,0,((double)(v.at(i)) / P.stddevs.at(i)));
  } else  {
    for (int i = 0; i < metrics.size(); i++)
      gsl_matrix_set(new_metric_data,i,0,v.at(i));
  }

  // matrix multiplication to get the transformed data
  // transformed_data = evec^T * data
  gsl_matrix *transformed_metric_data = gsl_matrix_calloc (metrics.size(),1);
  gsl_blas_dgemm (CblasTrans, CblasNoTrans,
                       1.0, P.evec, new_metric_data,
                       0.0, transformed_metric_data);

  // transform the matrix with the transformed data back into a vector
  if (use_correlation_matrix == true) {
    for (int i = 0; i < metrics.size(); i++)
	    result.push_back((int64_t) (gsl_matrix_get(transformed_metric_data,i,0) * 100)); // *100 to reduce rounding error
  }
  else {
    for (int i = 0; i < metrics.size(); i++)
	    result.push_back((int64_t) gsl_matrix_get(transformed_metric_data,i,0));
  }

  gsl_matrix_free(new_metric_data);
  gsl_matrix_free(transformed_metric_data);

  return result; // filled with new transformed values
}


// -------------------------- LEARN/UPDATE FUNCTION ---------------------------

// learn/update function for samples (called everytime test() is called)
//
void Stat::wkp_update ( Params & P, const std::vector<int64_t> & new_value ) {

  // Learning phase?
  if (P.Old.size() != sample_old_size) {

    P.Old.push_back(new_value);

    if (logfile_output_verbosity >= 3) {
      logfile << "  (WKP): Learning phase for sample_old ..." << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << P.Old << std::endl;
        logfile << "   sample_new: " << P.New << std::endl;
      }
    }

    return;
  }
  else if (P.New.size() != sample_new_size) {

    P.New.push_back(new_value);

    if (logfile_output_verbosity >= 3) {
      logfile << "  (WKP): Learning phase for sample_new..." << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << P.Old << std::endl;
        logfile << "   sample_new: " << P.New << std::endl;
      }
    }

    return;
  }

  // Learning phase over: update

  // pausing update for old sample

  bool at_least_one_wmw_test_was_attack = false;
  bool at_least_one_ks_test_was_attack = false;
  bool at_least_one_pcs_test_was_attack = false;
  for (int i = 0; i < P.last_wmw_test_was_attack.size(); i++) {
    if (P.last_wmw_test_was_attack.at(i) == true)
      at_least_one_wmw_test_was_attack = true;
    if (P.last_ks_test_was_attack.at(i) == true)
      at_least_one_ks_test_was_attack = true;
    if (P.last_pcs_test_was_attack.at(i) == true)
      at_least_one_pcs_test_was_attack = true;
  }
/*
  bool all_wmw_tests_were_attacks = true;
  bool all_ks_tests_were_attacks = true;
  bool all_pcs_tests_were_attacks = true;
  for (int i = 0; i < P.last_wmw_test_was_attack.size(); i++) {
    if (P.last_wmw_test_was_attack.at(i) == false)
      all_wmw_tests_were_attacks = false;
    if (P.last_ks_test_was_attack.at(i) == false)
      all_ks_tests_were_attacks = false;
    if (P.last_pcs_test_was_attack.at(i) == false)
      all_pcs_tests_were_attacks = false;
  }
*/

  if ( (pause_update_when_attack == 1
        && ( at_least_one_wmw_test_was_attack == true
          || at_least_one_ks_test_was_attack  == true
          || at_least_one_pcs_test_was_attack == true ))
    || (pause_update_when_attack == 2
        && ( at_least_one_wmw_test_was_attack == true
          && at_least_one_ks_test_was_attack  == true
          && at_least_one_pcs_test_was_attack == true )) ) {

    P.New.pop_front();
    P.New.push_back(new_value);

    if (logfile_output_verbosity >= 3) {
      logfile << "  (WKP): Update done (for new sample only)" << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << P.Old << std::endl;
        logfile << "   sample_new: " << P.New << std::endl;
      }
    }
  }
  // if parameter is 0 (or 3) or there was no attack detected
  // update both samples
  else {
    P.Old.pop_front();
    P.Old.push_back(P.New.front());
    P.New.pop_front();
    P.New.push_back(new_value);

    if (logfile_output_verbosity >= 3) {
      logfile << "  (WKP): Update done (for both samples)" << std::endl;
      if (logfile_output_verbosity >= 4) {
        logfile << "   sample_old: " << P.Old << std::endl;
        logfile << "   sample_new: " << P.New << std::endl;
      }
    }
  }

  P.wkp_updated = true;

  logfile << std::flush;
  return;
}


// and the update funciotn for the cusum-test
void Stat::cusum_update ( Params & P, const std::vector<int64_t> & new_value ) {

  // Learning phase for alpha?
  // that means, we dont have enough values to calculate alpha
  // until now (and so cannot perform the cusum test)
  if (P.ready_to_test == false) {
    if (P.learning_phase_nr_for_alpha < learning_phase_for_alpha) {

      // update the sum of the values of each metric
      // (needed to calculate the initial alpha)
      for (int i = 0; i != P.sum.size(); i++)
        P.sum.at(i) += new_value.at(i);

      if (logfile_output_verbosity >= 3)
        logfile << "  (CUSUM): Learning phase for alpha ...\n";

      P.learning_phase_nr_for_alpha++;

      return;
    }

    // Enough values? Calculate initial alpha per simple average
    // and set ready_to_test-flag to true (so we never visit this
    // code here again for the current endpoint)
    if (P.learning_phase_nr_for_alpha == learning_phase_for_alpha) {

      if (logfile_output_verbosity >= 3)
        logfile << "  (CUSUM): Learning phase for alpha is over.\n";
      if (logfile_output_verbosity >= 4)
        logfile << "   Calculated initial alphas: ( ";

      for (int i = 0; i != P.alpha.size(); i++) {
        // alpha = sum(values) / #(values)
        // Note: learning_phase_for_alpha is never 0, because
        // this is handled in init_cusum_test()
        P.alpha.at(i) = P.sum.at(i) / learning_phase_for_alpha;
        P.X_curr.at(i) = new_value.at(i);
        if (logfile_output_verbosity >= 4)
          logfile << P.alpha.at(i) << " ";
      }
      if (logfile_output_verbosity >= 4)
          logfile << ")\n";

      P.ready_to_test = true;
      return;
    }
  }

  // pausing update for alpha (depending on pause_update parameter)
  bool at_least_one_test_was_attack = false;
  for (int i = 0; i < P.last_cusum_test_was_attack.size(); i++)
    if (P.last_cusum_test_was_attack.at(i) == true)
      at_least_one_test_was_attack = true;

  bool all_tests_were_attacks = true;
  for (int i = 0; i < P.last_cusum_test_was_attack.size(); i++)
    if (P.last_cusum_test_was_attack.at(i) == false)
      all_tests_were_attacks = false;

  // pause, if at least one metric yielded an alarm
  if ( pause_update_when_attack == 1
    && at_least_one_test_was_attack == true) {
    if (logfile_output_verbosity >= 3)
      logfile << "  (CUSUM): Pausing update for alpha (at least one test was attack)\n";
    // update values for X
    for (int i = 0; i != P.X_curr.size(); i++) {
      P.X_last.at(i) = P.X_curr.at(i);
      P.X_curr.at(i) = new_value.at(i);
    }
    return;
  }
  // pause, if all metrics yielded an alarm
  else if ( pause_update_when_attack == 2
    && all_tests_were_attacks == true) {
    if (logfile_output_verbosity >= 3)
      logfile << "  (CUSUM): Pausing update for alpha (all tests were attacks)\n";
    // update values for X
    for (int i = 0; i != P.X_curr.size(); i++) {
      P.X_last.at(i) = P.X_curr.at(i);
      P.X_curr.at(i) = new_value.at(i);
    }
    return;
  }
  // pause for those metrics, which yielded an alarm
  // and update only the others
  else if (pause_update_when_attack == 3
    && at_least_one_test_was_attack == true) {
    if (logfile_output_verbosity >= 3)
      logfile << "  (CUSUM): Pausing update for alpha (for those metrics which were attacks)\n";
    for (int i = 0; i != P.alpha.size(); i++) {
      // update values for X
      P.X_last.at(i) = P.X_curr.at(i);
      P.X_curr.at(i) = new_value.at(i);
      if (P.last_cusum_test_was_attack.at(i) == false) {
        // update alpha
        P.alpha.at(i) = P.alpha.at(i) * (1 - smoothing_constant) + (double) P.X_last.at(i) * smoothing_constant;
      }
    }
    return;
  }


  // Otherwise update all alphas per EWMA
  for (int i = 0; i != P.alpha.size(); i++) {
    // update values for X
    P.X_last.at(i) = P.X_curr.at(i);
    P.X_curr.at(i) = new_value.at(i);
    // update alpha
    P.alpha.at(i) = P.alpha.at(i) * (1 - smoothing_constant) + (double) P.X_last.at(i) * smoothing_constant;
  }

  if (logfile_output_verbosity >= 3)
    logfile << "  (CUSUM): Update done for alpha\n";
  if (logfile_output_verbosity >= 4)
    logfile << P.alpha << "\n";

  P.cusum_updated = true;
  logfile << std::flush;
  return;
}

// ------- FUNCTIONS USED TO CONDUCT TESTS ON THE SAMPLES ---------

// statistical test function for wkp-tests
// (optional, depending on how often the user wishes to do it)
void Stat::wkp_test (Params & P) {

  // Containers for the values of single metrics
  std::list<int64_t> sample_old_single_metric;
  std::list<int64_t> sample_new_single_metric;

  std::vector<Metric>::iterator it = metrics.begin();

  // for every value (represented by *it) in metrics,
  // do the tests
  short index = 0;

  while (it != metrics.end()) {

    if (logfile_output_verbosity == 5) {
      if (use_pca == false)
        logfile << "### Performing WKP-Tests for metric " << getMetricName(*it) << ":\n";
      else {
        std::stringstream tmp;
        tmp << index;
        logfile << "### Performing WKP-Tests for pca component " << tmp.str() << ":\n";
      }
    }

    // storing the last-test flags
    bool tmp_wmw = P.last_wmw_test_was_attack.at(index);
    bool tmp_ks = P.last_ks_test_was_attack.at(index);
    bool tmp_pcs = P.last_pcs_test_was_attack.at(index);

    sample_old_single_metric = getSingleMetric(P.Old, index);
    sample_new_single_metric = getSingleMetric(P.New, index);

    double p_wmw, p_ks, p_pcs;
    p_wmw = p_ks = p_pcs = 1.0;

    // Wilcoxon-Mann-Whitney test:
    if (enable_wmw_test == true && P.wkp_updated) {
      p_wmw = stat_test_wmw(sample_old_single_metric, sample_new_single_metric, tmp_wmw);
      // New anomaly?
      if (significance_level > p_wmw && (report_only_first_attack == false || P.last_wmw_test_was_attack.at(index) == false))
        (P.wmw_alarms).at(index)++;
      P.last_wmw_test_was_attack.at(index) = tmp_wmw;
    }

    // Kolmogorov-Smirnov test:
    if (enable_ks_test == true && P.wkp_updated) {
      p_ks = stat_test_ks (sample_old_single_metric, sample_new_single_metric, tmp_ks);
      if (significance_level > p_ks && (report_only_first_attack == false || P.last_ks_test_was_attack.at(index) == false))
        (P.ks_alarms).at(index)++;
      P.last_ks_test_was_attack.at(index) = tmp_ks;
    }

    // Pearson chi-square test:
    if (enable_pcs_test == true && P.wkp_updated) {
      p_pcs = stat_test_pcs(sample_old_single_metric, sample_new_single_metric, tmp_pcs);
      if (significance_level > p_pcs && (report_only_first_attack == false || P.last_pcs_test_was_attack.at(index) == false))
        (P.pcs_alarms).at(index)++;
      P.last_pcs_test_was_attack.at(index) = tmp_pcs;
    }

    if (P.wkp_updated == false) {
      P.last_wmw_test_was_attack.at(index) = false;
      P.last_ks_test_was_attack.at(index) = false;
      P.last_pcs_test_was_attack.at(index) = false;
    }

    // generate output files, if wished
    if (createFiles == true) {

      std::string filename;
      if (use_pca == false)
        filename = P.correspondingEndPoint + "." + getMetricName(*it) + ".wkpparams.txt";
      else {
        std::stringstream tmp;
        tmp << index;
        filename = P.correspondingEndPoint + ".pca_comp_" + tmp.str() + ".wkpparams.txt";
      }

      chdir(output_dir.c_str());

      std::ofstream file(filename.c_str(), std::ios_base::app);

      // are we at the beginning of the file?
      // if yes, write the param names to the file ...
      long pos;
      pos = file.tellp();

      if (pos == 0) {
        file << "#Value" << "\t" << "p(wmw)" << "\t" << "alarms(wmw)" << "\t"
        << "p(ks)" << "\t" << "alarms(ks)" << "\t" << "p(pcs)" << "\t"
        << "alarms(pcs)" << "\t" << "Slevel" << "\t" << "Test-Run\n";
      }

      // metric p-value(wmw) #alarms(wmw) p-value(ks) #alarms(ks)
      // p-value(pcs) #alarms(pcs) counter
      if(P.wkp_updated) {
        file << sample_new_single_metric.back() << "\t" << p_wmw << "\t"
          << (P.wmw_alarms).at(index) << "\t" << p_ks << "\t"
          << (P.ks_alarms).at(index) << "\t" << p_pcs << "\t"
          << (P.pcs_alarms).at(index) << "\t" << significance_level << "\t"
          << test_counter << "\n";
      }
      else {
        file << "0" << "\t" << p_wmw << "\t"
          << (P.wmw_alarms).at(index) << "\t" << p_ks << "\t"
          << (P.ks_alarms).at(index) << "\t" << p_pcs << "\t"
          << (P.pcs_alarms).at(index) << "\t" << significance_level << "\t"
          << test_counter << "\n";
      }

      file.close();
      chdir("..");
    }

    it++;
    index++;
  }

  P.wkp_updated = false;
  logfile << std::flush;
  return;

}

// statistical test function / cusum-test
// (optional, depending on how often the user wishes to do it)
void Stat::cusum_test(Params & P) {

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

    if (logfile_output_verbosity == 5) {
      if (use_pca == false)
        logfile << "### Performing CUSUM-Test for metric " << getMetricName(*it) << ":\n";
      else {
        std::stringstream tmp;
        tmp << i;
        logfile << "### Performing CUSUM-Test for pca component " << tmp.str() << ":\n";
      }
    }

    // Calculate N and beta
    N = repetition_factor * (amplitude_percentage * fabs(P.alpha.at(i)) / 2.0);
    beta = P.alpha.at(i) + (amplitude_percentage * fabs(P.alpha.at(i)) / 2.0);

    if (logfile_output_verbosity == 5) {
      logfile << " Cusum test returned:\n"
              << "  Threshold: " << N << std::endl;
      logfile << "  reject H0 (no attack) if current value of statistic g > "
              << N << std::endl;
    }

    // TODO
    // "attack still in progress"-message?

    // perform the test and if g > N raise an alarm
    if ( P.cusum_updated && (cusum(P.X_curr.at(i), beta, P.g.at(i)) > N )) {

      if (report_only_first_attack == false
        || P.last_cusum_test_was_attack.at(i) == false) {

        (P.cusum_alarms).at(i)++;

        if (logfile_output_verbosity >= 2) {
          logfile
            << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
            << "    " << P.correspondingEndPoint << " for metric " << getMetricName(*it) << "\n"
            << "    Cusum test says we're under attack (g = " << P.g.at(i) << ")!\n"
            << "    ALARM! ALARM! Women und children first!" << std::endl;
          std::cout
            << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
            << "  " << P.correspondingEndPoint << " for metric " << getMetricName(*it) << "\n"
            << "  Cusum test says we're under attack!\n"
            << "  ALARM! ALARM! Women und children first!" << std::endl;
        }
        if (logfile_output_verbosity == 1) {
          logfile << "cusum: attack for metric " << getMetricName(*it) << " detected!\n" << std::flush;
          std::cout << "cusum: attack for metric " << getMetricName(*it) << " detected!\n" << std::endl;
        }
        #ifdef IDMEF_SUPPORT_ENABLED
          idmefMessage.setAnalyzerAttr("", "", "cusum-test", "");
          sendIdmefMessage("DDoS", idmefMessage);
          idmefMessage = getNewIdmefMessage();
        #endif
      }

      was_attack.at(i) = true;

    }
    else if (!P.cusum_updated) // reset g to 0 if no new value for that Endpoint occurred
      P.g.at(i) = 0;

    if (createFiles == true) {

      std::string filename;
      if (use_pca == false)
        filename = P.correspondingEndPoint  + "." + getMetricName(*it) + ".cusumparams.txt";
      else {
        std::stringstream tmp;
        tmp << i;
        filename = P.correspondingEndPoint  + ".pca_comp_" + tmp.str() + ".cusumparams.txt";
      }

      chdir(output_dir.c_str());

      std::ofstream file(filename.c_str(), std::ios_base::app);

      // are we at the beginning of the file?
      // if yes, write the param names to the file ...
      long pos;
      pos = file.tellp();

      if (pos == 0) {
        file << "#Value" << "\t" << "g" << "\t" << "N" << "\t"
        << "alpha" << "\t" << "beta" << "\t" << "alarms"
        << "\t" << "Test-Run\n";
      }

      // X  g N alpha beta #alarms counter
      if (P.cusum_updated) {
      file << (int) P.X_curr.at(i) << "\t" << (int) P.g.at(i)
          << "\t" << (int) N << "\t" << (int) P.alpha.at(i) << "\t"  << (int) beta
          << "\t" << (P.cusum_alarms).at(i) << "\t" << test_counter << "\n";
      }
      else {
      file << "0" << "\t" << (int) P.g.at(i)
        << "\t" << (int) N << "\t" << (int) P.alpha.at(i) << "\t"  << (int) beta
        << "\t" << (P.cusum_alarms).at(i) << "\t" << test_counter << "\n";
      }
      file.close();
      chdir("..");
    }

    i++;
  }

  if (P.cusum_updated) {
    for (int i = 0; i != was_attack.size(); i++) {
      if (was_attack.at(i) == true)
        P.last_cusum_test_was_attack.at(i) = true;
      else
        P.last_cusum_test_was_attack.at(i) = false;
    }
  }
  else {
    for (int i = 0; i != P.last_cusum_test_was_attack.size(); i++)
      P.last_cusum_test_was_attack.at(i) = false;
  }

  P.cusum_updated = false;
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
    case RECORDS_OUT_MINUS_RECORDS_IN:
      return std::string("records_out-records_in");
    case PACKETS_T_IN_MINUS_PACKETS_T_1_IN:
      return std::string("packets_in(t)-packets_in(t-1)");
    case PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT:
      return std::string("packets_out(t)-packets_out(t-1)");
    case BYTES_T_IN_MINUS_BYTES_T_1_IN:
      return std::string("bytes_in(t)-bytes_in(t-1)");
    case BYTES_T_OUT_MINUS_BYTES_T_1_OUT:
      return std::string("bytes_out(t)-bytes_out(t-1)");
    case RECORDS_T_IN_MINUS_RECORDS_T_1_IN:
      return std::string("records_in(t)-records_in(t-1)");
    case RECORDS_T_OUT_MINUS_RECORDS_T_1_OUT:
      return std::string("records_out(t)-records_out(t-1)");
    default:
      msgStr.print(MsgStream::ERROR, "Unknown type of Metric in getMetricName(). Exiting.", logfile, true);
      stop();
  }
}

double Stat::stat_test_wmw (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new, bool & last_wmw_test_was_attack) {

  double p;

  if (logfile_output_verbosity == 5) {
    logfile << " Wilcoxon-Mann-Whitney test details:\n";
    p = wmw_test(sample_old, sample_new, wmw_two_sided, logfile);
    logfile << " Wilcoxon-Mann-Whitney test returned:\n"
      << "  p-value: " << p << std::endl;
    logfile << "  reject H0 (no attack) to any significance level alpha > "
      << p << std::endl;
  }
  else {
    std::ofstream dump("/dev/null");
    p = wmw_test(sample_old, sample_new, wmw_two_sided, dump);
  }


  if (significance_level > p) {
    if (report_only_first_attack == false
    || last_wmw_test_was_attack == false) {
      if (logfile_output_verbosity >= 2) {
        logfile
          << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
          << "    Wilcoxon-Mann-Whitney test says we're under attack!\n"
          << "    ALARM! ALARM! Women und children first!" << std::endl;
        std::cout
          << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
          << "  Wilcoxon-Mann-Whitney test says we're under attack!\n"
          << "  ALARM! ALARM! Women und children first!" << std::endl;
      }
      if (logfile_output_verbosity == 1) {
        logfile << "wmw: " << p << std::endl;
        logfile << "attack to significance level " << significance_level << "!" << std::endl;
        std::cout << "wmw: attack to significance level " << significance_level << "!" << std::endl;
      }
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

  return p;
}

double Stat::stat_test_ks (std::list<int64_t> & sample_old,
			 std::list<int64_t> & sample_new, bool & last_ks_test_was_attack) {

  double p;

  if (logfile_output_verbosity == 5) {
    logfile << " Kolmogorov-Smirnov test details:\n";
    p = ks_test(sample_old, sample_new, logfile);
    logfile << " Kolmogorov-Smirnov test returned:\n"
      << "  p-value: " << p << std::endl;
    logfile << "  reject H0 (no attack) to any significance level alpha > "
      << p << std::endl;
  }
  else {
    std::ofstream dump ("/dev/null");
    p = ks_test(sample_old, sample_new, dump);
  }


  if (significance_level > p) {
    if (report_only_first_attack == false
    || last_ks_test_was_attack == false) {
      if (logfile_output_verbosity >= 2) {
        logfile << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
          << "    Kolmogorov-Smirnov test says we're under attack!\n"
          << "    ALARM! ALARM! Women und children first!" << std::endl;
        std::cout << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
          << "  Kolmogorov-Smirnov test says we're under attack!\n"
          << "  ALARM! ALARM! Women und children first!" << std::endl;
      }
      if (logfile_output_verbosity == 1) {
        logfile << "ks : " << p << std::endl;
        logfile << "attack to significance level " << significance_level << "!" << std::endl;
        std::cout << "ks: attack to significance level " << significance_level << "!" << std::endl;
      }
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

  return p;
}


double Stat::stat_test_pcs (std::list<int64_t> & sample_old,
			  std::list<int64_t> & sample_new, bool & last_pcs_test_was_attack) {

  double p;

  if (logfile_output_verbosity == 5) {
    logfile << " Pearson chi-square test details:\n";
    p = pcs_test(sample_old, sample_new, logfile);
    logfile << " Pearson chi-square test returned:\n"
      << "  p-value: " << p << std::endl;
    logfile << "  reject H0 (no attack) to any significance level alpha > "
      << p << std::endl;
  }
  else {
    std::ofstream dump ("/dev/null");
    p = pcs_test(sample_old, sample_new, dump);
  }


    if (significance_level > p) {
      if (report_only_first_attack == false
	    || last_pcs_test_was_attack == false) {
        if (logfile_output_verbosity >= 2) {
          logfile << "    ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
            << "    Pearson chi-square test says we're under attack!\n"
            << "    ALARM! ALARM! Women und children first!" << std::endl;
          std::cout << "  ATTACK! ATTACK! ATTACK! (@" << test_counter << ")\n"
            << "  Pearson chi-square test says we're under attack!\n"
            << "  ALARM! ALARM! Women und children first!" << std::endl;
        }
        if (logfile_output_verbosity == 1) {
          logfile << "pcs: " << p << std::endl;
          logfile << "attack to significance level " << significance_level << "!" << std::endl;
          std::cout << "pcs: attack to significance level " << significance_level << "!" << std::endl;
        }
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

  return p;
}

double Stat::covariance (const double & sumProduct, const double & sumX, const double & sumY) {
  // calculate the covariance
  // KOV = 1/N-1 * sum(x1 - n1)(x2 - n2)
  // = 1/N-1 * sum(x1x2 - x1n2 - x2n1 + n1n2)
  // = 1/N-1 * (sum(x1x2) - n2*sum(x1) - n1*sum(x2) + N*n1n2)
  // with n1 = 1/N * sum(x1) and n2 = 1/N * sum(x2)
  // KOV = 1/N-1 * ( sum(x1x2) - 1/N * (sum(x1)sum(x2)) )
  return (sumProduct - (sumX*sumY) / learning_phase_for_pca) / (learning_phase_for_pca - 1.0);
}

double Stat::standard_deviation (const double & sumProduct, const double & sumX) {
  // calculate the standard deviation with m = mean value of x
  // SA = sqrt(1/N-1 * (sum((x - m)^2)))
  //    = sqrt(1/N-1 * (sum(x^2 - 2*x*m + m^2)))
  //    = sqrt(1/N-1 * (sum(x^2) - 2*m*sum(x) + sum(m^2)))
  //    = sqrt(1/N-1 * (sum(x^2) - 2*m*sum(x) + N*m^2))
  // with m = 1/N * sum(x)
  //    = sqrt(1/N-1 * (sum(x^2) - 2/N*(sum(x))^2 + 1/N*(sum(x))^2))
  //    = sqrt(1/N-1 * (sum(x^2) - 1/N*(sum(x))^2))
  return sqrt(fabs((sumProduct - (sumX*sumX) / learning_phase_for_pca) / (learning_phase_for_pca - 1.0)));
}

void Stat::sigTerm(int signum)
{
	stop();
}

void Stat::sigInt(int signum)
{
	stop();
}
