/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
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

/* Some constants that are common to IpfixDbWriter and IpfixDbReader */
#ifdef DB_SUPPORT_ENABLED

#ifndef IPFIXDBREADER_H
#define IPFIXDBREADER_H

#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "ipfixlolib/ipfixlolib.h"
#include "reconf/Module.h"

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <boost/smart_ptr.hpp>

#include <mysql.h>

/**
 *      IpfixDbReader powered the communication to the database server
 *      also between the other structs
 */
class IpfixDbReader : public Module, public Source<IpfixRecord*>, public Destination<NullEmitable*> 
{
	public:
		IpfixDbReader(const std::string& hostname, const std::string& dbname,
				const std::string& username, const std::string& password,
				unsigned port, uint16_t observationDomainId);
		~IpfixDbReader();

		virtual void performStart();
		virtual void performShutdown();

		boost::shared_ptr<IpfixRecord::SourceID> srcId;

	protected:
		typedef struct {
			uint16_t ipfixId;  /**IPFIX_TYPEID*/
			uint8_t length;    /**IPFIX length*/
		} columnDB;

		std::vector<std::string> tables;
		std::vector<columnDB> columns;
		std::string columnNames; 
		std::string orderBy; 

		MYSQL* conn;             /** pointer to connection handle */    
		Thread thread;
		
		static InstanceManager<IpfixTemplateRecord> templateRecordIM;
		static InstanceManager<IpfixOptionsTemplateRecord> optionsTemplateRecordIM;
		static InstanceManager<IpfixDataTemplateRecord> dataTemplateRecordIM;
		static InstanceManager<IpfixDataRecord> dataRecordIM;
		static InstanceManager<IpfixOptionsRecord> optionsRecordIM;
		static InstanceManager<IpfixDataDataRecord> dataDataRecordIM;
		static InstanceManager<IpfixDataTemplateDestructionRecord> dataTemplateDestructionRecordIM;

		int getTables();
		int getColumns(const std::string& tableName);
		static void* readFromDB(void* ipfixDbReader_);
		int dbReaderSendNewTemplate(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo, const std::string& tableName);
		int dbReaderSendTable(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo, const std::string& tableName);
		int dbReaderDestroyTemplate(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo);
		int connectToDb( const std::string& hostName, const std::string& dbName, const std::string& userName, const std::string& password, unsigned int port);
};

        

#endif


#endif
