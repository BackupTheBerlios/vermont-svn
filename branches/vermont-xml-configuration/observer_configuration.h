#ifndef _OBSERVER_CONFIGURATION_H_
#define _OBSERVER_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


class ObserverConfiguration {
public:
	ObserverConfiguration(xmlNodePtr startPoint);

private:
	xmlNodePtr start;
};

#endif
