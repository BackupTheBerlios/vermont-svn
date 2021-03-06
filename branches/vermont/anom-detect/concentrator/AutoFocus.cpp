
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

#include "AutoFocus.h"
#include "crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>
#include "autofocus_iprecord.h"


InstanceManager<IDMEFMessage> AutoFocus::idmefManager("IDMEFMessage");

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortWormDetector
 * does not dare to delete it, in case it's used
 */
	AutoFocus::AutoFocus(uint32_t hashbits, uint32_t ttreeint, uint32_t nummaxr, uint32_t numtrees, uint32_t subbits,string analyzerid, string idmeftemplate)
: hashBits(hashbits),
	timeTreeInterval(ttreeint),
	numMaxResults(nummaxr),
	numTrees(numtrees),
	analyzerId(analyzerid),
	idmefTemplate(idmeftemplate),
	m_treeRecords(numtrees,NULL),
	m_minSubbits(subbits)

{
	hashSize = 1<<hashBits;
	lastTreeBuilt = time(0);
	statEntriesAdded = 0;

	m_treeCount = 0;
	m_treeRecords.clear();



	listIPRecords = new list<IPRecord*>[hashSize];
	initiateRecord(m_treeCount % numTrees);
	msg(MSG_INFO,"AutoFocus started");
}

AutoFocus::~AutoFocus()
{
	for (uint32_t i=0; i<hashSize; i++) {
		if (listIPRecords[i].size()==0) continue;

		list<IPRecord*>::iterator iter = listIPRecords[i].begin();
		while (iter != listIPRecords[i].end()) {
			std::map<report_enum,af_attribute*>::iterator iter2 = (*iter)->m_attributes.begin();

			while (iter2 != (*iter)->m_attributes.end())	
			{
				delete (iter2->second);	
				iter2++;
			}
			delete *iter;
			iter++;
		}
		listIPRecords[i].clear();
	}

	delete[] listIPRecords;

	for (int i = 0; i < numTrees;i++)
	{
		if (m_treeRecords[i] != NULL)
			deleteRecord(i);

	}
	msg(MSG_FATAL,"Autofocus is done");
}

void AutoFocus::onDataDataRecord(IpfixDataDataRecord* record)
{
	// convert ipfixrecord to connection struct
	Connection conn(record);


	uint32_t subnet = ntohl(2210136064UL);
	uint32_t mask = ntohl(4294901760UL);


	if ((conn.srcIP & mask) != subnet) 
	{
		conn.srcIP = conn.dstIP;
	}

	if ((conn.srcIP & mask) == subnet) 
	{
		addConnection(&conn);
	}

	record->removeReference();
}


void AutoFocus::addConnection(Connection* conn)
{
	IPRecord* te = getEntry(conn);

	std::map<report_enum,af_attribute*>::iterator iter = te->m_attributes.begin();

	while (iter != te->m_attributes.end())
	{
		(iter->second)->aggregate(te,conn);
		iter++;
	}

}

/**
 * returns entry in hashtable for the given connection
 * if it was not found, a new entry is created and returned
 */
IPRecord* AutoFocus::getEntry(Connection* conn)
{
	time_t curtime = time(0);
	uint32_t hash = crc32(0, 4, reinterpret_cast<char*>(&conn->srcIP)) & (hashSize-1);


	if (lastTreeBuilt+timeTreeInterval < (uint32_t) curtime) 
	{
		lastTreeBuilt = curtime;
		buildTree();

	}


	list<IPRecord*>::iterator iter = listIPRecords[hash].begin();
	while (iter != listIPRecords[hash].end()) {
		if ((*iter)->subnetIP == conn->srcIP) {
			// found the entry
			return *iter;
		}
		iter++;
	}

	// no entry found, create a new one
	IPRecord* te = createEntry(conn);
	listIPRecords[hash].push_back(te);

	return te;
}

/**
 * creates a new iprecord and sets status to pending
 */

void AutoFocus::deleteTree(treeNode* root) 
{

	if (root->left != NULL)
		deleteTree(root->left);

	if (root->right != NULL)
		deleteTree(root->right);

	std::map<report_enum,af_attribute*>::iterator iter = root->data.m_attributes.begin();

	while (iter != root->data.m_attributes.end())	
	{
		delete (iter->second);	
		iter++;
	}
	delete root;
	return;
}

IPRecord* AutoFocus::createEntry(Connection* conn)

{
	IPRecord* te = new IPRecord();
	te->subnetIP = conn->srcIP;
	te->subnetBits = 32;

	std::list<report*>::iterator iter  = m_treeRecords[m_treeCount % numTrees]->reports.begin();

	while (iter != m_treeRecords[m_treeCount % numTrees]->reports.end())
	{
		te->m_attributes[(*iter)->getID()] = (*iter)->createaf_attribute();
		iter++;
	}
	statEntriesAdded++;
	return te;
}


void AutoFocus::deleteRecord(int index)
{
	msg(MSG_FATAL,"Deleting Index %d",index);

	if (m_treeRecords[index]->root != NULL)
		deleteTree(m_treeRecords[index]->root);	

	std::list<report*>::iterator iter = m_treeRecords[index]->reports.begin();
	while (iter != m_treeRecords[index]->reports.end())
	{
		delete *iter;
		iter++;
	}

	delete m_treeRecords[index];
}



void AutoFocus::evaluate()
{

	uint32_t index = (m_treeCount-1) % numTrees;


	//msg(MSG_FATAL,"evaluating index %d",index);
	treeRecord* currentTree = m_treeRecords[index];
	treeRecord* last_tree = m_treeRecords[(index+numTrees-1) %numTrees];


	std::list<report*>::iterator iter = currentTree->reports.begin();

	while (iter != currentTree->reports.end())
	{
		(*iter)->post(m_treeRecords,index);
		iter++;
	}
	metalist();

}

void AutoFocus::metalist() 
{

	if (m_treeCount-1 < 1) 
	{
		msg(MSG_FATAL,"meta list skipped, waiting for valuable data");
		return;

	}
	uint32_t index = (m_treeCount-1) % numTrees;

	std::list<treeNode*> meta; 
	std::list<report*>::iterator iter = m_treeRecords[index]->reports.begin();

	while (iter != m_treeRecords[index]->reports.end())
	{
		std::list<treeNode*>::iterator specit = (*iter)->specNodes.begin();

		while (specit != (*iter)->specNodes.end())
		{

			if (find(meta.begin(),meta.end(),*specit) == meta.end())
			{
				meta.push_back(*specit);
			}
			specit++;

		}
		iter++;
	}

	meta.sort(AutoFocus::metasort);
	std::cerr << "meta size " << meta.size() << endl;

	std::list<treeNode*>::iterator metait = meta.begin();

	char num[50];

	while (metait != meta.end())
	{

		std::list<report*>::iterator iter = m_treeRecords[index]->reports.begin();


		msg(MSG_FATAL,"----");
		msg(MSG_FATAL,"SUBNET %s/%d\t\tPriority Value %d",IPToString((*metait)->data.subnetIP).c_str(),(*metait)->data.subnetBits,(*metait)->prio);

		std::string output;
		uint64_t data;
		double percentage;
		double change;	
		double change_global;

		while (iter != m_treeRecords[index]->reports.end())
		{
		change_global = (double) ((*iter)->numTotal * 100) / (double) m_treeRecords[(index - 1 + m_treeRecords.capacity()) % m_treeRecords.capacity()]->root->data.m_attributes[(*iter)->getID()]->numCount - 100.0;	
			data = (*metait)->data.m_attributes[(*iter)->getID()]->numCount;
			std::string locl;
			locl.append("\n");
			sprintf(num,"%25s",(*iter)->global.c_str());
			locl.append(num);
			locl.append("\t");
			sprintf(num,"%10llu\0",data);
			locl.append(num);
			locl.append(" \t");
			percentage = (double) (data*100) / (double) (*iter)->numTotal;
			sprintf(num,"%7.2f%%\0",percentage);
			locl.append(" ");
			locl.append(num);
			
			treeNode* before = (*iter)->getComparismValue(*metait,m_treeRecords,index);
			change = (double) (data*100) / (double) before->data.m_attributes[(*iter)->getID()]->numCount - 100.0;

			locl.append("\tChange: Absolute: ");
			sprintf(num,"%7.2f\0",change);
			locl.append(num);
			locl.append("\tRelative: ");
			sprintf(num,"%7.2f\0",change_global - change);
		
			locl.append(num);
	
			double percentage2 = (double) ((*metait)->data.m_attributes[(*iter)->getID()]->delta * 100) / (double) (*iter)->numTotal;
			if (find((*iter)->specNodes.begin(),(*iter)->specNodes.end(),*metait) != (*iter)->specNodes.end()) 
			{
			locl.append("\t<-------");
			}
			output.append(locl);
			iter++;
		}
		msg(MSG_FATAL,"%s",output.c_str());
		metait++;

	}

}

void AutoFocus::cleanUp()
{

	//atrributes are not deleted here, because the data is stll linked in the treeNodes
	for (uint32_t i=0; i<hashSize; i++) {
		if (listIPRecords[i].size()==0) continue;

		list<IPRecord*>::iterator iter = listIPRecords[i].begin();
		while (iter != listIPRecords[i].end()) {
			delete *iter;
			iter++;
		}
		listIPRecords[i].clear();
	}
	statEntriesAdded = 0;	

}

uint32_t AutoFocus::distance(treeNode* a,treeNode* b) 
{
	return ntohl(a->data.subnetIP) ^ ntohl(b->data.subnetIP);
}

void AutoFocus::initiateRecord(int index) 
{
	if (m_treeRecords[index % numTrees ] != NULL)
		deleteRecord(index % numTrees);

	m_treeRecords[index % numTrees] = new treeRecord;
	m_treeRecords[index % numTrees]->root = NULL;
	m_treeRecords[index % numTrees]->reports.push_back(new rep_payload_tcp(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_payload_udp(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_fanouts(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_fanins(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_packets_tcp(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_packets_udp(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_failed(m_minSubbits));
	m_treeRecords[index % numTrees]->reports.push_back(new rep_simult(m_minSubbits));


}
void AutoFocus::buildTree () 
{
	msg(MSG_FATAL,"STARTING TREE BUILDING");
	treeNode* root;
	list<treeNode*> tree;

	treeRecord* curTreeRecord = m_treeRecords[m_treeCount % numTrees];


	for (uint32_t i=0; i<hashSize; i++) {
		if (listIPRecords[i].size()==0) continue;

		list<IPRecord*>::iterator iter = listIPRecords[i].begin();

		while (iter != listIPRecords[i].end()) 
		{
			treeNode* entry = new treeNode;
			entry->data.subnetIP = (*iter)->subnetIP;
			entry->data.subnetBits = (*iter)->subnetBits;
			entry->left = NULL;
			entry->right = NULL;
			entry->prio = 0;
			entry->data.m_attributes = (*iter)->m_attributes;


			std::map<report_enum,af_attribute*>::iterator iter2 = (*iter)->m_attributes.begin();

			while (iter2 != (*iter)->m_attributes.end())
			{
				(iter2->second)->delta = (iter2->second)->numCount;
				iter2++;
			}
			/*
			   std::map<report_enum,af_attribute*>::iterator iter2 = (*iter)->m_attributes.begin();

			   while (iter2 != (*iter)->m_attributes.end())
			   {

			   entry->data.m_attributes[iter2->first] = (iter2->second)->getCopy();
			   iter2++;
			   }
			   */
			tree.push_back(entry);
			iter++;	
		}
	}

	msg(MSG_FATAL,"converted iprecords to treenodes");

	tree.sort(AutoFocus::comp_entries);

	msg(MSG_FATAL,"everything is sorted %d",tree.size());
	//* BUILD TREE *//

	list<treeNode*>::iterator iter = tree.begin();
	int count = 0;

	while (iter != tree.end())
	{


		check_node(curTreeRecord,*iter);

		if (*iter == tree.back()) { iter++; continue; }

		uint32_t a = distance(*iter,*(iter++));
		uint32_t b;
		count++;

		if (*iter != tree.back())
		{
			b=distance(*iter,*(iter++));
			iter--;		
		}
		else
		{
			b = (uint32_t) pow(2,32)-1;
		}
		count++;
		iter--;

		if (a<=b)
		{

			treeNode* newnode = new treeNode;
			newnode->prio = 0;
			tree.insert(iter,newnode);
			newnode->left = *iter;

			uint32_t ip1 = ntohl(((*iter)->data).subnetIP);

			iter = tree.erase(iter);

			newnode->right = *iter;

			check_node(curTreeRecord,*iter);

			uint32_t ip2 = ntohl(((*iter)->data).subnetIP);

			aggregate_newnode(newnode);


			iter = tree.erase(iter);

			int subbits = (uint32_t) (round(log(a)/log(2)+0.5));

			newnode->data.subnetIP = htonl((ip1>>subbits)<<subbits);
			newnode->data.subnetBits = 32 - subbits;

			check_node(curTreeRecord,newnode);

			iter--;
			if (*iter != tree.front()) iter--;
		}
		else
			iter++;



	}

	curTreeRecord->root = tree.front();

	m_treeCount++;



	evaluate();
	initiateRecord(m_treeCount % numTrees);
	cleanUp();

}

void AutoFocus::aggregate_newnode(treeNode* newnode)
{

	std::list<report*>::iterator iter  = m_treeRecords[m_treeCount % numTrees]->reports.begin();

	while (iter != m_treeRecords[m_treeCount % numTrees]->reports.end())
	{
		newnode->data.m_attributes[(*iter)->getID()] = (*iter)->createaf_attribute();
		newnode->data.m_attributes[(*iter)->getID()]->collect(newnode->left->data.m_attributes[(*iter)->getID()],newnode->right->data.m_attributes[(*iter)->getID()]);
		iter++;
	}
}

void AutoFocus::check_node(treeRecord* curTreeRecord,treeNode* newnode)
{

	std::list<report*>::iterator iter = curTreeRecord->reports.begin();

	while (iter != curTreeRecord->reports.end())
	{
		(*iter)->checkNode(newnode,numMaxResults);
		iter++;
	}

}
/*
 * compare fuction to sort entry list
 * 
 */
bool AutoFocus::comp_entries(treeNode* a,treeNode* b) {
	return ntohl(a->data.subnetIP) < ntohl(b->data.subnetIP);
}

bool AutoFocus::metasort(treeNode* a,treeNode* b)
{
	return a->prio > b->prio;

}
string AutoFocus::getStatistics()
{
	ostringstream oss;
	return oss.str();
}

std::string AutoFocus::getStatisticsXML()
{
	ostringstream oss;
	oss << "<AutoFocus>" << endl;
	oss << "<entrycount>" << statEntriesAdded  << "</entrycount>" << endl;
	oss << "<nexttreein>" << timeTreeInterval - (time(0) - lastTreeBuilt)   << "</nexttreein>" << endl;
	oss << "</AutoFocus>" << endl;
	return oss.str();
}


