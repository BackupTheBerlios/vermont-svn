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

#include "FlowSigMatcher.h"
#include "common/crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>



InstanceManager<IDMEFMessage> FlowSigMatcher::idmefManager("TRWPortscanIDMEFMessage", 0);

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, FlowSigMatcher
 * does not dare to delete it, in case it's used
 */
FlowSigMatcher::FlowSigMatcher(string homenet, string rulesfile, string analyzerid, string idmeftemplate)
	: homenet(homenet),
	  rulesfile(rulesfile),
	  analyzerId(analyzerid),
	  idmefTemplate(idmeftemplate)
{
    char buffer[256];
    infile.exceptions ( ifstream::eofbit | ifstream::failbit | ifstream::badbit );
    try {
    	infile.open(rulesfile.c_str(),ifstream::in);
	while(!infile.eof()) {
            infile.getline(buffer,256);
            parse_line(buffer);
        }
    }
    catch (ifstream::failure e) {
	cout << "Exception opening/reading file";
    }
    infile.close();
    list<IdsRule*>::iterator it;
    treeRoot=GenNode::newGenNode(0);
    for(it=parsedRules.begin();it!=parsedRules.end();it++) {
        treeRoot->insertRule(*it,0);
    }
}

FlowSigMatcher::~FlowSigMatcher()
{
}

void FlowSigMatcher::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	/*if((record->templateInfo->setId != TemplateInfo::NetflowTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}*/

	// convert ipfixrecord to connection struct
	Connection conn(record);

	conn.swapIfNeeded();

	// only use this connection if it was a connection attempt
/*	if (conn.srcTcpControlBits&Connection::SYN) {
		addConnection(&conn);
	}*/
        list<IdsRule*> matchingRules;
        treeRoot->findRule(&conn,matchingRules);
	record->removeReference();
}


void ProtoNode::findRule(Connection* conn,list<IdsRule*>& rules) {
	if(conn->protocol==6) {
		if(tcp!=NULL)	tcp->findRule(conn,rules);
	}
	else if(conn->protocol==17) {
		if(udp!=NULL) udp->findRule(conn,rules);
	}
	else if(conn->protocol==1) {
		if(icmp!=NULL) icmp->findRule(conn,rules);
	}
}

void ProtoNode::insertRule(IdsRule* rule,int depth) {
	if(rule->protocol==6) { //TCP
		if(tcp==NULL) tcp=newGenNode(depth+1);
		tcp->insertRule(rule,depth+1);
	}
        else if(rule->protocol==17) { //UDP
		if(udp==NULL) udp=newGenNode(depth+1);
		udp->insertRule(rule,depth+1);
        }
        else if(rule->protocol==1) { //ICMP
		if(icmp==NULL) icmp=newGenNode(depth+1);
		icmp->insertRule(rule,depth+1);
	}
}

ProtoNode::~ProtoNode() {
	if(tcp!=NULL) delete tcp;
	if(udp!=NULL) delete udp;
	if(icmp!=NULL) delete icmp;
}

void SrcPortNode::findRule(Connection* conn,list<IdsRule*>& rules) {
	if(any!=NULL) any->findRule(conn,rules);
	map<uint16_t,GenNode*>::iterator it;
	it=portmap.find(ntohs(conn->srcPort));
	if(it!=portmap.end()) it->second->findRule(conn,rules);
}

void SrcPortNode::insertRule(IdsRule* rule,int depth) {
	if(rule->srcPort==0) {
		if(any==NULL) any=newGenNode(depth+1);
		any->insertRule(rule,depth+1);
	}
        else if(rule->srcPortEnd!=0&&(rule->srcPort<=rule->srcPortEnd)) {
            for(uint16_t port=rule->srcPort;port<=rule->srcPortEnd;port++) {
                map<uint16_t,GenNode*>::iterator it;
		it=portmap.find(port);
		if(it==portmap.end()) portmap[port]=newGenNode(depth+1);
		portmap[port]->insertRule(rule,depth+1);
            }
        }
	else {
		map<uint16_t,GenNode*>::iterator it;
		it=portmap.find(rule->srcPort);
		if(it==portmap.end()) portmap[rule->srcPort]=newGenNode(depth+1);
		portmap[rule->srcPort]->insertRule(rule,depth+1);
	}
}

SrcPortNode::~SrcPortNode() {
	if(any!=NULL) delete any;
	map<uint16_t,GenNode*>::iterator it;
	for(it=portmap.begin();it!=portmap.end();it++) {
		if(it->second!=NULL)	delete it->second;
	}

}

void DstPortNode::findRule(Connection* conn,list<IdsRule*>& rules) {
	if(any!=NULL) any->findRule(conn,rules);
	map<uint16_t,GenNode*>::iterator it;
	it=portmap.find(ntohs(conn->dstPort));
	if(it!=portmap.end()) it->second->findRule(conn,rules);
}

void DstPortNode::insertRule(IdsRule* rule,int depth) {
	if(rule->dstPort==0) {
		if(any==NULL) any=newGenNode(depth+1);
		any->insertRule(rule,depth+1);
	}
        else if(rule->dstPortEnd!=0&&(rule->dstPort<=rule->dstPortEnd)) {
            for(uint16_t port=rule->dstPort;port<=rule->dstPortEnd;port++) {
                map<uint16_t,GenNode*>::iterator it;
		it=portmap.find(port);
		if(it==portmap.end()) portmap[port]=newGenNode(depth+1);
		portmap[port]->insertRule(rule,depth+1);
            }

        }
	else {
		map<uint16_t,GenNode*>::iterator it;
		it=portmap.find(rule->dstPort);
		if(it==portmap.end()) portmap[rule->dstPort]=newGenNode(depth+1);
		portmap[rule->dstPort]->insertRule(rule,depth+1);
	}
}

DstPortNode::~DstPortNode() {
	if(any!=NULL) delete any;
	map<uint16_t,GenNode*>::iterator it;
	for(it=portmap.begin();it!=portmap.end();it++) {
		if(it->second!=NULL)	delete it->second;
	}
}

void SrcIpNode::findRule(Connection* conn,list<IdsRule*>& rules) {
	map<uint32_t,GenNode*>::iterator it;
	for(int i=0;i<4;i++) {
		it=ipmaps[i].find(ntohl(conn->srcIP)>>(24-i*8));
		if(it!=ipmaps[i].end()) it->second->findRule(conn,rules);
	}
}

void SrcIpNode::insertRule(IdsRule* rule,int depth) {
        list<ipEntry*>::iterator listit;
        for(listit=rule->src.begin();listit!=rule->src.end();listit++) {
            if((*listit)->mask==0) {
                    if(any==NULL) any=newGenNode(depth+1);
                    any->insertRule(rule,depth+1);
            }
            else {
                    map<uint32_t,GenNode*>::iterator it;
                    int fact=((*listit)->mask-1)/8;
                    int mod=(*listit)->mask%8;
                    if(mod==0) {
                            it=ipmaps[fact].find((*listit)->ip>>(24-fact*8));
                            if(it==ipmaps[fact].end()) ipmaps[fact][(*listit)->ip>>(24-fact*8)]=newGenNode(depth+1);
                            ipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRule(rule,depth+1);
                    }
                    else {
                            for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
                                    it=ipmaps[fact].find(i);
                                    if(it==ipmaps[fact].end()) ipmaps[fact][i]=newGenNode(depth+1);
                                    ipmaps[fact][i]->insertRule(rule,depth+1);
                            }
                    }
            }
        }
}

SrcIpNode::~SrcIpNode() {
	if(any!=NULL) delete any;
	map<uint32_t,GenNode*>::iterator it;
	for(int i=0;i<4;i++) {
		for(it=ipmaps[i].begin();it!=ipmaps[i].end();it++) {
			if(it->second!=NULL)	delete it->second;
		}
	}
}

void DstIpNode::findRule(Connection* conn,list<IdsRule*>& rules) {
	map<uint32_t,GenNode*>::iterator it;
	for(int i=0;i<4;i++) {
		it=ipmaps[i].find(ntohl(conn->dstIP)>>(24-i*8));
		if(it!=ipmaps[i].end()) it->second->findRule(conn,rules);
	}
}

void DstIpNode::insertRule(IdsRule* rule,int depth) {
        list<ipEntry*>::iterator listit;
        for(listit=rule->dst.begin();listit!=rule->dst.end();listit++) {
            if((*listit)->mask==0) {
                    if(any==NULL) any=newGenNode(depth+1);
                    any->insertRule(rule,depth+1);
            }
            else {
                    map<uint32_t,GenNode*>::iterator it;
                    int fact=((*listit)->mask-1)/8;
                    int mod=(*listit)->mask%8;
                    if(mod==0) {
                            it=ipmaps[fact].find((*listit)->ip>>(24-fact*8));
                            if(it==ipmaps[fact].end()) ipmaps[fact][(*listit)->ip>>(24-fact*8)]=newGenNode(depth+1);
                            ipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRule(rule,depth+1);
                    }
                    else {
                            for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
                                    it=ipmaps[fact].find(i);
                                    if(it==ipmaps[fact].end()) ipmaps[fact][i]=newGenNode(depth+1);
                                    ipmaps[fact][i]->insertRule(rule,depth+1);
                            }
                    }
            }
        }
}

DstIpNode::~DstIpNode() {
	if(any!=NULL) delete any;
	map<uint32_t,GenNode*>::iterator it;
	for(int i=0;i<4;i++) {
		for(it=ipmaps[i].begin();it!=ipmaps[i].end();it++) {
			if(it->second!=NULL)	delete it->second;
		}
	}
}

void RuleNode::insertRule(IdsRule* rule,int depth) {
	rulesList.push_back(rule);
}

void RuleNode::findRule(Connection* conn,list<IdsRule*>& rules) {
	list<IdsRule*>::iterator it;
	for(it=rulesList.begin();it!=rulesList.end();it++) {
		rules.push_back(*it);
	}
}

RuleNode::~RuleNode() {}

GenNode* GenNode::newGenNode(int depth) {
	if(depth>=5) return new RuleNode;
        else if(order[depth]==0) return new ProtoNode;
	else if(order[depth]==1) return new SrcIpNode;
	else if(order[depth]==2) return new DstIpNode;
	else if(order[depth]==3) return new SrcPortNode;
	else return new DstPortNode;
}

uint16_t GenNode::order[5]={0,1,2,3,4};

int FlowSigMatcher::parse_line(string text) {
  boost::cmatch what;
  const boost::regex exp_line("(\\d+) +((?:\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(?:/\\d+)*)|(?:\\[.+\\])) +(\\d+|any|ANY|\\*|(?:\\d+\\:\\d+)) +(->|<>) +((?:\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(?:/\\d+)*)|(?:\\[.+\\])) +(\\d+|any|ANY|\\*|(?:\\d+\\:\\d+)) +(\\w+) +([\\w\\-]+) +([\\w\\-]+) +\"(.*)\"");
  if(!boost::regex_match(text.c_str(), what, exp_line)) return false;
  IdsRule* rule=new IdsRule;
  parsedRules.push_back(rule);
    // what[0] contains the whole string
    // what[1] contains the response code
    // what[2] contains the separator character
    // what[3] contains the text message.
    rule->sid=atoi(static_cast<string>(what[1]).c_str());
    parse_ip(what[2], *rule,0);
    parse_port(what[3],*rule,0);
    parse_ip(what[5], *rule,1);
    parse_port(what[6],*rule,1);
    string tcp("TCP");
    string udp("UDP");
    string icmp("ICMP");
    if(tcp.compare(what[7])==0) rule->protocol=6;
    else if(udp.compare(what[7])==0) rule->protocol=17;
    else if(icmp.compare(what[7])==0) rule->protocol=1;
    rule->type=what[8];
    rule->source=what[9];
    rule->msg=what[10];
	return true;
}

int FlowSigMatcher::parse_port(string text, IdsRule& rule, uint32_t dst) {
  boost::cmatch what;
  const boost::regex exp_port("(\\d+)\\:(\\d+)");
  if(boost::regex_match(text.c_str(), what, exp_port)) {
    if(dst==0) {
      rule.srcPort=atoi(static_cast<string>(what[1]).c_str());
      rule.srcPortEnd=atoi(static_cast<string>(what[2]).c_str());
    }
    else {
      rule.dstPort=atoi(static_cast<string>(what[1]).c_str());
      rule.dstPortEnd=atoi(static_cast<string>(what[2]).c_str());
    }
  }
  else {
    if(dst==0) {
      rule.srcPort=atoi(text.c_str());
      if((text.compare("*")==0)||(text.compare("any")==0)||(text.compare("ANY")==0)) rule.srcPort=0;
      rule.srcPortEnd=0;
    }
    else {
      rule.dstPort=atoi(text.c_str());
      if((text.compare("*")==0)||(text.compare("any")==0)||(text.compare("ANY")==0)) rule.dstPort=0;
      rule.dstPortEnd=0;
    }
  }
  return true;
}

int FlowSigMatcher::parse_ip(string text, IdsRule& rule, uint32_t dst) {
  const boost::regex exp_braces("\\[.+\\]");
  if(boost::regex_match(text,exp_braces)) {
	  text.erase(text.begin());
	  text.erase(text.end()-1);
	  const boost::regex expip(", *");
	  boost::sregex_token_iterator i(text.begin(), text.end(), expip, -1);
	  boost::sregex_token_iterator j;
	  if(dst==0)  while(i!=j) split_ip(*i++,rule.src);
	  else while(i!=j) split_ip(*i++,rule.dst);
  }
  else {
	if(dst==0) split_ip(text,rule.src);
	else split_ip(text,rule.dst);
  }
  return true;
}

void FlowSigMatcher::split_ip(string text, list<ipEntry*>& list) {
	boost::cmatch what;
	const boost::regex exp_split_ip("(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})(?:/(\\d+))*");
	boost::regex_match(text.c_str(),what,exp_split_ip);
	ipEntry* entry=new ipEntry;
	string emptymask="";
	if(emptymask.compare(what[5])==0) entry->mask=32;
	else entry->mask=atoi(static_cast<string>(what[5]).c_str());
	entry->ip=(atoi(static_cast<string>(what[1]).c_str())<<24)|(atoi(static_cast<string>(what[2]).c_str())<<16)|(atoi(static_cast<string>(what[3]).c_str())<<8)|(atoi(static_cast<string>(what[4]).c_str()));
	list.push_back(entry);
}