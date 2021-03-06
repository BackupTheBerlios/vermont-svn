#ifndef SENSORMANAGERCFG_H_
#define SENSORMANAGERCFG_H_

#include "SensorManager.h"

#include "cfg/XMLElement.h"
#include "cfg/Cfg.h"

#include <string>

using namespace std;


class SensorManagerCfg
	: public CfgHelper<SensorManager, SensorManagerCfg>
{
public:
	friend class ConfigManager;
	
	virtual ~SensorManagerCfg();
	
	virtual SensorManagerCfg* create(XMLElement* e);
	virtual SensorManager* createInstance();
	virtual bool deriveFrom(SensorManagerCfg* old);
	
	void setGraphIS(GraphInstanceSupplier* gis);
	
protected:
	SensorManagerCfg(XMLElement*);
	
private:
	GraphInstanceSupplier* graphIS;
	static bool instanceCreated;
	
	// config variables
	uint32_t checkInterval; /** sensor check interval in seconds */
	string sensorOutput; /** filename of output file which contains sensor data */
};

#endif /*SENSORMANAGERCFG_H_*/
