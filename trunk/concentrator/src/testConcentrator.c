/** \file
 * Seperate file to test Concentrator module
 */

#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "concentrator.h"

int mayRun;

void sigint() {
	mayRun = 0;
	}

int main(int argc, char *argv[]) {
	mayRun = 1;
	signal(SIGINT, sigint);

	initializeConcentrator();
	startExporter("127.0.0.1", 1501);
	startMyAggregator("aggregation_rules.conf", 5, 15);
	startCollector(1500);

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	while (mayRun) {
		pollMyAggregator();
		sleep(1);
		}

	debug("Stopping threads and tidying up.");
	destroyConcentrator();
		
	return 0;
	}

