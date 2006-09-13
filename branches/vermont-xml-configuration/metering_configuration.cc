#include "metering_configuration.h"
#include "exporter_configuration.h"

#include <ipfixlolib/ipfix_names.h>
#include <sampler/Template.h>
#include <sampler/ExporterSink.h>
#include <sampler/Filter.h>
#include <sampler/IPHeaderFilter.h>
#include <sampler/PacketProcessor.h>
#include <sampler/HookingFilter.h>
#include <sampler/PacketSink.h>
#include <concentrator/sampler_hook_entry.h>
#include <concentrator/ipfix.h>

#include <cctype>

/*************************** InfoElementId ***************************/

class MeteringConfiguration::InfoElementId {
public:
	InfoElementId(xmlNodePtr p, const MeteringConfiguration& m)
	{
		xmlNodePtr i = p->xmlChildrenNode;
		while (NULL != i) {
			if (!xmlStrcmp(i->name, (const xmlChar*)"ieName")) {
				ieName = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"match")) {
				match = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"modifier")) {
				modifier = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"ieId")) {
				ieId = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"ieLength")) {
				ieLength = m.getContent(i);
			} else if (!xmlStrcmp(i->name, (const xmlChar*)"enterpriseNumber")) {
				enterpriseNumber = m.getContent(i);
			}
			i = i->next;
		}
		std::transform(ieName.begin(), ieName.end(), ieName.begin(), std::tolower);
	}

	std::string getIeName() { return ieName; }
	std::string getIeId() { return ieId; }
	std::string getIeLength() { return ieLength; }
	std::string getMatch() { return match; }
	std::string getEnterpriseNumber() { return enterpriseNumber; }
	std::string getModifier() { return modifier; }

private:
	std::string ieName;
	std::string ieId;
	std::string ieLength;
	std::string match;
	std::string enterpriseNumber;
	std::string modifier;
};

/*************************** ReportedIE ***************************/

class MeteringConfiguration::ReportedIE {
public:
	ReportedIE(xmlNodePtr p, const MeteringConfiguration& m)
		: ieLength(-1), ieId(-1)
	{
		xmlNodePtr i = p->xmlChildrenNode;
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
	: Configuration(document, startPoint), t(0), templateId(0), filter(0),
	  sampling(false), aggregating(false), observationIdSet(false),
	  ipfixAggregator(0), gotSink(false)
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
	
	if (ipfixAggregator) {
		stopAggregator(ipfixAggregator);
		destroyAggregator(ipfixAggregator);
		deinitializeAggregators();
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
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"next")) {
			fillNextVector(i);
		}
		i = i->next;
	}
	setUp();
}

void MeteringConfiguration::readPacketSelection(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"countBased")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int interval = 0;
			int spacing = 0;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"interval")) {
					interval = atoi(getContent(j).c_str());
				} else if (!xmlStrcmp(j->name, (const xmlChar*)"spacing")) {
					spacing = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filters.push_back(new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED,
								interval, spacing));
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"timeBased")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int interval = 0;
			int spacing = 0;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"interval")) {
					interval = atoi(getContent(j).c_str());
				} else if (!xmlStrcmp(j->name, (const xmlChar*)"spacing")) {
					spacing = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filters.push_back(new SystematicSampler(SYSTEMATIC_SAMPLER_TIME_BASED,
								interval, spacing));
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
			sampling = true;
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
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"rawFilter")) {
			// TODO: remove the rawfilter ...
			std::string settings;
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"settings")) {
					settings = getContent(j);
				}
				j = j->next;
			}
			if (!settings.empty()) {
				filters.push_back(makeFilterProcessor("raw-filter", settings.c_str()));			
			}
		}
		i = i->next;
	}
}

void MeteringConfiguration::readPacketReporting(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
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
		throw std::runtime_error("MeteringConfiguration: Got PacketReporting without template ID");
	}
}

Rule* MeteringConfiguration::readRule(xmlNodePtr p) {
	// nonflowkey -> aggregate
	// flowkey -> keep

	xmlNodePtr i = p->xmlChildrenNode;

	Rule* rule = mallocRule();
	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"templateId")) {
			rule->id = atoi(getContent(i).c_str());
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"flowKey")) {
			try {
				InfoElementId ie(i, *this);
				RuleField* ruleField = mallocRuleField();
				if (ie.getModifier().empty()) {
					ruleField->modifier = FIELD_MODIFIER_KEEP;
				} else {
					ruleField->modifier = FIELD_MODIFIER_MASK_START;
					ruleField->modifier += atoi(ie.getModifier().c_str() + 5);
				}
				if (ie.getIeName() != "") {
					if (0 == (ruleField->type.id = string2typeid(ie.getIeName().c_str()))) {
						msg(MSG_ERROR, "Bad field type \"%s\"", ie.getIeName().c_str());
						throw std::exception();
					}
				} else {
					ruleField->type.id = atoi(ie.getIeId().c_str());
				}
				if (ie.getIeLength() != "") {
					ruleField->type.length = atoi(ie.getIeLength().c_str());
				} else {
					if (0 == (ruleField->type.length = string2typelength(ie.getIeName().c_str()))) {
						msg(MSG_ERROR, "Bad field type \"%s\", l.%s", ie.getIeName().c_str(), ie.getIeLength().c_str());
						throw std::exception();
					}
				}
				if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) || (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address)) {
					ruleField->type.length++; // for additional mask field
				}
				if (!ie.getMatch().empty()) {
					/* TODO: we need to
					   copy the string
					   because
					   parseProtoPattern
					   and
					   parseIPv4Pattern
					   violate the
					   original string 
					*/
					char* tmp = new char[ie.getMatch().length() + 1];
					strcpy(tmp, ie.getMatch().c_str());
					ruleField->pattern = NULL;
					
					switch (ruleField->type.id) {
					case IPFIX_TYPEID_protocolIdentifier:
						if (parseProtoPattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad protocol pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					case IPFIX_TYPEID_sourceIPv4Address:
					case IPFIX_TYPEID_destinationIPv4Address:
						if (parseIPv4Pattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad IPv4 pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					case IPFIX_TYPEID_sourceTransportPort:
					case IPFIX_TYPEID_destinationTransportPort:
						if (parsePortPattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad PortRanges pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					case IPFIX_TYPEID_tcpControlBits:
						if (parseTcpFlags(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad TCP flags pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					
					default:
						msg(MSG_ERROR, "Fields of type \"%s\" cannot be matched against a pattern %s", "", tmp);
						throw std::exception();
						break;
					}
				}
				rule->field[rule->fieldCount++] = ruleField;
			} catch (std::exception e) {}
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"nonFlowKey")) {
				InfoElementId ie(i, *this);
				RuleField* ruleField = mallocRuleField();
				ruleField->modifier = FIELD_MODIFIER_AGGREGATE;
				if (ie.getIeName() != "") {
					if (0 == (ruleField->type.id = string2typeid(ie.getIeName().c_str()))) {
						msg(MSG_ERROR, "Bad field type \"%s\"", ie.getIeName().c_str());
						throw std::exception();
					}
				} else {
					ruleField->type.id = atoi(ie.getIeId().c_str());
				}
				if (ie.getIeLength() != "") {
					ruleField->type.length = atoi(ie.getIeLength().c_str());
				} else {
					if (0 == (ruleField->type.length = string2typelength(ie.getIeName().c_str()))) {
						msg(MSG_ERROR, "Bad field type \"%s\", l.%s", ie.getIeName().c_str(), ie.getIeLength().c_str());
						throw std::exception();
					}
				}
				if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) || (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address)) {
					ruleField->type.length++; // for additional mask field
				}
				rule->field[rule->fieldCount++] = ruleField;
		}
		i = i->next;
	}
	rule->preceding = 0;
	msg(MSG_INFO, "Got aggregation rule: ");
	printRule(rule);
	return rule;
}

void MeteringConfiguration::readFlowMetering(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;

	aggregating = true;
	initializeAggregators();
// 	if (!observationIdSet) {
// 		throw std::runtime_error("MeteringConfiguration: Observation id for aggregator isn't set yet. But we need one right now!");
// 	}

	unsigned minBufferTime = 0;
	unsigned maxBufferTime = 0;

	Rules* rules = (Rules*)malloc(sizeof(Rules));
	rules->count = 0;


	while (NULL != i) {
		if (!xmlStrcmp(i->name, (const xmlChar*)"rule")) {
			Rule* r = readRule(i);
			if (r->fieldCount > 0) {
				rules->rule[rules->count++] = r;
			}
		} else if (!xmlStrcmp(i->name, (const xmlChar*)"expiration")) {
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				if (!xmlStrcmp(j->name, (const xmlChar*)"activeTimeout")) {
					// TODO: Fix me
					minBufferTime = getTimeInMsecs(j) / 1000;
				} else if (!xmlStrcmp(j->name, (const xmlChar*)"inactiveTimeout")) {
					// TODO: Fix me
					maxBufferTime = getTimeInMsecs(j) / 1000;
				}
				j = j->next;
			}
		}
		i = i->next;
	}

// 	if (!rules) {
// 		throw std::runtime_error("MeteringConfiguration: Could not read rules for ipfixAggregator");
// 	}

	ipfixAggregator = createAggregatorFromRules(rules,
						    minBufferTime,
						    maxBufferTime);
	if (!ipfixAggregator) {
		throw std::runtime_error("MeteringConfiguration: Could not create ipfixAggreagtor");
	}
}

void MeteringConfiguration::setUp()
{
	buildTemplate();
	buildFilter();
}

void MeteringConfiguration::buildFilter()
{
	filter = new Filter();
	
	for (unsigned i = 0; i != filters.size(); ++i) {
		filter->addProcessor(filters[i]);
	}
}


// TODO: remove this method
PacketProcessor* MeteringConfiguration::makeFilterProcessor(const char *name, const char *setting)
{
#define PROCESSOR_MAX_PARAM 1
#define PROCESSOR_INTERNAL 0
#define PROCESSOR_SYSTEMATIC 1
#define PROCESSOR_RANDOM 2
#define PROCESSOR_IPHEADER 3
	static char *PP_TAB[]={"Internal", "Systematic", "Random", "IPHeader", NULL };

	PacketProcessor *n;
	char *l, *token;
	int id;

	/*
	  the following is helluva dirty
	  keywords for grep: FUCK, DIRTY, MORON, SUCKS, SUCKZ

	  we don't accept filters with more than PROCESSOR_MAX_PARAM parameters
	*/
	int p_conf[PROCESSOR_MAX_PARAM];
	int p_conf_nr=0, i=0;

	/* really do not violate original string */
	if(!(l=strdup(setting))) {
		return NULL;
	}

	/* processor type is the first number */
	token=strsep(&l, ",");
	id=atoi(token);

	msg(MSG_DEBUG, "Filter: new PacketProcessor %s - type %d (%s), full params %s", name, id, PP_TAB[id], setting);
	while((token=strsep(&l, ",")) && p_conf_nr < PROCESSOR_MAX_PARAM) {
		p_conf[p_conf_nr]=atoi(token);
		/* include hardcode debuggin in case of */
		DPRINTF("PacketProcessor: token %s parsed as %d\n", token, p_conf[p_conf_nr]);
		p_conf_nr++;
	}

	/* just dump the settings if one is interested */
	while(i < p_conf_nr) {
		msg(MSG_INFO, "Processor %s param #%d: %d", name, i, p_conf[i]);
		i++;
	}

	/*
	  the following is helluva dirty, too
	  keywords for grep: FUCK, DIRTY, MORON, SUCKS, SUCKZ

	  PROCESSOR_* are defined in config_sampler.h
	  hardcoded parameters!
	*/
	switch(id) {
	case PROCESSOR_INTERNAL:
		n=NULL;
		break;
	case PROCESSOR_SYSTEMATIC:
		n=new SystematicSampler(p_conf[0], p_conf[1], p_conf[2]);
		break;
	case PROCESSOR_RANDOM:
		n=new RandomSampler(p_conf[0], p_conf[1]);
		break;
	case PROCESSOR_IPHEADER:
		n=new IPHeaderFilter(p_conf[0], p_conf[1], p_conf[2], p_conf[3], p_conf[4]);
		break;
	default:
		msg(MSG_FATAL, "Filter: cannot make PacketProcessor with ID %d, settings %s", id, setting);
		n=NULL;
	}
 out:
	free(l);

	return n;
#undef PROCESSOR_MAX_PARAM
#undef PROCESSOR_INTERNAL
#undef PROCESSOR_SYSTEMATIC
#undef PROCESSOR_RANDOM
#undef PROCESSOR_IPHEADER
}


void MeteringConfiguration::buildTemplate()
{
	if (!sampling)
		return;

	t = new Template(templateId);
	
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

void MeteringConfiguration::setObservationId(uint16_t id)
{
	observationId = id;
	observationIdSet = true;
}

IpfixAggregator* MeteringConfiguration::getAggregator() const
{
	if (!aggregating) {
		throw std::runtime_error("Metering process is not aggregating. Illegal call to getAggregator(). This is a bug! Please report it.");
	}
	return ipfixAggregator;
}

void MeteringConfiguration::connect(Configuration* c)
{	
	ExporterConfiguration* exporter = dynamic_cast<ExporterConfiguration*>(c);
	if (exporter) {
		if (!observationIdSet) {
			throw std::runtime_error("MeteringConfiguration: ObservationId not set. This is a bug!!! Please report it.");
		}
		if (sampling) {
			exporter->createExporterSink(t, observationId);
			filter->setReceiver(exporter->getExporterSink());
			gotSink = true;
		}
		if (aggregating) {
			exporter->createIpfixSender(observationId);
			addAggregatorCallbacks(ipfixAggregator, getIpfixSenderCallbackInfo(exporter->getIpfixSender()));
		}
		return;
	}

	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		// TODO: this is fuckin' dangerous! Check if
		// aggregator is thread-safe!!!!!!
		if (!metering->isAggregating()) {
			throw std::runtime_error("MeteringConfiguration: Cannot connect to another metering process, if that process isn't aggregating!");
		}
		if (!observationIdSet) {
			throw std::runtime_error("MeteringConfiguration: ObservationId not set. This is a bug!!! Please report it.");
		}
		
		metering->setObservationId(observationId);
		HookingFilter* h = new HookingFilter(sampler_hook_entry);
		h->setContext(metering->getAggregator());
		filter->addProcessor(h);
		return;
	}
	throw std::runtime_error("Cannot connect " + c->getId() + " to an metering process!");
}

void MeteringConfiguration::startSystem()
{
	if (!gotSink) {
		// we need at least one data sink. if we don't have
		// any, there will be a memory leak (no packets will
		// be freed within the sampler)
		filter->setReceiver(new PacketSink());
	}

	if (sampling || aggregating) {
		filter->startFilter();
	}
	if (aggregating) {
		startAggregator(ipfixAggregator);
	}
}

void MeteringConfiguration::pollAggregator()
{
	if (aggregating && ipfixAggregator) {
		//msg(MSG_DEBUG, "polling aggregator");
		::pollAggregator(ipfixAggregator);
	}
}
