#include "RegExFilter.h"

#include "StringFilter.h"
#include "common/Misc.h"


RegExFilter::RegExFilter(IDMEFExporter* idmefexp)
	: idmefExporter(idmefexp)
{
}

RegExFilter::~RegExFilter()
{
}

void RegExFilter::regcre()
{
	rexp.assign(match);

}

bool RegExFilter::compare(char *pdata)
{

	if (boost::regex_search(pdata, rexp)) {
		return true;
	} 

	return false;
}

bool RegExFilter::processPacket(const Packet* p)
{
	const unsigned char* pdata;
	unsigned int plength;
	unsigned int payloadOffset;
	bool result;

	payloadOffset = p->payloadOffset;
	if (payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if (pdata == NULL) return false;

	result = compare((char*)pdata);

	if (result) {
		idmefExporter->setVariable(StringFilter::PAR_FILTER_TYPE, "Regexfilter");
		idmefExporter->setVariable(IDMEFExporter::PAR_SOURCE_ADDRESS, IPToString(*reinterpret_cast<uint32_t*>(p->netHeader+12)));
		idmefExporter->setVariable(IDMEFExporter::PAR_TARGET_ADDRESS, IPToString(*reinterpret_cast<uint32_t*>(p->netHeader+16)));
		idmefExporter->exportMessage();
	}

	return result;
}
