/*
  released under GPL v2
  (C) Lothar Braun <mail@lobraun.de>
*/

#ifndef _DBWRITER_CONFIGURATION_H_
#define _DBWRITER_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <concentrator/IpfixDbWriter.h>



class DbWriterConfiguration : public Configuration {
public:
	DbWriterConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~DbWriterConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();

	IpfixDbWriter* getDbWriter() { return dbWriter; }

protected:
	void setUp();

private:
	IpfixDbWriter* dbWriter;
};

#endif
