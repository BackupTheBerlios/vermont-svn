#include "ObserverCfg.h"
#include "common/msg.h"
#include "cfg/XMLElement.h"

#include "sampler/Observer.h"

#include <string>
#include <vector>
#include <cassert>


ObserverCfg* ObserverCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new ObserverCfg(e);
}

ObserverCfg::ObserverCfg(XMLElement* elem)
	: CfgHelper<Observer, ObserverCfg>(elem, "observer"),
	interface(),
	pcap_filter(),
	capture_len(0)
{
	if (!elem) return;  // needed because of table inside ConfigManager
	
	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("interface")) {
			interface = e->getFirstText();
		} else if (e->matches("pcap_filter")) {
			pcap_filter = e->getFirstText();
		} else if (e->matches("timeBased")) {
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unkown observer config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	capture_len = getInt("capture_len", 0);
}

ObserverCfg::~ObserverCfg()
{

}

Observer* ObserverCfg::createInstance()
{
	instance = new Observer(interface);

	if (capture_len) {
		if(!instance->setCaptureLen(capture_len)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified - using %d",
					instance->getCaptureLen());
		}
	}

	if (!pcap_filter.empty()) {
		if (!instance->prepare(pcap_filter.c_str())) {
			msg(MSG_FATAL, "Observer: preparing failed");
			THROWEXCEPTION("Observer setup failed!");
		}
	}

	return instance;
}

bool ObserverCfg::deriveFrom(ObserverCfg* old)
{
	if (interface != old->interface)
		return false;
	if (capture_len != old->capture_len)
		return false;
	if (pcap_filter != old->pcap_filter)
		return false;

	return true;
}
