#include "IpfixExporterCfg.h"

IpfixExporterCfg::IpfixExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixSender, IpfixExporterCfg>(elem, "ipfixExporter"),
	templateRefreshTime(IS_DEFAULT_TEMPLATE_TIMEINTERVAL), templateRefreshRate(0),	
	sctpDataLifetime(0), sctpReconnectInterval(0),
	maxPacketSize(0), exportDelay(0),
	recordRateLimit(0), observationDomainId(0)
{

	if (!elem) {
		return;
	}
	
	recordRateLimit = getInt("maxRecordRate", IS_DEFAULT_MAXRECORDRATE);
	observationDomainId = getInt("observationDomainId", 0);
	msg(MSG_INFO, "Exporter: using maximum rate of %d records/second", recordRateLimit);
	sctpDataLifetime = getTimeInUnit("sctpDataLifetime", mSEC, IS_DEFAULT_SCTP_DATALIFETIME);
	sctpReconnectInterval = getTimeInUnit("sctpReconnectInterval", SEC, IS_DEFAULT_SCTP_RECONNECTINTERVAL);
	templateRefreshRate = getInt("templateRefreshRate", IS_DEFAULT_TEMPLATE_RECORDINTERVAL);
	templateRefreshTime = getTimeInUnit("templateRefreshInterval", SEC, IS_DEFAULT_TEMPLATE_TIMEINTERVAL);
	// Config for DTLS
	certificateChainFile = getOptional("cert");
	privateKeyFile = getOptional("key");
	caFile = getOptional("CAfile");
	caPath = getOptional("CApath");
	

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("collector")) {
			CollectorCfg *c = new CollectorCfg(e);
			if (c->getPeerFqdns().size() > 1) {
				delete c;
				THROWEXCEPTION("You specified more than one peerFqdn for an exporter.");
			}
			collectors.push_back(c);
		} else if (	e->matches("maxRecordRate") ||
				e->matches("sctpDataLifetime") ||
				e->matches("sctpReconnectInterval") ||
				e->matches("templateRefreshRate") ||
				e->matches("templateRefreshInterval") ||
				e->matches("observationDomainId") ||
				e->matches("cert") ||
				e->matches("key") ||
				e->matches("CAfile") ||
				e->matches("CApath") ) {
			// already done!
		} else {
			THROWEXCEPTION("Illegal Exporter config entry \"%s\" found",
					e->getName().c_str());
		}
	}
}

IpfixExporterCfg::~IpfixExporterCfg()
{
	for(size_t i = 0; i < collectors.size(); i++)
		delete collectors[i];
}

IpfixSender* IpfixExporterCfg::createInstance()
{
	instance = new IpfixSender(observationDomainId, recordRateLimit, sctpDataLifetime, 
			sctpReconnectInterval, templateRefreshTime, templateRefreshRate,
			certificateChainFile, privateKeyFile, caFile, caPath);

	std::vector<CollectorCfg*>::const_iterator it;
	for (it = collectors.begin(); it != collectors.end(); it++) {
		CollectorCfg *p = *it;
#ifdef DEBUG
		const char *protocol;
		switch (p->getProtocolType()) {
			case SCTP:
				protocol = "SCTP"; break;
			case DTLS_OVER_UDP:
				protocol = "DTLS_OVER_UDP"; break;
			case UDP:
				protocol = "UDP"; break;
			default:
				protocol = "unknown protocol"; break;
		}
		msg(MSG_DEBUG, "IpfixExporter: adding collector %s://%s:%d",
				protocol,
				p->getIpAddress().c_str(),
				p->getPort());
#endif
		void *aux_config = NULL;
		ipfix_aux_config_dtls aux_config_dtls;
		if (p->getProtocolType() == DTLS_OVER_UDP) {
			aux_config_dtls.peer_fqdn = NULL;
			const std::set<std::string> peerFqdns = p->getPeerFqdns();
			std::set<std::string>::const_iterator it = peerFqdns.begin();
			if (it != peerFqdns.end())
				aux_config_dtls.peer_fqdn = it->c_str();
			aux_config = &aux_config_dtls;
		}
		instance->addCollector(
			p->getIpAddress().c_str(),
			p->getPort(), p->getProtocolType(),
			aux_config);
	}

	return instance;
}

IpfixExporterCfg* IpfixExporterCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixExporterCfg(elem);
}

bool IpfixExporterCfg::deriveFrom(IpfixExporterCfg* other)
{
	return equalTo(other);
}

bool IpfixExporterCfg::equalTo(IpfixExporterCfg* other)
{
	if (maxPacketSize != other->maxPacketSize) return false;
	if (exportDelay != other->exportDelay) return false;
	if (templateRefreshTime != other->templateRefreshTime) return false;
	if (templateRefreshRate != other->templateRefreshRate) return false;
	if (collectors.size() != other->collectors.size()) return false;
	std::vector<CollectorCfg*>::const_iterator iter = collectors.begin();
	while (iter != collectors.end()) {
		std::vector<CollectorCfg*>::const_iterator biter = other->collectors.begin();
		bool found = false;
		while (biter != collectors.end()) {
			if ((*iter)->equalTo(*biter)) {
				found = true;
				break;
			}
			biter++;
		}
		if (!found) return false;
		iter++;
	}
	
	return true;
}