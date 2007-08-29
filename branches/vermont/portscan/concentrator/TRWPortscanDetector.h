#if !defined(TRWPORTSCANDETECTOR_H)
#define TRWPORTSCANDETECTOR_H

#include "ConnectionReceiver.h"

#include <list>

using namespace std;

class TRWPortscanDetector : public ConnectionReceiver
{
	public:
		TRWPortscanDetector();
		virtual ~TRWPortscanDetector();
		virtual void push(Connection* conn);

	private:
		struct TRWEntry {
			Connection* connection;
		};

		list<TRWEntry> trwEntries;
};

#endif
