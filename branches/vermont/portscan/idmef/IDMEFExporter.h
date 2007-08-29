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

#if !defined(IDMEFEXPORTER_H)
#define IDMEFEXPORTER_H

#include "common/Mutex.h"

#include <string>
#include <map>

using namespace std;

/**
 * this class is used for exporting IDMEF messages
 * to do that, a template for the IDMEF message is read and all embedded parameters are
 * replaced by the given ones (to set them, use function setVariable)
 * then the IDMEF message is stored in the destination directory with a generated filename
 */
class IDMEFExporter
{
	public:
		// default variable keys (should be set using setVariable)
		const static char* PAR_SOURCE_ADDRESS; // = "SOURCE_ADDRESS";
		const static char* PAR_TARGET_ADDRESS; // = "TARGET_ADDRESS";

		IDMEFExporter(const string tmplfilename, const string destdir, const string sendurl);
		virtual ~IDMEFExporter();

		void setVariable(const string key, const string value);
		void setVariable(const string key, const uint32_t value);
		void exportMessage();

	private:
		// private variable keys which are set by IDMEFExporter
		const static char* PAR_ANALYZER_ID; // = "ANALYZER_ID";
		const static char* PAR_ANALYZER_HOST; // = "ANALYZER_HOST";
		const static char* PAR_ANALYZER_IP; // = "ANALYZER_IP";
		const static char* PAR_CREATE_TIME; // = "CREATE_TIME";
		const static char* PAR_MESSAGE_ID; // = "MESSAGE_ID";
		const static char* PAR_NTP_TIME; // = "NTP_TIME";

		string templateFilename;
		string destinationDir;
		string sendURL;
		string templateContent;
		string hostname;
		string ipaddress;
		string analyzerId;

		map<string, string> parameterValues;
		static uint32_t filenameCounter;
		static Mutex mutex;
		static bool filewarningIssued;
		static uint32_t idCounter;
		static time_t idCounterTime;

		void readTemplate();
		string getFilename();
		string createMessageID();
		string getCreateTime(time_t t);
		string getNtpStamp(time_t t);
};

#endif
