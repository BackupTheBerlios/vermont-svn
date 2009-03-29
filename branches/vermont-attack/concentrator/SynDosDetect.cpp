#include "SynDosDetect.h"


void SynDosDetect::evaluateClusters()
{

	ipentry* max = NULL;

	msg(MSG_FATAL,"evaluating clusters");
	//FIND LEVEL1 Clusters
	//
	if (HashAttack == NULL) { msg(MSG_FATAL,"arsch"); };
	for (int i = 0;i < 2048;i++)
	{
		if (HashAttack->table[i] != NULL)
		{
			msg(MSG_FATAL,"Bucket %d was not empty",i);
			std::list<ipentry*>::iterator iter = HashAttack->table[i]->begin();

			while (iter != HashAttack->table[i]->end())
			{
				if (max == NULL) max = *iter;
				if (max->count < (*iter)->count) max = *iter;
				iter++;
			}	

		}
	}
	msg(MSG_FATAL,"biggest victim was %x with attack %d",max->ip,max->count);
}


int SynDosDetect::checkForAttack(const Packet* p)
{

	//check if packet is TCP
	if ((*(p->netHeader + 9) & 6) != 6) return 0;

	//get pointer to TCP Flags
	unsigned char* ddos_flags = p->transportHeader + 13;

	//get pointer to srcip
	unsigned char* ips = p->netHeader+12;


	uint32_t srcip = 0;
	for (int j=0; j <2;j++)
	{
		srcip <<=8;
		srcip ^= *(uint8_t*) (ips + j);
		//	printf("%d.",*(uint8_t*)(srcip+j));
	}

	//	printf(" -> ");

	uint32_t dstip = 0;

	for (int j=0; j <2;j++)
	{
		dstip <<=8;
		dstip ^= *(uint8_t*) (ips+4 + j);
		//	printf("%d.",*(uint8_t*)(srcip+4+j));
	}
	//printf("\n", dstip);



	if ((srcip ^ 0x8B3F) == 0 || (srcip ^ 0x6BEB) == 0 || (srcip^0x83BC) == 0)
	{
		if (*ddos_flags & 2)
		{
			Outgoing.varCountIn++; //SYN
			if (active_out) observePacketAttack(p);
		}

		if (*ddos_flags & 5)
		{
			Outgoing.varCountOut++; //FIN or RST
			if (active_in) observePacketDefend(p);
		}

	}
	else if ((dstip ^ 0x8B3F) == 0 || (dstip ^ 0x6BEB) == 0 || (dstip^0x83BC) == 0)
	{
		if (*ddos_flags & 2)
		{
			Incoming.varCountIn++; //SYN
			if (active_in) observePacketAttack(p);
		}
		if (*ddos_flags & 5)
		{
			Incoming.varCountOut++; //FIN or RST
			if (active_out) observePacketDefend(p);
		}
	}

	uint32_t now = time(0);

	if (now != lastCheck)
	{
		lastCheck = now;
		count++;

		if (active_in == true || active_out == true)
		{
			//identify clusters
			evaluateClusters();
			active_in = false;
			active_out = false;
		}

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
//			msg(MSG_FATAL,"%d %d %d %d",Incoming.expectedValueIn,Outgoing.expectedValueOut,Outgoing.expectedValueIn,Incoming.expectedValueOut);
		}

		double exp_ratio_in =  ( (double) Incoming.expectedValueIn  + 1.0) / ((double) Outgoing.expectedValueOut  + 1.0);
		double exp_ratio_out = ( (double) Outgoing.expectedValueIn  + 1.0) / ((double) Incoming.expectedValueOut  + 1.0);

		double ratio_in  = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);
		double ratio_out = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);

		double t_in = ((double) Incoming.expectedValueIn + 500) / ((double) Outgoing.expectedValueOut + 1.0);
		double t_out = ((double) Outgoing.expectedValueIn + 500) / ((double) Incoming.expectedValueOut + 1.0);

//		msg(MSG_FATAL,"outgoing ddos: %.2f > %.2f",ratio_out,t_out);
//		msg(MSG_FATAL,"outgoing ddos: %.2f > %.2f",ratio_in,t_in);

		Incoming.InHistory[count % 5] = Incoming.varCountIn;
		Incoming.OutHistory[count % 5] = Incoming.varCountOut;

		Outgoing.InHistory[count % 5] = Outgoing.varCountIn;
		Outgoing.OutHistory[count % 5] = Outgoing.varCountOut;
//		msg(MSG_FATAL,"%d %d %d %d",Incoming.varCountIn,Incoming.varCountOut,Outgoing.varCountIn,Outgoing.varCountOut);

		Incoming.varCountIn = 0;
		Incoming.varCountOut = 0;
		Outgoing.varCountIn = 0;
		Outgoing.varCountOut = 0;

		if (ratio_in > t_in)
		{
			active_in = true;
			msg(MSG_FATAL,"incoming ddos");
		}
		else if (ratio_out > t_out)
		{
			active_out = true;
			msg(MSG_FATAL,"outgoing ddos: %.2f > %.2f",ratio_out,t_out);
		}
	}


}

void SynDosDetect::observePacketDefend(const Packet* p)
{

	uint32_t srcip = ntohl(*(uint32_t*)(p->netHeader+16));

	msg(MSG_FATAL,"Analyzing possible defense packet from %x",srcip);
	observePacket(HashDefend,p,srcip);
}

void SynDosDetect::observePacketAttack(const Packet* p)
{
	uint32_t dstip = ntohl(*(uint32_t*)(p->netHeader+16));
	msg(MSG_FATAL,"Analyzing possible attack packet to %x",dstip);
	observePacket(HashAttack,p,dstip);
}

int SynDosDetect::compare_entry(pEntry e,const Packet* p)
{

	uint32_t srcip = ntohl(*(p->netHeader+12));
	if (srcip != e.srcip) return 0;

	uint32_t dstip = ntohl(*(p->netHeader+14));
	if (dstip != e.dstip) return 0;

	uint16_t srcprt = ntohs(*(p->transportHeader+0));
	if (srcprt != e.srcprt) return 0;

	uint16_t dstprt = ntohs(*(p->transportHeader+2));
	if (dstprt != e.dstprt) return 0;

	return 1;

}


void SynDosDetect::observePacket(DosHash*& hash,const Packet* p,uint32_t ip)
{
	if (hash == NULL)
	{
		hash = new DosHash();
		msg(MSG_FATAL,"lol");
	}


	uint32_t bucket = (ip % 2048);

	msg(MSG_FATAL,"bucket is %d",bucket);

	if (hash->table[bucket] == NULL)
	{
		msg(MSG_FATAL,"there was no ipentry list, creating for ip %x",ip);

		std::list<ipentry*>* ipentrylist = new std::list<ipentry*>();

		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		newipentry->entries.push_back(createNewpEntry(p));

		ipentrylist->push_front(newipentry);
		hash->table[bucket] = ipentrylist;

		return;

	}
	else
	{
		msg(MSG_FATAL,"there was already an ipentry list");
		std::list<ipentry*>::iterator iter = hash->table[bucket]->begin();

		while(iter != hash->table[bucket]->end())
		{
			if ((*iter)->ip == ip)
			{
				(*iter)->count++;
				msg(MSG_FATAL,"i already found an entry for ip %x",ip);

				std::list<pEntry>::iterator iter2 = (*iter)->entries.begin();
				while (iter2 != (*iter)->entries.end())
				{
					if (compare_entry(*iter2,p))
					{
						msg(MSG_FATAL,"this flow has already been monitored %x",ip);
						return;
					}
					iter2++;
				}
				msg(MSG_FATAL,"this flow could not be found, creating one");
				(*iter)->entries.push_back(createNewpEntry(p));
				return;
			}
			iter++;
		}

		msg(MSG_FATAL,"the ip entry list did not contain my ip, adding ipentry for %x",ip);
		ipentry* newipentry = new ipentry;
		newipentry->ip = ip;
		newipentry->count = 1;
		newipentry->entries.push_back(createNewpEntry(p));
		hash->table[bucket]->push_front(newipentry);
	}


}

pEntry SynDosDetect::createNewpEntry(const Packet* p)
{

	pEntry newpentry;

	uint32_t srcip = ntohl(*(uint32_t*)(p->netHeader+12));
	uint32_t dstip = ntohl(*(uint32_t*)(p->netHeader+14));
	uint16_t srcprt = ntohs(*(uint16_t*)(p->transportHeader+0));
	uint16_t dstprt = ntohs(*(uint16_t*)(p->transportHeader+2));
	newpentry.srcip = srcip;
	newpentry.dstip = dstip;
	newpentry.srcprt = srcprt;
	newpentry.dstprt = dstprt;
	return newpentry;


}

SynDosDetect::SynDosDetect() {
	setup = true;
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
	msg(MSG_FATAL,"created syndos object");
}

SynDosDetect::~SynDosDetect() { }
