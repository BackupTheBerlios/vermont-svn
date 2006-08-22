#ifndef _EXPORTER_CONFIGURATION_H_
#define _EXPORTER_CONFIGURATION_H_


#include "vermont_configuration.h"


class ExporterConfiguration : public Configuration {
public:
	ExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint);

	void configure();

private:
	void readPacketRestrictions(xmlNodePtr p);
	void readUdpTemplateManagement(xmlNodePtr p);
	void readCollector(xmlNodePtr i);

	uint16_t maxPacketSize;
	unsigned exportDelay;
	unsigned templateRefreshTime;
	unsigned templateRefreshRate;
	std::string ipAddress;
	unsigned protocolType;
	uint16_t port;
};

#endif