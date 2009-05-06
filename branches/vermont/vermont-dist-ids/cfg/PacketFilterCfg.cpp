#include "PacketFilterCfg.h"
#include "InfoElementCfg.h"
#include "RecordAnonymizerCfg.h"

#include <sampler/RegExFilter.h>
#include <sampler/StringFilter.h>
#include <sampler/SystematicSampler.h>
#include <sampler/StateConnectionFilter.h>
#include <sampler/ConnectionFilter.h>
#include <sampler/AnonFilter.h>
#include <sampler/PayloadFilter.h>
#include <sampler/HostFilter.h>
#include "common/msg.h"


#include <cassert>


PacketFilterCfg::PacketFilterCfg(XMLElement* elem)
	: CfgHelper<FilterModule, PacketFilterCfg>(elem, "filter")
{
	if (!elem)
		return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		Cfg* c;
		XMLElement* e = *it;

		if (e->matches("countBased")) {
			msg(MSG_INFO, "Filter: Creating count based sampler");
			c = new PacketCountFilterCfg(e);
		} else if (e->matches("hostBased")) {
			msg(MSG_INFO, "Filter: Creating host based sampler");
			c = new HostFilterCfg(e);
		} else if (e->matches("stringBased")) {
			msg(MSG_INFO, "Filter: Creating string based sampler");
			c = new PacketStringFilterCfg(e);
		} else if (e->matches("regexBased")) {
			msg(MSG_INFO, "Filter: Creating regex based sampler");
			c = new PacketRegexFilterCfg(e);
		} else if (e->matches("timeBased")) {
			msg(MSG_INFO, "Filter: Creating time based sampler");
			c = new PacketTimeFilterCfg(e);
		} else if (e->matches("stateConnectionBased")) {
			msg(MSG_INFO, "Filter: Creating state connection based sampler");
			c = new PacketStateConnectionFilterCfg(e);
#ifdef HAVE_CONNECTION_FILTER
		} else if (e->matches("connectionBased")) {
			msg(MSG_INFO, "Filter: Creating connection based sampler");
			c = new PacketConnectionFilterCfg(e);
#endif
		} else if (e->matches("anonFilter")) {
			msg(MSG_INFO, "Filter: Creating anonymization filter");
			c = new PacketAnonFilterCfg(e);
		} else if (e->matches("payloadFilter")) {
			msg(MSG_INFO, "Filter: Creating payload filter");
			c = new PacketPayloadFilterCfg(e);
		} else if (e->matches("next")) { // ignore next
			continue;
		} else {
			msg(MSG_FATAL, "Unkown packet filter %s\n", e->getName().c_str());
			THROWEXCEPTION("Unkown packet filter %s\n", e->getName().c_str());
			continue;
		}

		subCfgs.push_back(c);
	}
}

PacketFilterCfg::~PacketFilterCfg()
{
	for (std::vector<Cfg*>::iterator it = subCfgs.begin();
	     it != subCfgs.end();
	     it++) {
		delete *it;
	}
}

PacketFilterCfg* PacketFilterCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new PacketFilterCfg(e);
}

FilterModule* PacketFilterCfg::createInstance()
{
	instance = new FilterModule();
	for (std::vector<Cfg*>::iterator it = subCfgs.begin();
	     it != subCfgs.end();
	     it++) {
		instance->addProcessor(reinterpret_cast<PacketProcessor*>((*it)->getInstance()));
	}
	return instance;
}

bool PacketFilterCfg::deriveFrom(PacketFilterCfg* old)
{
	// check for same number of filters
	if (subCfgs.size() != old->subCfgs.size())
		return false;

	for (size_t i = 0; i < subCfgs.size(); i++) {
		if (!subCfgs[i]->deriveFrom(old->subCfgs[i]))
			return false;
	}

	return true;
}


PacketFilterHelperCfg::PacketFilterHelperCfg(XMLElement *e)
	: Cfg(e)
{

}

HostFilterCfg::HostFilterCfg(XMLElement *e)
	: PacketFilterHelperCfg(e), instance(NULL)
{
	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("addr")) {
			addrFilter = e->getFirstText();
		} else if (e->matches("ip")) {
			// cast e->getFirstText() to uint32_t to avoid string compare
			// no conversion from network byte order necessary for (ascii) strings
			std::string ip_str = e->getFirstText();
			uint32_t ip_addr = 0;
			size_t pos_1 = 0;
			size_t pos_2 = 0;
			for (int i = 0; i < 4; i++) {
				// shift ip_addr left 8 times (no effect in 1st round)
				// seperate ip_str at the dots, convert every number-part to integer
				// "save" the number in ip_addr
				pos_2 = ip_str.find(".", pos_1);
				ip_addr = (ip_addr << 8) | atoi((ip_str.substr(pos_1, pos_2)).c_str());
				pos_1 = pos_2;
			}
			ipList.insert(htonl(ip_addr));
		} else {
			msg(MSG_FATAL, "Unknown observer config statement %s\n", e->getName().c_str());
			continue;
		}
	}
}

HostFilterCfg::~HostFilterCfg()
{
}

Module* HostFilterCfg::getInstance()
{
	if (!instance) {
		instance = new HostFilter(addrFilter, ipList);
	}
		// WAS MACHEN?!? SystematicSampler passt mal garnicht.
		//instance = new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED,
		//				getAddrFilter(), getIpList());

	return (Module*)instance;
}

std::set<uint32_t> HostFilterCfg::getIpList()
{
	return ipList;
}


PacketCountFilterCfg::PacketCountFilterCfg(XMLElement *e)
	: PacketFilterHelperCfg(e), instance(NULL)
{
}

PacketCountFilterCfg::~PacketCountFilterCfg()
{

}

Module* PacketCountFilterCfg::getInstance()
{
	if (!instance)
		instance = new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED,
						getInterval(), getSpacing());

	return (Module*)instance;
}



PacketTimeFilterCfg::PacketTimeFilterCfg(XMLElement *e)
	: PacketFilterHelperCfg(e), instance(NULL)
{
}

PacketTimeFilterCfg::~PacketTimeFilterCfg()
{

}

Module* PacketTimeFilterCfg::getInstance()
{
	if (!instance)
		instance = new SystematicSampler(SYSTEMATIC_SAMPLER_TIME_BASED,
						getInterval(), getSpacing());

	return (Module*)instance;
}




/** helper function to return the real value of the string (HEX or normal) */
static std::string getRealValue(XMLElement* e)
{
	std::string str = e->getFirstText();
	XMLAttribute* a = e->getAttribute("type");

	if (a && a->getValue() == "HEX")
		return StringFilter::hexparser(str);

	return str;
}


Module* PacketStringFilterCfg::getInstance()
{
	if (!instance)
		instance = new StringFilter();

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("is")) {
			instance->addandFilter(getRealValue(e));
		} else if (e->matches("isnot")) {
			instance->addnotFilter(getRealValue(e));
		} else {
			msg(MSG_FATAL, "Unkown string packet filter config %s\n", e->getName().c_str());
			continue;
		}
	}

	return (Module*)instance;
}

bool PacketStringFilterCfg::deriveFrom(PacketStringFilterCfg* old)
{
	XMLNode::XMLSet<XMLElement*> newStatements = this->_elem->getElementChildren();
	XMLNode::XMLSet<XMLElement*> oldStatements = old->_elem->getElementChildren();

	if (newStatements.size() != oldStatements.size())
		return false;

	XMLNode::XMLSet<XMLElement*>::iterator itNew = newStatements.begin();
	XMLNode::XMLSet<XMLElement*>::iterator itOld = oldStatements.begin();
	for (; itNew != newStatements.end() && itOld != oldStatements.end();
	     itOld++ , itNew++) {
		XMLElement* oldE = *itOld;
		XMLElement* newE = *itNew;
		if (oldE->getFirstText() != newE->getFirstText())
			return false;
	}

	return true;
}

// ----------------------------------------------------------------------------

Module* PacketRegexFilterCfg::getInstance()
{
	if (!instance)
		instance = new RegExFilter();

	instance->match = get("matchPattern");
	instance->regcre();
	return (Module*)instance;
}

bool PacketRegexFilterCfg::deriveFrom(PacketRegexFilterCfg* old)
{
	if (get("matchPattern") == old->get("matchPattern"))
		return true;

	return false;
}


// ----------------------------------------------------------------------------

Module* PacketStateConnectionFilterCfg::getInstance()
{
	if (!instance) {
		instance = new StateConnectionFilter(
			getInt("timeout", 3),
			getInt("bytes", 100));
		instance->setExportControlPackets(getBool("exportControlPackets", true));
	}

	return (Module*)instance;
}

bool PacketStateConnectionFilterCfg::deriveFrom(PacketStateConnectionFilterCfg* old)
{
	if (get("timeout") == old->get("timeout") && get("bytes") == old->get("bytes"))
		return true;
	return false;
}

// ----------------------------------------------------------------------------

#ifdef HAVE_CONNECTION_FILTER
Module* PacketConnectionFilterCfg::getInstance()
{
	if (!instance) {
		unsigned seed = getInt("seed", 0);
		if (seed == 0) {
			instance = new ConnectionFilter(
				getInt("timeout", 3),
				getInt("bytes", 100),
				getInt("hashFunctions", 3),
				getInt("filterSize", 1000));
		} else {
			instance = new ConnectionFilter(
				getInt("timeout", 3),
				getInt("bytes", 100),
				getInt("hashFunctions", 3),
				getInt("filterSize", 1000),
				seed);
		}
		instance->setExportControlPackets(getBool("exportControlPackets", true));
	}

	return (Module*)instance;

}

bool PacketConnectionFilterCfg::deriveFrom(PacketConnectionFilterCfg* old)
{
	if (get("timeout") == old->get("timeout") &&
	    get("bytes") == old->get("bytes") &&
	    get("hashFunctions") == old->get("hashFunctions") &&
	    get("filterSize") == old->get("filterSize")) {
		return true;
	}
	return false;
}
#endif

// ----------------------------------------------------------------------------

Module* PacketAnonFilterCfg::getInstance()
{
	if (!instance) {
		instance = new AnonFilter();
	}

	RecordAnonymizerCfg::initInstance(this, instance, _elem->getElementChildren());

	return (Module*)instance;

}

bool PacketAnonFilterCfg::deriveFrom(PacketAnonFilterCfg* old)
{
	/*
	if (get("timeout") == old->get("timeout") &&
	    get("bytes") == old->get("bytes") &&
	    get("hashFunctions") == old->get("hashFunctions") &&
	    get("filterSize") == old->get("filterSize")) {
		return true;
	}
	*/
	return false;
}

// ----------------------------------------------------------------------------

Module* PacketPayloadFilterCfg::getInstance()
{
	if (!instance) {
		instance = new PayloadFilter();
	}
	return (Module*)instance;
}

bool PacketPayloadFilterCfg::deriveFrom(PacketPayloadFilterCfg* old)
{
	return true;
}

