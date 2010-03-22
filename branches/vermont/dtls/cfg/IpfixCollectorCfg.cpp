#include "IpfixCollectorCfg.h"
#include <concentrator/IpfixReceiverUdpIpV4.hpp>
#include <concentrator/IpfixReceiverDtlsUdpIpV4.hpp>
#include <concentrator/IpfixReceiverSctpIpV4.hpp>
#include <concentrator/IpfixReceiverDtlsSctpIpV4.hpp>

IpfixCollectorCfg::IpfixCollectorCfg(XMLElement* elem)
	: CfgHelper<IpfixCollector, IpfixCollectorCfg>(elem, "ipfixCollector"),
	listener(NULL)
{

	if (!elem)
		return;
	
	msg(MSG_INFO, "CollectorConfiguration: Start reading packetReporting section");

	// Config for DTLS
	certificateChainFile = getOptional("cert");
	privateKeyFile = getOptional("key");
	caFile = getOptional("CAfile");
	caPath = getOptional("CApath");
	// observationDomainId = getInt("observationDomainId", 0);
	
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("listener")) {
			if (listener)
				THROWEXCEPTION("listener already set. There can only be one <listener> Element per Collector.");
			listener = new CollectorCfg(e);
			if (listener->getMtu() != 0) {
				delete listener;
				THROWEXCEPTION("You can not set the MTU for a listener.");
			}
		} else if (e->matches("udpTemplateLifetime")) {
			msg(MSG_DEBUG, "Don't know how to handle udpTemplateLifetime! Ignored.");
		} else if (e->matches("next")) { // ignore next
		} else if (e->matches("cert") || e->matches("key") ||
				e->matches("CAfile") || e->matches("CApath")) {
			// already done!
		} else {
			msg(MSG_FATAL, "Unkown observer config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	if (listener == NULL)
		THROWEXCEPTION("collectingProcess has to listen on one address!");
	if (listener->getProtocolType() != UDP &&
			listener->getProtocolType() != SCTP &&
			listener->getProtocolType() != DTLS_OVER_UDP &&
			listener->getProtocolType() != DTLS_OVER_SCTP)
		THROWEXCEPTION("collectingProcess can handle only UDP or SCTP!");
	
	msg(MSG_INFO, "CollectorConfiguration: Successfully parsed collectingProcess section");
}

IpfixCollectorCfg::~IpfixCollectorCfg()
{
	// FIXME: Shouldn't we delete listener here?
}

IpfixCollectorCfg* IpfixCollectorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixCollectorCfg(elem);
}

IpfixCollector* IpfixCollectorCfg::createInstance()
{
	IpfixReceiver* ipfixReceiver;
	if (listener->getProtocolType() == SCTP)
		ipfixReceiver = new IpfixReceiverSctpIpV4(listener->getPort(), listener->getIpAddress());	
	else if (listener->getProtocolType() == DTLS_OVER_UDP)
		ipfixReceiver = new IpfixReceiverDtlsUdpIpV4(listener->getPort(),
			listener->getIpAddress(), certificateChainFile,
			privateKeyFile, caFile, caPath, listener->getPeerFqdns());
	else if (listener->getProtocolType() == DTLS_OVER_SCTP)
		ipfixReceiver = new IpfixReceiverDtlsSctpIpV4(listener->getPort(),
			listener->getIpAddress(), certificateChainFile,
			privateKeyFile, caFile, caPath, listener->getPeerFqdns());
	else 
		ipfixReceiver = new IpfixReceiverUdpIpV4(listener->getPort(), listener->getIpAddress());	

	if (!ipfixReceiver) {
		THROWEXCEPTION("Could not create IpfixReceiver");
	}

	instance = new IpfixCollector(ipfixReceiver);
	return instance;
}

bool IpfixCollectorCfg::deriveFrom(IpfixCollectorCfg* old)
{
	/*
	 * The template handling in IpfixCollector/IpfixParser must be reworked before implementing
	 * this function, because the templates must all be invalidated on preReConfigure1().
	 * Invalid templates must be removed in preReconfigure2() and the new templates
	 * must be transmited on preConnect()
	 */
	return false;   // FIXME: implement it, to gain performance increase in reconnect
}
