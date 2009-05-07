/*
 * HostStatisticsCfg.h
 *
 *  Created on: 03.05.2009
 *      Author: Matthias Segschneider (simasegs)
 */

#ifndef HOSTSTATISTICSCFG_H_
#define HOSTSTATISTICSCFG_H_

#include "XMLElement.h"
#include "Cfg.h"
#include "concentrator/HostStatistics.h"

#include <string>


class HostStatisticsCfg : public CfgHelper<HostStatistics, HostStatisticsCfg>
{
public:
	friend class ConfigManager;

	std::string getName() { return "hostStatistics"; }

	HostStatistics* createInstance();
	virtual HostStatisticsCfg* create(XMLElement* e);
	virtual bool deriveFrom(HostStatisticsCfg* old);

protected:
	HostStatisticsCfg(XMLElement*);

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
