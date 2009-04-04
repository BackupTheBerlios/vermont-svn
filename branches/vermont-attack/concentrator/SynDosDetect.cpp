#include "SynDosDetect.h"


void SynDosDetect::evaluateClusters()
{

	ipentry* max = NULL;
	uint32_t maxdifference = 0;
	msg(MSG_FATAL,"evaluating clusters");
	//FIND LEVEL1 Clusters
	//
	if (HashAttack == NULL) { msg(MSG_FATAL,"arsch"); };
	for (int i = 0;i < 2048;i++)
	{
		if (HashAttack->table[i] != NULL)
		{
			msg(MSG_FATAL,"Bucket %d was not empty, found %d entries",i,HashAttack->table[i]->size());
			for (int j = 0;j < HashAttack->table[i]->size();j++)
			{
				//now find corresponding defense bucket
				msg(MSG_FATAL,"searching correspondig defense entry");
				ipentry* current = HashAttack->table[i]->at(j);
				uint32_t bucket = current->ip % 2048;

				uint32_t defend_count = 0;

				if (HashDefend->table[i] != NULL) 
				{
					for (int p = 0;p < HashDefend->table[i]->size();p++)
					{
						msg(MSG_FATAL,"found existing vector in defense");
						if (HashDefend->table[i]->at(p)->ip == current->ip)
						{
							defend_count = HashDefend->table[i]->at(p)->count;
							msg(MSG_FATAL,"found ipentry in defense, offering %d",defend_count);
							break;			
						}
					}
				}
				else msg(MSG_FATAL,"didnt find defense");
				if (maxdifference < (current->count - defend_count) && ((current->count+1)/(defend_count+1)) > 3) { max = current; maxdifference = current->count - defend_count;}
			}	

		}
	}
	if (max!=NULL)
	{
		msg(MSG_FATAL,"biggest victim was %x with %d unmatched syns",max->ip,maxdifference);
		DosCluster cluster;
		if (active_out)
		{
			cluster.entry.srcip = max->ip;
			cluster.entry.dstip = 0;
			cluster.entry.srcprt= 0;
			cluster.entry.dstprt= 0;
			cluster.mask = 0;
			//todo viele schoene defines fuer die mask
			cluster.mask |= 14;
			cluster.wasFiltered = false;
		}
		if (active_in)
		{
			cluster.entry.srcip = 0;
			cluster.entry.dstip = max->ip;
			cluster.entry.srcprt= 0;
			cluster.entry.dstprt= 0;
			cluster.mask = 0;
			//todo viele schoene defines fuer die mask
			cluster.mask |= 13;
			cluster.wasFiltered = false;
		}
		clusters.push_back(cluster);
	}
}


int SynDosDetect::checkForAttack(const Packet* p)
{

	//we are currently seraching for clusters
	if (busy) return 0;

	uint32_t now = time(0);

	//check if packet is TCP
	if ((*(p->netHeader + 9) & 6) != 6) return 0;

	//check if tcp packet has syn rst or fin set; drop unless one second passed
	unsigned char ddos_flags = *(p->transportHeader + 13);
	if (((ddos_flags & 7) != 0) && (now == lastCheck)) return 0;

	pEntry currentPacket;
	currentPacket.srcip = ntohl(*(uint32_t*)(p->netHeader+12));
	currentPacket.dstip = ntohl(*(uint32_t*)(p->netHeader+16));
	currentPacket.srcprt = ntohs(*(uint16_t*)(p->transportHeader+0));
	currentPacket.dstprt = ntohs(*(uint16_t*)(p->transportHeader+2));



	//check if the received packet is in dos cluster
	for (int i = 0;i < clusters.size();i++)
	{
		if (!clusters[i].entry.srcip || clusters[i].entry.srcip == currentPacket.srcip) {
			if (!clusters[i].entry.dstip || clusters[i].entry.dstip == currentPacket.dstip) {
				if (!clusters[i].entry.srcprt || clusters[i].entry.srcprt == currentPacket.srcprt) {
					if (!clusters[i].entry.dstprt || clusters[i].entry.dstprt == currentPacket.dstprt) {
						msg(MSG_FATAL,"bad packet");
						return clusters[i].mask; } } } }		
	}



	//check wheter packet is incoming or outgoing
	uint16_t homenet = (currentPacket.srcip&0xFFFF0000)>>16;
	uint16_t destnet = (currentPacket.dstip&0xFFFF0000)>>16;
	if ((homenet == 0x8B3F) || (homenet == 0x6BEB) || (homenet == 0x83BC))
	{
		if (ddos_flags & 2)
		{
			Outgoing.varCountIn++; //SYN
			if (active_out) observePacketAttack(currentPacket);
		}

		else if (ddos_flags & 5)
		{
			Outgoing.varCountOut++; //FIN or RST
			if (active_in) observePacketDefend(currentPacket);
		}

	}
	else if ((destnet == 0x8B3F) || (destnet == 0x6BEB) || (destnet == 0x83BC))
	{
		if (ddos_flags & 2)
		{
			Incoming.varCountIn++; //SYN
			if (active_in) observePacketAttack(currentPacket);
		}
		else if (ddos_flags & 5)
		{
			Incoming.varCountOut++; //FIN or RST
			if (active_out) observePacketDefend(currentPacket);
		}
	}


	if (now != lastCheck)
	{
		//one second has passed, we have to adjust expected values and check for threshold
		lastCheck = now;

		count++;

		Incoming.expectedValueIn = 0;
		Incoming.expectedValueOut = 0;

		Outgoing.expectedValueIn = 0;
		Outgoing.expectedValueOut = 0;

		for (int i = 0;i<5;i++)
		{
			Incoming.expectedValueIn += Incoming.InHistory[(count+i) % 5] / 5.0;
			Incoming.expectedValueOut += Incoming.OutHistory[(count+i) % 5] / 5.0;

			Outgoing.expectedValueIn += Outgoing.InHistory[(count+i) % 5] / 5.0;
			Outgoing.expectedValueOut += Outgoing.OutHistory[(count+i) % 5] / 5.0;
			//	msg(MSG_FATAL,"%d %d %d %d",Incoming.expectedValueIn,Outgoing.expectedValueOut,Outgoing.expectedValueIn,Incoming.expectedValueOut);
		}

		//this was for testing if expectation values are ok
		//double exp_ratio_in =  ( (double) Incoming.expectedValueIn  + 1.0) / ((double) Outgoing.expectedValueOut  + 1.0);
		//double exp_ratio_out = ( (double) Outgoing.expectedValueIn  + 1.0) / ((double) Incoming.expectedValueOut  + 1.0);

		double ratio_in  = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);
		double ratio_out = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);

		double t_in = ((double) Incoming.expectedValueIn + 500) / ((double) Outgoing.expectedValueOut + 1.0);
		double t_out = ((double) Outgoing.expectedValueIn + 500) / ((double) Incoming.expectedValueOut + 1.0);

		//		msg(MSG_FATAL,"incoming ddos: %.2f > %.2f",ratio_out,t_out);
		//		msg(MSG_FATAL,"outgoing ddos: %.2f > %.2f",ratio_in,t_in);

		if (active_in == true || active_out == true)
		{
			//identify clusters
			thread = new Thread(SynDosDetect::threadWrapper);
			thread->run(NULL);
			active_in = false;
			active_out = false;
			delete HashAttack;
			delete HashDefend;
			Incoming.varCountIn = 0;
			Incoming.varCountOut = 0;
			Outgoing.varCountIn = 0;
			Outgoing.varCountOut = 0;
			return 0;
		}

		if (ratio_in > t_in)
		{
			active_in = true;
			HashDefend = new DosHash();
			HashAttack = new DosHash();
			msg(MSG_FATAL,"incoming ddos");
			return 0;
		}
		else if (ratio_out > t_out)
		{
			active_out = true;
			HashDefend = new DosHash();
			HashAttack = new DosHash();
			msg(MSG_FATAL,"outgoing ddos: %.2f > %.2f",ratio_out,t_out);
			return 0;
		}

		//update history
		Incoming.InHistory[count % 5] = Incoming.varCountIn;
		Incoming.OutHistory[count % 5] = Incoming.varCountOut;
		Outgoing.InHistory[count % 5] = Outgoing.varCountIn;
		Outgoing.OutHistory[count % 5] = Outgoing.varCountOut;
		//msg(MSG_FATAL,"%d %d %d %d",Incoming.varCountIn,Incoming.varCountOut,Outgoing.varCountIn,Outgoing.varCountOut);

		//reset counter
		Incoming.varCountIn = 0;
		Incoming.varCountOut = 0;
		Outgoing.varCountIn = 0;
		Outgoing.varCountOut = 0;

	}
	return 0;
}

void SynDosDetect::observePacketDefend(pEntry currentPacket)
{

	msg(MSG_FATAL,"Analyzing possible defense packet from %x",currentPacket.srcip);
	observePacket(HashDefend,currentPacket,currentPacket.srcip);
}

void SynDosDetect::observePacketAttack(pEntry currentPacket)
{
	msg(MSG_FATAL,"Analyzing possible attack packet to %x",currentPacket.dstip);
	observePacket(HashAttack,currentPacket,currentPacket.dstip);
}

int SynDosDetect::compare_entry(pEntry e,pEntry p)
{

	if (p.srcip != e.srcip) return 0;
	if (p.dstip != e.dstip) return 0;
	if (p.srcprt != e.srcprt) return 0;
	if (p.dstprt != e.dstprt) return 0;
	return 1;

}


void SynDosDetect::observePacket(DosHash* hash,pEntry p,uint32_t ip)
{
	uint32_t bucket = (ip % 2048);

	msg(MSG_FATAL,"bucket is %d",bucket);

	if (hash->table[bucket] == NULL)
	{
		msg(MSG_FATAL,"there was no ipentry vector, creating for ip %x",ip);

		std::vector<ipentry*>* ipentrylist = new std::vector<ipentry*>();

		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		newipentry->entries.push_back(p);

		ipentrylist->push_back(newipentry);
		hash->table[bucket] = ipentrylist;

		return;

	}
	else
	{
		msg(MSG_FATAL,"there was already an ipentry vector");
		for (int i = 0;i < hash->table[bucket]->size();i++)
		{
			ipentry* current = hash->table[bucket]->at(i);
			if (current->ip == ip)
			{
				current->count++;
				msg(MSG_FATAL,"i already found an entry for ip %x",ip);

				for (int j = 0;j < current->entries.size();j++)
				{
					if (compare_entry(current->entries[j],p))
					{
						msg(MSG_FATAL,"this flow has already been monitored %x",ip);
						return;
					}
				}
				msg(MSG_FATAL,"this flow could not be found, creating one");
				current->entries.push_back(p);
				return;
			}
		}

		msg(MSG_FATAL,"the ip entry list did not contain my ip, adding ipentry for %x",ip);
		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		newipentry->entries.push_back(p);
		hash->table[bucket]->push_back(newipentry);
	}


}

SynDosDetect::SynDosDetect() {
	setup = true;
	busy = false;
	active_in  = false;
	active_out = false;
	count = 0;
	for (int i = 0; i < 5;i++)
	{
		Incoming.InHistory[i] = 0;
		Incoming.OutHistory[i] = 0;
		Outgoing.InHistory[i] = 0;
		Outgoing.OutHistory[i] = 0;
	}
	Incoming.varCountIn = 0;
	Incoming.varCountOut = 0;
	Outgoing.varCountIn = 0;
	Outgoing.varCountOut = 0;
}

void* SynDosDetect::threadWrapper(void* instance)
	{
	msg(MSG_FATAL,"cluster finding thread started");
	SynDosDetect* inst = (SynDosDetect*) instance;
	inst->busy = true;
	inst->evaluateClusters();
	inst->busy = false;
	msg(MSG_FATAL,"cluster finding thread ended");
	}

SynDosDetect::~SynDosDetect() { }
