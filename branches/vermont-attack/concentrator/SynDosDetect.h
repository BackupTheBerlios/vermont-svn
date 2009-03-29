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
		void observePacketAttack(const Packet* p);
		void observePacketDefend(const Packet* p);
		void evaluateClusters();
		void observePacket(DosHash*&,const Packet* p,uint32_t);
		int compare_entry(pEntry e,const Packet* p);
		pEntry createNewpEntry(const Packet* p);	
		
	public:
		int checkForAttack(const Packet* p);
	
		SynDosDetect();
		~SynDosDetect();
};

#endif
