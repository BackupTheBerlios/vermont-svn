#include "metering_configuration.h"

class MeteringConfiguration::InfoElementId {
public:
	InfoElementId(xmlNodePtr i, const MeteringConfiguration& m) {
		i = i->xmlChildrenNode;
		while (NULL != i) {
			if (!xmlStrcmp(i->name, (const xmlChar*)"ieName")) {
				ieName = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"match")) {
				match = m.getContent(i);
			}
			i = i->next;
		}
	}

private:
	std::string ieName;
	std::string match;
};

class MeteringConfiguration::ReportedIE {
public:
	ReportedIE(xmlNodePtr i, const MeteringConfiguration& m) {
		i = i->xmlChildrenNode;
		while (NULL != i) {
			if (!xmlStrcmp(i->name, (const xmlChar*)"ieId")) {
				ieId = atoi(m.getContent(i).c_str());
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"ieLength")) {
				ieLength = atoi(m.getContent(i).c_str());
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"ieName")) {
				ieName = m.getContent(i);
			}
			i = i->next;
		}
	}

private:
	std::string ieName;
	unsigned ieLength;
	unsigned ieId;
};




MeteringConfiguration::MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got metering process without unique id!");
	}
	id = configTypes::metering + (const char*)idString;
	xmlFree(idString);
}

MeteringConfiguration::~MeteringConfiguration()
{
	for (unsigned i = 0; i != filters.size(); ++i) {
		delete filters[i];
	}
	for (unsigned i = 0; i != exportedFields.size(); ++i) {
		delete exportedFields[i];
	}
}

void MeteringConfiguration::configure()
{
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"packetSelection")) {
			readPacketSelection(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"packetReporting")) {
			readPacketReporting(i);
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"flowMetering")) {
			readFlowMetering(i);
		}
		i = i->next;
	}
}

void MeteringConfiguration::readPacketSelection(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"countBased")) {
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"interval")) {
					interval = atoi(getContent(j).c_str());
				} else if (!xmlStrcmp(j->name, (const xmlChar*)"spacing")) {
					spacing = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"filterMatch")) {
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"infoElementId")) {
					filters.push_back(new InfoElementId(j, *this));
				} 
				j = j->next;
			}
		}
		i = i->next;
	}
}

void MeteringConfiguration::readPacketReporting(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"reportedIE")) {
			exportedFields.push_back(new ReportedIE(i, *this));
		}
		i = i->next;
	}
}

void MeteringConfiguration::readFlowMetering(xmlNodePtr i)
{
	throw std::runtime_error("not yet implemented");
}
