
#include "regExFilter.h"


inline bool regExFilter::compare(char *pdata)
{

	int status;
	regex_t expr;
	char *tmp = (char*)malloc(match.size() + 2);
	tmp = (char*)match.c_str();

//	tmp = "[A-Z][A-Z][A-Z]";
/*	for (unsigned int i=0; i<strlen(tmp); i++) {
		printf("char[%d] %d, %c\n", i, tmp[i], tmp[i]);
	}*/


	if (regcomp(&expr, tmp, REG_EXTENDED) != 0) {
		return false;
	}
	status = regexec(&expr, pdata, (size_t) 0, NULL, 0);
	regfree(&expr);

	if( status != 0 ) {
		return false;
	}
	return true;

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
