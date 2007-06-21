/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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
 *
 */

#include "msg.h"
#include "FlowSink.hpp"

FlowSink::FlowSink() : thread(flowSinkProcess), exitFlag(false) {
}

FlowSink::~FlowSink() {
	msg(MSG_DEBUG, "FlowSink: destructor called");
	terminateSink();
}

void FlowSink::push(boost::shared_ptr<IpfixRecord> ipfixRecord) {
	ipfixRecords.push(ipfixRecord);
}

void* FlowSink::flowSinkProcess(void* flowSink_) {
	FlowSink* flowSink = (FlowSink*)flowSink_;
	flowSink->flowSinkProcess();
	return 0;
}

void FlowSink::flowSinkProcess()
{
	msg(MSG_INFO, "Sink: now running FlowSink thread");
	while(!exitFlag) {
		boost::shared_ptr<IpfixRecord> ipfixRecord;
		if (!ipfixRecords.pop(1000, &ipfixRecord)) continue;
		{
			IpfixDataRecord* rec = dynamic_cast<IpfixDataRecord*>(ipfixRecord.get());
			if (rec) onDataRecord(rec->sourceID.get(), rec->templateInfo.get(), rec->dataLength, rec->data);
		}
		{
			IpfixDataDataRecord* rec = dynamic_cast<IpfixDataDataRecord*>(ipfixRecord.get());
			if (rec) onDataDataRecord(rec->sourceID.get(), rec->dataTemplateInfo.get(), rec->dataLength, rec->data);
		}
		{
			IpfixOptionsRecord* rec = dynamic_cast<IpfixOptionsRecord*>(ipfixRecord.get());
			if (rec) onOptionsRecord(rec->sourceID.get(), rec->optionsTemplateInfo.get(), rec->dataLength, rec->data);
		}
		{
			IpfixTemplateRecord* rec = dynamic_cast<IpfixTemplateRecord*>(ipfixRecord.get());
			if (rec) onTemplate(rec->sourceID.get(), rec->templateInfo.get());
		}
		{
			IpfixDataTemplateRecord* rec = dynamic_cast<IpfixDataTemplateRecord*>(ipfixRecord.get());
			if (rec) onDataTemplate(rec->sourceID.get(), rec->dataTemplateInfo.get());
		}
		{
			IpfixOptionsTemplateRecord* rec = dynamic_cast<IpfixOptionsTemplateRecord*>(ipfixRecord.get());
			if (rec) onOptionsTemplate(rec->sourceID.get(), rec->optionsTemplateInfo.get());
		}
		{
			IpfixTemplateDestructionRecord* rec = dynamic_cast<IpfixTemplateDestructionRecord*>(ipfixRecord.get());
			if (rec) onTemplateDestruction(rec->sourceID.get(), rec->templateInfo.get());
		}
		{
			IpfixDataTemplateDestructionRecord* rec = dynamic_cast<IpfixDataTemplateDestructionRecord*>(ipfixRecord.get());
			if (rec) onDataTemplateDestruction(rec->sourceID.get(), rec->dataTemplateInfo.get());
		}
		{
			IpfixOptionsTemplateDestructionRecord* rec = dynamic_cast<IpfixOptionsTemplateDestructionRecord*>(ipfixRecord.get());
			if (rec) onOptionsTemplateDestruction(rec->sourceID.get(), rec->optionsTemplateInfo.get());
		}
	}
}

bool FlowSink::runSink() {
	return(thread.run(this));
}

bool FlowSink::terminateSink() {
	exitFlag = true;
	msg(MSG_DEBUG, "FlowSink: waiting for exporter thread");
	thread.join();
	msg(MSG_DEBUG, "FlowSink: exporter thread joined");
	return true;
}
