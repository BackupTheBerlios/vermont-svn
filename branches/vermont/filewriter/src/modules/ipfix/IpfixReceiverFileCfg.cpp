#include "IpfixReceiverFileCfg.h"

#include <modules/ipfix/IpfixReceiverFile.hpp>

IpfixReceiverFileCfg::IpfixReceiverFileCfg(XMLElement* elem)
	: CfgHelper<IpfixCollector,IpfixReceiverFileCfg>(elem, "ipfixReceiverFile"),
	ipfixCollector(NULL),
        observationDomainId(0)
{

	if (!elem)
		return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("packetFilePath")) {
			packetFilePath = e->getFirstText();
		} else {
			msg(MSG_FATAL, "Unkown ReceiverFile config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	observationDomainId = getInt("observationDomainId", 0);
	msg(MSG_INFO, "CollectorConfiguration: Successfully parsed collectingProcess section");
}

IpfixReceiverFileCfg::~IpfixReceiverFileCfg()
{
}

IpfixReceiverFileCfg* IpfixReceiverFileCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixReceiverFileCfg(elem);
}

IpfixCollector* IpfixReceiverFileCfg::createInstance()
{
	IpfixReceiverFile* ipfixReceiver;
	ipfixReceiver = new IpfixReceiverFile(packetFilePath);

	if (!ipfixReceiver) {
		THROWEXCEPTION("Could not create IpfixReceiver");
	}

	instance = new IpfixCollector(ipfixReceiver);
	return instance;
}

bool IpfixReceiverFileCfg::deriveFrom(IpfixReceiverFileCfg* old)
{
	/*
	 * The template handling in IpfixCollector/IpfixParser must be reworked before implementing
	 * this function, because the templates must all be invalidated on preReConfigure1().
	 * Invalid templates must be removed in preReconfigure2() and the new templates
	 * must be transmited on preConnect()
	 */
	return false;   // FIXME: implement it, to gain performance increase in reconnect
}
