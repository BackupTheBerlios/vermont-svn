#ifndef __class__SYNDOSDETECT__
#define __class__SYNDOSDETECT__


#include "BaseTCPDosDetect.h"
#include "ipfix.hpp"

class SynDosDetect :public BaseTCPDosDetect
{

	private:
		bool active_in;
		bool setup;
		uint16_t count;
		bool active_out;
		void observePacketAttack(pEntry p);
		void observePacketDefend(pEntry p);
		void evaluateClusters(double);
		void observePacket(DosHash*&,pEntry p,uint32_t);
		int compare_entry(pEntry e,pEntry p);
		
	public:
		int checkForAttack(const Packet* p);
	
		SynDosDetect();
		~SynDosDetect();
};


#endif