#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


class MeteringConfiguration {
public:
	MeteringConfiguration(xmlNodePtr startPoint);

private:
	xmlNodePtr start;
};

#endif
