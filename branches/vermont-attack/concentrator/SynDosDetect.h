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
		void cleanUpCluster();
		bool active_out;
		int active_thres;
		void observePacketAttack(pEntry p);
		void observePacketDefend(pEntry p);
		void evaluateClusters();
		void observePacket(DosHash*,pEntry p,uint32_t);
		int compare_entry(pEntry e,pEntry p);
		void updateMaps(ipentry*,pEntry,uint32_t);
	public:
	static	void* threadWrapper(void* instance);
		int checkForAttack(const Packet* p,uint32_t*);
		bool busy;
		SynDosDetect(int,int,int,std::map<uint32_t,uint32_t>);
		~SynDosDetect();
};


#endif
