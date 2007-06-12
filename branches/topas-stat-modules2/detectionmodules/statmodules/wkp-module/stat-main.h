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

#ifndef _STAT_MAIN_H_
#define _STAT_MAIN_H_

#include "stat-store.h"
#include "shared.h"
#include "params.h"
#include <detectionbase.h>
#include <sstream>
#include <algorithm> // sort(...), unique(...)

// ========================== CLASS Stat ==========================

#define DEFAULT_alarm_time 10
#define DEFAULT_warning_verbosity 3
#define DEFAULT_logfile_output_verbosity 3
#define DEFAULT_noise_threshold_packets 0
#define DEFAULT_noise_threshold_bytes 0
#define DEFAULT_endpointlist_maxsize 500
#define DEFAULT_x_frequent_endpoints 10
#define DEFAULT_amplitude_percentage 1.5
#define DEFAULT_repetition_factor 2
#define DEFAULT_learning_phase_for_alpha 10
#define DEFAULT_smoothing_constant 0.15
#define DEFAULT_sample_old_size 111
#define DEFAULT_sample_new_size 11
#define DEFAULT_stat_test_frequency 1
#define DEFAULT_learning_phase_for_pca 50

// constants for metrics
enum Metric {
  PACKETS_IN,
  PACKETS_OUT,
  BYTES_IN,
  BYTES_OUT,
  RECORDS_IN,
  RECORDS_OUT,
  BYTES_IN_PER_PACKET_IN,
  BYTES_OUT_PER_PACKET_OUT,
  PACKETS_IN_PER_RECORD_IN,
  PACKETS_OUT_PER_RECORD_OUT,
  BYTES_IN_PER_RECORD_IN,
  BYTES_OUT_PER_RECORD_OUT,
  PACKETS_OUT_MINUS_PACKETS_IN,
  BYTES_OUT_MINUS_BYTES_IN,
  PACKETS_T_IN_MINUS_PACKETS_T_1_IN,
  PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT,
  BYTES_T_IN_MINUS_BYTES_T_1_IN,
  BYTES_T_OUT_MINUS_BYTES_T_1_OUT,
};

// main class: does tests, stores samples and params,
// reads and stores test parameters...

class Stat
#ifdef OFFLINE_ENABLED
      : public DetectionBase<StatStore, OfflineInputPolicy<StatStore> >
#else
      : public DetectionBase<StatStore>
#endif
{

 public:

  Stat(const std::string & configfile);
  ~Stat();

  /* Test function. This function will be called in periodic intervals.
   *   Set interval size with @c setAlarmTime().
   * @param store : Pointer to a storage object of class StatStore containing
   *   all data collected since last call to test. This pointer points to
   *   your memory. That means that you may store the pointer.
   *   IMPORTANT: _YOU_ have to free the object's memory when you don't need
   *   it any longer (use delete-operator to do that). */
  void test(StatStore * store);

#ifdef IDMEF_SUPPORT_ENABLED
	/**
         * Update function. This function will be called, whenever a message
         * for subscribed key is received from xmlBlaster.
         * @param xmlObj Pointer to data structure, containing xml data
         *               You have to delete the memory allocated for the object.
         */
  void update(XMLConfObj* xmlObj);
#endif


 private:

  // signal handlers
  static void sigTerm(int);
  static void sigInt(int);

  // this function is called by the Stat constructor, its job is to extract
  // user's preferences and test parameters from the XML config file:
  void init(const std::string & configfile);

  // as the init function is really huge, we divide it into tasks:
  // those related to the user's preferences regarding the module...
  void init_logfile(XMLConfObj *);
  void init_accepted_source_ids(XMLConfObj *);
  void init_alarm_time(XMLConfObj *);
  void init_warning_verbosity(XMLConfObj *);
  void init_logfile_output_verbosity(XMLConfObj *);
  void init_offline_file(XMLConfObj *);
  void init_output_dir(XMLConfObj *);
  void init_endpoint_key(XMLConfObj *);
  void init_netmask(XMLConfObj *);
  void init_pca(XMLConfObj *);
  void init_metrics(XMLConfObj *);
  void init_noise_thresholds(XMLConfObj *);
  void init_endpointlist_maxsize(XMLConfObj *);
  void init_endpoints_to_monitor(XMLConfObj *);
  void init_stat_test_freq(XMLConfObj *);
  void init_report_only_first_attack(XMLConfObj *);
  void init_pause_update_when_attack(XMLConfObj *);

  // ... and those related to the statistical tests parameters
  void init_cusum_test(XMLConfObj *);

  void init_which_test(XMLConfObj *);
  void init_sample_sizes(XMLConfObj *);
  void init_two_sided(XMLConfObj *);
  void init_significance_level(XMLConfObj *);


  // the following functions are called by the test()-function:
  std::vector<int64_t> extract_data (const Info &, const Info &);
  std::vector<int64_t> extract_pca_data (Params &, const Info &, const Info &);
  void wkp_update(WkpParams &, const std::vector<int64_t> &);
  void wkp_test(WkpParams &);
  void cusum_update(CusumParams &, const std::vector<int64_t> &);
  void cusum_test(CusumParams &);

  // this function is called by extract_pca_data() to calculate a single entry
  // of the covariance matrix
  double covariance (const long long int &, const int &, const int &);

  // this function is called by wkp_test() and cusum_test() to extract a
  // single metric to test from a std::vector<int64_t>
  std::list<int64_t> getSingleMetric(const std::list<std::vector<int64_t> > &, const short &);
  std::string getMetricName(const enum Metric &);

  // the following functions are called by the wkp_test()-function
  double stat_test_wmw(std::list<int64_t> &, std::list<int64_t> &, bool &);
  double stat_test_ks (std::list<int64_t> &, std::list<int64_t> &, bool &);
  double stat_test_pcs(std::list<int64_t> &, std::list<int64_t> &, bool &);

  // contains endpoints and their number of occurance
  // (type is FilterEndPoint, because then we dont need two filter containers
  // in StatStore, one which could hold the x_most_frequently and another one,
  // which could hold the endpoints from the file ... )
  std::map<FilterEndPoint,int> endPointCount;

  // determines how many endpoints will be considered
  // meaning that only the X most frequently appeared endpoints
  // will be investigated
  uint16_t x_frequently_endpoints;

  // File where data will be stored to (in ONLINE MODE)
  // or be read from (in OFFLINE MODE)
  std::string offlineFile;

  // If the user specifies an output_dir, this flag will be set to true
  // and output files (for test-params and metrics) will be generated and
  // stored into the output_dir
  bool createFiles;
  std::string output_dir;

  // for metric and params output files
  std::vector<int> cusum_alarms;  // counters for detected attacks
  std::vector<int> wmw_alarms;    // for every metric
  std::vector<int> ks_alarms;     // and avery test
  std::vector<int> pcs_alarms;


  // here is the params container for the wkp-tests:
  std::map<EndPoint, WkpParams> WkpData;

  // another container for the cusum-test
  std::map<EndPoint, CusumParams> CusumData;

  // source id's to accept
  std::vector<int> accept_source_ids;

#ifdef IDMEF_SUPPORT_ENABLED
  // IDMEF-Message
  IdmefMessage idmefMessage;
#endif


  // user's preferences (defined in the XML config file):
  std::ofstream logfile;
  int logfile_output_verbosity;
  // holds the constants for the (splitted) metrics
  std::vector<Metric> metrics;
  int noise_threshold_packets;
  int noise_threshold_bytes;
  int endpointlist_maxsize;
  int stat_test_frequency;
  bool report_only_first_attack;
  short pause_update_when_attack;
  // fix PCA stuff
  bool use_pca;
  int learning_phase_for_pca;


  // test parameters (defined in the XML config file):

  // for cusum-test
  bool enable_cusum_test;
  double amplitude_percentage;
  int repetition_factor;
  int learning_phase_for_alpha;
  double smoothing_constant;

  // for wkp-tests
  bool enable_wmw_test;
  bool enable_ks_test;
  bool enable_pcs_test;
  bool enable_wkp_test; // summarizes the above three
  int sample_old_size;
  int sample_new_size;
  bool two_sided;
  double significance_level;


  // the following counter enables us to call a statistical test every
  // X=stat_test_frequency times that the test() method is called,
  // rather than everytime
  int test_counter;

  std::ofstream storefile;
};

#endif
