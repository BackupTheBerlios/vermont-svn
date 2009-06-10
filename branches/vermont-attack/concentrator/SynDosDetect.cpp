#include "SynDosDetect.h"


void SynDosDetect::evaluateClusters()
{

	DPRINTF("at %d",active_thres);
	ipentry* max = NULL;
	uint32_t maxdifference = 0;
	DPRINTF("evaluating clusters");
	//FIND LEVEL1 Clusters, that is clusters with a constant dest ip
	for (int i = 0;i < DOSHASH_SIZE;i++)
	{
		if (HashAttack->table[i] != NULL)
		{
			//Bucket i was not empty
			for (int j = 0;j < HashAttack->table[i]->size();j++)
			{
				//now find corresponding defense bucket
				ipentry* current = HashAttack->table[i]->at(j);
				uint32_t bucket = current->ip % DOSHASH_SIZE;

				uint32_t defend_count = 0;

				if (HashDefend->table[i] != NULL) 
				{
					for (int p = 0;p < HashDefend->table[i]->size();p++)
					{
						if (HashDefend->table[i]->at(p)->ip == current->ip)
						{
							defend_count = HashDefend->table[i]->at(p)->count;
							break;			
						}
					}
				}
				
				//check if found entry is our new maximum
				if ( (maxdifference < (int) ( current->count - defend_count)) && ( (int) ( current->count - defend_count)  > (int) (0.8 * active_thres)) ) { 
					max = current; 
					maxdifference = current->count - defend_count;
				}
			}	

		}
	}
	if (max!=NULL)
	{
		DPRINTF("level1 cluster was %x with %d unmatched syns",max->ip,maxdifference);


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

		//check if srcport was constant
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
				max_srcprt = iter2->first;
				break;
			}
			iter2++;				
		}


		//check if dstport was constant
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
				max_dstprt = iter3->first;
				break;
			}
			iter3++;				
		}


		//create cluster from found fields
		DosCluster cluster;
		cluster.time= time(0);
		if (active_out)
		{
			cluster.entry.srcip = max->ip;
			cluster.entry.dstip = max_cip;
			cluster.mask = 0;
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
			if (max_cip == 0) cluster.mask |= 1;
			cluster.wasFiltered = false;
		}
		cluster.entry.srcprt= max_srcprt;
		cluster.entry.dstprt= max_dstprt;
		if (max_srcprt == 0) cluster.mask |= 4;
		if (max_dstprt == 0) cluster.mask |= 8;

		DPRINTF("DOS CLUSTER IS: srcip %x dstip %x srcprt %d dstprt %d",cluster.entry.srcip,cluster.entry.dstip,cluster.entry.srcprt,cluster.entry.dstprt);
		clusters.push_back(cluster);
	}
}


int SynDosDetect::checkForAttack(const Packet* p,uint32_t* dummy)
{

	if (busy) {
		//we are currently seraching for clusters
		return 0;
	}


	uint32_t now = time(0);

	//check if packet is TCP
	if ((*(p->netHeader + 9) & 6) != 6) return 0;

	//check if tcp packet has syn rst or fin set; drop unless one second passed
	unsigned char ddos_flags = *(p->transportHeader + 13);
	if (((ddos_flags & 7) == 0) && (now == lastCheck)) return 0;


	//copy header information into local vars for faster look-up
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
			DPRINTF("dos cluster timed out!");
			continue;
		}
		if (!clusters[i].entry.srcip || clusters[i].entry.srcip == currentPacket.srcip) {
			if (!clusters[i].entry.dstip || clusters[i].entry.dstip == currentPacket.dstip) {
				if (!clusters[i].entry.srcprt || clusters[i].entry.srcprt == currentPacket.srcprt) {
					if (!clusters[i].entry.dstprt || clusters[i].entry.dstprt == currentPacket.dstprt) {
						return clusters[i].mask; } } } }		
	}



	//Check if packet is incoming or outgoing
	bool outgoing = false;
	bool incoming = false;
	map<uint32_t,uint32_t>::iterator iter = internals.begin();
	while (iter != internals.end()) 
	{
		if ((currentPacket.srcip & iter->second) == iter->first) 
		{
			outgoing=true;
			break;
		}
		if ((currentPacket.dstip & iter->second) == iter->first) 
		{

			incoming=true;
			break;
		}

		iter++;
	}
	if (outgoing)
	{
		if (ddos_flags & 2)
		{
			Outgoing.varCountIn++; //SYN
			if (active_out) observePacketAttack(currentPacket);
		}

		else if (ddos_flags & 5)
		{
			Outgoing.varCountOut++; //FIN 
			if (active_in) observePacketDefend(currentPacket);
		}

	}
	else if (incoming)
	{
		if (ddos_flags & 2)
		{
			Incoming.varCountIn++; //SYN
			if (active_in) observePacketAttack(currentPacket);
		}
		else if (ddos_flags & 5)
		{
			Incoming.varCountOut++; //FIN 
			if (active_out) observePacketDefend(currentPacket);
		}
	}
	else {
		//this packet was received at a non monitored interface
		return 0;
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


		//calc average value from last 5 seconds
		for (int i = 0;i<5;i++)
		{
			Incoming.expectedValueIn += Incoming.InHistory[(count+i) % 5] / 5.0;
			Incoming.expectedValueOut += Incoming.OutHistory[(count+i) % 5] / 5.0;

			Outgoing.expectedValueIn += Outgoing.InHistory[(count+i) % 5] / 5.0;
			Outgoing.expectedValueOut += Outgoing.OutHistory[(count+i) % 5] / 5.0;
		}


		//compute ratios and thresholds
		double ratio_in  = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);
		double ratio_out = ((double) Outgoing.varCountIn + 1.0) / ((double) Incoming.varCountOut + 1.0);


		int threshold_in = Incoming.expectedValueIn < 225? 75 : Incoming.expectedValueIn/3;
		double t_in = ((double) Incoming.expectedValueIn + threshold_in) / ((double) Outgoing.expectedValueOut + 1.0);
		int threshold_out = Outgoing.expectedValueIn < 225? 75 : Outgoing.expectedValueIn/3;
		double t_out = ((double) Outgoing.expectedValueIn + threshold_out) / ((double) Incoming.expectedValueOut + 1.0);

		if (active_in == true || active_out == true)
		{
			//identify clusters in a new thread
			thread = new Thread(SynDosDetect::threadWrapper);
			thread->run(this);
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
			DPRINTF("incoming ddos %.2f > %.2f",ratio_in,t_in);
			return 0;
		}
		else if (ratio_out > t_out)
		{
			active_out = true;
			active_thres = threshold_out;
			HashDefend = new DosHash();
			HashAttack = new DosHash();
			DPRINTF("outgoing ddos: %.2f > %.2f",ratio_out,t_out);
			return 0;
		}

		//update history
		Incoming.InHistory[count % 5] = Incoming.varCountIn;
		Incoming.OutHistory[count % 5] = Incoming.varCountOut;
		Outgoing.InHistory[count % 5] = Outgoing.varCountIn;
		Outgoing.OutHistory[count % 5] = Outgoing.varCountOut;

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
	observePacket(HashDefend,currentPacket,currentPacket.srcip);
}

void SynDosDetect::observePacketAttack(pEntry currentPacket)
{
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
	}
	else {
		ip->sum_dstprt[p.dstprt] = 1;
	}


}

void SynDosDetect::observePacket(DosHash* hash,pEntry p,uint32_t ip)
{
	uint32_t bucket = (ip % DOSHASH_SIZE);

	if (hash->table[bucket] == NULL)
	{
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
		for (int i = 0;i < hash->table[bucket]->size();i++)
		{
			ipentry* current = hash->table[bucket]->at(i);
			if (current->ip == ip)
			{
				current->count++;
				updateMaps(current,p,ip);
				return;
			}
		}

		//the ip entry list did not contain my ip, adding ipentry
		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		updateMaps(newipentry,p,ip);
		hash->table[bucket]->push_back(newipentry);
	}


}

SynDosDetect::SynDosDetect(int dosTemplateId,int minimumRate,int clusterLifetime,std::map<uint32_t,uint32_t> subnets) {

	//constructor setup
	setup = true;
	busy = false;
	active_in  = false;
	active_out = false;
	count = 0;
	this->dosTemplateId = dosTemplateId;
	clusterLifeTime = clusterLifetime;
	internals = subnets;
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
	DPRINTF("SYN DoS started with: %d %d %d",dosTemplateId,minimumRate,clusterLifetime);
	DPRINTF("My internal networks are:");

	map<uint32_t,uint32_t>::iterator iter = internals.begin();
	while (iter != internals.end()) 
	{
		DPRINTF("%x:%x",iter->first,iter->second);
		iter++;
	}

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
