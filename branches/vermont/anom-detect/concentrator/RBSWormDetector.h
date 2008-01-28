/*
 * VERMONT 
 * Copyright (C) 2008 David Eckhoff <sidaeckh@informatik.stud.uni-erlangen.de>
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

#if !defined(RBSWORMDETECTOR_H)
#define RBSWORMDETECTOR_H

#include "idmef/IDMEFExporter.h"
#include "IpfixRecordDestination.h"
#include "Connection.h"
#include "reconf/Source.h"

#include <list>
#include <string>

using namespace std;

class RBSWormDetector
	: public Module, 
	  public IpfixRecordDestination, 
	  public Source<IDMEFMessage*>
{
	public:
		RBSWormDetector(uint32_t hashbits, uint32_t texppend, uint32_t texpworm, 
				uint32_t texpben, uint32_t tadaptint,uint32_t tcleanupint, float lambda_ratio, string analyzerid, string idmeftemplate);
		virtual ~RBSWormDetector();
		
		virtual void onDataDataRecord(IpfixDataDataRecord* record);

	private:
		enum RBSDecision { PENDING, WORM, BENIGN };
		struct RBSEntry {
			uint32_t srcIP;
			uint32_t dstSubnet;
			uint32_t dstSubnetMask;
			uint32_t numFanouts;
			uint32_t startTime;
			int32_t timeExpire;
			list<uint32_t> accessedHosts;
			RBSDecision decision;
		};

		uint32_t hashSize;
		uint32_t hashBits;	/**< amount of bits used for hashtable */
		uint32_t timeExpirePending; // time in seconds until pending entries are expired
		uint32_t timeExpireWorm; // time in seconds until worm entries are expired
		uint32_t timeExpireBenign; // time in seconds until benign entries are expired
		uint32_t timeCleanupInterval; // time in seconds of interval when hashtable with source hosts is cleaned up (rbsEntries)
		uint32_t timeAdaptInterval; // time in seconds of interval when lamdbas (expected benign & worm frequencies) are changed
		string analyzerId;	/**< analyzer id for IDMEF messages */
		string idmefTemplate;	/**< template file for IDMEF messages */


		// idmef parameters
		const static char* PAR_FAN_OUT; // = "FAN_OUT";
		

		list<RBSEntry*>* rbsEntries;
		uint32_t statEntriesAdded;
		uint32_t statEntriesRemoved;
		uint32_t statNumWorms;
		float lambda_0,lambda_1,lambda_ratio;
		float slope_0,slope_1;
		time_t lastCleanup,lastAdaption;
		
		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;

		RBSEntry* createEntry(Connection* conn);
		RBSEntry* getEntry(Connection* conn);
		void addConnection(Connection* conn);
		static bool comp_entries(RBSEntry*,RBSEntry*);
		virtual string getStatistics();
		void cleanupEntries();
		void adaptFrequencies();
};

#endif
