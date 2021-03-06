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

#if !defined(FLOWSIGMATCHER_H)
#define FLOWSIGMATCHER_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <list>
#include <set>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;


struct IpEntry{
        uint32_t ip;
        uint8_t mask;
        uint8_t notFlag;
};

struct PortEntry{
	uint16_t port;
	uint16_t portEnd;
        uint8_t notFlag;
};

struct PortListEntry{
	struct PortEntry* entry;
	struct GenNode* node;
};

struct IpListEntry{
	struct IpEntry* entry;
	struct GenNode* node;
};

struct IdsRule {
        uint8_t protocol;
	list<PortEntry*> sPort;
	list<PortEntry*> dPort;
        list<IpEntry*> src;
        list<IpEntry*> dst;
        uint32_t uid;
        uint8_t type;
        uint8_t source;
	uint8_t mode;
	uint32_t flag;
        string msg;
};

struct FlagsRule {
        set<uint32_t> flags;
        uint32_t uid;
        uint8_t type;
        uint8_t source;
        string msg;
};

class GenNode {
	public:
	enum GenType {
		proto,
		srcIP,
		dstIP,
		srcPort,
		dstPort,
		rule
	};
	static GenType order[6];
	static uint32_t treeElements;
	static GenNode* newNode(int depth);
	static void parse_order(string order);
	virtual void findRule(Connection* conn, set<IdsRule*>& rules)=0;
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules)=0;
	virtual void insertRule(IdsRule* rule,int depth)=0;
	virtual void insertRevRule(IdsRule* rule,int depth)=0;
	virtual ~GenNode() {};
};

class ProtoNode : public GenNode {
	GenNode* any;
	GenNode* udp;
	GenNode* tcp;
	GenNode* icmp;
	public:
	virtual void findRule(Connection* conn, set<IdsRule*>& rules);
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	virtual void insertRevRule(IdsRule* rule,int depth);
        ProtoNode();
	~ProtoNode();
};

class SrcIpNode : public GenNode {
	GenNode* any;
	map<uint32_t,GenNode*> ipmaps[4];
	map<uint32_t,GenNode*> notipmaps[4];
	list<IpListEntry*> iplist;
	list<IpListEntry*> notiplist;
	public:
	virtual void findRule(Connection* conn, set<IdsRule*>& rules);
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	virtual void insertRevRule(IdsRule* rule,int depth);
        SrcIpNode();
	~SrcIpNode();
};

class DstIpNode : public GenNode {
	GenNode* any;
	map<uint32_t,GenNode*> ipmaps[4];
	map<uint32_t,GenNode*> notipmaps[4];
	list<IpListEntry*> iplist;
	list<IpListEntry*> notiplist;
	public:
	virtual void findRule(Connection* conn, set<IdsRule*>& rules);
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	virtual void insertRevRule(IdsRule* rule,int depth);
        DstIpNode();
	~DstIpNode();
};

class SrcPortNode : public GenNode {
	GenNode* any;
	map<uint16_t,GenNode*> portmap;
	map<uint16_t,GenNode*> notportmap;
	list<PortListEntry*> portlist;
	list<PortListEntry*> notportlist;
	public:
	virtual void findRule(Connection* conn, set<IdsRule*>& rules);
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	virtual void insertRevRule(IdsRule* rule,int depth);
        SrcPortNode();
	~SrcPortNode();
};

class DstPortNode : public GenNode {
	GenNode* any;
	map<uint16_t,GenNode*> portmap;
	map<uint16_t,GenNode*> notportmap;
	list<PortListEntry*> portlist;
	list<PortListEntry*> notportlist;
	public:
	virtual void findRule(Connection* conn, set<IdsRule*>& rules);
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	virtual void insertRevRule(IdsRule* rule,int depth);
        DstPortNode();
        ~DstPortNode();
};

class RuleNode : public GenNode {
	list<IdsRule*> rulesList;
	public:
	virtual void findRule(Connection* conn, set<IdsRule*>& rules);
	virtual void invalidateRule(Connection* conn, set<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	virtual void insertRevRule(IdsRule* rule,int depth);
	~RuleNode();
};

class FlowSigMatcher
	: public Module,
	  public IpfixRecordDestination,
	  public Source<IDMEFMessage*>
{
	public:
		FlowSigMatcher(string homenet, string rulesfile, string rulesorder, string analyzerid, string idmeftemplate, string flagstimeout);
		virtual ~FlowSigMatcher();

		virtual void onDataRecord(IpfixDataRecord* record);
		std::string getStatisticsXML(double);

	private:
                ifstream infile;
                string homenet;
                string rulesfile;
		string analyzerId;	/**< analyzer id for IDMEF messages */
		string idmefTemplate;	/**< template file for IDMEF messages */
		uint64_t flagsTimeout;
		uint64_t matchedRules;
		uint64_t statTotalRecvRecords;


		// idmef parameters
		const static char* PAR_SOURCE_PORT; // = "SOURCE_PORT";
		const static char* PAR_TARGET_PORT; // = "TARGET_PORT";
		const static char* PAR_SOURCE; // = "SOURCE";
		const static char* PAR_UID; // = "UID";
		const static char* PAR_TYPE; // = "TYPE";
		const static char* PAR_MSG; // = "MSM";
	
                list<IdsRule*> parsedRules;
                list<FlagsRule*> flagRules;
		map< uint32_t,map<uint32_t,uint64_t> > activeFlags;
                GenNode* treeRoot;
		vector<string> idsRuleType;
		vector<string> idsRuleSource;

		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;
		int parseFlags(string text, FlagsRule& rule);
                int parse_line(string text);
                int parse_port(string text, IdsRule& rule, uint32_t dst);
                void split_port(string text, list<PortEntry*>& list);
                void split_ip(string text, list<IpEntry*>& list);
                int parse_ip(string text, IdsRule& rule, uint32_t dst);
		uint8_t findVectorNr(string text, vector<string>& vec) ;
		void sendMessage(Connection& conn,uint8_t source, uint8_t type, uint32_t uid, string msg);
		/*void addConnection(Connection* conn);
		virtual string getStatisticsXML(double interval);
		void cleanupEntries();*/
};

#endif
