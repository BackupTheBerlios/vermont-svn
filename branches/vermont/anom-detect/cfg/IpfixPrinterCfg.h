#ifndef IPFIXPRINTERCFG_H_
#define IPFIXPRINTERCFG_H_

#include "cfg/Cfg.h"
#include "concentrator/IpfixPrinter.hpp"

class IpfixPrinterCfg
	: public CfgHelper<IpfixPrinter, IpfixPrinterCfg>
{
public:
	friend class ConfigManager;
	
	virtual ~IpfixPrinterCfg();
	
	virtual IpfixPrinterCfg* create(XMLElement* e);
	
	virtual IpfixPrinter* createInstance();

	virtual bool deriveFrom(IpfixPrinterCfg* old);
	
protected:
	IpfixPrinterCfg(XMLElement*);

};

#endif /*IPFIXPRINTERCFG_H_*/
