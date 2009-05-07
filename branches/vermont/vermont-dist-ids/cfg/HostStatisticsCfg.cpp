/*
 * HostStatisticsCfg.cpp
 *
 *  Created on: 06.05.2009
 *      Author: Matthias Segschneider (simasegs)
 */

#include "HostStatisticsCfg.h"

Module* HostStatisticsCfg::createInstance()
{
	if (!instance) {
		instance = new HostStatistics(ipSubnet, addrFilter, logPath, logInt);
	}
	return (Module*)instance;
}
