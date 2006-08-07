#ifndef _EXPORTER_CONFIGURATION_H_
#define _EXPORTER_CONFIGURATION_H_


#include <libxml/parser.h>
#include <libxml/tree.h>


class ExporterConfiguration {
public:
	ExporterConfiguration(xmlNodePtr startPoint);

private:
	xmlNodePtr start;
};

#endif
