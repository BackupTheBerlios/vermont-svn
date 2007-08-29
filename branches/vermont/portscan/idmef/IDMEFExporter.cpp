
#include "IDMEFExporter.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

	 
uint32_t IDMEFExporter::filenameCounter = 0;
const char* IDMEFExporter::PAR_MESSAGE_ID = "MESSAGE_ID";
const char* IDMEFExporter::PAR_SOURCE_ADDRESS = "SOURCE_ADDRESS";
const char* IDMEFExporter::PAR_TARGET_ADDRESS = "TARGET_ADDRESS";
const char* IDMEFExporter::PAR_ANALYZER_ID = "ANALYZER_ID";
const char* IDMEFExporter::PAR_ANALYZER_HOST = "ANALYZER_HOST";
const char* IDMEFExporter::PAR_ANALYZER_IP = "ANALYZER_IP";
const char* IDMEFExporter::PAR_CREATE_TIME = "CREATE_TIME";
const char* IDMEFExporter::PAR_NTP_TIME = "NTP_TIME";
bool IDMEFExporter::filewarningIssued = false;
Mutex IDMEFExporter::mutex;
uint32_t IDMEFExporter::idCounter = 0;
time_t IDMEFExporter::idCounterTime = time(0);


/**
 * initializes a new instance of IDMEFExporter
 * @param tmplfilename filename of the template xml file for the IDMEF message
 * @param destdir destination directory, where all exported IDMEF messages are to be stored
 * @param sendurl destination URL which is to be used by the perl script which processes the 
 *                destination directory
 */
IDMEFExporter::IDMEFExporter(const string tmplfilename, const string destdir, const string sendurl)
	: templateFilename(tmplfilename), 
	  destinationDir(destdir),
	  sendURL(sendurl)
{
	readTemplate();

	// check if destination directory is existent
	struct stat s;
	int retval = stat(destdir.c_str(), &s);
	if (retval != 0) {
		THROWEXCEPTION("failed to access idmef destination directory %s, error %s", destdir.c_str(), strerror(errno));
	}
	if (!S_ISDIR(s.st_mode)) {
		THROWEXCEPTION("given idmef destination directory %s is not a directory", destdir.c_str());
	}

	// retrieve default values
	struct utsname un;
	if (uname(&un) != 0) THROWEXCEPTION("failed to call uname, error: %s", strerror(errno));
	char hostname[512];
	if (gethostname(hostname, 512) != 0) THROWEXCEPTION("failed to call gethostname, error: %s", strerror(errno));
	struct hostent* he = gethostbyname(hostname);
	if (he == NULL) THROWEXCEPTION("failed to call gethostbyname, error: %s", strerror(errno));

	this->hostname = string(hostname) + "." + string(un.domainname);
	ipaddress = inet_ntoa(*((struct in_addr *)he->h_addr));
	msg(MSG_DIALOG, "using hostname %s and ip address %s", this->hostname.c_str(), ipaddress.c_str());

	// FIXME: let analyzerId be set by configuration
	analyzerId = "11111";

	srand(time(0));
}

IDMEFExporter::~IDMEFExporter()
{
}

/**
 * reads a template into the internal buffer
 */
void IDMEFExporter::readTemplate()
{
	FILE* f = fopen(templateFilename.c_str(), "r");
	if (f == NULL) {
		THROWEXCEPTION("failed to open template file %s, error %s", templateFilename.c_str(), strerror(errno));
	}
	char temp[1025];
	templateContent = "";
	uint32_t bytes;
	while ((bytes = fread(temp, 1, 1024, f)) > 0) {
		temp[bytes] = 0;
		templateContent += temp;
	}
	if (fclose(f) != 0) THROWEXCEPTION("failed to close template file %s, error %s", templateFilename.c_str(), strerror(errno));
}

/**
 * internally, a map of all possible variables inside the IDMEF template is stored
 * a variable is marked as %KEY% inside the template file and is replaced with the value
 * given in parameter 'value' in this function
 */
void IDMEFExporter::setVariable(const string key, const string value)
{
	parameterValues[key] = value;
}

/**
 * for documentation see setVariable(string, string)
 */
void IDMEFExporter::setVariable(const string key, const uint32_t value)
{
	char valtext[15];
	snprintf(valtext, 15, "%u", value);
	parameterValues[key] = valtext;
}

/**
 * get a filename in the destination directory which is not used
 * filenames are started with 00000000, then counting upwards until the 32 bit value is wrapped around
 * we issue a warning, if files are already present in the directory
 */
string IDMEFExporter::getFilename()
{
	struct stat s;
	char number[100];
	snprintf(number, 100, "%010d", filenameCounter);
	string filename = destinationDir + "/" + number + ".xml";
	uint32_t counter = 0;
	while (stat(filename.c_str(), &s) == 0) {
		if (!filewarningIssued) {
			msg(MSG_ERROR, "files in IDMEF destination directory are already present, either two processes are writing there simultaneously (VERY BAD, may result in lost or corrupt events), or files from previous run have not been processed yet (ALSO BAD)");
			filewarningIssued = true;
		}
		if (counter == 0xFFFFFFFF) {
			THROWEXCEPTION("failed to find an accessible file in IDMEF destination directory %s", destinationDir.c_str());
		}
		filenameCounter++;
		counter++;
		snprintf(number, 100, "%010d", filenameCounter);
		filename = destinationDir + "/" + number + ".xml";
	}

	filenameCounter++;

	return filename;
}

/**
 * creates a unique id for the IDMEF message
 * this function takes input from the current time, the processid, a random variable
 * and an internal counter
 */
string IDMEFExporter::createMessageID()
{
	char id[64];

	uint32_t randvar = rand()%10000000;
	if (idCounter > 0x1000000) {
		// idCounterTime contains time of last reset of idCounter
		if (idCounterTime == time(0)) {
			// this ensures, that all created ids are unique, but is pretty crude
			// usually we don't send more than 0x1000000 idmef messages per second
			sleep(1);
		}
		idCounterTime = time(0);
		idCounter = 0;
	}
	snprintf(id, 64, "%012X%X%08X%06X", static_cast<uint32_t>(time(0)), randvar, getpid(), idCounter++);

	return id;
}

/**
 * return time string for idmef message
 */
string IDMEFExporter::getCreateTime(time_t t)
{
	char timestr[100];
	struct tm* tms;
	tms = localtime(&t);
	strftime(timestr, sizeof(timestr), "%F-T%TZ", tms);
	return timestr;
}

/**
 * returns ntp time string for idmef message
 */
string IDMEFExporter::getNtpStamp(time_t t)
{
	char timestr[100];

	// 2208988800 is the amount of seconds between 1900-01-01 and 1970-01-01
	snprintf(timestr, sizeof(timestr), "0x%lX.0x0", t+2208988800UL); 

	return timestr;
}

/**
 * exports an IDMEF messag
 * to do this, the internally stored IDMEF template is processed with all keys of the internal
 * parameterMap and then the file is saved in the destination directory
 * the external process which sends the IDMEF messages uses the URL given in the first line of 
 * the saved message
 */
void IDMEFExporter::exportMessage()
{
	DPRINTF("sending IDMEF message");
	string idmefmsg = templateContent;

	// ensure that no idmef messages are simultaneously written by several threads!
	mutex.lock();

	time_t t = time(0);

	// set idmef parameters
	setVariable(PAR_ANALYZER_ID, analyzerId);
	setVariable(PAR_ANALYZER_IP, ipaddress);
	setVariable(PAR_ANALYZER_HOST, hostname);
	setVariable(PAR_CREATE_TIME, getCreateTime(t));
	setVariable(PAR_NTP_TIME, getNtpStamp(t));
	setVariable(PAR_MESSAGE_ID, createMessageID());

	// replace all variable values inside the template
	map<string, string>::iterator iter = parameterValues.begin();
	while (iter != parameterValues.end()) {
		string key = string("%") + iter->first + "%";
		string val = iter->second;

		uint32_t pos;
		while ((pos = idmefmsg.find(key)) != string::npos) {
			idmefmsg.replace(pos, key.size(), val);
		}

		iter++;
	}

	string filename = getFilename();

	// save message to destination directory
	FILE* f = fopen(filename.c_str(), "w+");
	flockfile(f);
	if (f == NULL) goto error;
	// first line is URL where processing script should send event to
	if (fwrite(sendURL.c_str(), 1, sendURL.size(), f) != sendURL.size()) goto error;
	if (fwrite("\n", 1, 1, f) != 1) goto error;
	if (fwrite(idmefmsg.c_str(), 1, idmefmsg.size(), f) != idmefmsg.size()) goto error;
	if (fwrite("\n", 1, 1, f) != 1) goto error;
	funlockfile(f);
	if (fclose(f) != 0) goto error;

	mutex.unlock();

	return;

error:
	THROWEXCEPTION("failed to write to file %s, error: %s", filename.c_str(), strerror(errno));
}
