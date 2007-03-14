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
: DetectionBase<CountStore>(configfile)
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
    threshold = 0;
    unsigned alarm = 10;
    unsigned bfSize = 1024;
    unsigned bfHF = 3;
    int id = 0;
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

    if(NULL != config->getValue("preferences", "threshold"))
	threshold = atoi(config->getValue("preferences", "threshold"));

    if(NULL != config->getValue("preferences", "alarm_time"))
	alarm = atoi(config->getValue("preferences", "alarm_time"));
    setAlarmTime(alarm);

    if(NULL != config->getValue("counting", "bf_size"))
	bfSize = atoi(config->getValue("counting", "bf_size"));
    if(NULL != config->getValue("counting", "bf_hashfunctions"))
	bfHF = atoi(config->getValue("counting", "bf_hashfunctions"));
    //FIXME:
    if(NULL != config->getValue("counting", "key_id"))
	id = ipfix_name_lookup(config->getValue("counting", "key_id"));
    if(id<0)
	id = 0;
    CountStore::init(bfSize, bfHF);
    CountStore::countPerSrcIp = true;

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
    IdmefMessage& idmefMessage = getNewIdmefMessage("Countmodule", "Count classification");
#endif
    std::cout << "Test" << std::endl;
    outfile << "******************** Test started *********************" << std::endl;
    for (CountStore::IpCountMap::const_iterator i = store->srcIpCounts.begin(); i != store->srcIpCounts.end(); ++i) {
	if (i->second.flows > threshold) {
/*
	    switch(CountStore::getCountKeyId()) {
		case IPFIX_TYPEID_sourceIPv4Address:
		    outfile << "SourceIp " << IpAddress(i->first.data) << " had " << i->second << " connection(s)." << std::endl;
		    break;
		case IPFIX_TYPEID_destinationIPv4Address:
		    outfile << "DestinationIp " << IpAddress(i->first.data) << " had " << i->second << " connection(s)." << std::endl;
		    break;
		case IPFIX_TYPEID_sourceTransportPort:
		    outfile << "SourcePort " << ntohs(*((uint16_t*)&(i->first.data))) << " had " << i->second << " connection(s)." << std::endl;
		    break;
		case IPFIX_TYPEID_destinationTransportPort:
		    outfile << "DestinationPort " << ntohs(*((uint16_t*)&(i->first.data))) << " had " << i->second << " connection(s)." << std::endl;
		    break;
		case IPFIX_TYPEID_protocolIdentifier:
		    outfile << "Protocol " << i->first.data[0] << " had " << i->second << " connection(s)." << std::endl;
		    break;
		default:
		    outfile << i->second << " connection(s)." << std::endl;
	    }
	    */

#ifdef IDMEF_SUPPORT_ENABLED
	    idmefMessage.createTargetNode("Don't know what decoy is!!!!!", "ipv4-addr",
		    IpAddress(i->first.data).toString(), "0.0.0.0");
	    std::string tmp;
	    std::stringstream sstream;
	    sstream << i->second;
	    tmp = sstream.str();
	    idmefMessage.createExtStatisticsNode("0", tmp, "0", "0");
	    sendIdmefMessage("Dummy", idmefMessage);
	    idmefMessage = getNewIdmefMessage();
#endif          
	}
    }
    outfile << "********************  Test ended  *********************" << std::endl;

    delete store;
}

void CountModule::sigTerm(int signum)
{
    stop();
}

void CountModule::sigInt(int signum)
{
    stop();
}
