#include "BaseTCPDosDetect.h"



BaseTCPDosDetect::BaseTCPDosDetect()
	{ 
		HashAttack = NULL;
		HashDefend = NULL;
	}
	
BaseTCPDosDetect::~BaseTCPDosDetect()
	{

	if (HashAttack)
	delete HashAttack;
	
	if (HashDefend)
	delete HashDefend;

	}
