#ifndef   	PACKETSELECTION_CONFIGURATION_H_
#define   	PACKETSELECTION_CONFIGURATION_H_

#include "ipfix_configuration.h"

#include <vector>


class Filter;
class PacketProcessor;
class MeteringConfiguration;

class PacketSelectionConfiguration : public Configuration {
public:
	PacketSelectionConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	PacketSelectionConfiguration();
	~PacketSelectionConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();

	Filter* getFilters() { return filter; }

protected:
	std::vector<PacketProcessor*> filters;

	PacketProcessor* makeFilterProcessor(const char *name, const char *setting);

	Filter* filter;

	friend class MeteringConfiguration;
};

#endif 	    /* !PACKETSELECTION_CONFIGURATION_H_ */
