#ifndef BASETCPDOSDETECT_H
#define BASETCPDOSDETECT_H

#include "ipfix.hpp"
#include <iostream>
#include <vector>
#include <map>
#include "sampler/Packet.h"
#include "common/Thread.h"

#define DOSHASH_SIZE 2048 
typedef struct pEntry {
	uint32_t srcip;
	uint32_t dstip;
	uint16_t srcprt;
	uint16_t dstprt;
}pEntry;

typedef struct DosCluster {
	pEntry entry;
	uint32_t time;
	bool wasFiltered; //for later use
	uint32_t mask;
}DosCluster;

typedef struct ipentry {
	uint32_t ip;
	std::map<uint32_t,uint32_t> sum_ip;
	std::map<uint16_t,uint32_t> sum_dstprt;
	std::map<uint16_t,uint32_t> sum_srcprt;
	uint32_t count;
}ipentry;

class DosHash {
	public:
		std::vector<ipentry*>** table;

		DosHash() { 
			table = new std::vector<ipentry*>*[DOSHASH_SIZE];
			for (int i = 0;i<DOSHASH_SIZE;i++)
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
	uint32_t clusterLifeTime;
	std::vector<DosCluster> clusters;
	public:
	BaseTCPDosDetect(); 
	virtual ~BaseTCPDosDetect();

	static uint8_t idToMask(uint16_t field);
	virtual int checkForAttack(const Packet* p,uint32_t*) = 0;

};

#endif
