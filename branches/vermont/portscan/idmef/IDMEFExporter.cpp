
#include "IDMEFExporter.h"

#include <sys/stat.h>
	 
uint32_t IDMEFExporter::filenameCounter = 0;

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
	while ((bytes = fread(temp, 1024, 1, f)) > 0) {
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
	while (stat(filename.c_str(), &s) != 0) {
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

	return filename;
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
	string idmefmsg = templateContent;

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

	// ensure that no idmef messages are simultaneously written by several threads!
	mutex.lock();

	string filename = getFilename();

	// save message to destination directory
	FILE* f = fopen(filename.c_str(), "w");
	// first line is URL where processing script should send event to
	if (fwrite(sendURL.c_str(), sendURL.size(), 1, f) != sendURL.size()) goto error;
	if (fwrite("\n", 1, 1, f) != 1) goto error;
	if (fwrite(idmefmsg.c_str(), idmefmsg.size(), 1, f) != idmefmsg.size()) goto error;
	if (fwrite("\n", 1, 1, f) != 1) goto error;
	if (fclose(f) != 0) goto error;

	return;

error:
	THROWEXCEPTION("failed to write to file %s, error %s", filename.c_str(), strerror(errno));
}
