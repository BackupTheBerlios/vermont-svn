/** \file
 * Receives raw network messages.
 * Passes them on to higher levels.
 */

#ifndef RCVMESSAGE_H
#define RCVMESSAGE_H

#include "common.h"

/***** Constants ************************************************************/


/***** Data Types ***********************************************************/

typedef boolean(MessageCallbackFunction)(byte* data, uint16 length);

/***** Prototypes ***********************************************************/

void initializeRcvMessage();

void deinitializeRcvMessage();

void startRcvMessage();

void stopRcvMessage();

void setMessageCallback(MessageCallbackFunction* messageCallbackFunction);

int rcvMessageUdpIpv4(uint16 port);

void rcvMessageClose(int handle);

#endif
