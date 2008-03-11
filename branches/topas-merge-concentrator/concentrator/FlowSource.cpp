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

#include "FlowSource.hpp"

namespace VERMONT {

FlowSource::FlowSource() {
}

FlowSource::~FlowSource() {
}

void FlowSource::addFlowSink(FlowSink* flowSink) {
	flowSinks.push_back(flowSink);
}

void FlowSource::push(boost::shared_ptr<IpfixRecord> ipfixRecord) {
	for (FlowSinks::iterator i = flowSinks.begin(); i != flowSinks.end(); i++) {
		(*i)->push(ipfixRecord);
	}
}

};
