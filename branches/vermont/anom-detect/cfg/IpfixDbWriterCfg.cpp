#ifdef DB_SUPPORT_ENABLED

#include "IpfixDbWriterCfg.h"


IpfixDbWriterCfg* IpfixDbWriterCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbWriterCfg(e);
}


IpfixDbWriterCfg::IpfixDbWriterCfg(XMLElement* elem)
    : CfgHelper<IpfixDbWriter, IpfixDbWriterCfg>(elem, "ipfixDbWriter"),
      port(0),
      bufferRecords(30)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("host")) {
			hostname = e->getFirstText();
		} else if (e->matches("port")) {
			port = getInt("port");
		} else if (e->matches("dbname")) {
			dbname = e->getFirstText();
		} else if (e->matches("username")) {
			user = e->getFirstText();
		} else if (e->matches("password")) {
			password = e->getFirstText();
		} else if (e->matches("bufferrecords")) {
			bufferRecords = getInt("bufferrecords");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbWriterCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbWriterCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbWriterCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbWriterCfg: username not set in configuration!");
	if (password=="") THROWEXCEPTION("IpfixDbWriterCfg: password not set in configuration!");
}


IpfixDbWriterCfg::~IpfixDbWriterCfg()
{
}


IpfixDbWriter* IpfixDbWriterCfg::createInstance()
{
    instance = new IpfixDbWriter(hostname.c_str(), dbname.c_str(), user.c_str(), password.c_str(), port, 0, bufferRecords);
    return instance;
}


bool IpfixDbWriterCfg::deriveFrom(IpfixDbWriterCfg* old)
{
    return false;
}

#endif /*DB_SUPPORT_ENABLED*/
