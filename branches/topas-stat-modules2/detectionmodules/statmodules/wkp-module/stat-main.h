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
#include <detectionbase.h>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> // sort(...), unique(...)
// for pca (matrices etc.)
#include <gsl/gsl_matrix.h>

// ========================== CLASS Stat ==========================

#define DEFAULT_alarm_time 10
#define DEFAULT_warning_verbosity 0
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

// ======================== STRUCT Samples ========================

// we use this structure as value field in std::map< key, value >
// it contains a list of the old and the new metrics.
// Letting Old and New be lists of vector<int64_t> enables us
// to store values for multiple metrics
// It also holds last-test-was-an-attack flags: they are useful
// to keep for every endpoint in mind results of tests if
// report_only_first_attack==true

struct Samples {

public:
  std::list<std::vector<int64_t> > Old;
  std::list<std::vector<int64_t> > New;

  // TODO: pause update for every metric individually (pause_update = 3)
  // see Cusum-test ...
  // every metric has its was-attack-flag for every test
  bool last_wmw_test_was_attack;
  bool last_ks_test_was_attack;
  bool last_pcs_test_was_attack;

  // pca
  int learning_phase_nr_for_pca;
  // flag to identify if still in learning phase
  bool pca_ready;
  // the following two are needed for the learning phase
  // and with their help, covariances of the metrics can be calculated.
  // holds the sum of the product of each two metrics
  // (should be interpreted as a matrix, with each vector containing one row)
  std::vector<std::vector<long long int> > sumsOfProducts;
  // holds the sum of each metric
  std::vector<int> sumsOfMetrics;
  // this is the covariance matrix calculated after the learning phase
  gsl_matrix *cov;
  // matrix containing the eigenvectors of cov
  gsl_matrix *evec;

  // BEGIN TESTING
  // count raised alarms for every metric, endpoint and test seperately
  std::vector<int> wmw_alarms;
  std::vector<int> ks_alarms;
  std::vector<int> pcs_alarms;
  // END TESTING

  Samples()
  {
    last_wmw_test_was_attack = false;
    last_ks_test_was_attack = false;
    last_pcs_test_was_attack = false;
    learning_phase_nr_for_pca = 0;
    pca_ready = false;
  }

};

// ======================== STRUCT CusumParams ========================

// we use this structure as value field in std::map< key, value > to
// store the cusum test parameters for each endpoint.
// As several metrics could be monitored, we need vectors instead of
// single values here.

struct CusumParams {

public:
  // the current sum of the first x values for each metric
  // needed to calculate the initial alphas
  std::vector<int64_t> sum;
  // to identify the end of the learning phase
  int learning_phase_nr_for_alpha;
  // the means of every metric
  std::vector<double> alpha;
  // current values of the test statistic of each metric
  std::vector<double> g;
  // current observed value for each metric
  // needed for the cusum test
  std::vector<int> X_curr;
  // last observed value for each metric
  // needed to update alpha
  std::vector<int> X_last;

  // every metric has its was-attack-flag
  std::vector<bool> last_cusum_test_was_attack;

  // BEGIN TESTING
  // count raised alarms for every metric and endpoint seperately
  std::vector<int> cusum_alarms;
  // END TESTING

  bool ready_to_test;

  // pca
  int learning_phase_nr_for_pca;
  // flag to identify if still in learning phase
  bool pca_ready;
  // the following two are needed for the learning phase
  // and with their help, covariances of the metrics can be calculated.
  // holds the sum of the product of each two metrics
  // (should be interpreted as a matrix, with each vector containing one row)
  std::vector<std::vector<long long int> > sumsOfProducts;
  // holds the sum of each metric
  std::vector<int> sumsOfMetrics;
  // this is the covariance matrix calculated after the learning phase
  gsl_matrix *cov;
  // matrix containing the eigenvectors of cov
  gsl_matrix *evec;

  CusumParams()
  {
    ready_to_test = false;
    learning_phase_nr_for_alpha = 0;
    learning_phase_nr_for_pca = 0;
    pca_ready = false;
  }
};


// main class: does tests, stores samples,
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
  void init_pca(XMLConfObj *);
  void init_metrics(XMLConfObj *);
  void init_noise_thresholds(XMLConfObj *);
  void init_endpointlist_maxsize(XMLConfObj *);
  void init_endpointfilter(XMLConfObj *);
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
  void update(Samples &, const std::vector<int64_t> &);
  void stat_test(Samples &);
  void update_c(CusumParams &, const std::vector<int64_t> &);
  void cusum_test(CusumParams &);

  // this function is called by stat_test() and cusum_test() to extract a
  // single metric to test from a std::vector<int64_t>
  std::list<int64_t> getSingleMetric(const std::list<std::vector<int64_t> > &, const short &);
  std::string getMetricName(const enum Metric &);

  // the following functions are called by the stat_test()-function
  double stat_test_wmw(std::list<int64_t> &, std::list<int64_t> &, bool &);
  double stat_test_ks (std::list<int64_t> &, std::list<int64_t> &, bool &);
  double stat_test_pcs(std::list<int64_t> &, std::list<int64_t> &, bool &);

  // contains endpoints and their number of occurance
  std::map<EndPoint,int> endPointCount;

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


  // BEGIN TESTING
  std::vector<int> cusum_alarms;  // counters for detected attacks
  std::vector<int> wmw_alarms;    // for every metric
  std::vector<int> ks_alarms;     // and avery test
  std::vector<int> pcs_alarms;
  // END TESTING


  // here is the sample container for the wkp-tests:
  std::map<EndPoint, Samples> SampleData;

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
  int warning_verbosity;
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
  // config parameters
  bool use_pca; // is pca activated?
  int learning_phase_for_pca; // length of the learning phase
  // functions
  // calculate the covariance of two metrics
  // (this will yield one entry for the cov-matrix)
  double covariance (const long long int &, const int &, const int &);
  // extract pca metrics
  std::vector<int64_t> extract_pca_data (Samples &, const Info &, const Info &);
  std::vector<int64_t> extract_pca_data (CusumParams &, const Info &, const Info &);


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
