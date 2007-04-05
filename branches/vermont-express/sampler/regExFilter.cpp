
#include "regExFilter.h"


inline bool regExFilter::compare(char *pdata)
{

	char *tmp = (char*)malloc(match.size() + 2);
	tmp = (char*)match.c_str();
	boost::regex rexp(tmp); 
	if (boost::regex_search(pdata, rexp)) {
		return true;
	} 

	return false;

};

bool regExFilter::processPacket(const Packet *p)
{
        unsigned char* pdata;
	unsigned int plength;
	unsigned int payloadOffset;

	payloadOffset = p->payloadOffset;
	if( payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if(pdata == NULL) return false;

	return compare((char*)pdata);





	return false;

};
