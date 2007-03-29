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
#include <stdlib.h>

bool CountModule::verbose = false;

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

    XMLConfObj config = XMLConfObj(configfile, XMLConfObj::XML_FILE);

    if(config.nodeExists("preferences"))
    {
	config.enterNode("preferences");

	if(config.nodeExists("output_file"))
	    outfile.open(config.getValue("output_file").c_str());
	else
	    outfile.open(filename);
	if(!outfile) 
	{
	    msg(MSG_ERROR, "CountModule: Could not open output file.\n");
	    stop();
	}

	if(config.nodeExists("alarm_time"))
	    alarm = atoi(config.getValue("alarm_time").c_str());
	setAlarmTime(alarm);

	if(config.nodeExists("accept_source_ids"))
	{
	    std::string str = config.getValue("accept_source_ids");
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

	if(config.nodeExists("octet_threshold"))
	    octetThreshold = atoi(config.getValue("octet_threshold").c_str());
	if(config.nodeExists("packet_threshold"))
	    packetThreshold = atoi(config.getValue("packet_threshold").c_str());
	if(config.nodeExists("flow_threshold"))
	    flowThreshold = atoi(config.getValue("flow_threshold").c_str());

	if(config.nodeExists("verbose"))
	    if(config.getValue("verbose") != "false")
	    {
		CountModule::verbose = true;
		msg_setlevel(MSG_INFO);
	    }

	config.leaveNode();
    }

    if(config.nodeExists("counting"))
    {
	config.enterNode("counting");

	if(config.nodeExists("bf_size"))
	    bfSize = atoi(config.getValue("bf_size").c_str());
	if(config.nodeExists("bf_hashfunctions"))
	    bfHF = atoi(config.getValue("bf_hashfunctions").c_str());
	CountStore::init(bfSize, bfHF);

	if(config.nodeExists("count_per_src_ip"))
	    if(config.getValue("count_per_src_ip") != "false")
		CountStore::countPerSrcIp = true;
	if(config.nodeExists("count_per_dst_ip"))
	    if(config.getValue("count_per_dst_ip") != "false")
		CountStore::countPerDstIp = true;
	if(config.nodeExists("count_per_src_port"))
	    if(config.getValue("count_per_src_port") != "false")
		CountStore::countPerSrcPort = true;
	if(config.nodeExists("count_per_dst_port"))
	    if(config.getValue("count_per_dst_port") != "false")
		CountStore::countPerDstPort = true;
    }

#ifdef IDMEF_SUPPORT_ENABLED
    /* register module */
    registerModule("countmodule");
#endif

    subscribeTypeId(IPFIX_TYPEID_sourceIPv4Address);
    subscribeTypeId(IPFIX_TYPEID_sourceTransportPort);
    subscribeTypeId(IPFIX_TYPEID_destinationIPv4Address);
    subscribeTypeId(IPFIX_TYPEID_destinationTransportPort);
    subscribeTypeId(IPFIX_TYPEID_protocolIdentifier);
    subscribeTypeId(IPFIX_TYPEID_octetDeltaCount);
    subscribeTypeId(IPFIX_TYPEID_packetDeltaCount);

}

#ifdef IDMEF_SUPPORT_ENABLED
void CountModule::update(XMLConfObj* xmlObj)
{
    if (xmlObj->nodeExists("stop")) {
	msg(MSG_INFO, "CountModule update: Stopping module.");
	stop();
    } else if (xmlObj->nodeExists("restart")) {
	msg(MSG_INFO, "CountModule update: Restarting module.");
	restart();
	/*
    } else if (xmlObj->nodeExists("config")) {
	msg(MSG_INFO, "changing module configuration.");
	*/
    } else { // add your commands here
	msg(MSG_INFO, "CountModule update: Unsupported operation.");
    }
}
#endif

/* Test */
void CountModule::test(CountStore* store) 
{
#ifdef IDMEF_SUPPORT_ENABLED
    IdmefMessage& idmefMessage;
#endif

    msg(MSG_INFO, "CountModule: Generating report...");
    outfile << "******************** Report *********************" << std::endl;
    outfile << "thresholds: octets>=" << octetThreshold << " packets>=" << packetThreshold << " flows>=" << flowThreshold << std::endl;

    if(CountStore::countPerSrcIp)
    {
	outfile << "per source IP address:" << std::endl;
	for (CountStore::IpCountMap::const_iterator i = store->srcIpCounts.begin(); i != store->srcIpCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << i->first << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;

#ifdef IDMEF_SUPPORT_ENABLED
		idmefMessage = getNewIdmefMessage("Countmodule", "threshold detection");
		idmefMessage.createSourceNode("no", "ipv4-addr", i->first.toString(), "255.255.255.255");
		// FIXME: where to put i->second data?
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

#ifdef IDMEF_SUPPORT_ENABLED
		idmefMessage = getNewIdmefMessage("Countmodule", "threshold detection");
		idmefMessage.createTargetNode("no", "ipv4-addr", i->first.toString(), "255.255.255.255");
		// FIXME: where to put i->second data?
		sendIdmefMessage("Dummy", idmefMessage);
#endif       
	    }
	}
    }

    if(CountStore::countPerSrcPort)
    {
	outfile << "per source protocol.port:" << std::endl;
	for (CountStore::PortCountMap::const_iterator i = store->srcPortCounts.begin(); i != store->srcPortCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << (i->first >> 16) << "." << (0x0000FFFF & i->first) << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;

#ifdef IDMEF_SUPPORT_ENABLED
		idmefMessage = getNewIdmefMessage("Countmodule", "threshold detection");
		idmefMessage.createSourceNode("unknown", "ipv4-addr", "0.0.0.0", "0.0.0.0");
		idmefMessage.createServiceNode("Source", "", itoa(0x0000FFFF & i->first), "", itoa(i->first >> 16)); 
		// FIXME: where to put i->second data?
		sendIdmefMessage("Dummy", idmefMessage);
#endif       
	    }
	}
    }

    if(CountStore::countPerDstPort)
    {
	outfile << "per destination protocol.port:" << std::endl;
	for (CountStore::PortCountMap::const_iterator i = store->dstPortCounts.begin(); i != store->dstPortCounts.end(); ++i) 
	{
	    if(checkThresholds(i->second))
	    {
		outfile << (i->first >> 16) << "." << (0x0000FFFF & i->first) << " \to:" << i->second.octetCount << " \tp:" << i->second.packetCount << " \tf:" << i->second.flowCount << std::endl;

#ifdef IDMEF_SUPPORT_ENABLED
		idmefMessage = getNewIdmefMessage("Countmodule", "threshold detection");
		idmefMessage.createTargetNode("unknown", "ipv4-addr", "0.0.0.0", "0.0.0.0");
		idmefMessage.createServiceNode("Target", "", itoa(0x0000FFFF & i->first), "", itoa(i->first >> 16)); 
		// FIXME: where to put i->second data?
		sendIdmefMessage("Dummy", idmefMessage);
#endif       
	    }
	}
    }

    outfile << "********************* End ***********************" << std::endl;

    delete store;
}

bool CountModule::checkThresholds(const Counters& count)
{
    return ((count.octetCount >= octetThreshold) || (count.packetCount >= packetThreshold) || (count.flowCount >= flowThreshold));
}

void CountModule::sigTerm(int signum)
{
    stop();
}

void CountModule::sigInt(int signum)
{
    stop();
}
