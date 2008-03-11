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

#ifndef _FILE_POLICY_H_
#define _FILE_POLICY_H_


#include "inputpolicybase.h"
#include "detectflowsink.h"

#include <commonutils/global.h>
#include <commonutils/sharedobj.h>
#include <commonutils/packetstats.h>
#include <concentrator/IpfixRecord.hpp>
#include <concentrator/IpfixParser.hpp>


#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>


#include <vector>
#include <iostream>

namespace TOPAS {

/**
 * Uses signals, semaphores and a shared memory block
 * to communicate with the collector.
 * Collector writes its data into files on a filesystem
 * (ramdisk). 
 */ 
class SemShmNotifier : public InputNotificationBase {
public:
        SemShmNotifier();
        ~SemShmNotifier();

        /**
         * Waits for new data
         * @returns always returns 1
         */
        int wait() const;

        /**
         * Informs collector that all data was processed.
         */
        int notify() const;

        /**
         * Returns first filenumber
         * @return First filenumber
         */
        shared::FileCounter getFrom() {
                return nps->from();
        }

        /**
         * Returns last file number
         * @return last file number
         */
        shared::FileCounter getTo() {
                return nps->to();
        }

        /**
         * Get temporary packet direcories, where the IPFIX packets are stored.
         * The directory is provided by the collector
         * @return directory name wherer IPFIX packets are stored
         */
        std::string getPacketDir() {
                return packetDir;
        }

	/**
	 * TODO: REMOVE THIS TESTING WORKAROUND!
	 */
	bool useFiles() { return useFiles_; }
private:
        key_t semKey, shmKey;
        int semId;
        shared::SharedObj* nps;

        std::string packetDir;

	bool useFiles_;
};



template <
        class Notifier,
        class Buffer
>
class FilePolicy {
public:
        FilePolicy(DetectFlowSink<Buffer>* fs)
                :data(NULL)
        {
                data = new uint8_t[config_space::MAX_IPFIX_PACKET_LENGTH];
		sourceId = new VERMONT::IpfixRecord::SourceID();

		flowSink = fs;
		flowSink->run();
		ipfixParser.addFlowSink(flowSink);
        }

        ~FilePolicy() 
        {
		flowSink->terminateSink();
		delete flowSink;
        }


        void import(Notifier& notifier) {
                static FILE* fd;
                static shared::FileCounter i;

                static int filesize = strlen(notifier.getPacketDir().c_str()) + 30;
                static char* filename = new char[filesize];
		static uint16_t len = 0;

                for ( i = notifier.getFrom(); i != notifier.getTo(); ++i) {
			if (notifier.useFiles()) {
				snprintf(filename, filesize, "%s%i", notifier.getPacketDir().c_str(), (int)i);
				if (NULL == (fd = fopen(filename, "rb"))) {
					std::cerr << "Detection modul: Could not open file"
						  << filename << ": " << strerror(errno) 
						  << std::endl;
				}
				
				read(fileno(fd), &len, sizeof(uint16_t));
				read(fileno(fd), data, len);
				if (isSourceIdInList(*(uint16_t*)(data + 12))) {
					ipfixParser.processPacket(boost::shared_array<uint8_t>(data), len, boost::shared_ptr<VERMONT::IpfixRecord::SourceID>(sourceId));
				}
				if (EOF == fclose(fd)) {
					std::cerr << "Detection Modul: Could not close "
						  << filename << ": " << strerror(errno)
						  << std::endl;
				}
			} else {
				len = IpfixShm::readPacket(&data);
				if (isSourceIdInList(*(uint16_t*)(data+12))) {
					ipfixParser.processPacket(boost::shared_array<uint8_t>(data), len, boost::shared_ptr<VERMONT::IpfixRecord::SourceID>(sourceId));
				}
			}
                }
        }


	void subscribeSourceId(uint16_t id) {
		sourceIdList.push_back(id);
	}

protected:
        VERMONT::IpfixParser ipfixParser;
	VERMONT::IpfixRecord::SourceID* sourceId;
        uint8_t* data;
	DetectFlowSink<Buffer>* flowSink;
	std::vector<uint16_t> sourceIdList;

	bool isSourceIdInList(uint16_t id) const
	{
		if (sourceIdList.empty())
			return true;

		for (std::vector<uint16_t>::const_iterator i = sourceIdList.begin(); i != sourceIdList.end(); ++i) {
			if ((*i) == id) {
				return true;
			}
		}
		return false;
	}
};

template <
class Notifier,
class Storage
>
class BufferedFilesInputPolicy : public InputPolicyBase<Notifier, Storage>
{
public:
	BufferedFilesInputPolicy()
		: filePolicy(new DetectFlowSink<Storage>())
	{
		
	}

        void importToStorage() 
        {
        	filePolicy.import();        
        }

        Storage* getStorage()
        {
		filePolicy.getStorage();
        }


private:
	FilePolicy<Notifier, Storage> filePolicy;
	
};

template <
class Notifier,
class Storage
>
class UnBufferedFilesInputPolicy : public InputPolicyBase<Notifier, Storage>
{
public:
	UnBufferedFilesInputPolicy()
		: filePolicy(new UnBufferedDetectSink<Storage>())
	{
		
	}

	void importToStorage() 
	{
		filePolicy.import(this->getNotifier());
	}

	Storage* getStorage()
	{
		filePolicy.getStorage();
	}

private:
	FilePolicy<Notifier, Storage> filePolicy;
};



///** 
// * Extracts IPFIX packets from files and imports them direcly into a storage
// * class. All data is buffered into one storage class till the data is fetched
// * using @c getStorage().
// */ 
//template <
//	class Notifier,
//	class Storage
//>
//class BufferedFilesInputPolicy : public InputPolicyBase<Notifier, Storage>, public PacketReader<Notifier, Storage> {
//public:
//	BufferedFilesInputPolicy() {
//		buffer = new Storage();
//	}
//
//	~BufferedFilesInputPolicy() {
//		if (buffer) 
//			delete buffer;
//	}
//
//	void importToStorage() {
//		packetLock.lock();
//		import(this->getNotifier());
//		packetLock.unlock();
//	}
//
//	/**
//	 * Returns a storage object. This object contains all data buffered since last call to @c getStorage().
//	 * The method returns an empty object if no data was buffered.
//	 * @return buffered IFPIX data.
//	 */
//        Storage* getStorage()
//        {
//                packetLock.lock();
//                Storage* ret = buffer;
//                buffer = new Storage();
//                packetLock.unlock();
//                return ret;
//        }
//		
//private:
//	Storage* buffer;
//	Mutex packetLock;
//
//	Storage* getBuffer() { return buffer; }
//};
//
//
//
///** 
// * Extracts IPFIX packets from files and imports them into a storage class. 
// * Each IPFIX record is seperately stored within an instance of the Storage class.
// */ 
//template <
//	class Notifier,
//	class Storage
//>
//class UnbufferedFilesInputPolicy : public InputPolicyBase<Notifier, Storage>, public PacketReader<Notifier, Storage> {
//public:
//	UnbufferedFilesInputPolicy() : maxBuffers(256), bufferErrors(0) {
//		packetLock.lock();
//	}
//
//	~UnbufferedFilesInputPolicy() {
//	}
//
//	void importToStorage() {
//		import(this->getNotifier());
//	}
//
//	/**
//	 * Returns storage if there are new records available. Blocks if no records available.
//	 */
//        Storage* getStorage()
//        {
//		packetLock.lock();
//		PacketReader<Notifier, Storage>::recordMutex.lock();
//		Storage* ret = *buffers.begin();
//		buffers.pop_front();
//		PacketReader<Notifier, Storage>::recordMutex.unlock();
//		return ret;
//        }
//
//private:
//	Storage* getBuffer() {
//		if(buffers.size() >= maxBuffers) // Buffer is full
//		{
//			bufferErrors++;
//			msg(MSG_ERROR, "DetectionBase: getBuffer() returns NULL, record will be dropped! %lu", bufferErrors);
//			return NULL;
//		}
//		Storage* ret = new Storage();
//		buffers.push_back(ret);
//		packetLock.unlock();
//		return ret;
//	}
//
//	std::list<Storage*> buffers;
//	unsigned maxBuffers;	
//	unsigned bufferErrors;
//	Mutex packetLock; // locked as long as buffers is empty
//};

};

#endif
