#ifndef COLLECTORCFG_H_
#define COLLECTORCFG_H_

#include "Cfg.h"

#include "ipfixlolib/ipfixlolib.h"

#include <string>
#include <set>

/**
 * This class holds the <collector> ... </collector> and the <listener> ...
 * </listener> information of the config
 */
class CollectorCfg
	: public CfgBase
{
public:
	std::string getName() { return "collector"; }
	
	CollectorCfg(XMLElement* elem)
		: CfgBase(elem)
	{
		try {
			uint16_t defaultPort;
			ipAddress = getOptional("ipAddress");
			string prot = get("transportProtocol");
			if (prot=="17" || prot=="UDP") {
				protocolType = UDP;
				defaultPort = 4739;
			} else if (prot=="132" || prot=="SCTP") {
				protocolType = SCTP;
				defaultPort = 4739;
			} else if (prot=="DTLS_OVER_UDP") {
				protocolType = DTLS_OVER_UDP;
				defaultPort = 4740;
			} else if (prot=="DTLS_OVER_SCTP") {
				protocolType = DTLS_OVER_SCTP;
				defaultPort = 4740;
			} else 
				THROWEXCEPTION("Invalid configuration parameter for transportProtocol (%s)", prot.c_str());
			port = (uint16_t)getInt("port", defaultPort);
			mtu = (uint16_t)getInt("mtu",0);
			XMLNode::XMLNodeSet childs = _elem->findChildren("peerFqdn");
			for (XMLNode::XMLNodeSet::iterator it = childs.begin();
			     it != childs.end();
			     it++) {
				XMLNode* e = *it;
				string strdnsname(e->getFirstText());
				transform(strdnsname.begin(),strdnsname.end(),strdnsname.begin(),
						::tolower);
				peerFqdns.insert(strdnsname);
			}
			
		} catch(IllegalEntry ie) {
			THROWEXCEPTION("Illegal Collector entry in config file, transport protocol required");
		}
	}

	std::string getIpAddress() { return ipAddress; }
	//unsigned getIpAddressType() { return ipAddressType; }
	ipfix_transport_protocol getProtocolType() { return protocolType; }
	std::set<std::string> getPeerFqdns() { return peerFqdns; }
	uint16_t getPort() { return port; }
	uint16_t getMtu() { return mtu; }
	
	bool equalTo(CollectorCfg* other)
	{
		if ((ipAddress == other->ipAddress) &&
			//(ipAddressType == other->ipAddressType) &&
			(protocolType == other->protocolType) &&
			(port == other->port) &&
			(mtu == other->mtu) &&
			(peerFqdns == other->peerFqdns)) return true;
		
		return false;
	}
	
private:	
	std::string ipAddress;
	//unsigned ipAddressType;
	ipfix_transport_protocol protocolType;
	uint16_t port;
	uint16_t mtu;
	std::set<std::string> peerFqdns;
};

#endif /*COLLECTORCFG_H_*/
