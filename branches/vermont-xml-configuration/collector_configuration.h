#ifndef _COLLECTOR_CONFIGURATION_H_
#define _COLLECTOR_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


class CollectorConfiguration {
public:
	CollectorConfiguration(xmlNodePtr startPoint);
private:
	xmlNodePtr start;
};

#endif
