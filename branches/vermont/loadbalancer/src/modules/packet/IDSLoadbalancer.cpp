/*
 * Vermont IDS Load Balancer
 * Copyright (C) 2010 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "IDSLoadbalancer.hpp"

#include "modules/packet/Packet.h"
#include "modules/packet/PCAPExporterPipe.h"
#include "modules/packet/PCAPExporterMem.h"
#include "common/Time.h"
#include "idsloadbalancer/PriorityPacketSelector.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <list>

using namespace std;

IDSLoadbalancer::IDSLoadbalancer(BasePacketSelector* _selector, uint64_t updateinterval)
	: selector(_selector),
	  qcount(0),
	  thread(threadWrapper),
	  shutdownThread(false),
	  updateInterval(updateinterval)
{
}

IDSLoadbalancer::~IDSLoadbalancer()
{
}
/**
 * Startup method for the module
 */
void IDSLoadbalancer::performStart()
{
	//SignalHandler::getInstance().registerSignalHandler(SIGPIPE, this);

	msg(MSG_INFO, "Started IDSLoadbalancer with the following parameters:");
	msg(MSG_INFO, "  - PacketSelector = %s", selector->getName().c_str());
	qcount = getSucceedingModuleCount();
	selector->setQueueCount(qcount);
	msg(MSG_INFO, "  - QueueCount = %d", qcount);
	msg(MSG_INFO, "  - updateInterval = %.03fs", (float)updateInterval/1000);
	selector->setUpdateInterval(updateInterval);

	shutdownThread = false;
	if (updateInterval>0) thread.run(this);

	selector->start();
}

void IDSLoadbalancer::performShutdown()
{
	shutdownThread = true;
	thread.join();

	selector->stop();
}


void IDSLoadbalancer::receive(Packet* packet)
{
	int res = selector->decide(packet);
	if (res == -1){
		DPRINTFL(MSG_VDEBUG, "Dropping packet");
		packet->removeReference();
	} else {
		send(packet, res);
	}
}

void IDSLoadbalancer::forwardPacket(Packet* packet, int queue)
{
	//msg(MSG_INFO, "Forwarding packet to queue %d", queue);
}

void* IDSLoadbalancer::threadWrapper(void* data)
{
	IDSLoadbalancer* ilb = reinterpret_cast<IDSLoadbalancer*>(data);
	ilb->registerCurrentThread();
	ilb->updateWorker();
	ilb->unregisterCurrentThread();
	return NULL;
}

void IDSLoadbalancer::updateWorker()
{
	struct timeval nextint;
	addToCurTime(&nextint, updateInterval);
	while (!shutdownThread) {
		msg(MSG_INFO, "IDSLoadbalancer: worker started work");
		updateBalancingLists();
		msg(MSG_INFO, "IDSLoadbalancer: worker finished work");

		struct timeval difftime;
		struct timeval curtime;
		gettimeofday(&curtime, 0);
		if (timeval_subtract(&difftime, &nextint, &curtime)!=1) {
			// restart nanosleep with the remaining sleep time
			// if we got interrupted by a signal
			struct timespec ts;
			TIMEVAL_TO_TIMESPEC(&difftime, &ts);
			while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
		}
		addToCurTime(&nextint, updateInterval);
	}
}

void IDSLoadbalancer::updateBalancingLists()
{
	list<IDSLoadStatistics> stats;

	// get load data from succeeding modules
	for (uint32_t i=0; i<qcount; i++) {
		Destination<Packet*>* dp = getSucceedingModuleInstance(i);
		PCAPExporterProcessBase* psp = dynamic_cast<PCAPExporterProcessBase*>(dp);
		if (!psp) THROWEXCEPTION("IDSLoadBalancer: succeeding module #%u is not of type PCAPExporterProcessBase! Use module type PcapExporterPipe/Mem!", i);
		uint32_t ujiffies = 0;
		uint32_t sjiffies = 0;
		uint64_t dpkts = 0, fpkts = 0;
		uint64_t docts = 0, focts = 0;
		psp->getPacketStats(dpkts, fpkts);
		psp->getOctetStats(docts, focts);
		if (psp->getProcessStatistics(sjiffies, ujiffies)) {
			stats.push_back(IDSLoadStatistics(true, dpkts, docts, fpkts, focts, sjiffies, ujiffies));
		} else {
			stats.push_back(IDSLoadStatistics(true, dpkts, docts, fpkts, focts));
		}
		uint32_t maxsize, cursize;
		if (psp->getQueueStats(&maxsize, &cursize)) {
			stats.back().maxQueueSize = maxsize;
			stats.back().curQueueSize = cursize;
		}
	}
	selector->updateData(stats);
}
