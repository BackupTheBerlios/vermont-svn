/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
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

#include "PacketAggregatorCfg.h"

PacketAggregatorCfg::PacketAggregatorCfg(XMLElement* elem)
	: CfgHelper<PacketAggregator, PacketAggregatorCfg>(elem, "packetAggregator"),
	AggregatorBaseCfg(elem)
{
}

PacketAggregatorCfg::~PacketAggregatorCfg()
{
	if (instance == NULL)
		delete rules;
}

PacketAggregatorCfg* PacketAggregatorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new PacketAggregatorCfg(elem);
}

PacketAggregator* PacketAggregatorCfg::createInstance()
{
	instance = new PacketAggregator(pollInterval);
	instance->buildAggregator(rules, minBufferTime, maxBufferTime, htableBits);

	return instance;
}

bool PacketAggregatorCfg::deriveFrom(PacketAggregatorCfg* old)
{
	return false;  // FIXME: implement it, to gain performance increase in reconnect
}