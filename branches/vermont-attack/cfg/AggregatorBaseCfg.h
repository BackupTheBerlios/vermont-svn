#ifndef AGGREGATORBASECFG_H_
#define AGGREGATORBASECFG_H_

#include "Cfg.h"
#include "concentrator/Rule.hpp"
#include "concentrator/BaseTCPDosDetect.h"

// forward declarations
class Rule;
class Rules;

class AggregatorBaseCfg
	: private CfgBase
{
public:
	AggregatorBaseCfg(XMLElement* elem);
	virtual ~AggregatorBaseCfg();

protected:
	Rule* readRule(XMLElement* elem);
	
	static Rule::Field* readFlowKeyRule(XMLElement* e);
	static Rule::Field* readNonFlowKeyRule(XMLElement* e);
	BaseTCPDosDetect* readDos(XMLElement* e);
	unsigned maxBufferTime;	
	unsigned minBufferTime;
	unsigned pollInterval;
	uint8_t htableBits;
	BaseTCPDosDetect* baseTCP;
	Rules* rules;
};

#endif /*AGGREGATORBASECFG_H_*/
