#ifndef __class__TCPDOSDETECT__
#define __class__TCPDOSDETECT__


#include "BaseTCPDosDetect.h"
#include "ipfix.hpp"

class TCPDosDetect :public BaseTCPDosDetect
{

	private:
		
		uint32_t History[30];
		bool active_in;
		bool setup;
		uint16_t count;
		bool bogus;
		int threshold_saved;
		int active_thres;
		void cleanUpCluster();
		bool active_out;
		void observePacketAttack(pEntry p);
		void observePacketDefend(pEntry p);
		void evaluateClusters();
		void observePacket(DosHash*,pEntry p,uint32_t);
		int compare_entry(pEntry e,pEntry p);
		void updateMaps(ipentry*,pEntry,uint32_t);
		int32_t firstTrigger;
		int windowCount;
		int32_t addedDeviations;
	public:
	static	void* threadWrapper(void* instance);
		int checkForAttack(const Packet* p,uint32_t*);
		bool busy;
		TCPDosDetect();
		~TCPDosDetect();
};


#endif
