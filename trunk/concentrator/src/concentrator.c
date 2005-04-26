/** \file
 * Concentrator meta-module.
 * Simple interface to the Collector, Aggregator and Exporter modules.
 * Use either this module or the individual modules - do not use both.
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "rcvIpfix.h"
#include "aggregator.h"
#include "sndIpfix.h"

IpfixSender* ipfixSender;
IpfixAggregator* ipfixAggregator;
IpfixReceiver* ipfixReceiver;

/**
 * Initializes Memory. Call this on application startup.
 */
void initializeConcentrator() {
	ipfixSender = 0;
	ipfixAggregator = 0;
	ipfixReceiver = 0;

	debug("initializing collector");
	initializeRcvIpfix();

	debug("initializing aggregator");
	initializeAggregators();

	debug("initializing exporter");
	initializeSndIpfix();
	}

/**
 * Starts the Exporter.
 */
void startExporter(char* ip, uint16_t port) {
	if (ipfixSender) {
		fatal("Exporter already started");
		return;
		}

	debug("starting exporter");
	ipfixSender = sndIpfixUdpIpv4(ip, port);
	startSndIpfix(ipfixSender);
	}

/**
 * Starts the Aggregator. Exporter must be running.
 */
void startMyAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime) {
	if (!ipfixSender) {
		fatal("Exporter not started");
		return;
		}
	if (ipfixAggregator) {
		fatal("Concentrator already started");
		return;
		}	

	debug("starting Aggregator");
	ipfixAggregator = createAggregator(ruleFile, minBufferTime, maxBufferTime);
	setAggregatorDataTemplateCallback(ipfixAggregator, sndNewDataTemplate, ipfixSender);
	setAggregatorDataDataRecordCallback(ipfixAggregator, sndDataDataRecord, ipfixSender);
	setAggregatorDataTemplateDestructionCallback(ipfixAggregator, sndDestroyDataTemplate, ipfixSender);
	startAggregator(ipfixAggregator);
	}

/**
 * Starts the Collector. Aggregator must be running.
 */
void startCollector(uint16_t port) {
	if (!ipfixSender) {
		fatal("Exporter not started");
		return;
		}
	if (!ipfixAggregator) {
		fatal("Aggregator not started");
		return;
		}
	if (ipfixReceiver) {
		fatal("Collector already started");
		return;
		}
			
	debug("starting collector");
	ipfixReceiver = rcvIpfixUdpIpv4(port);
	setDataRecordCallback(ipfixReceiver, aggregateDataRecord, ipfixAggregator);
	setDataDataRecordCallback(ipfixReceiver, aggregateDataDataRecord, ipfixAggregator);
	startRcvIpfix(ipfixReceiver);
	}

/**
 * Exports aggregated flows; needs to be periodically called.
 */
void pollMyAggregator() {
	stopRcvIpfix(ipfixReceiver);
	pollAggregator(ipfixAggregator);
	startRcvIpfix(ipfixReceiver);
	}

/**
 * Frees all memory. Call this when the application terminates
 */
void destroyConcentrator() {
	if (ipfixReceiver) {
		debug("stopping collector");
		stopRcvIpfix(ipfixReceiver);
		rcvIpfixClose(ipfixReceiver);
		ipfixReceiver = 0;
		}
	
	if (ipfixAggregator) {
		debug("stopping aggregator");
		stopAggregator(ipfixAggregator);
		destroyAggregator(ipfixAggregator);
		ipfixAggregator = 0;
		}

	if (ipfixSender) {
		debug("stopping exporter");
		stopSndIpfix(ipfixSender);
		sndIpfixClose(ipfixSender);
		ipfixSender = 0;
		}

	deinitializeRcvIpfix();
	deinitializeSndIpfix();	
	deinitializeAggregators();
	}
