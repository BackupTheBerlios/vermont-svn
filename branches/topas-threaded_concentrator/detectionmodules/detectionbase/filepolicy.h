/**************************************************************************/
/*    Copyright (C) 2005-2007 Lothar Braun <mail@lobraun.de>              */
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
#include "detectcallbacks.h"


#include <commonutils/global.h>
#include <commonutils/sharedobj.h>
#include <commonutils/metering.h>
#include <commonutils/packetstats.h>


#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>


#include <list>
#include <iostream>

#include "concentrator/IpfixParser.hpp"
#include "concentrator/FlowSink.hpp"

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




/**
 * Extracts whole Ipfix-Packets out of files and imports them directly into
 * a storage class
 */
template <
        class Notifier,
        class Buffer
>
class PacketReader : public FlowSink {
public:
        PacketReader()
                : packetProcessor(NULL), data(NULL)
        {
		Metering::setDirectoryName("metering/");
		metering = new Metering("packetreader");
                data = new uint8_t[config_space::MAX_IPFIX_PACKET_LENGTH];

                /*
                  create an packetProcessor and ipfixParser
                  we don't need an receiver because we do the "receiving" work by hand
                */
		packetProcessor = new IpfixParser();
		static_cast<IpfixParser*>(packetProcessor)->addFlowSink(this);
        }

        ~PacketReader() 
        {
                if (packetProcessor) delete packetProcessor;
                delete data;
		delete metering;
        }

	/**
	 * tries to feed to the callback methods one of the IpfixRecords put on the queue by the IpfixParser.
	 * Immediately returns if the queue is empty.
	 */
	void tryProcessIpfixRecord() {
		boost::shared_ptr<IpfixRecord> ipfixRecord;
		if (!ipfixRecords.pop(1000, &ipfixRecord)) return;
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

        void import(Notifier& notifier) {
                static FILE* fd;
                static shared::FileCounter i;

                static int filesize = strlen(notifier.getPacketDir().c_str()) + 30;
                static char* filename = new char[filesize];
		static uint16_t len = 0;
		boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID); //FIXME: initialize SourceID to something (remotely) sensible

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
					packetProcessor->processPacket(boost::shared_array<uint8_t>(data), len, sourceID);
					while (ipfixRecords.getCount() > 0) tryProcessIpfixRecord();
				}
				metering->addValue();
				if (EOF == fclose(fd)) {
					std::cerr << "Detection Modul: Could not close "
						  << filename << ": " << strerror(errno)
						  << std::endl;
				}
			} else {
				len = IpfixShm::readPacket(&data);
                                metering->addValue();
				if (isSourceIdInList(*(uint16_t*)(data+12))) {
					packetProcessor->processPacket(boost::shared_array<uint8_t>(data), len, sourceID);
					while (ipfixRecords.getCount() > 0) tryProcessIpfixRecord();
				}
			}
                }
        }



        void subscribeId(int id) 
        {
                idList.push_back(id);
        }

	void subscribeSourceId(uint16_t id) {
		sourceIdList.push_back(id);
	}

protected:
        std::vector<int> idList;
	std::vector<uint16_t> sourceIdList;
        IpfixPacketProcessor* packetProcessor;
	Mutex recordMutex;
        uint8_t* data;
	Metering* metering;

	virtual Buffer* getBuffer() = 0;


        bool isIdInList(int id) const
        {
		if (idList.empty()) {
			return true;
		}
                /* TODO: think about hashing */
                for (std::vector<int>::const_iterator i = idList.begin(); i != idList.end(); ++i) {
                        if ((*i) == id)
                                return true;
                }
                
                return false;
        }

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

        friend int newTemplateArrived<PacketReader, Buffer>(void* handle,  IpfixRecord::SourceID sourceID, IpfixRecord::TemplateInfo* ti);
        friend int newDataRecordArrived<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::TemplateInfo* ti,
							      uint16_t length, IpfixRecord::Data* data);
        friend int templateDestroyed<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::TemplateInfo* ti);
        friend int newOptionsTemplateArrived<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
        friend int newOptionRecordArrived<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::OptionsTemplateInfo* oti,
								uint16_t length, IpfixRecord::Data* data);
        friend int optionsTemplateDestroyed<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
        friend int newDataTemplateArrived<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
        friend int newDataRecordFixedFieldsArrived<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID,
									 IpfixRecord::DataTemplateInfo* ti, uint16_t length,
									 IpfixRecord::Data* data);
        friend int dataTemplateDestroyed<PacketReader, Buffer>(void* handle, IpfixRecord::SourceID sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);


	virtual int onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) { 
		 newTemplateArrived<PacketReader, Buffer>(this, *sourceID, templateInfo);
	};

	virtual int onOptionsTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) { 
		 newOptionsTemplateArrived<PacketReader, Buffer>(this, *sourceID, optionsTemplateInfo);
	};

	virtual int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo) {
		 newDataTemplateArrived<PacketReader, Buffer>(this, *sourceID, dataTemplateInfo);
	};

	virtual int onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data) {
		 newDataRecordArrived<PacketReader, Buffer>(this, *sourceID, templateInfo, length, data);
	};

	virtual int onOptionsRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, IpfixRecord::Data* data) {
		 newOptionRecordArrived<PacketReader, Buffer>(this, *sourceID, optionsTemplateInfo, length, data);
	};

	virtual int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data) {
		 newDataRecordFixedFieldsArrived<PacketReader, Buffer>(this, *sourceID, dataTemplateInfo, length, data);
	};

	virtual int onTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) {
		 templateDestroyed<PacketReader, Buffer>(this, *sourceID, templateInfo);
	};

	virtual int onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) {
		 optionsTemplateDestroyed<PacketReader, Buffer>(this, *sourceID, optionsTemplateInfo);
	};

	virtual int onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo) {
		 dataTemplateDestroyed<PacketReader, Buffer>(this, *sourceID, dataTemplateInfo);
	};

};



/** 
 * Extracts IPFIX packets from files and imports them direcly into a storage
 * class. All data is buffered into one storage class till the data is fetched
 * using @c getStorage().
 */ 
template <
	class Notifier,
	class Storage
>
class BufferedFilesInputPolicy : public InputPolicyBase<Notifier, Storage>, public PacketReader<Notifier, Storage> {
public:
	BufferedFilesInputPolicy() {
		buffer = new Storage();
	}

	~BufferedFilesInputPolicy() {
		if (buffer) 
			delete buffer;
	}

	void importToStorage() {
		packetLock.lock();
		import(this->getNotifier());
		packetLock.unlock();
	}

	/**
	 * Returns a storage object. This object contains all data buffered since last call to @c getStorage().
	 * The method returns an empty object if no data was buffered.
	 * @return buffered IFPIX data.
	 */
        Storage* getStorage()
        {
                packetLock.lock();
                Storage* ret = buffer;
                buffer = new Storage();
                packetLock.unlock();
                return ret;
        }
		
private:
	Storage* buffer;
	Mutex packetLock;

	Storage* getBuffer() { return buffer; }
};



/** 
 * Extracts IPFIX packets from files and imports them into a storage class. 
 * Each IPFIX record is seperately stored within an instance of the Storage class.
 */ 
template <
	class Notifier,
	class Storage
>
class UnbufferedFilesInputPolicy : public InputPolicyBase<Notifier, Storage>, public PacketReader<Notifier, Storage> {
public:
	UnbufferedFilesInputPolicy() : maxBuffers(256), bufferErrors(0) {
		packetLock.lock();
	}

	~UnbufferedFilesInputPolicy() {
	}

	void importToStorage() {
		import(this->getNotifier());
	}

	/**
	 * Returns storage if there are new records available. Blocks if no records available.
	 */
        Storage* getStorage()
        {
		packetLock.lock();
		PacketReader<Notifier, Storage>::recordMutex.lock();
		Storage* ret = *buffers.begin();
		buffers.pop_front();
		PacketReader<Notifier, Storage>::recordMutex.unlock();
		return ret;
        }

private:
	Storage* getBuffer() {
		if(buffers.size() >= maxBuffers) // Buffer is full
		{
			bufferErrors++;
			msg(MSG_ERROR, "DetectionBase: getBuffer() returns NULL, record will be dropped! %lu", bufferErrors);
			return NULL;
		}
		Storage* ret = new Storage();
		buffers.push_back(ret);
		packetLock.unlock();
		return ret;
	}

	std::list<Storage*> buffers;
	unsigned maxBuffers;	
	unsigned bufferErrors;
	Mutex packetLock; // locked as long as buffers is empty
};

#endif
