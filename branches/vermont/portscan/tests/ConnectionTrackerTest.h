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

#if !defined(ConnectionTrackerTest_H)
#define ConnectionTrackerTest_H

#include <boost/test/unit_test.hpp>

#include "sampler/Filter.h"
#include "sampler/HookingFilter.h"
#include "concentrator/IpfixAggregator.hpp"
#include "concentrator/IpfixConnectionTracker.h"
#include "common/InstanceManager.h"
#include "sampler/PacketSink.h"

using boost::unit_test::test_suite;
using boost::shared_ptr;
using boost::unit_test::test_case;

class ConnectionTrackerTestSink : public ConnectionReceiver
{
	public:
		bool receivedConnection;

		ConnectionTrackerTestSink();
		virtual void push(Connection* conn);

};

class ConnectionTrackerTest
{
	public:
		ConnectionTrackerTest();
		~ConnectionTrackerTest();

		void setup();
		void start(unsigned int numpackets);
		void shutdown();
		void normalTest();
		void expressTest();

	private:
		Filter* filter;
		PacketProcessor* hookingFilter;
		IpfixAggregator* ipfixAggregator;
		InstanceManager<Packet>* packetManager;
		PacketSink* packetSink;
		IpfixConnectionTracker* connTracker;
		ConnectionTrackerTestSink* connSink;

		Rule::Field* createRuleField(const string& typeId);
		Rules* createRules();

		int numPackets;
};

class ConnectionTrackerTestSuite : public test_suite
{
	public:
		ConnectionTrackerTestSuite()
			: test_suite("ConnectionTrackerTest")
		{
			shared_ptr<ConnectionTrackerTest> inst(new ConnectionTrackerTest());
			test_case* normaltest = BOOST_CLASS_TEST_CASE(&ConnectionTrackerTest::normalTest, inst);

			add(normaltest);
		}
};

#endif
