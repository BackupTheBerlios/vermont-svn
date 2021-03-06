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

#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <stdint.h>
#include <string>

#include "common/ManagedInstance.h"
#include "IpfixRecord.hpp"
#include "common/Time.h"

using namespace std;


class Connection
{
	public:
		static const uint8_t FIN = 0x01;
		static const uint8_t SYN = 0x02;
		static const uint8_t RST = 0x04;
		static const uint8_t ACK = 0x10;

		// ATTENTION: next four elements MUST be declared sequentially without another element interrupting it
		// because hash and compare is performed by accessing the memory directly from srcIP on
		// (see function calcHash and compareTo)
		// one more notice: additional fields are to be added to function swapDataFields!
		uint32_t srcIP;
		uint32_t dstIP;
		uint16_t srcPort;
		uint16_t dstPort;

		// fields to be aggregated
		uint64_t srcTimeStart; /**< milliseconds since 1970 */
		uint64_t srcTimeEnd; /**< milliseconds since 1970 */
		uint64_t dstTimeStart; /**< milliseconds since 1970 */
		uint64_t dstTimeEnd; /**< milliseconds since 1970 */
		uint64_t srcOctets;
		uint64_t dstOctets;
		uint64_t srcPackets;
		uint64_t dstPackets;
		uint8_t srcTcpControlBits;
		uint8_t dstTcpControlBits;
		uint8_t protocol;
		char* srcPayload;
		uint32_t srcPayloadLen;
		char* dstPayload;
		uint32_t dstPayloadLen;

		/**
		 * time in seconds from 1970 on when this record will expire
		 * this value is always updated when it is aggregated
		 */
		uint32_t timeExpire;

		Connection(IpfixDataDataRecord* record);
		virtual ~Connection();
		void init(uint32_t connTimeout);
		void addFlow(Connection* c);
		string toString();
		string printTcpControlBits(uint8_t bits);
		bool compareTo(Connection* c, bool to);
		uint32_t getHash(bool to, uint32_t maxval);
		void aggregate(Connection* c, uint32_t expireTime, bool to);
		void swapDataFields();
		bool swapIfNeeded();
		void convertNtp64(uint64_t ntptime, uint64_t& result);
};

#endif
