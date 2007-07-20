#ifndef _PARAMS_H_
#define _PARAMS_H_

#include <fstream>
#include <string>
#include <vector>
#include <list>
// for pca (matrices etc.)
#include <gsl/gsl_matrix.h>

// ======================== class Params ========================
// this is an abstract class which holds all the variables, the WKP and
// Cusum tests have in common. Both, the WKPParams and the CusumParams
// classes can be derived from this class.

class Params {

public:
  // store current number of learning phase
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

  // name of the corresponding EndPoint
  // (needed for creating file names for params output files)
  std::string correspondingEndPoint;

  bool updated;

  Params() {}

  ~Params() {}

  void init(int);

};

// ======================== class WkpParams ========================

// we use this class as value field in std::map< key, value >
// it contains a list of the old and the new metrics.
// Letting Old and New be lists of vector<int64_t> enables us
// to store values for multiple metrics
// It also holds last-test-was-an-attack flags: they are useful
// to keep for every endpoint in mind results of tests if
// report_only_first_attack==true
// this class is derived from the abstract class Params and inherits
// the params needed for pca

class WkpParams : public Params {

public:
  std::list<std::vector<int64_t> > Old;
  std::list<std::vector<int64_t> > New;

  // every test has its was-attack flag for every metric
  // (needed to count the alarms correctly)
  // (Formerly, if e. g. WMW raised an alarm for packets_in
  // and in the next test-run, it detects another one for
  // bytes_out, this wont be updated, because its last test-flag
  // was true ...)
  std::vector<bool> last_wmw_test_was_attack;
  std::vector<bool> last_ks_test_was_attack;
  std::vector<bool> last_pcs_test_was_attack;

  // count raised alarms for every metric, endpoint and test seperately
  std::vector<int> wmw_alarms;
  std::vector<int> ks_alarms;
  std::vector<int> pcs_alarms;

  WkpParams() {};

  ~WkpParams() {};

};

// ======================== class CusumParams ========================

// we use this class as value field in std::map< key, value > to
// store the cusum test parameters for each endpoint.
// As several metrics could be monitored, we need vectors instead of
// single values here.
// this class is derived from the abstract class Params and inherits
// the params needed for pca

class CusumParams : public Params {

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

  // count raised alarms for every metric and endpoint seperately
  std::vector<int> cusum_alarms;

  bool ready_to_test;

  CusumParams()
  {
    ready_to_test = false;
    learning_phase_nr_for_alpha = 0;
  }

  ~CusumParams() {};

};

#endif
