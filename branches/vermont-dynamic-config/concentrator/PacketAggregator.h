/*
 * PacketAggregator
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
 *
 */

#ifndef PACKETAGGREGATOR_H_
#define PACKETAGGREGATOR_H_

#include "Rules.hpp"
#include "reconf/Module.h"
#include "reconf/Source.h"
#include "reconf/Destination.h"
#include "common/StatisticsManager.h"


#include <pthread.h>


/**
 * does the same as IpfixAggregator, only faster and only for raw packets
 * (IpfixAggregator is mainly used for aggregation of IPFIX flows)
 * inherits functionality of ExpressAggregator
 */
class PacketAggregator : public Module, public StatisticsModule, public Destination<Packet*>, public Source<IpfixRecord>
{
public:
	PacketAggregator();
	virtual ~PacketAggregator();

	void buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime);

	void start();
	void stop();
	
	virtual void receive(Packet* e);

	
protected:
	Rules* rules; /**< Set of rules that define the aggregator */
	pthread_mutex_t mutex; /**< Mutex to synchronize and/or pause aggregator */
};

#endif /*PACKETAGGREGATOR_H_*/
