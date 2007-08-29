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
		IDMEFExporter(const string tmplfilename, const string destdir, const string sendurl);
		virtual ~IDMEFExporter();

		void setVariable(const string key, const string value);
		void exportMessage();

	private:
		string templateFilename;
		string destinationDir;
		string sendURL;
		string templateContent;
		map<string, string> parameterValues;
		static uint32_t filenameCounter;
		static Mutex mutex;
		static bool filewarningIssued;

		void readTemplate();
		string getFilename();
};

#endif
