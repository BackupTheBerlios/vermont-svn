#include "SynDosDetect.h"

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
			if (active_out) observePacketIn(p);
		}

		if (*ddos_flags & 5)
		{
			Outgoing.varCountOut++; //FIN or RST
			if (active_in) observePacketOut(p);
		}

	}
	else if ((dstip ^ 0x8B3F) == 0 || (dstip ^ 0x6BEB) == 0 || (dstip^0x83BC) == 0)
	{
		if (*ddos_flags & 2)
		{
			Incoming.varCountIn++; //SYN
			if (active_in) observePacketIn(p);
		}
		if (*ddos_flags & 5)
		{
			Incoming.varCountOut++; //FIN or RST
			if (active_out) observePacketOut(p);
		}
	}

	uint32_t now = time(0);

	if (now != lastCheck)
	{
		lastCheck = now;

		Incoming.expectedValueIn = 0;
		Incoming.expectedValueOut = 0;

		Outgoing.expectedValueIn = 0;
		Outgoing.expectedValueOut = 0;

		for (int i = 0;i<5;i++)
		{
			Incoming.expectedValueIn += Incoming.InHistory[(now+i) % 5];
			Incoming.expectedValueOut += Incoming.OutHistory[(now+i) % 5];

			Outgoing.expectedValueIn += Outgoing.InHistory[(now+i) % 5];
			Outgoing.expectedValueOut += Outgoing.OutHistory[(now+i) % 5];
		}

		double exp_ratio_in =  ( (double) Incoming.expectedValueIn / 5.0 + 1.0) / ((double) Outgoing.expectedValueOut / 5.0 + 1.0);
		double exp_ratio_out = ( (double) Outgoing.expectedValueIn / 5.0 + 1.0) / ((double) Incoming.expectedValueOut / 5.0 + 1.0);

		double ratio_in  = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);
		double ratio_out = ((double) Incoming.varCountIn + 1.0) / ((double) Outgoing.varCountOut + 1.0);

		double t_in = ((double) Incoming.expectedValueIn/5.0 + 500) / ((double) Outgoing.expectedValueOut/5.0 + 1.0);
		double t_out = ((double) Outgoing.expectedValueIn /5.0 + 500) / ((double) Incoming.expectedValueOut /5.0 + 1.0);


		if (ratio_in > t_in)
		{
			active_in = true;
		}
		else if (ratio_out > t_out)
		{
			active_out = true;
		}

		Incoming.InHistory[now % 5] = Incoming.varCountIn;
		Incoming.OutHistory[now % 5] = Incoming.varCountOut;

		Outgoing.InHistory[now % 5] = Outgoing.varCountIn;
		Outgoing.OutHistory[now % 5] = Outgoing.varCountOut;


		Incoming.varCountIn = 0;
		Incoming.varCountOut = 0;
		Outgoing.varCountIn = 0;
		Outgoing.varCountOut = 0;
	}


}

void SynDosDetect::observePacketIn(const Packet* p)
{
	if (HashIncoming == NULL)
		HashIncoming = new DosHash();


	uint32_t srcip = ntohl(*(p->netHeader+12));

	uint32_t bucket = (srcip % 2048);

	if (HashIncoming->table[bucket] == NULL)
	{

		std::list<ipentry*>* ipentrylist = new std::list<ipentry*>();

		ipentry* newipentry = new ipentry;
		newipentry->ip = srcip;
		newipentry->count = 1;
		newipentry->entries.push_back(createNewpEntry(p));

		ipentrylist->push_front(newipentry);
		HashIncoming->table[bucket] = ipentrylist;

		return;

	}
	else
	{
		std::list<ipentry*>::iterator iter = HashIncoming->table[bucket]->begin();

		while(iter != HashIncoming->table[bucket]->end())
		{
			if ((*iter)->ip == srcip)
			{
				(*iter)->count++;

				std::list<pEntry>::iterator iter2 = (*iter)->entries.begin();
				while (iter2 != (*iter)->entries.end())
				{
					if (compare_entry(*iter2,p))
						return;
					iter2++;
				}
				(*iter)->entries.push_back(createNewpEntry(p));
				return;
			}
			iter++;
		}

		ipentry* newipentry = new ipentry;
		newipentry->ip = srcip;
		newipentry->count = 1;
		newipentry->entries.push_back(createNewpEntry(p));
		HashIncoming->table[bucket]->push_front(newipentry);
	}
}

int compare_entry(pEntry e,const Packet* p)
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

void SynDosDetect::observePacketOut(const Packet* p)
{



}
pEntry SynDosDetect::createNewpEntry(const Packet* p)
{

	pEntry newpentry;

	uint32_t srcip = ntohl(*(p->netHeader+12));
	uint32_t dstip = ntohl(*(p->netHeader+14));
	uint16_t srcprt = ntohs(*(p->transportHeader+0));
	uint16_t dstprt = ntohs(*(p->transportHeader+2));
	newpentry.srcip = srcip;
	newpentry.dstip = dstip;
	newpentry.srcprt = srcprt;
	newpentry.dstprt = dstprt;
	return newpentry;


}

SynDosDetect::SynDosDetect() {
	active_in = false;
	active_out= false;
}

SynDosDetect::~SynDosDetect() { }
