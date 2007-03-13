#ifndef _SW_BASE_H_
#define _SW_BASE_H_

#include "sw-store.h"
#include <datastore.h>
#include <detectionbase.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <string>


// ========================== CLASS SWBase ==========================

class SWBase : public DetectionBase<SWStore> {


	public:

		SWBase(const std::string & configfile);
		~SWBase() {}

		void test(SWStore * store);

	private:

		// signal handlers
		static void sigTerm(int);
		static void sigInt(int);

		void init(const std::string & configfile);
		void init_alarm_time(ConfObj *);

		std::map<EndPoint,int64_t> extract_data (SWStore *);

};

#endif
