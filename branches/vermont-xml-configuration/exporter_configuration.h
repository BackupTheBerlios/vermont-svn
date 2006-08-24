#ifndef _EXPORTER_CONFIGURATION_H_
#define _EXPORTER_CONFIGURATION_H_


#include "vermont_configuration.h"


#include <concentrator/sndIpfix.h>


class ExporterSink;
class Template;


class ExporterConfiguration : public Configuration {
public:
	ExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~ExporterConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();

	void createExporterSink(Template* t, uint16_t sourceId);
	ExporterSink* getExporterSink() const { return exporterSink; }
	
	void createIpfixSender(uint16_t sourceId);
	IpfixSender* getIpfixSender() const { return ipfixSender; }
protected:
	void setUp();

private:
	void readPacketRestrictions(xmlNodePtr p);
	void readUdpTemplateManagement(xmlNodePtr p);
	void readCollector(xmlNodePtr i);

	uint16_t maxPacketSize;
	unsigned exportDelay;
	unsigned templateRefreshTime;
	unsigned templateRefreshRate;
	std::string ipAddress;
	std::string protocolType;
	uint16_t port;

	bool hasCollector;
	ExporterSink* exporterSink;
	IpfixSender* ipfixSender;
};

#endif
