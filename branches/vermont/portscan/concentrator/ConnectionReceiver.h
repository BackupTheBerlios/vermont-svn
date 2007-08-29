#if !defined CONNECTIONRECEIVER_H
#define CONNECTIONRECEIVER_H

#include "Connection.h"

class ConnectionReceiver
{
	public:
		virtual ~ConnectionReceiver() {}
		virtual void push(Connection* conn) = 0;
};


#endif
