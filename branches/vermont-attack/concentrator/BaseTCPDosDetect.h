#ifndef BASETCPDOSDETECT_H
#define BASETCPDOSDETECT_H

#include "ipfix.hpp"
#include <iostream>
#include <vector>
#include "sampler/Packet.h"
#include "common/Thread.h"

typedef struct pEntry {
	uint32_t srcip;
	uint32_t dstip;
	uint16_t srcprt;
	uint16_t dstprt;
}pEntry;

typedef struct DosCluster {
	pEntry entry;
	bool wasFiltered;
	uint32_t mask;
}DosCluster;

typedef struct ipentry {
	uint32_t ip;
	std::vector<pEntry> entries;
	uint32_t count;
}ipentry;

class DosHash {
	public:
		std::vector<ipentry*>** table;

		DosHash() { 
			table = new std::vector<ipentry*>*[2048];
			for (int i = 0;i<2048;i++)
			{
				table[i] = NULL;
			}
		};
		~DosHash() {};
};

class BaseTCPDosDetect {

	struct DdosDefense {
		uint32_t varCountIn;
		uint32_t varCountOut;
		uint32_t InHistory[5];
		uint32_t OutHistory[5];
		uint32_t expectedValueIn;
		uint32_t expectedValueOut;
	};

	protected:
	uint32_t lastCheck;
	DdosDefense Incoming;
	DdosDefense Outgoing;
	Thread* thread;
	DosHash* HashDefend;
	DosHash* HashAttack;
	std::vector<DosCluster> clusters;
	public:
	BaseTCPDosDetect(); 
	virtual ~BaseTCPDosDetect();

	static uint8_t idToMask(uint16_t field);
	virtual int checkForAttack(const Packet* p) = 0;

};

#endif
