/*
 * HostStatisticsCfg.cpp
 *
 *  Created on: 06.05.2009
 *      Author: Matthias Segschneider (simasegs)
 */

#include "HostStatisticsCfg.h"

HostStatisticsCfg::HostStatisticsCfg(XMLElement* elem) : CfgHelper<HostStatistics, HostStatisticsCfg>(elem, "hostStatistics")
{
	try {
		ipSubnet = get("subnet");
		addrFilter = get("addrFilter");
		logPath = get("logPath");
		logInt = (uint16_t)getInt("logIntervall", 10);
	} catch(IllegalEntry ie) {
		THROWEXCEPTION("Illegal hostStatistics entry in config file");
	}
}

bool HostStatisticsCfg::deriveFrom(HostStatisticsCfg* old)
{
	return false;
}

HostStatisticsCfg* HostStatisticsCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new HostStatisticsCfg(e);
}

HostStatistics* HostStatisticsCfg::createInstance()
{
	if (!instance) {
		instance = new HostStatistics(ipSubnet, addrFilter, logPath, logInt);
	}
	return instance;
}
