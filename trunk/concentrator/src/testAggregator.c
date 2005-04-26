/** \file
 * Separate Program to test the Aggregator module
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "rcvIpfix.h"
#include "aggregator.h"
#include "sndIpfix.h"

int mayRun;

void sigint() {
	mayRun = 0;
	}

int main(int argc, char *argv[]) {
	mayRun = 1;
	signal(SIGINT, sigint);

	debug("initializing collectors");
	initializeRcvIpfix();

	debug("initializing aggregators");
	initializeAggregators();
	
	debug("initializing exporters");
	initializeSndIpfix();

	debug("starting exporter");
	IpfixSender* ipfixSender = sndIpfixUdpIpv4("127.0.0.1", 1501);
	startSndIpfix(ipfixSender);
		
	debug("starting aggregator");
	IpfixAggregator* ipfixAggregator = createAggregator("aggregation_rules.conf", 5, 15);
	setAggregatorDataTemplateCallback(ipfixAggregator, sndNewDataTemplate, ipfixSender);
	setAggregatorDataDataRecordCallback(ipfixAggregator, sndDataDataRecord, ipfixSender);
	setAggregatorDataTemplateDestructionCallback(ipfixAggregator, sndDestroyDataTemplate, ipfixSender);
			
	debug("starting collector");
	IpfixReceiver* ipfixReceiver = rcvIpfixUdpIpv4(1500);
	setDataRecordCallback(ipfixReceiver, aggregateDataRecord, ipfixAggregator);
	setDataDataRecordCallback(ipfixReceiver, aggregateDataDataRecord, ipfixAggregator);
	startRcvIpfix(ipfixReceiver);

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	while (mayRun) {
		stopRcvIpfix(ipfixReceiver);
		pollAggregator(ipfixAggregator);
		startRcvIpfix(ipfixReceiver);
		sleep(1);
		}
	debug("Stopping threads and tidying up.");

	debug("stopping collector");
	stopRcvIpfix(ipfixReceiver);
	rcvIpfixClose(ipfixReceiver);

	debug("stopping aggregator");
	stopAggregator(ipfixAggregator);
	destroyAggregator(ipfixAggregator);

	debug("stopping exporter");
	stopSndIpfix(ipfixSender);
	sndIpfixClose(ipfixSender);

	debug("deinitializing collectors");
	deinitializeRcvIpfix();

	debug("deinitializing aggregators");
	deinitializeAggregators();
	
	debug("deinitializing exporters");
	deinitializeSndIpfix();	
			
	return 0;
	}

