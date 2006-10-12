/*
  released under GPL v2
  (C) Lothar Braun <mail@lobraun.de>
*/

#ifndef _IPFIX_CONFIGURATION_H_
#define _IPFIX_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


#include <string>
#include <stdexcept>
#include <vector>
#include <map>

#include <concentrator/ipfix.h>
#include <concentrator/aggregator.h>
#include <ipfixlolib/ipfix_names.h>


class MeteringConfiguration;
class ObserverConfiguration;
class CollectorConfiguration;
class ExporterConfiguration;

namespace configTypes
{
	const std::string observer  = "observer";
	const std::string exporter  = "exporter";
	const std::string collector = "collector";
	const std::string metering  = "metering";
	const std::string main      = "main";
};


/*************************** Configuration ***************************/

/**
 * base class for all subsystem configuration classes
 */
class Configuration {
public:
	Configuration(xmlDocPtr document, xmlNodePtr startNode) {
		start = startNode;
		doc = document;
	}
	virtual ~Configuration() {};

	virtual void configure() = 0;
	virtual void startSystem() = 0;

	/**
	 * connects c to this
	 */
	virtual void connect(Configuration* c) = 0;
	
	bool tagMatches(const xmlNodePtr node, const std::string& tagName) const;

	std::string getId() const{
		return id;
	}

	const std::vector<std::string>& getNextVector() { return nextVector; }

protected:
	class InfoElementId;
	class ReportedIE;


	xmlNodePtr start;
	xmlDocPtr doc;
	std::string id;

	std::vector<std::string> nextVector;
	std::string getContent(xmlNodePtr p) const; 
	void fillNextVector(xmlNodePtr p);

	unsigned getTimeInMsecs(xmlNodePtr i) const;
	unsigned getTimeInSecs(xmlNodePtr i) const;
	unsigned getTimeInUsecs(xmlNodePtr i) const;
};


/*************************** IpfixConfiguration ***************************/

/**
 * holding the configuration data for vermont and it's subsystems.
 */
class IpfixConfiguration {
public:
	IpfixConfiguration(const std::string& configFile);
	~IpfixConfiguration();
	
	void readSubsystemConfiguration();

	void connectSubsystems();
	void startSubsystems();

	void pollAggregatorLoop();
		
private:
	typedef std::map<std::string, Configuration*> SubsystemConfiguration;
	
	SubsystemConfiguration subsystems;
	std::vector<IpfixAggregator*> aggregators;

	xmlDocPtr document;
	xmlNodePtr current;
	
	// points to vermont specific configuration data
	xmlNodePtr vermontNode;

	bool stop;
	bool isAggregating;
};


/*************************** InfoElementId ***************************/

class Configuration::InfoElementId {
public:
	InfoElementId(xmlNodePtr p, const Configuration& m)
	{
		xmlNodePtr i = p->xmlChildrenNode;
		while (NULL != i) {
			if (m.tagMatches(i, "ieName")) {
				ieName = m.getContent(i);
			} else if (m.tagMatches(i, "match")) {
				match = m.getContent(i);
			} else if (m.tagMatches(i, "modifier")) {
				modifier = m.getContent(i);
			} else if (m.tagMatches(i, "ieId")) {
				ieId = m.getContent(i);
			} else if (m.tagMatches(i, "ieLength")) {
				ieLength = m.getContent(i);
			} else if (m.tagMatches(i, "enterpriseNumber")) {
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

class Configuration::ReportedIE {
public:
	ReportedIE(xmlNodePtr p, const Configuration& m)
		: ieLength(-1), ieId(-1)
	{
		xmlNodePtr i = p->xmlChildrenNode;
		while (NULL != i) {
			if (m.tagMatches(i, "ieId")) {
				ieId = atoi(m.getContent(i).c_str());
				ieName = m.getContent(i);
			} else if (m.tagMatches(i, "ieLength")) {
				ieLength = atoi(m.getContent(i).c_str());
			} else if (m.tagMatches(i, "ieName")) {
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


#endif
