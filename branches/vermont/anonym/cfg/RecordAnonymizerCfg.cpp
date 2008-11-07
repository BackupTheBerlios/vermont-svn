#include "RecordAnonymizerCfg.h"
#include "InfoElementCfg.h"

RecordAnonymizerCfg* RecordAnonymizerCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new RecordAnonymizerCfg(e);
}

RecordAnonymizerCfg::RecordAnonymizerCfg(XMLElement* elem)
	: CfgHelper<RecordAnonymizer, RecordAnonymizerCfg>(elem, "anonRecord")
{
}

RecordAnonymizerCfg::~RecordAnonymizerCfg()
{

}

RecordAnonymizer* RecordAnonymizerCfg::createInstance()
{
	if (!instance) {
		instance = new RecordAnonymizer();
	}

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("anonField")) {
			InfoElementCfg* cfg = NULL;
			std::string method;
			std::string method_parameter;
			XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
			for (XMLNode::XMLSet<XMLElement*>::iterator jt = set.begin();
			     jt != set.end();
			     ++jt) {
				XMLElement* e = *jt;
				if (e->matches("anonIE")) {
					if(cfg) {
						THROWEXCEPTION("Only on anonIE tag per anonField tag allowed");
					}
					cfg = new InfoElementCfg(*jt);
				} else if (e->matches("anonMethod")) {
					method = get("anonMethod", e);
				} else if (e->matches("anonParam")) {
					method_parameter = get("anonParam", e);
				} else {
					msg(MSG_ERROR, "Unknown field in anonField");
					continue;
				}
			}
			if (!cfg) {
				msg(MSG_FATAL, "Missing information element in anonField");
				THROWEXCEPTION("Missing information element in anonField");
			}
			if (method.empty()) {
				msg(MSG_FATAL, "Missing anonymization method in anonField");
				THROWEXCEPTION("Missing anonymization method in anonField");
			}
			instance->addAnonymization(cfg->getIeId(), cfg->getIeLength(), AnonMethod::stringToMethod(method), method_parameter);
			delete cfg;
		} else if (e->matches("next")) {
			// ignore next
		} else {
			msg(MSG_FATAL, "Unkown anonymization field %s\n", e->getName().c_str());
			continue;
		}
	}

	return instance;
}

bool RecordAnonymizerCfg::deriveFrom(RecordAnonymizerCfg* old)
{
	return true;
}
