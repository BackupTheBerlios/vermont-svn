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
#include <map>
#include <string>
#include <algorithm> // sort(...), unique(...)

// ========================== CLASS Stat ==========================

#define DEFAULT_warning_verbosity 0
#define DEFAULT_output_verbosity 3
#define DEFAULT_iplist_maxsize 421
#define DEFAULT_sample_old_size 111
#define DEFAULT_sample_new_size 11
#define DEFAULT_stat_test_frequency 1

// constants for (splitted) monitored values
enum Metric {
  PACKETS_IN,
  PACKETS_OUT,
  BYTES_IN,
  BYTES_OUT,
  RECORDS_IN,
  RECORDS_OUT,
  BYTES_IN_PER_PACKET_IN,
  BYTES_OUT_PER_PACKET_OUT,
  PACKETS_OUT_MINUS_PACKETS_IN,
  BYTES_OUT_MINUS_BYTES_IN,
  PACKETS_T_IN_MINUS_PACKETS_T_1_IN,
  PACKETS_T_OUT_MINUS_PACKETS_T_1_OUT,
  BYTES_T_IN_MINUS_BYTES_T_1_IN,
  BYTES_T_OUT_MINUS_BYTES_T_1_OUT
};

// main class: does tests, stores samples,
// reads and stores test parameters...

class Stat : public DetectionBase<StatStore> {

 public:

  Stat(const std::string & configfile);
  ~Stat() {}

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

  // setting all values of struct Values to 0
  void init_values(Values &);

  // this function is called by the Stat constructor, its job is to extract
  // user's preferences and test parameters from the XML config file:
  void init(const std::string & configfile);

  // as the init function is really huge, we divide it into tasks:
  // those related to the user's preferences regarding the module...
  void init_output_file(XMLConfObj *);
  void init_accept_source_ids(XMLConfObj *);
  void init_alarm_time(XMLConfObj *);
  void init_warning_verbosity(XMLConfObj *);
  void init_output_verbosity(XMLConfObj *);
  void init_endpoint_key(XMLConfObj *);
  void init_monitored_values(XMLConfObj *);
  void init_noise_thresholds(XMLConfObj *);
  void init_protocols(XMLConfObj *);
  void init_netmask(XMLConfObj *);
  void init_ports(XMLConfObj *);
  void init_ip_addresses(XMLConfObj *);
  void init_stat_test_freq(XMLConfObj *);
  void init_report_only_first_attack(XMLConfObj *);
  void init_pause_update_when_attack(XMLConfObj *);

  // ... and those related to the statistical tests parameters
  void init_which_test(XMLConfObj *);
  void init_sample_sizes(XMLConfObj *);
  void init_two_sided(XMLConfObj *);
  void init_significance_level(XMLConfObj *);


  // the following functions are called by the test()-function:
  Values extract_data (const Info &, const Info &);
  void update(std::list<Values> &, std::list<Values> &, const Values &);
  void stat_test(std::list<Values> &, std::list<Values> &);

  // the following functions are called by the extract_data()-function:
  void extract_data_packets (const Info &, Values &);
  void extract_data_octets (const Info &, Values &);
  void extract_data_records (const Info &, Values &);
  void extract_data_octets_per_packets (const Info &, Values &);
  void extract_data_packets_out_minus_packets_in (const Info &, Values &);
  void extract_data_octets_out_minus_octets_in (const Info &, Values &);
  void extract_packets_t_minus_packets_t_1 (const Info &, const Info &, Values &);
  void extract_octets_t_minus_octets_t_1 (const Info &, const Info &, Values &);

  // this function is called by stat_test() to extract a single metric to test
  // from the struct Values
  std::list<int64_t> getSingleMetric(const std::list<Values> &, const enum Metric &);
  std::string getMetricName(const enum Metric &);

  // and the following functions are called by the stat_test()-function:
  void stat_test_wmw(std::list<int64_t> &, std::list<int64_t> &);
  void stat_test_ks (std::list<int64_t> &, std::list<int64_t> &);
  void stat_test_pcs(std::list<int64_t> &, std::list<int64_t> &);


  // here is the sample container:
  std::map<EndPoint, Samples> Records;

  // source id's to accept
  std::vector<int> accept_source_ids;

#ifdef IDMEF_SUPPORT_ENABLED
  // IDMEF-Message
  IdmefMessage idmefMessage;
#endif


  // user's preferences (defined in the XML config file):
  std::ofstream outfile;
  int warning_verbosity;
  int output_verbosity;
  // holds the constants for the (splitted) monitored values
  std::vector<Metric> monitored_values;
  int noise_threshold_packets;
  int noise_threshold_bytes;
  std::string ipfile;
  int iplist_maxsize;
  int stat_test_frequency;
  bool report_only_first_attack;
  bool pause_update_when_attack;


  // test parameters (defined in the XML config file):
  bool enable_wmw_test;
  bool enable_ks_test;
  bool enable_pcs_test;
  int sample_old_size;
  int sample_new_size;
  bool two_sided;
  double significance_level;


  // the following counter enables us to call a statistical test every
  // X=stat_test_frequency times that the test() method is called,
  // rather than everytime
  int test_counter;

  bool port_monitoring;
  bool ports_relevant; // if only ICMP AND/OR RAW --> ports_relevant = false
  bool ip_monitoring;
  bool protocol_monitoring;

  // last-test-was-an-attack flags: useful to keep in mind results of
  // tests if report_only_first_attack==true
  bool last_wmw_test_was_an_attack;
  bool last_ks_test_was_an_attack;
  bool last_pcs_test_was_an_attack;

};

#endif
