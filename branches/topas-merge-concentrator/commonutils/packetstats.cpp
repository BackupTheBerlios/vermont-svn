/**************************************************************************/
/*    Copyright (C) 2005-2008 Lothar Braun <mail@lobraun.de>              */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

#include "packetstats.h"


#include <stdexcept>
#include <cstring>

namespace TOPAS {

IpfixFile* IpfixFile::ipfixFile = NULL;
Mutex IpfixFile::listLock;
std::list<std::string> IpfixFile::fileNames;


IpfixFile* IpfixFile::writePacket(const char* filename, boost::shared_array<uint8_t> message, uint16_t length)
{
        if (!ipfixFile)
                ipfixFile = new IpfixFile();

	/* TODO: Check if file already exists */
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out.is_open()) {
		VERMONT::msg(MSG_ERROR, "Collector: Couldn't open file %s for writing: %s\n", filename, strerror(errno));
                return NULL;
	}
                

	if (!out.write((const char*)&length, sizeof(length))) {
		VERMONT::msg(MSG_FATAL, "Collector: Couldn't write packet length to file system: %s\n", strerror(errno));
                return NULL;
	}

	if (!out.write((const char*)message.get(), length)) {
		VERMONT::msg(MSG_FATAL, "Collector: Couldn't write packet data to file system: %s\n", strerror(errno));
                return NULL;
	}
                
	out.close();
        listLock.lock();
        fileNames.push_back(filename);
        listLock.unlock();
        
        return ipfixFile;
}

void IpfixFile::proceedOnePacket()
{
        listLock.lock();
	unlink(fileNames.begin()->c_str());
        fileNames.pop_front();
        listLock.unlock();
}


/********************************************************************************/

uint8_t* IpfixShm::startLocation = NULL;
uint8_t* IpfixShm::writePosition = NULL;
uint8_t* IpfixShm::readPosition  = NULL;
bool  IpfixShm::writeBeforeRead = false;
size_t IpfixShm::size = 0;
uint16_t IpfixShm::packetSize = 0;
IpfixShm* IpfixShm::instance = NULL;


IpfixShm::IpfixShm()
{
}

IpfixShm* IpfixShm::writePacket(boost::shared_array<uint8_t> message, uint16_t len)
{
        if (!instance) {
                instance = new IpfixShm();
        }
	
        if (len == 0) {
                VERMONT::msg(MSG_ERROR, "IpfixShm: Got empty packet!!!!");
                return NULL;
	}

	if (startLocation == NULL) {
		VERMONT::msg(MSG_ERROR, "IpfixShm: No shared memory storage area allocated!");
                return NULL;
	}

	if (len > size) {
		VERMONT::msg(MSG_ERROR, "IpfixShm: Packet too long for shared memory");
                return NULL;
	}
	
	if ((writePosition + len + sizeof(len)) >= (startLocation + size)) {
		// we only write a complete packet to the shared memory
		// if there is not enough space to do that, we'll start at the
		// the beginning of the buffer
		// We signal this jump to the reader by zeroing the remaining
		// buffer.
		bzero(writePosition, ((startLocation + size) - writePosition));

		if (readPosition >= writePosition) {
			VERMONT::msg(MSG_ERROR, "IpfixShm: Shared memory block too small. Trashing packet!!!!!");
                        return NULL;
		}

		writeBeforeRead = true;
		writePosition = startLocation;
	}

	if (writeBeforeRead && (writePosition + len + sizeof(len)) > readPosition) {
		//msg(MSG_ERROR, "%i %i", (writePosition - startLocation), (readPosition - startLocation));
		VERMONT::msg(MSG_ERROR, "IpfixShm: Shared memory block too small!");
                return NULL;
	}

	//msg(MSG_ERROR, "Writing packet len: %i", len);
	memcpy(writePosition, &len, sizeof(len));
	//msg(MSG_FATAL, "written: %i", *(uint16_t*)writePosition);
	memcpy(writePosition + sizeof(len), message.get(), len);
	//msg(MSG_FATAL, "written: %#06x", ntohs(*(uint16_t*)(writePosition+sizeof(len))));
	writePosition += sizeof(len) + len;

        return instance;
}

uint16_t IpfixShm::readPacket(uint8_t** data) {
	// go to the next packet;
	// packetSize == 0 for the first packet
	readPosition += packetSize;

	if (sizeof(uint16_t) > ((startLocation + size) - readPosition)) {
		writeBeforeRead = false;
		readPosition = startLocation;
	}

	// get the packet length
	packetSize = *(uint16_t*)readPosition;
	readPosition += sizeof(uint16_t);

	if (packetSize == 0) {
		writeBeforeRead = false;
		readPosition = startLocation;
		packetSize = *(uint16_t*)readPosition;
		readPosition += sizeof(uint16_t);
	}
	
	//msg(MSG_FATAL, "reading: %#06x",  ntohs(*(uint16_t*)readPosition));
	*data = readPosition;
	
	
	return packetSize;
}

void IpfixShm::proceedOnePacket()
{
        readPacket(&readPosition);
}

void IpfixShm::setShmPointer(uint8_t* ptr)
{
	startLocation = writePosition = readPosition = ptr;
}

void IpfixShm::setShmSize(size_t s)
{
	size = s;
}

};
