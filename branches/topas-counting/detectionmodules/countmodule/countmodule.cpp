/**************************************************************************/
/*    Copyright (C) 2007 Gerhard Muenz                                    */
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
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

#include "countmodule.h"
#include <concentrator/ipfix_names.h>

#include <iostream>

/* Constructor and destructor */
CountModule::CountModule(const std::string& configfile)
: DetectionBase<CountStore>(configfile), octetThreshold(0), packetThreshold(0), flowThreshold(0)
{
    /* signal handlers */
    if (signal(SIGTERM, sigTerm) == SIG_ERR) {
	msg(MSG_ERROR, "CountModule: Couldn't install signal handler for SIGTERM.\n ");
    } 
    if (signal(SIGINT, sigInt) == SIG_ERR) {
	msg(MSG_ERROR, "CountModule: Couldn't install signal handler for SIGINT.\n ");
    } 	

    init(configfile);
}

CountModule::~CountModule() {

}

/* Initialization and update*/
void CountModule::init(const std::string& configfile)
{
    /* Default values */
    unsigned alarm = 10;
    unsigned bfSize = 1024;
    unsigned bfHF = 3;
    char filename[] = "countmodule.txt";

    ConfObj * config;
    config = new ConfObj(configfile);

    if(NULL != config->getValue("preferences", "output_file"))
	outfile.open(config->getValue("preferences", "output_file"));
    else
	outfile.open(filename);
    if(!outfile) {
	msg(MSG_ERROR, "CountModule: Could not open output file.\n");
	exit(0);
    }

    if(NULL != config->getValue("preferences", "octet_threshold"))
	octetThreshold = atoi(config->getValue("preferences", "octet_threshold"));
    if(NULL != config->getValue("preferences", "packet_threshold"))
	packetThreshold = atoi(config->getValue("preferences", "packet_threshold"));
    if(NULL != config->getValue("preferences", "flow_threshold"))
	flowThreshold = atoi(config->getValue("preferences", "flow_threshold"));

    if(NULL != config->getValue("preferences", "alarm_time"))
	alarm = atoi(config->getValue("preferences", "alarm_time"));
    setAlarmTime(alarm);

    if(NULL != config->getValue("preferences", "accept_source_ids"))
    {
	std::string str = config->getValue("preferences", "accept_source_ids");
	if(str.size()>0)
	{
	    unsigned startpos = 0, endpos = 0;
	    do {
		endpos = str.find(',', endpos);
		if (endpos == std::string::npos) {
		    subscribeSourceId(atoi((str.substr(startpos)).c_str()));
		    break;
		}
		subscribeSourceId(atoi((str.substr(startpos, endpos-startpos)).c_str()));
		endpos++;
	    }
	    while(true);
	}
    }

    subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
    subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
    subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
    subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
    subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
    subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
    subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);

    if(NULL != config->getValue("counting", "bf_size"))
	bfSize = atoi(config->getValue("counting", "bf_size"));
    if(NULL != config->getValue("counting", "bf_hashfunctions"))
	bfHF = atoi(config->getValue("counting", "bf_hashfunctions"));
    CountStore::init(bfSize, bfHF);

    if((NULL != config->getValue("counting", "count_per_src_ip")) && ("false" != config->getValue("counting", "count_per_src_ip")))
	CountStore::countPerSrcIp = true;
    if(NULL != config->getValue("counting", "count_per_dst_ip"))
	CountStore::countPerDstIp = true;
    if(NULL != config->getValue("counting", "count_per_src_port"))
	CountStore::countPerSrcPort = true;
    if(NULL != config->getValue("counting", "count_per_dst_port"))
	CountStore::countPerDstPort = true;

#ifdef IDMEF_SUPPORT_ENABLED
    /* register module */
    registerModule("countmodule");
#endif

    delete(config);
}

#ifdef IDMEF_SUPPORT_ENABLED
void CountModule::update(XMLConfObj* xmlObj)
{
    std::cout << "Update received!" << std::endl;
    if (xmlObj->nodeExists("stop")) {
	std::cout << "-> stopping module..." << std::endl;
	stop();
    } else if (xmlObj->nodeExists("restart")) {
	std::cout << "-> restarting module..." << std::endl;
	restart();
void Bitmap::clear()
{
    memset(bitmap, 0, len_octets);
}
    } else if (xmlObj->nodeExists("config")) {
	std::cout << "-> updating module configuration..." << std::endl;
    } else { // add your commands here
	std::cout << "-> unknown operation" << std::endl;
    }
}
#endif

/* Test */
void CountModule::test(CountStore* store) 
{
#ifdef IDMEF_SUPPORT_ENABLED
    IdmefMessage& idmefMessage;
#endif

    std::cout << "Report" << std::endl;
    outfile << "******************** Report *********************" << std::endl;

    if(CountStore::countPerSrcIp)
    {
	outfile << "per source IP address:" << std::endl;
	for (CountStore::IpCountMap::const_iterator i = store->srcIpCounts.begin(); i != store->srcIpCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << i->first << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;

#ifdef IDMEF_SUPPORT_ENABLED
		idmefMessage; = getNewIdmefMessage("Countmodule", "Count classification");
		idmefMessage.createTargetNode("Don't know what decoy is!!!!!", "ipv4-addr",
			IpAddress(i->first.data).toString(), "0.0.0.0");
		std::string tmp;
		std::stringstream sstream;
		sstream << i->second;
		tmp = sstream.str();
		idmefMessage.createExtStatisticsNode("0", tmp, "0", "0");
		sendIdmefMessage("Dummy", idmefMessage);
#endif       
	    }   
	}
    }

    if(CountStore::countPerDstIp)
    {
	outfile << "per destination IP address:" << std::endl;
	for (CountStore::IpCountMap::const_iterator i = store->dstIpCounts.begin(); i != store->dstIpCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << i->first << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;
	    }
	}
    }

    if(CountStore::countPerSrcPort)
    {
	outfile << "per source protocol/port:" << std::endl;
	for (CountStore::PortCountMap::const_iterator i = store->srcPortCounts.begin(); i != store->srcPortCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << (i->first >> 16) << "." << (0x0000FFFF & i->first) << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;
	    }
	}
    }

    if(CountStore::countPerDstPort)
    {
	outfile << "per destination protocol/port:" << std::endl;
	for (CountStore::PortCountMap::const_iterator i = store->dstPortCounts.begin(); i != store->dstPortCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << (i->first >> 16) << "." << (0x0000FFFF & i->first) << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;
	    }
	}
    }

    outfile << "********************  End  *********************" << std::endl;

    delete store;
}

bool CountModule::checkThresholds(const Counters& count)
{
    return ((count.octetCount > octetThreshold) || (count.packetCount > packetThreshold) || (count.flowCount > flowThreshold));
}

void CountModule::sigTerm(int signum)
{
    stop();
}

void CountModule::sigInt(int signum)
{
    stop();
}
