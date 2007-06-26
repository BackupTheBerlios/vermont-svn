#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#include "AggregationPerfTest.h"

#include "sampler/Filter.h"
#include "common/Time.h"

#include <boost/test/minimal.hpp>
#include <sys/time.h>
#include <time.h>


AggregationPerfTest::AggregationPerfTest()
{
}

AggregationPerfTest::~AggregationPerfTest()
{
	delete filter;
	delete packetSink;
	delete ipfixAggregator;
	delete packetManager;
}

Rule::Field* AggregationPerfTest::createRuleField(const string& typeId)
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


Rules* AggregationPerfTest::createRules()
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

void AggregationPerfTest::setup()
{
	packetSink = new PacketSink();

	packetManager = new InstanceManager<Packet>();

	Rules* rules = createRules();
	int inactiveBufferTime = 5;   // maximum number of seconds until non-active flows are exported
	int activeBufferTime = 10;   // maximum number of seconds until active flows are exported
	// note: we do not need to specify any receiving module for the ipfixaggregator,
	// as deconstruction of unused instances is done with shared pointers
	ipfixAggregator = new IpfixAggregator(rules, inactiveBufferTime, activeBufferTime);

	hookingFilter = new HookingFilter(ipfixAggregator);

	filter = new Filter();
	filter->addProcessor(hookingFilter);
	filter->setReceiver(packetSink);

	// start all needed threads
	packetSink->runSink();
	ipfixAggregator->runSink();
	ipfixAggregator->start();
	filter->startFilter();
}

void AggregationPerfTest::shutdown()
{
	TimeoutSemaphore::shutdown();
	filter->terminate();
	packetSink->terminateSink();
	ipfixAggregator->terminateSink();
}


void AggregationPerfTest::start(unsigned int numpackets)
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
}


int test_main(int, char *[]) 
{
	unsigned int numpackets = 50000;
	msg_setlevel(MSG_DEFAULT+1);

	AggregationPerfTest perftest;
	perftest.setup();
	struct timeval starttime;
	BOOST_REQUIRE(gettimeofday(&starttime, 0) == 0);
	perftest.start(numpackets);
	struct timeval stoptime;
	BOOST_REQUIRE(gettimeofday(&stoptime, 0) == 0);
	struct timeval difftime;
	BOOST_REQUIRE(timeval_subtract(&difftime, &stoptime, &starttime) == 0);

	printf("needed time for processing %d packets: %d.%06d seconds\n", numpackets, (int)difftime.tv_sec, (int)difftime.tv_usec);

	perftest.shutdown();
	

	/*
	// six ways to detect and report the same error:
	BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error
	BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error
	if( add( 2,2 ) != 4 )
	BOOST_ERROR( "Ouch..." );            // #3 continues on error
	if( add( 2,2 ) != 4 )
	BOOST_FAIL( "Ouch..." );             // #4 throws on error
	if( add( 2,2 ) != 4 ) throw "Oops..."; // #5 throws on error

	return add( 2, 2 ) == 4 ? 0 : 1;       // #6 returns error code
	*/

	return 0;
}

#endif //HAVE_BOOST_UNIT_TEST_FRAMEWORK
#ifndef HAVE_BOOST_UNIT_TEST_FRAMEWORK
#include <iostream>
int main(int argc, char* argv[]) {
	std::cerr << "Not configured with HAVE_BOOST_UNIT_TEST_FRAMEWORK. No tests have been built." << std::endl;
}
#endif //HAVE_BOOST_UNIT_TEST_FRAMEWORK

