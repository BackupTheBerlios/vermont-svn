#include "main_configuration.h"

#include "msg.h"


MainConfiguration::MainConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), log_interval(0), poll_interval(0)
{
	id = configTypes::main;	
}

MainConfiguration::~MainConfiguration()
{
	
}

void MainConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (i) {
		if (tagMatches(i, "poll_interval")) {
			poll_interval = getTimeInMsecs(i);
		} else if (tagMatches(i, "log_file")) {
			logfile = getContent(i);
		} else if (tagMatches(i, "log_interval")) {
			log_interval = getTimeInMsecs(i);
		}
		i = i->next;
	}
}

void MainConfiguration::startSystem()
{
	if (logfile.empty()) {
		msg(MSG_DEBUG, "Main: logging subsystem is off");
		return;
	}

	FILE* FD;
	if(!(FD=fopen(logfile.c_str(), "a"))) {
	        throw std::runtime_error( "Main: could not init message subsystem, opening log " + logfile+ "failed");
	}

	/*
	  setting line buffering
	  per default a file-stream is full-buffered, so the output won't show up
	  too soon
	*/
        setvbuf(FD, (char *)NULL, _IOLBF, 0);

	msg(MSG_INFO, "Logging: using %s as statistics log", logfile.c_str());

	/*
	  set up the logger thread
	*/
	if(log_interval == 0) {
		msg(MSG_DEBUG, "Main: logging thread disabled");
	} else {
		/* set up logging thread */
		msg(MSG_DEBUG, "Main: logging all %d milliseconds", log_interval);
		msg_thread_set_timeout(log_interval);
	}
	
	if (-1 == (msg_stat_setup(MSG_SETUP_NEW, FD))) {
		throw std::runtime_error("Main: Could not set up msg logging thread");
	}

	msg_thread_start();
}

void MainConfiguration::connect(Configuration*)
{
	throw std::runtime_error("Cannot connect anything to MainConfiguration");
}
