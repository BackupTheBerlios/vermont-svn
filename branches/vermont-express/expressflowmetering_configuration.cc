/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "expressflowmetering_configuration.h"
#include "msg.h"


ExpressFlowMeteringConfiguration::ExpressFlowMeteringConfiguration(xmlDocPtr doc, xmlNodePtr start)
	: Configuration(doc, start), ipfixExpressAggregator(0)
{

}

ExpressFlowMeteringConfiguration::~ExpressFlowMeteringConfiguration()
{
	if (ipfixExpressAggregator) {
		stopExpressAggregator(ipfixExpressAggregator);
		destroyExpressAggregator(ipfixExpressAggregator);
		deinitializeExpressAggregators();
	}
}

void ExpressFlowMeteringConfiguration::configure()
{
	msg(MSG_INFO, "ExpressFlowMeteringConfiguration: Start reading expressflowMetering section");
	xmlNodePtr i = start->xmlChildrenNode;

	initializeExpressAggregators();
// 	if (!observationIdSet) {
// 		throw std::runtime_error("MeteringConfiguration: Observation id for aggregator isn't set yet. But we need one right now!");
// 	}

	unsigned minBufferTime = 0;
	unsigned maxBufferTime = 0;

	ExpressRules* rules = (ExpressRules*)malloc(sizeof(ExpressRules));
	rules->count = 0;


	while (NULL != i) {
		if (tagMatches(i, "rule")) {
			ExpressRule* r = readExpressRule(i);
			if (r->fieldCount > 0) {
				rules->rule[rules->count++] = r;
			}
		} else if (tagMatches(i, "expiration")) {
			xmlNodePtr j = i->xmlChildrenNode;

			while (NULL != j) {
				if (tagMatches(j, "activeTimeout")) {
					minBufferTime = getTimeInSecs(j);
				} else if (tagMatches(j, "inactiveTimeout")) {
					maxBufferTime = getTimeInSecs(j);
				}
				j = j->next;
			}
		}
		i = i->next;
	}

	ipfixExpressAggregator = createExpressAggregatorFromRules(rules,
						    minBufferTime,
						    maxBufferTime);
	if (!ipfixExpressAggregator) {
		throw std::runtime_error("MeteringConfiguration: Could not create aggreagtor");
	}
	msg(MSG_INFO, "ExpressFlowMeteringConfiguration: Successfully parsed flowMetering section");
}

void ExpressFlowMeteringConfiguration::setUp()
{
	// nothing to perform before connect()
}

ExpressRule* ExpressFlowMeteringConfiguration::readExpressRule(xmlNodePtr p) {
	// nonflowkey -> aggregate
	// flowkey -> keep

	xmlNodePtr i = p->xmlChildrenNode;

	ExpressRule* rule = ExpressmallocRule();
	while (NULL != i) {
		if (tagMatches(i, "templateId")) {
			rule->id = atoi(getContent(i).c_str());
		} else if (tagMatches(i, "flowKey")) {
			try {
				InfoElementId ie(i, *this);
				ExpressRuleField* ruleField = ExpressmallocRuleField();
				if (ie.getModifier().empty()) {
					ruleField->modifier = FIELD_MODIFIER_KEEP;
				} else {
					ruleField->modifier = FIELD_MODIFIER_MASK_START;
					ruleField->modifier += atoi(ie.getModifier().c_str() + 5);
				}
				if (ie.getIeName() != "") {
					if (0 == (ruleField->type.id = Expressstring2typeid(ie.getIeName().c_str()))) {
						msg(MSG_ERROR, "Bad field type \"%s\"", ie.getIeName().c_str());
						throw std::exception();
					}
				} else {
					ruleField->type.id = atoi(ie.getIeId().c_str());
				}
				if (ie.getIeLength() != "") {
					ruleField->type.length = atoi(ie.getIeLength().c_str());
				} else {
					if (0 == (ruleField->type.length = Expressstring2typelength(ie.getIeName().c_str()))) {
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
						if (ExpressparseProtoPattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad protocol pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					case IPFIX_TYPEID_sourceIPv4Address:
					case IPFIX_TYPEID_destinationIPv4Address:
						if (ExpressparseIPv4Pattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad IPv4 pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					case IPFIX_TYPEID_sourceTransportPort:
					case IPFIX_TYPEID_destinationTransportPort:
						if (ExpressparsePortPattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
							msg(MSG_ERROR, "Bad PortRanges pattern \"%s\"", tmp);
							throw std::exception();
						}
						break;
					case IPFIX_TYPEID_tcpControlBits:
						if (ExpressparseTcpFlags(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
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
		} else if (tagMatches(i, "nonFlowKey")) {
				InfoElementId ie(i, *this);
				ExpressRuleField* ruleField = ExpressmallocRuleField();
				ruleField->modifier = FIELD_MODIFIER_AGGREGATE;
				if (ie.getIeName() != "") {
					if (0 == (ruleField->type.id = Expressstring2typeid(ie.getIeName().c_str()))) {
						msg(MSG_ERROR, "Bad field type \"%s\"", ie.getIeName().c_str());
						throw std::exception();
					}
				} else {
					ruleField->type.id = atoi(ie.getIeId().c_str());
				}
				if (ie.getIeLength() != "") {
					ruleField->type.length = atoi(ie.getIeLength().c_str());
				} else {
					if (0 == (ruleField->type.length = Expressstring2typelength(ie.getIeName().c_str()))) {
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
	ExpressprintRule(rule);
	return rule;
}


void ExpressFlowMeteringConfiguration::connect(Configuration*)
{
}

void ExpressFlowMeteringConfiguration::startSystem()
{
	msg(MSG_DEBUG, "Starting Express aggregator");
	startExpressAggregator(ipfixExpressAggregator);
}
