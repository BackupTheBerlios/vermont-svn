#include "BaseTCPDosDetect.h"



BaseTCPDosDetect::BaseTCPDosDetect 
	{ 
		HashIncoming = NULL;
		HashOutgoing = NULL;
	}
	
BaseTCPDosDetect::~BaseTCPDosDetect
	{

	if (HashIncoming)
	delete HashIncoming;
	
	if (HashOutgoing)
	delete HashOutgoing;

	}