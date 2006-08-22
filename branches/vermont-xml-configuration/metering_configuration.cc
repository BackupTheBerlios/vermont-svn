#include "metering_configuration.h"

#include <ipfixlolib/ipfix_names.h>

#include <cctype>

/*************************** InfoElementId ***************************/

class MeteringConfiguration::InfoElementId {
public:
	InfoElementId(xmlNodePtr i, const MeteringConfiguration& m)
	{
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

/*************************** ReportedIE ***************************/

class MeteringConfiguration::ReportedIE {
public:
	ReportedIE(xmlNodePtr i, const MeteringConfiguration& m)
		: ieLength(-1), ieId(-1)
	{
		i = i->xmlChildrenNode;
		while (NULL != i) {
			if (!xmlStrcmp(i->name, (const xmlChar*)"ieId")) {
				ieId = atoi(m.getContent(i).c_str());
				ieName = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"ieLength")) {
				ieLength = atoi(m.getContent(i).c_str());
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"ieName")) {
				ieName = m.getContent(i);
			}
			i = i->next;
		}
		std::transform(ieName.begin(), ieName.end(), ieName.begin(), std::tolower);
	}

	bool hasOptionalLength() const { return ieLength != -1; }
	std::string getName() const { return ieName; }
	unsigned getLength() const { return ieLength; }
	unsigned getId() const { return (ieId == -1)?ipfix_name_lookup(ieName.c_str()):ieId; }
private:
	std::string ieName;
	int ieLength;
	int ieId;
};

/*************************** MeteringConfiguration ***************************/

MeteringConfiguration::MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), t(0), templateId(0), filter(0)
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
	delete t;
	delete filter;
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
	setUp();
}

void MeteringConfiguration::readPacketSelection(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"countBased")) {
			/*
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"interval")) {
					interval = atoi(getContent(j).c_str());
				} else if (!xmlStrcmp(j->name, (const xmlChar*)"spacing")) {
					spacing = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			*/
			throw std::runtime_error("what does countbased mean?");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"timeBased")) {
			throw std::runtime_error("timeBased not yet implemented!");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"filterMatch")) {
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				// TODO: construct filter ...
				throw std::runtime_error("filterMatch not yet implemented!");
				j = j->next;
			}
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"randOutOfN")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int N, n;
			n = N = 0;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"population")) {
					N = atoi(getContent(j).c_str());
				} else if (!xmlStrcmp(j->name, (const xmlChar*)"sample")) {
					n = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filters.push_back(new RandomSampler(n, N));
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"uniProb")) {
			throw std::runtime_error("uniProb not yet implemented!");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"nonUniProb")) {
			throw std::runtime_error("nonUniProb not yet implemented");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"flowState")) {
			throw std::runtime_error("flowState not yet implemted");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"filterHash")) {
			throw std::runtime_error("filterHash not yet implemented");
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"filterRState")) {
			throw std::runtime_error("filterRState not yet implemented");
		}
		i = i->next;
	}
}

void MeteringConfiguration::readPacketReporting(xmlNodePtr i)
{
	i = i->xmlChildrenNode;
	bool gotTemplateId = false;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"reportedIE")) {
			exportedFields.push_back(new ReportedIE(i, *this));
		}
		if (!xmlStrcmp(i->name, (const xmlChar*)"templateId")) {
			templateId = atoi(getContent(i).c_str());
			gotTemplateId = true;
		}
		i = i->next;
	}

	if (!gotTemplateId) {
		throw std::runtime_error("Got PacketReporting without template ID");
	}
}

void MeteringConfiguration::readFlowMetering(xmlNodePtr i)
{
	throw std::runtime_error("not yet implemented");
}

void MeteringConfiguration::setUp()
{
	buildTemplate();
	buildFilter();
}

void MeteringConfiguration::buildFilter()
{
	filter = new Filter();

	// TODO: Where to put the paket hook?

	for (unsigned i = 0; i != filters.size(); ++i) {
		filter->addProcessor(filters[i]);
	}
}

void MeteringConfiguration::buildTemplate()
{
	Template* t = new Template(templateId);
	
	for (unsigned i = 0; i != exportedFields.size(); ++i) {
		int tmpId = exportedFields[i]->getId();
		if (!ipfix_id_rangecheck(tmpId)) {
			msg(MSG_ERROR, "Template: ignoring template field %s -> %d - rangecheck not ok", exportedFields[i]->getName().c_str(), tmpId);
			continue;
		}
		
		const ipfix_identifier *id;
		if( (tmpId == -1) || ((id=ipfix_id_lookup(tmpId)) == NULL) ) {
			msg(MSG_ERROR, "Template: ignoring unknown template field %s", exportedFields[i]->getName().c_str());
			continue;
		}
		
		int fieldLength = id->length;
		if (exportedFields[i]->hasOptionalLength()) {
			if (fieldLength == 0) {
				fieldLength = exportedFields[i]->getLength();
			} else {
				msg(MSG_ERROR, "Template: this is not a variable length field, ignoring optional length");
			}
		}
		msg(MSG_INFO, "Template: adding %s -> ID %d with size %d", exportedFields[i]->getName().c_str(), id->id, fieldLength);
		t->addField((uint16_t)id->id, fieldLength);
	}
	msg(MSG_DEBUG, "Template: got %d fields", t->getFieldCount());
}

void MeteringConfiguration::connect(Configuration*)
{

}
