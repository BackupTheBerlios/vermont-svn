#ifndef __class__SYNDOSDETECT__
#define __class__SYNDOSDETECT__


#include "BaseTCPDosDetect.h"
#include "ipfix.hpp"

class SynDosDetect :public BaseTCPDosDetect
{

	private:
		bool active_in;
		bool active_out;
		void observePacketIn(const Packet* p);
		void observePacketOut(const Packet* p);
		void observePacket(DosHash*,const Packet* p,uint32_t);
		int compare_entry(pEntry e,const Packet* p);
		pEntry createNewpEntry(const Packet* p);	
		
	public:
		int checkForAttack(const Packet* p);
	
		SynDosDetect();
		~SynDosDetect();
};

#endif
