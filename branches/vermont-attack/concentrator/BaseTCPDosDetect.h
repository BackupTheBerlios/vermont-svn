#ifndef __class__BASETCPDOSDETECT__
#define __class__BASETCPDOSDETECT__
#include "ipfix.hpp"
#include <iostream>
#include <list>

typedef struct pEntry {
	uint32_t srcip;
	uint32_t dstip;
	uint16_t srcprt;
	uint16_t dstprt;
}pEntry;

typedef struct ipentry {
	uint32_t ip;
	std::list<pEntry> entries;
	uint32_t count;
}ipentry;
class DosHash {
	public:
		std::list<ipentry*>** table;

		DosHash() { 
			table = new std::list<ipentry*>*[2048];
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

	DosHash* HashIncoming;
	DosHash* HashOutgoing;

	public:
	BaseTCPDosDetect() { };
	virtual ~BaseTCPDosDetect() { 
		HashIncoming = NULL;
		HashOutgoing = NULL;
	};

	virtual int checkForAttack(const Packet* p) = 0;

};

#endif
