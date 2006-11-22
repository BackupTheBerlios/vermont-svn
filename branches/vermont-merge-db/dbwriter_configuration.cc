/*
  released under GPL v2
  (C) Lothar Braun <mail@lobraun.de>
*/


#include "dbwriter_configuration.h"
#include "msg.h"


DbWriterConfiguration::DbWriterConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), dbWriter(NULL)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got dbwriter without unique id!");
	}
	id = configTypes::dbwriter + (const char*)idString;
	xmlFree(idString);
}

DbWriterConfiguration::~DbWriterConfiguration()
{
	if (dbWriter) {
		stopIpfixDbWriter(dbWriter);
		destroyIpfixDbWriter(dbWriter);
		deinitializeIpfixDbWriters();
	}
}

void DbWriterConfiguration::configure()
{
	msg(MSG_INFO, "DbWriterConfiguration: Start reading dbwriter section");

	msg(MSG_INFO, "DbWriterConfiguration: Successfully parsed dbwriter section");
	setUp();
}

void DbWriterConfiguration::setUp()
{
	initializeIpfixDbWriters();
	dbWriter = createIpfixDbWriter();
	if (!dbWriter) {
		throw std::runtime_error("DbWriterConfiguration: Could not create IpfixDbWriter");
	}
}

void DbWriterConfiguration::connect(Configuration*)
{
	throw std::runtime_error("DbWriter is an end target and cannot be connected to something!");
}

void DbWriterConfiguration::startSystem()
{
	msg(MSG_INFO, "DbWriterConfiguration: Starting dbWriter");
	startIpfixDbWriter(dbWriter);
	msg(MSG_INFO, "DbWriterConfiguration: Successfully started dbWriter");
}
