/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "dbreader_configuration.h"
#include "exporter_configuration.h"
#include "metering_configuration.h"
#include "msg.h"


DbReaderConfiguration::DbReaderConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), ipfixDbReader(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got dbreader without unique id!");
	}
	id = configTypes::dbreader + (const char*)idString;
	xmlFree(idString);
}

DbReaderConfiguration::~DbReaderConfiguration()
{
	if (ipfixDbReader) {
		stopIpfixDbReader(ipfixDbReader);
		destroyIpfixDbReader(ipfixDbReader);
		deinitializeIpfixDbReaders();
	}
}

void DbReaderConfiguration::configure()
{
	msg(MSG_INFO, "DbReaderConfiguration: Start reading dbreader section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "next")) {
			fillNextVector(i);
		}
		i = i->next;
	}
	msg(MSG_INFO, "DbReaderConfiguration: Successfully parsed dbreader section");
	setUp();
}

void DbReaderConfiguration::setUp()
{
	initializeIpfixDbReaders();
	ipfixDbReader = createIpfixDbReader();
	if (!ipfixDbReader) {
		throw std::runtime_error("DbReaderConfiguration: Could not create IpfixDbReader!");
	}
}

void DbReaderConfiguration::connect(Configuration* c)
{
	ExporterConfiguration* exporter = dynamic_cast<ExporterConfiguration*>(c);
	if (exporter) {
		exporter->createIpfixSender(ipfixDbReader->srcId);
		IpfixSender* ipfixSender = exporter->getIpfixSender();
		msg(MSG_INFO, "DbReaderConfiguration: Adding ipfixSender-callbacks to dbReader");
		addIpfixDbReaderCallbacks(ipfixDbReader, getIpfixSenderCallbackInfo(ipfixSender));
		msg(MSG_INFO, "DbReaderConfiguration: Successfully set up connection between dbReader and Exporter");
		return;
	}

	throw std::runtime_error("Cannot connect " + c->getId() + " to dbReader!");
}

void DbReaderConfiguration::startSystem()
{
	msg(MSG_INFO, "DbReaderConfiguration: Starting dbReader...");
	startIpfixDbReader(ipfixDbReader);
	msg(MSG_INFO, "DbReaderConfiguration: Successfully started dbReader");
}
