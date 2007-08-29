/** @file
 * Filter a packet by checking if it is containing a predefined regEx string
 */

#ifndef REGEXFILTER_H
#define REGEXFILTER_H

#include "common/msg.h"
#include "PacketProcessor.h"
#include "idmef/IDMEFExporter.h"

#include <list>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <boost/regex.hpp>



class RegExFilter : public PacketProcessor
{
	public:
		int filtertype;
		std::string match;
		boost::regex rexp;

		RegExFilter(IDMEFExporter* idmefexp, string filterid);
		virtual ~RegExFilter();

		void regcre();
		virtual bool processPacket (const Packet * p);


	private:
		IDMEFExporter* idmefExporter;
		string filterId;

		bool compare (char *data);
};

#endif
