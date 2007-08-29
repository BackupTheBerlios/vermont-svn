/*
 * VERMONT 
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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
 */

#include "ConnectionTrackerTest.h"

#include "sampler/ExpressHookingFilter.h"
#include "sampler/Filter.h"
#include "common/Time.h"

#include <boost/test/auto_unit_test.hpp>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


ConnectionTrackerTestSink::ConnectionTrackerTestSink()
	: receivedConnection(false)
{
}

void ConnectionTrackerTestSink::push(Connection* conn)
{
	receivedConnection = true;
}

/**
 * @param fast determines if this test should be performed really fast or slower for performance measurements
 */
ConnectionTrackerTest::ConnectionTrackerTest()
{
	numPackets = 3000;

}

ConnectionTrackerTest::~ConnectionTrackerTest()
{
	delete filter;
	delete ipfixAggregator;
	delete packetManager;
	delete packetSink;
	delete connTracker;
	delete connSink;
}

void ConnectionTrackerTest::normalTest()
{
	printf("Testing: IpfixConnectionTracker ...\n");
	setup();
	start(numPackets);
	shutdown();
}

Rule::Field* ConnectionTrackerTest::createRuleField(const string& typeId)
{
	Rule::Field* ruleField = new Rule::Field();
	ruleField->modifier = Rule::Field::KEEP;
	ruleField->type.id = string2typeid(typeId.c_str());
	BOOST_REQUIRE(ruleField->type.id != 0);
	ruleField->type.length = string2typelength(typeId.c_str());
	BOOST_REQUIRE(ruleField->type.length != 0);
	if ((ruleField->type.id==IPFIX_TYPEID_sourceIPv4Address)
			|| (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address)) {
		ruleField->type.length++;
	}

	return ruleField;
}


Rules* ConnectionTrackerTest::createRules()
{
	Rule* rule = new Rule();
	rule->id = 1111;
	const char* rulefields[] = { "sourceipv4address", "destinationipv4address", "destinationtransportport",
								  "sourcetransportport", "packetdeltacount", "octetdeltacount",
								  "flowstartseconds", "flowendseconds", "protocolidentifier", 0 };

	for (int i=0; rulefields[i] != 0; i++) {
		rule->field[rule->fieldCount++] = createRuleField(rulefields[i]);
	}

	Rules* rules = new Rules();
	rules->rule[0] = rule;
	rules->count = 1;

	return rules;
}

void ConnectionTrackerTest::setup()
{
	connTracker = new IpfixConnectionTracker(1);
	packetSink = new PacketSink();
	connSink = new ConnectionTrackerTestSink();

	packetManager = new InstanceManager<Packet>();

	Rules* rules = createRules();
	int inactiveBufferTime = 0;   // maximum number of seconds until non-active flows are exported
	int activeBufferTime = 0;   // maximum number of seconds until active flows are exported
	// note: we do not need to specify any receiving module for the ipfixaggregator,
	// as deconstruction of unused instances is done with shared pointers
	ipfixAggregator = new IpfixAggregator(rules, inactiveBufferTime, activeBufferTime);
	ipfixAggregator->addFlowSink(connTracker);

	hookingFilter = new HookingFilter(ipfixAggregator);
	connTracker->addConnectionReceiver(connSink);

	filter = new Filter();
	filter->addProcessor(hookingFilter);
	filter->setReceiver(packetSink);

	// start all needed threads
	connTracker->runSink();
	connTracker->startThread();
	packetSink->runSink();
	ipfixAggregator->runSink();
	ipfixAggregator->start();
	filter->startFilter();
}

void ConnectionTrackerTest::shutdown()
{
	TimeoutSemaphore::shutdown();
	filter->terminate();
	packetSink->terminateSink();
	ipfixAggregator->terminateSink();
	connTracker->terminateSink();
	connTracker->stopThread();
	TimeoutSemaphore::restart();
}


void ConnectionTrackerTest::start(unsigned int numpackets)
{
	char packetdata[] = { 0x00, 0x12, 0x1E, 0x08, 0xE0, 0x1F, 0x00, 0x15, 0x2C, 0xDB, 0xE4, 0x00, 0x08, 0x00, 0x45, 0x00, 
						  0x00, 0x2C, 0xEF, 0x42, 0x40, 0x00, 0x3C, 0x06, 0xB3, 0x51, 0xC3, 0x25, 0x84, 0xBE, 0x5B, 0x20, 
						  0xF9, 0x33, 0x13, 0x8B, 0x07, 0x13, 0x63, 0xF2, 0xA0, 0x06, 0x2D, 0x07, 0x36, 0x2B, 0x50, 0x18, 
						  0x3B, 0x78, 0x67, 0xC9, 0x00, 0x00, 0x6F, 0x45, 0x7F, 0x40 };
	unsigned int packetdatalen = 58;

	// just push our sample packet a couple of times into the filter
	struct timeval curtime;
	BOOST_REQUIRE(gettimeofday(&curtime, 0) == 0);

	ConcurrentQueue<Packet*>* filterq = filter->getQueue();
	for (unsigned int i=0; i<numpackets; i++) {
		Packet* p = packetManager->getNewInstance();
		p->init((char*)packetdata, packetdatalen, curtime);
		filterq->push(p);
	}
	msg(MSG_INFO, "packets pushed");
	while (filterq->getCount()>0) {
		usleep(100000);
	}
	msg(MSG_INFO, "queue emptied");

	ipfixAggregator->poll();
	while (!connSink->receivedConnection) {
		usleep(100000);
	}
	msg(MSG_INFO, "connection received!");
}

