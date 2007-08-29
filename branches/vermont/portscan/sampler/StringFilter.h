/** @file
 * Filter a packet by checking if it is containing a predefined string
 */

#ifndef StringFilter_H
#define StringFilter_H

#include <vector>
#include <string>
#include "common/msg.h"
#include "PacketProcessor.h"
#include "idmef/IDMEFExporter.h"



class StringFilter : public PacketProcessor
{

	public:
		static const char* PAR_FILTER_TYPE; // = "FILTER_TYPE";
		static const char* PAR_FILTER_ID; // = "FILTER_ID";

		std::vector<std::string> andFilters;
		std::vector<std::string> notFilters;
		IDMEFExporter* idmefExporter;

		StringFilter(IDMEFExporter* idmefexp, string filterid);
		virtual ~StringFilter();

		void addandFilter(std::string string);
		void addnotFilter(std::string string);
		std::string hexparser(const std::string input);
		virtual bool processPacket (const Packet * p);

	protected:
		string filterId;

		bool compare (unsigned char *data, std::string toMatch, unsigned int plength);
};

#endif
