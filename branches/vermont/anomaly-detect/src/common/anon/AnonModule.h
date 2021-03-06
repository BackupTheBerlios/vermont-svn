/*
 * Vermont Anonymization Subsystem
 * Copyright (C) 2008 Lothar Braun
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

#ifndef _ANON_MODULE_H_
#define _ANON_MODULE_H_

#include "AnonPrimitive.h"
#include <common/msg.h>
#include <map>
#include <vector>
#include <stdint.h>

class AnonMethod 
{
public:
	typedef enum {
		BytewiseHashHmacSha1,
		BytewiseHashSha1,
		ConstOverwrite,
		ContinuousChar,
		HashHmacSha1,
		HashSha1,
		Randomize,
		Shuffle,
		Whitenoise,
		CryptoPan
	} Method;

	static Method stringToMethod(const std::string& m)
	{
		if (m == "BytewiseHashHmacSha1") {
			return BytewiseHashHmacSha1;
		} else if (m == "BytewiseHashSha1") {
                        return BytewiseHashSha1;
                } else if (m == "ConstOverwrite") {
                        return ConstOverwrite;
                } else if (m == "ContinuousChar") {
                        return ContinuousChar;
                }else if (m == "HashHmacSha1") {
                        return HashHmacSha1;
                }else if (m == "HashSha1") {
                        return HashSha1;
                }else if (m == "Randomize") {
                        return Randomize;
                }else if (m == "Shuffle") {
                        return Shuffle;
                }else if (m == "Whitenoise") {
                        return Whitenoise;
                }else if (m == "CryptoPan") {
			return CryptoPan;
		}
		THROWEXCEPTION("Unknown anonymization method");

		// make compile happy
		return BytewiseHashHmacSha1;
	}
};

struct AnonIE {
	uint16_t offset; // used by AnonFilter
	unsigned short header; // used by AnonFilter
	unsigned long packetClass; // used by AnonFilter
	int len;
	std::vector<AnonPrimitive*> primitive;
};

class AnonModule {
public:
	~AnonModule();
	//TODO: enterprise number should be considered (Gerhard 12/2009)
	void addAnonymization(uint16_t id, int len, AnonMethod::Method methodName, const std::string& parameter="");
	void anonField(uint16_t id, void* data, int len = -1);
protected:
	typedef std::map<uint16_t, AnonIE> MethodMap;
	MethodMap methods;
private:
	AnonPrimitive* createPrimitive(AnonMethod::Method m, const std::string& parameter);
};


#endif
