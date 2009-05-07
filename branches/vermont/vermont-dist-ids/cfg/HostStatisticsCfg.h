/*
 * HostStatisticsCfg.h
 *
 *  Created on: 03.05.2009
 *      Author: Matthias Segschneider (simasegs)
 */

#ifndef HOSTSTATISTICSCFG_H_
#define HOSTSTATISTICSCFG_H_

#include "Cfg.h"

#include <string>

class HostStatisticsCfg : public Cfg
{
public:
	friend class ConfigManager;

	std::string getName() { return "hostStatistics"; }

	Module* getInstance();

	HostStatisticsCfg(XMLElement* elem)
	: CfgBase(elem)
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

	std::string getSubnet() { return ipSubnet; }
	std::string getAddrFilter() { return addrFilter; }
	std::string getLogPath() { return logPath; }
	uint16_t getLogIntervall() { return logInt; }

private:
	std::string ipSubnet;
	std::string addrFilter;
	std::string logPath;
	uint16_t logInt;
};

#endif /* HOSTSTATISTICSCFG_H_ */
