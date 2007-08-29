/*
 * VERMONT 
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#if !defined(TRWPORTSCANDETECTOR_H)
#define TRWPORTSCANDETECTOR_H

#include "ConnectionReceiver.h"
#include "common/StatisticsManager.h"
#include "idmef/IDMEFExporter.h"

#include <list>

using namespace std;

class TRWPortscanDetector : public ConnectionReceiver, StatisticsModule
{
	public:
		TRWPortscanDetector(IDMEFExporter* idmefexporter);
		virtual ~TRWPortscanDetector();
		virtual void push(Connection* conn);

	private:
		enum TRWDecision { PENDING, SCANNER, BENIGN };
		struct TRWEntry {
			uint32_t srcIP;
			uint32_t dstSubnet;
			uint32_t dstSubnetMask;
			uint32_t numFailedConns;
			uint32_t numSuccConns;
			int32_t timeExpire;
			list<uint32_t> accessedHosts;
			float S_N;
			TRWDecision decision;
		};

		// some configuration settings
		// TODO: this has to be done in configuration file
		const static int HASH_BITS = 20;
		const static int HASH_SIZE = 1<<(HASH_BITS-1);
		const static int TIME_EXPIRE_PENDING = 60*60*24; // time in seconds until pending entries are expired
		const static int TIME_EXPIRE_SCANNER = 60*30; // time in seconds until scanner entries are expired
		const static int TIME_EXPIRE_BENIGN = 60*30; // time in seconds until benign entries are expired
		const static int TIME_CLEANUP_INTERVAL = 10; // time in seconds of interval when hashtable with source hosts is cleaned up (trwEntries)

		// idmef parameters
		const static char* PAR_SUCC_CONNS; // = "SUCC_CONNS";
		const static char* PAR_FAILED_CONNS; // = "FAILED_CONNS";

		list<TRWEntry*> trwEntries[HASH_SIZE];
		uint32_t statEntriesAdded;
		uint32_t statEntriesRemoved;
		uint32_t statNumScanners;
		float logeta_0, logeta_1;
		float X_0, X_1;
		time_t lastCleanup;
		IDMEFExporter* idmefExporter;

		TRWEntry* createEntry(Connection* conn);
		TRWEntry* getEntry(Connection* conn);
		void addConnection(Connection* conn);
		virtual string getStatistics();
		void cleanupEntries();
};

#endif
