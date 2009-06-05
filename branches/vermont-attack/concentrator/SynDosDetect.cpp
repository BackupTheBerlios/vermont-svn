#include "SynDosDetect.h"


void SynDosDetect::evaluateClusters()
{

	msg(MSG_FATAL,"at %d",active_thres);
	ipentry* max = NULL;
	uint32_t maxdifference = 0;
	//msg(MSG_FATAL,"evaluating clusters");
	//FIND LEVEL1 Clusters
	//
	for (int i = 0;i < DOSHASH_SIZE;i++)
	{
		if (HashAttack->table[i] != NULL)
		{
			//msg(MSG_FATAL,"Bucket %d was not empty, found %d entries",i,HashAttack->table[i]->size());
			for (int j = 0;j < HashAttack->table[i]->size();j++)
			{
				//now find corresponding defense bucket
				//msg(MSG_FATAL,"searching correspondig defense entry");
				ipentry* current = HashAttack->table[i]->at(j);
				uint32_t bucket = current->ip % DOSHASH_SIZE;

				uint32_t defend_count = 0;

				if (HashDefend->table[i] != NULL) 
				{
					for (int p = 0;p < HashDefend->table[i]->size();p++)
					{
						//msg(MSG_FATAL,"found existing vector in defense");
						if (HashDefend->table[i]->at(p)->ip == current->ip)
						{
							defend_count = HashDefend->table[i]->at(p)->count;
							//msg(MSG_FATAL,"found ipentry in defense, offering %d",defend_count);
							break;			
						}
					}
				}
				else { //msg(MSG_FATAL,"didnt find defense"); 
					}
				if ( (maxdifference < (int) ( current->count - defend_count)) && ( (int) ( current->count - defend_count)  > (int) (0.8 * active_thres)) ) { 

				msg(MSG_FATAL,"%d > %d",current->count - defend_count,(int) (0.8*active_thres));
				max = current; 
				maxdifference = current->count - defend_count;
				}
			}	

		}
	}
	if (max!=NULL)
	{
		msg(MSG_FATAL,"level1 cluster was %x with %d unmatched syns",max->ip,maxdifference);


		//find L2 clusters
		//test if attack was from/to specific ip
		ipentry* def = NULL;
		uint32_t index = max->ip % 2048;	
		if (HashDefend->table[index] != NULL) 
			{
			for (int p = 0;p < HashDefend->table[index]->size();p++)
				{
				if (HashDefend->table[index]->at(p)->ip == max->ip)
					{
						def = HashDefend->table[index]->at(p);
					}
				}			
			}

		uint32_t difference = 0;
		uint32_t max_cip = 0;
		std::map<uint32_t,uint32_t>::iterator iter = max->sum_ip.begin();
		while (iter != max->sum_ip.end())
		{
		
			difference = iter->second;
			if (def != NULL)
			{
				if (def->sum_ip.find(iter->first) != def->sum_ip.end())
					difference  -= def->sum_ip[iter->first];

			}

			if (difference > maxdifference * 0.9)
			{
				max_cip = iter->first;
				break;
			}
			iter++;				
		}

		uint32_t max_srcprt = 0;
		std::map<uint16_t,uint32_t>::iterator iter2 = max->sum_srcprt.begin();
		while (iter2 != max->sum_srcprt.end())
		{
			difference = iter2->second;
			if (def != NULL)
			{
				if (def->sum_dstprt.find(iter2->first) != def->sum_dstprt.end())
					difference  -= def->sum_dstprt[iter2->first];

			}

			if (difference > maxdifference * 0.9)
			{
				//msg(MSG_FATAL,"srcprt: we got a winner, its port %d with %d entries",iter2->first,iter2->second);
				max_srcprt = iter2->first;
				break;
			}
			iter2++;				
		}

		uint32_t max_dstprt = 0;
		std::map<uint16_t,uint32_t>::iterator iter3 = max->sum_dstprt.begin();
		while (iter3 != max->sum_dstprt.end())
		{
			difference = iter3->second;
			if (def != NULL)
			{
				if (def->sum_srcprt.find(iter3->first) != def->sum_srcprt.end())
					difference  -= def->sum_srcprt[iter3->first];

			}

			if (difference > maxdifference * 0.9)
			{
				//msg(MSG_FATAL,"dstprt: we got a winner, its port %d with %d entries",iter3->first,iter3->second);
				max_dstprt = iter3->first;
				break;
			}
			iter3++;				
		}

		DosCluster cluster;
		cluster.time= time(0);
		if (active_out)
		{
			cluster.entry.srcip = max->ip;
			cluster.entry.dstip = max_cip;
			cluster.mask = 0;
			//todo viele schoene defines fuer die mask
			if (max_cip == 0) cluster.mask |= 2;
			cluster.wasFiltered = false;
		}
		if (active_in)
		{
			cluster.entry.srcip = max_cip;
			cluster.entry.dstip = max->ip;
			cluster.entry.srcprt= max_srcprt;
			cluster.entry.dstprt= max_dstprt;
			cluster.mask = 0;
			//todo viele schoene defines fuer die mask
			if (max_cip == 0) cluster.mask |= 1;
			cluster.wasFiltered = false;
		}
			cluster.entry.srcprt= max_srcprt;
			cluster.entry.dstprt= max_dstprt;
			if (max_srcprt == 0) cluster.mask |= 4;
			if (max_dstprt == 0) cluster.mask |= 8;

		msg(MSG_FATAL,"DOS CLUSTER IS: srcip %x dstip %x srcprt %d dstprt %d",cluster.entry.srcip,cluster.entry.dstip,cluster.entry.srcprt,cluster.entry.dstprt);
		clusters.push_back(cluster);
	}
	else {
		msg(MSG_FATAL,"false positive trigger, ignoring, maxdifference was not bigger than %d",active_thres);
	}
}


int SynDosDetect::checkForAttack(const Packet* p,uint32_t* dummy)
{

	//we are currently seraching for clusters
	if (busy) {
	//	msg(MSG_FATAL,"i'm busy");
		return 0;
	}



	uint32_t now = time(0);

	//check if packet is TCP
	if ((*(p->netHeader + 9) & 6) != 6) return 0;

	//check if tcp packet has syn rst or fin set; drop unless one second passed
	unsigned char ddos_flags = *(p->transportHeader + 13);
	if (((ddos_flags & 7) == 0) && (now == lastCheck)) return 0;

	pEntry currentPacket;
	currentPacket.srcip = ntohl(*(uint32_t*)(p->netHeader+12));
	currentPacket.dstip = ntohl(*(uint32_t*)(p->netHeader+16));
	currentPacket.srcprt = ntohs(*(uint16_t*)(p->transportHeader+0));
	currentPacket.dstprt = ntohs(*(uint16_t*)(p->transportHeader+2));



	//check if the received packet is in dos cluster
	for (int i = 0;i < clusters.size();i++)
	{
		if (now >= clusters[i].time + clusterLifeTime)
			{
				clusters.erase(clusters.begin() + i--);	
				msg(MSG_FATAL,"dos cluster timed out!");
				continue;
			}
		if (!clusters[i].entry.srcip || clusters[i].entry.srcip == currentPacket.srcip) {
			if (!clusters[i].entry.dstip || clusters[i].entry.dstip == currentPacket.dstip) {
				if (!clusters[i].entry.srcprt || clusters[i].entry.srcprt == currentPacket.srcprt) {
					if (!clusters[i].entry.dstprt || clusters[i].entry.dstprt == currentPacket.dstprt) {
						//msg(MSG_FATAL,"bad packet");
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
			//	//msg(MSG_FATAL,"%d %d %d %d",Incoming.expectedValueIn,Outgoing.expectedValueOut,Outgoing.expectedValueIn,Incoming.expectedValueOut);
		}

		//this was for testing if expectation values are ok
		//double exp_ratio_in =  ( (double) Incoming.expectedValueIn  + 1.0) / ((double) Outgoing.expectedValueOut  + 1.0);
		//double exp_ratio_out = ( (double) Outgoing.expectedValueIn  + 1.0) / ((double) Incoming.expectedValueOut  + 1.0);

		double ratio_in  = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);
		double ratio_out = ((double) Outgoing.varCountIn + 1.0) / ((double) Incoming.varCountOut + 1.0);


		int threshold_in = Incoming.expectedValueIn < 225? 75 : Incoming.expectedValueIn/3;
		double t_in = ((double) Incoming.expectedValueIn + threshold_in) / ((double) Outgoing.expectedValueOut + 1.0);
		int threshold_out = Outgoing.expectedValueIn < 150? 75 : Outgoing.expectedValueIn/2;
		double t_out = ((double) Outgoing.expectedValueIn + threshold_out) / ((double) Incoming.expectedValueOut + 1.0);

//		msg(MSG_FATAL,"current ratio: %.2f > %.2f",ratio_out,t_out);
//		msg(MSG_FATAL,"current ratio: %.2f > %.2f",ratio_in,t_in);

		if (active_in == true || active_out == true)
		{
			//identify clusters
			//thread = new Thread(SynDosDetect::threadWrapper);
			//thread->run(this);
			Incoming.varCountIn = 0;
			Incoming.varCountOut = 0;
			Outgoing.varCountIn = 0;
			Outgoing.varCountOut = 0;
			return 0;
		}

		if (ratio_in > t_in)
		{
			active_in = true;
			active_thres = threshold_in;
			HashDefend = new DosHash();
			HashAttack = new DosHash();
			msg(MSG_FATAL,"incoming ddos %.2f > %.2f",ratio_in,t_in);
			return 0;
		}
		else if (ratio_out > t_out)
		{
			active_out = true;
			active_thres = threshold_out;
			HashDefend = new DosHash();
			HashAttack = new DosHash();

			msg(MSG_FATAL,"outgoing ddos: %.2f > %.2f",ratio_out,t_out);
			msg(MSG_FATAL,"measured syns sent: %d  fins received: %d",Outgoing.varCountIn,Incoming.varCountOut);
			msg(MSG_FATAL,"expected syns %d, thresh: %d,expected fins %d ",Outgoing.expectedValueIn,threshold_out,Incoming.expectedValueOut);
			return 0;
		}

		//update history
		Incoming.InHistory[count % 5] = Incoming.varCountIn;
		Incoming.OutHistory[count % 5] = Incoming.varCountOut;
		Outgoing.InHistory[count % 5] = Outgoing.varCountIn;
		Outgoing.OutHistory[count % 5] = Outgoing.varCountOut;
		//msg(MSG_FATAL,"%d %d %d %d",Incoming.varCountIn,Outgoing.varCountOut,Outgoing.varCountIn,Incoming.varCountOut);

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

	//msg(MSG_FATAL,"Analyzing possible defense packet from %x",currentPacket.srcip);
	observePacket(HashDefend,currentPacket,currentPacket.srcip);
}

void SynDosDetect::observePacketAttack(pEntry currentPacket)
{
	//msg(MSG_FATAL,"Analyzing possible attack packet to %x",currentPacket.dstip);
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

void SynDosDetect::updateMaps(ipentry* ip,pEntry p,uint32_t cip)
{
	//ip list
	if (cip == p.dstip) cip = p.srcip;
	else cip = p.dstip;

	if (ip->sum_ip.find(cip) != ip->sum_ip.end())
	{
		ip->sum_ip[cip]++;
	}
	else {
		ip->sum_ip[cip] = 1;
	}

	if (ip->sum_srcprt.find(p.srcprt) != ip->sum_srcprt.end())
	{
		ip->sum_srcprt[p.srcprt]++;
	}
	else {
		ip->sum_srcprt[p.srcprt] = 1;
	}

	if (ip->sum_dstprt.find(p.dstprt) != ip->sum_dstprt.end())
	{
		ip->sum_dstprt[p.dstprt]++;
		//msg(MSG_FATAL,"new dstprt value for %x on %d is %d",ip->ip,p.dstprt,ip->sum_dstprt[p.dstprt]);
	}
	else {
		ip->sum_dstprt[p.dstprt] = 1;
		//msg(MSG_FATAL,"new dstprt value for %x on %d is %d",ip->ip,p.dstprt,ip->sum_dstprt[p.dstprt]);
	}


}

void SynDosDetect::observePacket(DosHash* hash,pEntry p,uint32_t ip)
{
	uint32_t bucket = (ip % DOSHASH_SIZE);

	//msg(MSG_FATAL,"bucket is %d",bucket);

	if (hash->table[bucket] == NULL)
	{
		//msg(MSG_FATAL,"there was no ipentry vector, creating for ip %x",ip);

		std::vector<ipentry*>* ipentrylist = new std::vector<ipentry*>();

		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		updateMaps(newipentry,p,ip);

		ipentrylist->push_back(newipentry);
		hash->table[bucket] = ipentrylist;

		return;

	}
	else
	{
		//msg(MSG_FATAL,"there was already an ipentry vector");
		for (int i = 0;i < hash->table[bucket]->size();i++)
		{
			ipentry* current = hash->table[bucket]->at(i);
			if (current->ip == ip)
			{
				current->count++;
				//msg(MSG_FATAL,"i already found an entry for ip %x",ip);
				updateMaps(current,p,ip);
				return;
			}
		}

		//msg(MSG_FATAL,"the ip entry list did not contain my ip, adding ipentry for %x",ip);
		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		updateMaps(newipentry,p,ip);
		hash->table[bucket]->push_back(newipentry);
	}


}

SynDosDetect::SynDosDetect(int dosTemplateId,int minimumRate,int clusterLifetime) {
	setup = true;
	busy = false;
	active_in  = false;
	active_out = false;
	count = 0;
	this->dosTemplateId = dosTemplateId;
	clusterLifeTime = clusterLifetime;
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
	msg(MSG_FATAL,"SYN DoS started with: %d %d %d",dosTemplateId,minimumRate,clusterLifetime);
}

void* SynDosDetect::threadWrapper(void* instance)
{
	SynDosDetect* inst = (SynDosDetect*) instance;
	inst->busy = true;
	inst->evaluateClusters();
	inst->lastCheck = time(0);
	inst->cleanUpCluster();
	inst->active_in = false;
	inst->Incoming.InHistory[inst->count % 5] += 2*inst->active_thres;
        inst->Outgoing.InHistory[inst->count % 5] += 2*inst->active_thres;
	inst->active_thres = 0;
	inst->active_out = false;
	inst->busy = false;
}

void SynDosDetect::cleanUpCluster()
{
	for (int i = 0;i<DOSHASH_SIZE;i++)
	{
		if (HashAttack->table[i] != NULL)
		{
			for (int j = 0;j < HashAttack->table[i]->size();j++)
			{
				delete HashAttack->table[i]->at(j);
			}
			delete HashAttack->table[i];
		}
		if (HashDefend->table[i] != NULL)
		{
			for (int j = 0;j < HashDefend->table[i]->size();j++)
			{
				delete HashDefend->table[i]->at(j);
			}
			delete HashDefend->table[i];
		}
	}
	delete HashAttack;
	delete HashDefend;

	HashAttack = NULL;
	HashDefend = NULL;
}
SynDosDetect::~SynDosDetect() { }
